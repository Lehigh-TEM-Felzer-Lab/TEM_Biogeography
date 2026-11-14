// tem_input_check.cpp - see tem_input_check.h
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Created: Grid, resolution, time axis, start year and cell checks of the NetCDF inputs.
// 20241111 Changed: the model no longer regrids inputs (model_resolution, regrid_method, regrid_dir removed): an input
//          on another grid than the land cover stops the run with a message; regrid before the run (cdo, xesmf).
// 20241111 Changed: errors are reported once, by rank 0, through tem_log.h; the message of missing input data
//          gives the number of cells, the first position and what to do.

#include "tem_input_check.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

#include <netcdf.h>
#include <sys/stat.h>

#include "mpi_layer.h"
#include "tem_log.h"

namespace tem_check
{

void fail( const std::string& msg )
{
  // every rank checks the same inputs and finds the same error: rank 0 reports it, then all ranks stop
  tem::log::fatalRoot( "input error: " + msg );
}

double spacing( std::vector<double> c )
{
  if( c.size() < 2 ) { return 0.0; }

  std::sort( c.begin(), c.end() );

  return ( c.back() - c.front() ) / static_cast<double>( c.size() - 1 );
}

std::vector<int> matchAxis( const std::string& label, const std::string& reflabel, const std::string& axis,
                            const std::vector<double>& native, const std::vector<double>& reference,
                            const double& cellwidth )
{
  if( native.size() != reference.size() )
  {
    std::ostringstream msg;
    msg << label << " has " << native.size() << " " << axis << " points, " << reflabel << " has " << reference.size()
        << ": all inputs need the same grid and resolution (set model_resolution to regrid the other inputs, or regrid with cdo or xesmf)";
    fail( msg.str() );
  }

  const double dn = spacing( native ), dr = spacing( reference );

  if( dn > 0.0 && dr > 0.0 && std::fabs( dn - dr ) > 1e-3 * dr )
  {
    std::ostringstream msg;
    msg << label << " has a " << axis << " resolution of " << dn << " degrees, " << reflabel << " has " << dr
        << ": all inputs need the same resolution (set model_resolution to regrid the other inputs, or regrid with cdo or xesmf)";
    fail( msg.str() );
  }

  const double tol = 1e-3 * std::max( cellwidth, 1e-6 ) + 1e-6;
  std::vector<int> map( reference.size(), -1 );

  for( size_t i = 0; i < reference.size(); ++i )
  {
    for( size_t j = 0; j < native.size(); ++j )
    {
      if( std::fabs( native[j] - reference[i] ) < tol )
      {
        map[i] = static_cast<int>( j );
        break;
      }
    }

    if( map[i] < 0 )
    {
      std::ostringstream msg;
      msg << label << " has no " << axis << " point at " << reference[i] << " (" << reflabel
          << " has): all inputs need the same grid points (set model_resolution to regrid the other inputs, or regrid with cdo or xesmf)";
      fail( msg.str() );
    }
  }

  return map;
}

std::string timeSpacing( const std::string& label, const std::vector<double>& v, const double& unitseconds,
                         const long long& stepseconds, const bool& monthlystep )
{
  if( v.size() < 2 ) { return "step"; }

  const double day = 86400.0;
  const double dt = static_cast<double>( stepseconds );
  std::string kind;

  for( size_t i = 1; i < v.size(); ++i )
  {
    const double s = ( v[i] - v[i - 1] ) * unitseconds;
    std::string k;

    if( !monthlystep && std::fabs( s - dt ) < 1e-3 * dt ) { k = "step"; }
    else if( s >= 28.0 * day - 1.0 && s <= 31.0 * day + 1.0 ) { k = monthlystep ? "step" : "month"; }
    else if( std::fabs( s - 365.0 * day ) < 1.0 || std::fabs( s - 366.0 * day ) < 1.0 ) { k = "year"; }

    if( k.empty() || ( !kind.empty() && k != kind ) )
    {
      std::ostringstream msg;
      msg << label << ": the time axis spacing between records " << i - 1 << " and " << i << " is " << s
          << " s; it must be the model step (" << stepseconds << " s), a calendar month (28 to 31 days) or a year, and the"
          << " same throughout";
      fail( msg.str() );
    }

    if( kind.empty() ) { kind = k; }
  }

  return kind;
}

void timeCoversRun( const std::string& label, const size_t& have, const size_t& need, const std::string& kind,
                    const std::string& runtext, const std::string& filerange, const std::string& needrange )
{
  if( have < need )  // more records than the run needs are allowed: the first ones are used
  {
    std::ostringstream msg;
    msg << label << ": the time axis has " << have << " records, but " << runtext << ", so this input (" << kind
        << ") needs at least " << need << " records.";
    if( !filerange.empty() ) { msg << " The run ends " << needrange << ", after the range of the file, " << filerange << "."; }
    msg << " Shorten run_length or use an input file that covers the run.";
    fail( msg.str() );
  }
}

void geoAxis( const std::string& label, const std::string& axisname, const std::string& axis,
              const std::string& units, const std::vector<double>& values )
{
  std::ostringstream msg;
  msg << label << ": the " << axis << " axis '" << axisname << "'";

  std::string u;
  for( size_t i = 0; i < units.size(); ++i ) { u += static_cast<char>( std::tolower( static_cast<unsigned char>( units[i] ) ) ); }

  if( !u.empty() && u.find( "degree" ) == std::string::npos && u != "degrees" && u != "deg" )
  {
    msg << " has units '" << units << "', not degrees: the model needs a regular longitude and latitude grid (regrid a projected or radian grid, for example with cdo remapbil)";
    fail( msg.str() );
  }

  const bool lon = ( axis == "longitude" );
  const double lo = lon ? -360.0 : -90.0, hi = lon ? 360.0 : 90.0;

  for( size_t i = 0; i < values.size(); ++i )
  {
    if( !( values[i] >= lo && values[i] <= hi ) )
    {
      msg << " has the value " << values[i] << " at index " << i << ", outside " << lo << " to " << hi << " degrees";
      fail( msg.str() );
    }
  }

  if( values.size() < 2 ) { return; }

  const double step = ( values.back() - values.front() ) / static_cast<double>( values.size() - 1 );

  if( step == 0.0 ) { msg << " does not change"; fail( msg.str() ); }

  for( size_t i = 1; i < values.size(); ++i )
  {
    const double d = values[i] - values[i - 1];

    if( !( d * step > 0.0 ) || std::fabs( d - step ) > 0.01 * std::fabs( step ) )
    {
      msg << " is not evenly spaced and monotonic at index " << i << " (" << values[i - 1] << " then " << values[i]
          << ", mean step " << step << "): irregular and Gaussian grids must be regridded to a regular lon/lat grid";
      fail( msg.str() );
    }
  }
}

void StartYear::check( const std::string& label, const long& year )
{
  if( -999999 == year_ )
  {
    year_ = year;
    label_ = label;
  }
  else if( year != year_ )
  {
    std::ostringstream msg;
    msg << label << ": the time axis starts in " << year << ", " << label_ << " starts in " << year_
        << ": all time-dependent inputs must start at the same date (cut the files with cdo seldate or ncks)";
    fail( msg.str() );
  }
}

void Domain::addInput( const std::string& label, const std::vector<char>& valid )
{
  labels_.push_back( label );
  valid_.push_back( valid );
}

std::vector<size_t> Domain::cells( const std::vector<double>& lon, const std::vector<double>& lat ) const
{
  std::vector<size_t> out;
  std::vector<long> missing( labels_.size(), 0 );
  std::vector<std::string> first( labels_.size() );

  for( size_t j = 0; j < nlat_; ++j )
  {
    for( size_t i = 0; i < nlon_; ++i )
    {
      const size_t k = i + j * nlon_;

      if( !land_[k] || !mask_[k] ) { continue; }

      bool ok = true;

      for( size_t s = 0; s < labels_.size(); ++s )
      {
        if( !valid_[s][k] )
        {
          ok = false;

          if( 1 == ++missing[s] )
          {
            std::ostringstream where;
            where << "lon " << lon[i] << ", lat " << lat[j];
            first[s] = where.str();
          }
        }
      }

      if( ok ) { out.push_back( k ); }
    }
  }

  size_t nland = 0;
  for( size_t k = 0; k < land_.size(); ++k ) { nland += ( land_[k] && mask_[k] ) ? 1 : 0; }

  std::ostringstream msg;
  bool bad = false;
  size_t width = 0;

  for( size_t s = 0; s < labels_.size(); ++s ) { if( missing[s] > 0 && labels_[s].size() > width ) { width = labels_[s].size(); } }

  for( size_t s = 0; s < labels_.size(); ++s )
  {
    if( missing[s] > 0 )
    {
      if( !bad )
      {
        msg << "some inputs have no data in cells that have land cover" << ( havemask_ ? " and mask > 0" : "" ) << ".\n";
      }
      bad = true;
      msg << "  " << labels_[s] << std::string( width - labels_[s].size(), ' ' ) << "  " << missing[s] << " of " << nland
          << " cells have no data, the first at " << first[s] << "\n";
    }
  }

  if( bad )
  {
    msg << "Every cell that runs needs data in every input. Fix the input so that it covers all cells with land cover (fill the gaps or regrid it), "
        << "or leave the cells out of the run with a mask (varmap: mask, a static variable that is 0 outside the run).";
    fail( msg.str() );
  }

  if( out.empty() ) { fail( "no grid cell has data in every NetCDF input" ); }

  return out;
}

std::string Domain::summary( const size_t& ncells ) const
{
  size_t nland = 0, nmask = 0;

  for( size_t k = 0; k < land_.size(); ++k )
  {
    nland += land_[k] ? 1 : 0;
    nmask += ( land_[k] && mask_[k] ) ? 1 : 0;
  }

  std::ostringstream o;
  o << nlon_ << " lon x " << nlat_ << " lat, " << ncells << " cells run; " << nlon_ * nlat_ - nland
    << " without land cover";

  if( havemask_ ) { o << ", " << nland - nmask << " outside the mask"; }

  return o.str();
}

// ---- grids and files ----------------------------------------------------------------------------------------

namespace
{
std::string low( std::string s )
{
  for( size_t k = 0; k < s.size(); ++k ) { s[k] = static_cast<char>( std::tolower( static_cast<unsigned char>( s[k] ) ) ); }
  return s;
}

std::string text( const int& nc, const int& var, const char* name )
{
  size_t len = 0;

  if( NC_NOERR != nc_inq_attlen( nc, var, name, &len ) ) { return ""; }

  std::vector<char> b( len + 1, 0 );

  if( NC_NOERR != nc_get_att_text( nc, var, name, &b[0] ) ) { return ""; }

  return std::string( &b[0], len );
}

void ok( const int& status, const std::string& what )
{
  if( NC_NOERR != status ) { fail( what + ": " + nc_strerror( status ) ); }
}

// 1 longitude, 2 latitude, 0 other: names and CF attributes, as the reader of the inputs recognises them
int role( const int& nc, const std::string& name )
{
  const std::string n = low( name );

  if( "lon" == n || "longitude" == n || "x" == n || "nav_lon" == n || "xc" == n || "long" == n ) { return 1; }
  if( "lat" == n || "latitude" == n || "y" == n || "nav_lat" == n || "yc" == n ) { return 2; }

  int v;

  if( NC_NOERR == nc_inq_varid( nc, name.c_str(), &v ) )
  {
    const std::string ax = low( text( nc, v, "axis" ) ), sn = low( text( nc, v, "standard_name" ) ), un = low( text( nc, v, "units" ) );

    if( "x" == ax || "longitude" == sn || "degrees_east" == un || "degree_east" == un ) { return 1; }
    if( "y" == ax || "latitude" == sn || "degrees_north" == un || "degree_north" == un ) { return 2; }
  }

  return 0;
}

std::vector<double> coord( const int& nc, const std::string& name, std::string* units )
{
  int v, d;
  size_t len = 0;
  ok( nc_inq_dimid( nc, name.c_str(), &d ), "dimension " + name );
  ok( nc_inq_dimlen( nc, d, &len ), "dimension " + name );
  std::vector<double> x( len );

  if( NC_NOERR != nc_inq_varid( nc, name.c_str(), &v ) ) { fail( "no coordinate variable " + name ); }

  ok( nc_get_var_double( nc, v, &x[0] ), "coordinate " + name );
  *units = text( nc, v, "units" );
  return x;
}

struct VarDims
{
  int nd, ilon, ilat;
  int dim[NC_MAX_VAR_DIMS];
};

VarDims dimsOf( const int& nc, const int& v )
{
  VarDims r;
  r.ilon = r.ilat = -1;
  ok( nc_inq_varndims( nc, v, &r.nd ), "variable" );
  ok( nc_inq_vardimid( nc, v, r.dim ), "variable" );

  for( int a = 0; a < r.nd; ++a )
  {
    char nm[NC_MAX_NAME + 1];
    ok( nc_inq_dimname( nc, r.dim[a], nm ), "dimension" );
    const int ro = role( nc, nm );

    if( 1 == ro && r.ilon < 0 ) { r.ilon = a; }
    if( 2 == ro && r.ilat < 0 ) { r.ilat = a; }
  }

  return r;
}

std::string dimName( const int& nc, const int& d )
{
  char nm[NC_MAX_NAME + 1];
  ok( nc_inq_dimname( nc, d, nm ), "dimension" );
  return nm;
}

}  // namespace

Grid readGrid( const std::string& path )
{
  int nc;
  ok( nc_open( path.c_str(), NC_NOWRITE, &nc ), "cannot open " + path );
  int nvars = 0;
  ok( nc_inq_nvars( nc, &nvars ), path );
  Grid g;

  for( int v = 0; v < nvars; ++v )
  {
    const VarDims d = dimsOf( nc, v );

    if( d.ilon < 0 || d.ilat < 0 || d.nd < 2 ) { continue; }

    g.lonname = dimName( nc, d.dim[d.ilon] );
    g.latname = dimName( nc, d.dim[d.ilat] );
    std::string ulon, ulat;
    g.lon = coord( nc, g.lonname, &ulon );
    g.lat = coord( nc, g.latname, &ulat );
    nc_close( nc );
    geoAxis( path, g.lonname, "longitude", ulon, g.lon );
    geoAxis( path, g.latname, "latitude", ulat, g.lat );
    return g;
  }

  nc_close( nc );
  fail( path + " has no variable on a longitude and latitude grid" );
}

bool sameGrid( const Grid& a, const Grid& b )
{
  if( a.lon.size() != b.lon.size() || a.lat.size() != b.lat.size() ) { return false; }

  const double dl = spacing( a.lon ), da = spacing( a.lat );

  if( std::fabs( dl - spacing( b.lon ) ) > 1e-3 * dl || std::fabs( da - spacing( b.lat ) ) > 1e-3 * da ) { return false; }

  for( size_t pass = 0; pass < 2; ++pass )
  {
    const std::vector<double>& x = 0 == pass ? a.lon : a.lat;
    const std::vector<double>& y = 0 == pass ? b.lon : b.lat;
    const double tol = 1e-3 * ( 0 == pass ? dl : da );

    for( size_t k = 0; k < x.size(); ++k )
    {
      bool found = false;

      for( size_t j = 0; j < y.size() && !found; ++j )
      {
        double diff = std::fabs( x[k] - y[j] );

        if( 0 == pass ) { diff = std::fabs( std::fmod( diff + 180.0, 360.0 ) - 180.0 ); diff = std::min( diff, std::fabs( x[k] - y[j] ) ); }

        found = diff <= tol;
      }

      if( !found ) { return false; }
    }
  }

  return true;
}

}  // namespace tem_check
