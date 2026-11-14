// mpi_layer.h - the only interface the model uses for the parallel runtime, as FMS mpp does. Two
// implementations: mpi_serial.cpp (no MPI, default) and mpi_parallel.cpp (make mpi).
// Under MPI the cells of the run are dealt to the processes in turn; the output of every process goes to rank 0, which writes the one
// output file (diag_manager.h).
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241113 Added: point-to-point byte messages (send_async, wait_sends, recv_open, recv_next) for the diag manager; no-ops in the serial build.
// 20241113 Added: broadcast from any root and of arrays (netcdf_io).
// 20241111 Added: broadcast(); abort(); a process that leaves with exit() while MPI runs aborts the others.
// 20241109 Updated: MPI layer; the cells of the run are shared between processes.
#ifndef MPI_LAYER_H
#define MPI_LAYER_H

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace tem
{
namespace mpi
{

void init();           // start the runtime
void finalize();       // stop it
void barrier();        // wait for all processes
long broadcast( long value ); // the value of rank 0, on every rank
long broadcast( long value, int root ); // the value of rank root, on every rank
void broadcast( double* data, size_t n, int root ); // the n values of rank root, on every rank
// Byte messages to rank 0, used by the diag manager. Both sides are non-blocking.
// send_async copies the data and returns at once (MPI_Isend); the copy is kept until the message is delivered.
void send_async( const void* data, size_t n, int dest, int tag );
void wait_sends( size_t keep );                           // return when at most keep sent messages are still undelivered
// Rank 0: post one receive of at most capacity bytes for every other rank (MPI_Irecv), so that a sender never waits for rank 0 to
// reach a receive call. recv_next hands out the next message that has arrived, in arrival order, and posts that slot again.
// With wait it blocks until a message arrives (MPI_Waitany); without, it returns false when none has (MPI_Testany).
void recv_open( int tag, size_t capacity );
bool recv_next( std::vector<char>& message, bool wait );
// All of these do nothing in the serial build, where nobody sends.
[[noreturn]] void abort( int code ); // stop every process at once (MPI_Abort); a serial run exits
int rank();            // index of this process, 0 .. size()-1
int size();            // number of processes
const char* backend(); // "serial" or "mpi"

// File name of process r: the tag goes before the extension, so tem.nc becomes tem.0003.nc
inline std::string rankPath( const std::string& path, int r )
{
  char tag[16];
  std::snprintf( tag, sizeof tag, ".%04d", r );
  const size_t dot = path.rfind( '.' ), slash = path.find_last_of( '/' );
  if( dot == std::string::npos || ( slash != std::string::npos && dot < slash ) )
    return path + tag;
  return path.substr( 0, dot ) + tag + path.substr( dot );
}

} // namespace mpi
} // namespace tem

#endif
