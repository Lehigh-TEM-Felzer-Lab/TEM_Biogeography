// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Updated: serial MPI layer (the default build).
// Under mpirun a serial build stops with a message instead of running every rank on all cells.

#include "mpi_layer.h"

#include <cstdlib>
#include <iostream>

namespace tem
{
namespace mpi
{

void init()
{
  const char* sizes[] = { "OMPI_COMM_WORLD_SIZE", "PMI_SIZE", "PMIX_SIZE" };
  for( const char* key : sizes )
  {
    const char* value = std::getenv( key );
    if( value && std::atoi( value ) > 1 )
    {
      std::cerr << "This is the serial TEM executable. Use make mpi and build_mpi/xtem for multiple ranks.\n";
      std::exit( EXIT_FAILURE );
    }
  }
}
void finalize()
{
}
void abort( int code )
{
  std::_Exit( code );
}
void barrier()
{
}
long broadcast( long value )
{
  return value;
}
long broadcast( long value, int )
{
  return value;
}
void broadcast( double*, size_t, int )
{
}
void send_async( const void*, size_t, int, int )
{
}

void wait_sends( size_t )
{
}

void recv_open( int, size_t )
{
}

bool recv_next( std::vector<char>&, bool )
{
  return false;
}

int rank()
{
  return 0;
}
int size()
{
  return 1;
}
const char* backend()
{
  return "serial";
}

} // namespace mpi
} // namespace tem
