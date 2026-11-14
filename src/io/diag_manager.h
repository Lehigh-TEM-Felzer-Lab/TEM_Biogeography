// diag_manager.h
// Output collection for TEM, adapted from the FMS diag_manager and fms2_io (NOAA-GFDL/FMS): every process sends its data with
// send_data, and one I/O root (rank 0) writes the one output file. There is no file per process and no merge at the end of
// the run. FMS does the same with io_layout = (1,1): the ranks of the I/O group gather their data on the root with mpp_gather
// and only the root calls the netCDF library (fms2_io/include/domain_write.inc).
//
// What differs from FMS: FMS ranks run in step and gather one time level of a field at a time. TEM ranks run their own cells
// from the first year to the last, so a rank collects the series of the cells it has finished and sends them in batches
// (BATCH bytes) with a non-blocking send (MPI_Isend). With two or more ranks, rank 0 runs no cells: it reads the inputs, checks
// them, writes the log and the output file, and the other ranks compute. It has a non-blocking receive (MPI_Irecv) posted for every
// other rank, so a sender never waits for it, and it sleeps in MPI_Waitany between batches.
//
// What is not taken over: an io_layout larger than (1,1) (several I/O roots that write tile files to be combined afterwards
// with mppnccombine) and collective netCDF writes.
//
// A serial run and a run with one rank have no messages: send_data calls the sink directly.
//
// Version: XTEM version 4.5
// 20241113 Created.
#ifndef DIAG_MANAGER_H
#define DIAG_MANAGER_H

#include <functional>
#include <string>

namespace tem
{
namespace diag
{

// Writes one record: the model steps of variable var in one cell and cohort (cohort 0: climate variable) in one year.
// Called on rank 0 only.
typedef std::function<void( const std::string& var, double lon, double lat, int cohort, int year, const double* values, int n )>
    Sink;

// Collective. Registers the sink that writes on rank 0 (the other ranks never call it).
void init( Sink sink );

// Name of the records of the TEM state file. They share the transport with the output records, so there is one set of
// messages and one rank 0 that writes; they go to the sink of init_state (cohort and values are the record, year is 0).
const char* const STATE_VAR = "\x01state";

// Collective. Registers the sink that writes the state file on rank 0.
void init_state( Sink sink );

// Any rank: hand over the n values of var for one cell, cohort and year. The values are copied.
void send_data( const std::string& var, double lon, double lat, int cohort, int year, const double* values, int n );

// Rank 0 (one-rank runs only: with more ranks rank 0 serves in end()): write the records that have arrived. The other ranks do nothing.
void poll();

// Collective, after the last send_data of every rank and before any other collective call (a barrier would stop rank 0 from
// receiving): the other ranks send what they hold, and rank 0 writes everything until each of them has sent its last batch.
// When it returns, all data have passed through the sink. A second call does nothing.
void end();

} // namespace diag
} // namespace tem

#endif
