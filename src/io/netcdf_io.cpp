// netcdf_io.cpp - see netcdf_io.h
//
// Version: XTEM version 4.5
// 20241113 Created: adapted from fms2_io (netcdf_io.F90, netcdf_read_data.inc) of FMS 2026.03.

#include "netcdf_io.h"
#include "mpi_layer.h"

#include <netcdf.h>
#ifdef TEM_NETCDF_PAR
#include <mpi.h>
#include <netcdf_par.h>
#endif

#include <algorithm>
#include <cstdio>
#include <map>

namespace tem
{
namespace netcdf_io
{

namespace
{
NetcdfSettings g_settings;

std::map<int, NetcdfFile_t>& files()
{
  static std::map<int, NetcdfFile_t> f;
  return f;
}
} // namespace

void netcdf_io_init( const NetcdfSettings& s )
{
  g_settings = s;
}

const NetcdfSettings& netcdf_io_settings()
{
  return g_settings;
}

int open_file( NetcdfFile_t& fileobj, const std::string& path, const std::string& mode )
{
  fileobj.path = path;
  fileobj.pelist.resize( static_cast<size_t>( mpi::size() ) );
  for( size_t r = 0; r < fileobj.pelist.size(); ++r )
    fileobj.pelist[r] = static_cast<int>( r );
  fileobj.io_root = fileobj.pelist.front();
  fileobj.is_root = ( mpi::rank() == fileobj.io_root );
  fileobj.use_netcdf_mpi = g_settings.use_netcdf_mpi && mode == "read";
  fileobj.use_collective = fileobj.use_netcdf_mpi && g_settings.use_collective;
  const int flag = ( mode == "write" ) ? NC_WRITE : NC_NOWRITE;
  int status;
  if( fileobj.use_netcdf_mpi )
  {
#ifdef TEM_NETCDF_PAR
    status = nc_open_par( path.c_str(), flag | NC_MPIIO, MPI_COMM_WORLD, MPI_INFO_NULL, &fileobj.ncid );
#else
    static bool said = false;
    if( !said && fileobj.is_root )
      std::fprintf( stderr, "use_netcdf_mpi is set but this build has no parallel NetCDF (make NETCDF_PAR=1): reading without it\n" );
    said = true;
    fileobj.use_netcdf_mpi = fileobj.use_collective = false;
    size_t chunksize = g_settings.ncchksz;
    status = nc__open( path.c_str(), flag, &chunksize, &fileobj.ncid );
#endif
  }
  else
  {
    size_t chunksize = g_settings.ncchksz;
    status = nc__open( path.c_str(), flag, &chunksize, &fileobj.ncid );
  }
  if( status == NC_NOERR )
    files()[fileobj.ncid] = fileobj;
  return status;
}

int close_file( const NetcdfFile_t& fileobj )
{
  files().erase( fileobj.ncid );
  return nc_close( fileobj.ncid );
}

int netcdf_read_data( int ncid, int varid, const size_t* corner, const size_t* edge_lengths, double* buf, size_t n,
                      bool broadcast )
{
  const std::map<int, NetcdfFile_t>::const_iterator it = files().find( ncid );
  const bool known = it != files().end();
#ifdef TEM_NETCDF_PAR
  if( known && it->second.use_netcdf_mpi )
  { // every process reads its own hyperslab; collective reads need the same call on every process
    nc_var_par_access( ncid, varid, ( broadcast && it->second.use_collective ) ? NC_COLLECTIVE : NC_INDEPENDENT );
    return nc_get_vara_double( ncid, varid, corner, edge_lengths, buf );
  }
#endif
  if( broadcast && g_settings.netcdf_read_broadcast && known && mpi::size() > 1 )
  {
    const NetcdfFile_t& f = it->second;
    long status = NC_NOERR;
    if( f.is_root )
      status = nc_get_vara_double( ncid, varid, corner, edge_lengths, buf );
    status = mpi::broadcast( status, f.io_root );
    if( status != NC_NOERR )
      return static_cast<int>( status );
    mpi::broadcast( buf, n, f.io_root );
    return NC_NOERR;
  }
  return nc_get_vara_double( ncid, varid, corner, edge_lengths, buf );
}

int netcdf_end_def( int ncid )
{
  return nc__enddef( ncid, static_cast<size_t>( std::max( 0, g_settings.header_buffer_val ) ), 4, 0, 4 );
}

int netcdf_add_variable( int ncid, int varid )
{
  const int level = std::min( 9, std::max( 0, g_settings.deflate_level ) );
  if( level == 0 && !g_settings.shuffle )
    return NC_NOERR;
  return nc_def_var_deflate( ncid, varid, g_settings.shuffle ? 1 : 0, level > 0 ? 1 : 0, level );
}

int find_buf_index( int key, const std::vector<int>& ibuf )
{
  for( size_t i = 0; i < ibuf.size(); ++i )
    if( ibuf[i] == key )
      return static_cast<int>( i );
  return -1;
}

} // namespace netcdf_io
} // namespace tem
