// diag_manager.cpp
// See diag_manager.h.
//
// Version: XTEM version 4.5
// 20241113 Created.

#include "diag_manager.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "mpi_layer.h"

namespace tem
{
namespace diag
{

namespace
{

const int TAG = 7301;                      // message tag of the diag manager
// FMS (mpp_gather) sends each field once with a blocking call and has no batch size. Here a batch only has to be larger than the eager limit of MPI;
// 64 KiB to 16 MiB took the same time in the transport test, so the size follows the memory of rank 0, which holds one receive of MAXMSG for each other rank
const size_t BATCH = 256u << 10;            // bytes a rank collects before it sends (about 90 daily cell series)
const size_t MAXMSG = BATCH + ( 256u << 10 ); // largest message; a record is at most 8 bytes x steps per year (70 KB for hourly steps)
const size_t MAX_IN_FLIGHT = 4;             // sent batches not yet received that a rank lets pile up before it waits (at most 1 MiB per rank)

// A message is [int32 last][int32 nrecords] and the records. A record is [uint16 len][name][double lon][double lat]
// [int32 cohort][int32 year][int32 n][n doubles]. The last message of a rank has last = 1 (and may hold no records).
const size_t HEADER = 2 * sizeof( int32_t );

Sink& sink()
{
  static Sink s;
  return s;
}

Sink& state_sink()
{
  static Sink s;
  return s;
}

bool& active()
{
  static bool a = false;
  return a;
}

std::vector<char>& outbox()
{
  static std::vector<char> b( HEADER, 0 );
  return b;
}

int& nrecords()
{
  static int n = 0;
  return n;
}

int& ranks_done()
{
  static int n = 0;
  return n;
}

template <class T>
void put( std::vector<char>& b, const T& v )
{
  const char* p = reinterpret_cast<const char*>( &v );
  b.insert( b.end(), p, p + sizeof( T ) );
}

template <class T>
T take( const std::vector<char>& b, size_t& at )
{
  T v;
  std::memcpy( &v, b.data() + at, sizeof( T ) );
  at += sizeof( T );
  return v;
}

void flush( bool last )
{
  std::vector<char>& b = outbox();
  const int32_t l = last ? 1 : 0, n = nrecords();
  std::memcpy( b.data(), &l, sizeof l );
  std::memcpy( b.data() + sizeof l, &n, sizeof n );
  tem::mpi::wait_sends( MAX_IN_FLIGHT - 1 ); // back pressure: the root is behind
  tem::mpi::send_async( b.data(), b.size(), 0, TAG );
  b.assign( HEADER, 0 );
  nrecords() = 0;
}

void unpack( const std::vector<char>& m )
{
  size_t at = 0;
  const int32_t last = take<int32_t>( m, at ), n = take<int32_t>( m, at );
  std::vector<double> values;
  for( int32_t k = 0; k < n; ++k )
  {
    const uint16_t len = take<uint16_t>( m, at );
    const std::string var( m.data() + at, len );
    at += len;
    const double lon = take<double>( m, at ), lat = take<double>( m, at );
    const int32_t cohort = take<int32_t>( m, at ), year = take<int32_t>( m, at ), count = take<int32_t>( m, at );
    values.resize( count );
    std::memcpy( values.data(), m.data() + at, count * sizeof( double ) ); // the buffer is not aligned for double
    at += count * sizeof( double );
    ( var == STATE_VAR ? state_sink() : sink() )( var, lon, lat, cohort, year, values.data(), count );
  }
  if( last )
    ++ranks_done();
}

// Writes the next message that has arrived; false if there was none (with wait, only if there is nobody to wait for).
bool drain_one( bool wait )
{
  std::vector<char> m;
  if( !tem::mpi::recv_next( m, wait ) )
    return false;
  unpack( m );
  return true;
}

bool& finished()
{
  static bool f = false;
  return f;
}

} // namespace

namespace
{

// The first registration starts the transport; the output sink and the state sink share it.
void start()
{
  if( active() )
    return;
  active() = true;
  outbox().assign( HEADER, 0 );
  nrecords() = 0;
  ranks_done() = 0;
  finished() = false;
  if( tem::mpi::rank() == 0 && tem::mpi::size() > 1 )
    tem::mpi::recv_open( TAG, MAXMSG ); // a receive for every other rank is posted before any of them sends
}

} // namespace

void init( Sink s )
{
  start();
  sink() = s;
}

void init_state( Sink s )
{
  start();
  state_sink() = s;
}

void send_data( const std::string& var, double lon, double lat, int cohort, int year, const double* values, int n )
{
  if( tem::mpi::rank() == 0 )
  { // a serial run, or a run with one rank: no messages
    ( var == STATE_VAR ? state_sink() : sink() )( var, lon, lat, cohort, year, values, n );
    poll();
    return;
  }
  std::vector<char>& b = outbox();
  const size_t bytes = sizeof( uint16_t ) + var.size() + 2 * sizeof( double ) + 3 * sizeof( int32_t ) + n * sizeof( double );
  if( HEADER + bytes > MAXMSG )
  {
    std::cerr << "diag_manager: one record of " << bytes << " bytes does not fit a message of " << MAXMSG << " bytes\n";
    tem::mpi::abort( 1 );
  }
  if( b.size() + bytes > MAXMSG )
    flush( false );
  put<uint16_t>( b, static_cast<uint16_t>( var.size() ) );
  b.insert( b.end(), var.begin(), var.end() );
  put<double>( b, lon );
  put<double>( b, lat );
  put<int32_t>( b, cohort );
  put<int32_t>( b, year );
  put<int32_t>( b, n );
  const char* p = reinterpret_cast<const char*>( values );
  b.insert( b.end(), p, p + n * sizeof( double ) );
  ++nrecords();
  if( b.size() >= BATCH )
    flush( false );
}

void poll()
{
  if( tem::mpi::rank() != 0 || tem::mpi::size() < 2 )
    return;
  while( drain_one( false ) )
  {
  }
}

void end()
{
  if( tem::mpi::size() < 2 || finished() )
    return;
  finished() = true;
  active() = false;
  if( tem::mpi::rank() != 0 )
  {
    flush( true );
    tem::mpi::wait_sends( 0 );
    return;
  }
  while( ranks_done() < tem::mpi::size() - 1 ) // rank 0 has nothing else to do: it sleeps in MPI_Waitany until a batch arrives
    drain_one( true );
}

} // namespace diag
} // namespace tem
