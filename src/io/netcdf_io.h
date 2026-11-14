// netcdf_io.h - NetCDF file handling of the model, adapted from the fms2_io module of FMS (NOAA-GFDL/FMS, release 2026.03:
// fms2_io/netcdf_io.F90, fms2_io/include/netcdf_read_data.inc, time_interp/time_interp_external2.F90), with the names of
// FMS without the fms2_ prefix.
//
// netcdf_io_init        sets the library parameters of the run (the fms2_io_nml keys ncchksz, header_buffer_val, deflate_level,
//                       shuffle, and the time_interp_external_nml key num_io_buffers) from tem_config.yaml
// open_file             opens an input, plain or through the MPI-IO driver of HDF5 (use_netcdf_mpi, use_collective)
// netcdf_read_data      reads a hyperslab; with broadcast the pelist root reads and the other processes receive the data
// netcdf_end_def        leaves define mode with header_buffer_val bytes of room in the header (NF__ENDDEF)
// netcdf_add_variable   sets deflate_level and shuffle of an output variable
// find_buf_index        finds a record, here the first longitude of a tile, in the ring of num_io_buffers buffers
//
// Version: XTEM version 4.5
// 20241113 Created: adapted from fms2_io for the slow reading of the daily climate input (every process read every column of
//          every input at the start of the run, in an order that defeated the column cache).
#ifndef NETCDF_IO_H
#define NETCDF_IO_H

#include <cstddef>
#include <string>
#include <vector>

namespace tem
{
namespace netcdf_io
{

// Settings of the run; the keys of tem_config.yaml have the same names.
struct NetcdfSettings
{
  size_t ncchksz = 64 * 1024;     // bytes: buffer size hint of nc__open (read by the classic formats; netCDF-4 files ignore it)
  int header_buffer_val = 16384;  // bytes of free space after the header of a file made in define mode (nc__enddef h_minfree)
  int deflate_level = 2;          // 0 none, 1 to 9: compression of the output variables
  bool shuffle = true;            // shuffle filter of the output variables
  int output_chunk_lonlat = 16;   // lon and lat chunk of the output variables (time is whole, cohort 1); 0: the chunking NetCDF chooses
  int num_io_buffers = 2;         // tiles of an input kept in memory; -1 reads the whole input once
  size_t io_buffer_mb = 256;      // MiB of one tile
  bool netcdf_read_broadcast = true; // the root process reads what every process needs at once and broadcasts it (the check of the inputs at the start)
  bool use_netcdf_mpi = false;    // open the inputs with the HDF5 MPI-IO driver: every process reads its own hyperslab
  bool use_collective = false;    // with use_netcdf_mpi: collective reads in the check of the inputs
};

// A file opened by open_file. The pelist holds the processes that read it together (all processes; the root is the first).
struct NetcdfFile_t
{
  std::string path;
  int ncid = -1;
  bool is_root = true;
  int io_root = 0;
  std::vector<int> pelist;
  bool use_netcdf_mpi = false;
  bool use_collective = false;
};

void netcdf_io_init( const NetcdfSettings& s );
const NetcdfSettings& netcdf_io_settings();

// NC_NOERR or the NetCDF status. use_netcdf_mpi needs a build with TEM_NETCDF_PAR (make NETCDF_PAR=1), else a message is
// written once and the file is opened without it. mode: "read" or "write".
int open_file( NetcdfFile_t& fileobj, const std::string& path, const std::string& mode = "read" );
int close_file( const NetcdfFile_t& fileobj );

// Reads count values from corner of variable varid of an open input (values converted to double). With broadcast
// (and netcdf_read_broadcast, and no MPI-IO) every process must make the same call: the pelist root reads and the others
// receive the values; a failure on the root is returned on every process.
int netcdf_read_data( int ncid, int varid, const size_t* corner, const size_t* edge_lengths, double* buf, size_t n,
                      bool broadcast );

// nc__enddef with header_buffer_val
int netcdf_end_def( int ncid );
// deflate_level and shuffle of variable varid (netCDF-4 files)
int netcdf_add_variable( int ncid, int varid );

// Index of key in ibuf, or -1
int find_buf_index( int key, const std::vector<int>& ibuf );

} // namespace netcdf_io
} // namespace tem

#endif
