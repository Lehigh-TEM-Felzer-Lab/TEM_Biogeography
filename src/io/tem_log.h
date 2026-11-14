// tem_log.h: the run files of all processes, after the logging of FMS (mpp_error, stdlog).
//
// FMS lets the root process write the run log and prints a fatal error once, with the process number, before it
// aborts all processes. XTEM does the same, with two files per run that share a run id (<id>):
//   - tem_<id>.out is the progress: the time, the cell (longitude and latitude) each process is working on, the
//     cells finished and the run time.
//   - tem_<id>.log is everything else: setup, grid, cohorts, warnings and errors.
//   The id is TEM_RUN_ID when that is set (a launcher can set it and so knows the file names), else the date and time the run
//   started (YYYYMMDD_HHMMSS, taken on rank 0 and shared by all ranks). All processes append whole lines to the same two files
//   (O_APPEND, one write per line), so lines of different processes never mix.
//   - root() is the stream for the setup and summary text (to the .log). Only rank 0 writes it, because every rank would
//     write the same lines.
//   - note() and warn() (.log), cell() and progress() (.out) write from any rank and name the rank.
//   - fatal() writes the message to the .log (and to stderr when stderr is a terminal or there is no log file), once, then stops every process
//     (MPI_Abort). fatalRoot() is for errors found by every rank with the same text (the inputs): rank 0 reports, the others wait for the abort.
//   - A process that leaves with exit() while MPI is running aborts all the others, so no rank waits in a barrier
//     for a rank that has gone (the cause of hangs after an error in one rank).
//   - A launcher appends its own stdout and stderr (MPI or srun messages, a crash) to the .log.
//
// Version: XTEM version 4.5
// 20241111 Added: .out progress file and .log file with a run id, rank 0 setup text, fatal errors that stop all ranks.
#ifndef TEM_LOG_H
#define TEM_LOG_H

#include <ostream>
#include <string>

namespace tem
{
namespace log
{

// Creates tem_<id>.out and tem_<id>.log in the working directory (rank 0 creates them); call after mpi::init(), before any
// other call. A failure to open writes nothing to a file; fatal() always reaches stderr. Returns the run id.
std::string open( const std::string& prefix = "tem" );
void close();

// Setup and summary text, written by rank 0 only; lines end with std::endl
std::ostream& root();

void note( const std::string& text );             // any rank, level INFO, to the .log
void progress( const std::string& text );         // any rank, to the .out
void warn( const std::string& text );             // any rank, level WARN, also on stderr
void cell( const long& index, const double& lon, const double& lat, const std::string& detail ); // progress of a cell, to the .out

[[noreturn]] void fatal( const std::string& text );      // any rank: log, stderr, abort all
[[noreturn]] void fatalRoot( const std::string& text );  // same text on every rank: rank 0 reports

// "12.3 s", "4 min 05 s", "2 h 03 min": seconds since open()
std::string elapsed();

} // namespace log
} // namespace tem

#endif
