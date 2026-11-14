// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Updated: MPI layer (make mpi); the cells of the run are shared between processes.

#include <mpi.h>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <vector>

#include "mpi_layer.h"

namespace tem
{
namespace mpi
{

namespace
{
// A rank that leaves through exit() before MPI_Finalize would leave the others waiting for it in a barrier or a
// collective call, so every exit of a running MPI program becomes an abort of all ranks.
void abortIfRunning()
{
  int started = 0, stopped = 0;
  MPI_Initialized( &started );
  MPI_Finalized( &stopped );
  if( started && !stopped )
    MPI_Abort( MPI_COMM_WORLD, 1 );
}
struct Pending
{
  std::vector<char> data;
  MPI_Request request;
};

std::deque<Pending>& pending()
{
  static std::deque<Pending> q;
  return q;
}

std::vector<std::vector<char>>& slots()
{
  static std::vector<std::vector<char>> v;
  return v;
}

std::vector<MPI_Request>& requests()
{
  static std::vector<MPI_Request> v;
  return v;
}

size_t& recv_cap()
{
  static size_t c = 0;
  return c;
}

int& recv_tag()
{
  static int t = 0;
  return t;
}

} // namespace

void init()
{
  int started = 0;
  MPI_Initialized( &started );
  if( !started )
  {
    MPI_Init( nullptr, nullptr );
    std::atexit( abortIfRunning );
  }
}
void abort( int code )
{
  MPI_Abort( MPI_COMM_WORLD, code );
  std::_Exit( code );
}
void finalize()
{
  int stopped = 0;
  MPI_Finalized( &stopped );
  if( !stopped )
    MPI_Finalize();
}
void barrier()
{
  MPI_Barrier( MPI_COMM_WORLD );
}
long broadcast( long value )
{
  MPI_Bcast( &value, 1, MPI_LONG, 0, MPI_COMM_WORLD );
  return value;
}
long broadcast( long value, int root )
{
  MPI_Bcast( &value, 1, MPI_LONG, root, MPI_COMM_WORLD );
  return value;
}
void broadcast( double* data, size_t n, int root )
{
  const size_t step = 1u << 28; // an int count: at most 2 GiB per call
  for( size_t done = 0; done < n; done += step )
    MPI_Bcast( data + done, static_cast<int>( std::min( step, n - done ) ), MPI_DOUBLE, root, MPI_COMM_WORLD );
}
void send_async( const void* data, size_t n, int dest, int tag )
{
  if( n > static_cast<size_t>( INT_MAX ) )
  {
    std::fputs( "mpi::send_async: message larger than 2 GiB\n", stderr );
    abort( 1 );
  }
  pending().emplace_back();
  Pending& p = pending().back(); // a deque keeps the address, so the buffer MPI reads from does not move
  p.data.assign( static_cast<const char*>( data ), static_cast<const char*>( data ) + n );
  MPI_Isend( p.data.data(), static_cast<int>( n ), MPI_BYTE, dest, tag, MPI_COMM_WORLD, &p.request );
}

void wait_sends( size_t keep )
{
  std::deque<Pending>& q = pending();
  while( !q.empty() )
  { // messages from one rank arrive in the order they were sent, so the oldest is the one to wait for
    if( q.size() > keep )
      MPI_Wait( &q.front().request, MPI_STATUS_IGNORE );
    else
    {
      int done = 0;
      MPI_Test( &q.front().request, &done, MPI_STATUS_IGNORE );
      if( !done )
        break;
    }
    q.pop_front();
  }
}

void recv_open( int tag, size_t capacity )
{
  int n = 1;
  MPI_Comm_size( MPI_COMM_WORLD, &n );
  slots().resize( n > 1 ? n - 1 : 0 );
  requests().assign( slots().size(), MPI_REQUEST_NULL );
  recv_cap() = capacity;
  recv_tag() = tag;
  for( size_t k = 0; k < slots().size(); ++k )
  {
    slots()[k].resize( capacity );
    MPI_Irecv( slots()[k].data(), static_cast<int>( capacity ), MPI_BYTE, static_cast<int>( k ) + 1, tag, MPI_COMM_WORLD, &requests()[k] );
  }
}

bool recv_next( std::vector<char>& message, bool wait )
{
  if( requests().empty() )
    return false;
  int index = MPI_UNDEFINED, flag = 1;
  MPI_Status st;
  if( wait )
    MPI_Waitany( static_cast<int>( requests().size() ), requests().data(), &index, &st );
  else
    MPI_Testany( static_cast<int>( requests().size() ), requests().data(), &index, &flag, &st );
  if( !flag || index == MPI_UNDEFINED )
    return false;
  int count = 0;
  MPI_Get_count( &st, MPI_BYTE, &count );
  std::vector<char>& slot = slots()[index];
  slot.resize( count );
  message.swap( slot ); // no copy; the slot gets a fresh buffer
  slot.resize( recv_cap() );
  MPI_Irecv( slot.data(), static_cast<int>( recv_cap() ), MPI_BYTE, index + 1, recv_tag(), MPI_COMM_WORLD, &requests()[index] );
  return true;
}

int rank()
{
  int r = 0;
  MPI_Comm_rank( MPI_COMM_WORLD, &r );
  return r;
}
int size()
{
  int n = 1;
  MPI_Comm_size( MPI_COMM_WORLD, &n );
  return n;
}
const char* backend()
{
  return "mpi";
}

} // namespace mpi
} // namespace tem
