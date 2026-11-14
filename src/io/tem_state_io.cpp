// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Updated: state of the cohorts as one NetCDF file.

#include "tem_state_io.h"
#include "netcdf_io.h"

#include <netcdf.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "diag_manager.h"
#include "mpi_layer.h"

namespace tem
{
namespace state
{
namespace
{

[[noreturn]] void die( const std::string& msg )
{
  std::cerr << "TEM state file: " << msg << std::endl;
  std::exit( -1 );
}

void check( int status, const std::string& what )
{
  if( status != NC_NOERR )
    die( what + ": " + nc_strerror( status ) );
}

long scaled( double x )
{
  return std::lround( x * 1.0e4 );
}

} // namespace

void Writer::open( const std::string& path )
{
  path_ = path;
  // Records of all ranks reach rank 0 through the diag manager, which is also the path of the output; rank 0 writes the one file.
  tem::diag::init_state( [this]( const std::string&, double lon, double lat, int cohort, int, const double* v, int n ) {
    write( lon, lat, cohort, v, n );
  } );
}

void Writer::create( size_t nitem )
{
  nitem_ = nitem;
  int drec, ditem;
  check( nc_create( path_.c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid_ ), "cannot create " + path_ );
  check( nc_def_dim( ncid_, "record", NC_UNLIMITED, &drec ), "defining record" );
  check( nc_def_dim( ncid_, "item", nitem, &ditem ), "defining item" );
  check( nc_def_var( ncid_, "lon", NC_DOUBLE, 1, &drec, &vlon_ ), "defining lon" );
  check( nc_def_var( ncid_, "lat", NC_DOUBLE, 1, &drec, &vlat_ ), "defining lat" );
  check( nc_def_var( ncid_, "cohort", NC_INT64, 1, &drec, &vcoh_ ), "defining cohort" );
  const int dims[2] = { drec, ditem };
  check( nc_def_var( ncid_, "state", NC_DOUBLE, 2, dims, &vstate_ ), "defining state" );
  const char* title = "TEM state: one record per cell and cohort";
  nc_put_att_text( ncid_, NC_GLOBAL, "title", strlen( title ), title );
  check( tem::netcdf_io::netcdf_end_def( ncid_ ), "ending definitions" );
}

void Writer::put( double lon, double lat, int cohort, const Out& s )
{
  if( !path_.empty() )
    tem::diag::send_data( tem::diag::STATE_VAR, lon, lat, cohort, 0, s.v.data(), static_cast<int>( s.v.size() ) );
}

void Writer::write( double lon, double lat, int cohort, const double* v, int n )
{
  if( ncid_ < 0 )
    create( n );
  if( static_cast<size_t>( n ) != nitem_ )
    die( "the state of a cohort changed its length" );
  const size_t r = nrec_++;
  check( nc_put_var1_double( ncid_, vlon_, &r, &lon ), "writing lon" );
  check( nc_put_var1_double( ncid_, vlat_, &r, &lat ), "writing lat" );
  check( nc_put_var1_int( ncid_, vcoh_, &r, &cohort ), "writing cohort" );
  const size_t start[2] = { r, 0 }, count[2] = { 1, nitem_ };
  check( nc_put_vara_double( ncid_, vstate_, start, count, v ), "writing state" );
}

void Writer::close()
{
  tem::diag::end(); // rank 0 writes what is still on its way from the other ranks
  if( ncid_ >= 0 )
    check( nc_close( ncid_ ), "closing " + path_ );
  ncid_ = -1;
}

bool Reader::Key::operator<( const Key& o ) const
{
  if( lon != o.lon )
    return lon < o.lon;
  if( lat != o.lat )
    return lat < o.lat;
  return cohort < o.cohort;
}

bool Reader::open( const std::string& path )
{
  if( nc_open( path.c_str(), NC_NOWRITE, &ncid_ ) != NC_NOERR )
    return false;
  int dr, ditem, vlon, vlat, vcoh;
  size_t nrec;
  check( nc_inq_dimid( ncid_, "record", &dr ), "reading " + path );
  nc_inq_dimlen( ncid_, dr, &nrec );
  nc_inq_dimid( ncid_, "item", &ditem );
  nc_inq_dimlen( ncid_, ditem, &nitem_ );
  nc_inq_varid( ncid_, "lon", &vlon );
  nc_inq_varid( ncid_, "lat", &vlat );
  nc_inq_varid( ncid_, "cohort", &vcoh );
  nc_inq_varid( ncid_, "state", &vstate_ );
  std::vector<double> lon( nrec ), lat( nrec );
  std::vector<int> coh( nrec );
  if( nrec > 0 )
  {
    nc_get_var_double( ncid_, vlon, &lon[0] );
    nc_get_var_double( ncid_, vlat, &lat[0] );
    nc_get_var_int( ncid_, vcoh, &coh[0] );
  }
  for( size_t k = 0; k < nrec; ++k )
  {
    Key key;
    key.lon = scaled( lon[k] );
    key.lat = scaled( lat[k] );
    key.cohort = coh[k];
    index_.push_back( std::make_pair( key, k ) );
  }
  std::sort( index_.begin(), index_.end(),
             []( const std::pair<Key, size_t>& a, const std::pair<Key, size_t>& b ) { return a.first < b.first; } );
  row_.resize( nitem_ );
  return true;
}

bool Reader::get( double lon, double lat, int cohort, In& s )
{
  Key key;
  key.lon = scaled( lon );
  key.lat = scaled( lat );
  key.cohort = cohort;
  std::vector<std::pair<Key, size_t> >::const_iterator it = std::lower_bound(
      index_.begin(), index_.end(), std::make_pair( key, size_t( 0 ) ),
      []( const std::pair<Key, size_t>& a, const std::pair<Key, size_t>& b ) { return a.first < b.first; } );
  if( it == index_.end() || it->first < key || key < it->first )
    return false;
  const size_t start[2] = { it->second, 0 }, count[2] = { 1, nitem_ };
  check( nc_get_vara_double( ncid_, vstate_, start, count, &row_[0] ), "reading state" );
  s.v = &row_;
  s.pos = 0;
  return true;
}

void Reader::close()
{
  if( ncid_ >= 0 )
    nc_close( ncid_ );
  ncid_ = -1;
}

} // namespace state
} // namespace tem
