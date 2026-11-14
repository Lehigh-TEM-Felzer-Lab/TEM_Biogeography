// diag_test.cpp: every record sent with tem::diag::send_data by any rank reaches the sink on rank 0 exactly once, with its values.
// Serial:  make test (build/diag_test).  MPI: mpirun -np N build_mpi/diag_test (N = 1 .. any).
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "diag_manager.h"
#include "mpi_layer.h"

namespace
{
const int NCELLS = 400, NYEARS = 3, NSTEPS = 365;
const char* const VARS[] = { "gpp", "vegc" };

double value( int cell, int cohort, int year, int var, int step )
{
  return cell * 1e3 + cohort * 1e2 + year + var * 0.5 + step * 1e-3;
}
} // namespace

int main()
{
  tem::mpi::init();
  const int rank = tem::mpi::rank(), size = tem::mpi::size();
  std::map<std::string, int> seen; // key -> times received
  int bad = 0;
  tem::diag::init( [&]( const std::string& var, double lon, double lat, int cohort, int year, const double* v, int n ) {
    const int cell = static_cast<int>( lon ), var_i = var == "gpp" ? 0 : 1;
    if( n != NSTEPS || lat != cell * 0.5 )
      ++bad;
    for( int s = 0; s < n; ++s )
      if( v[s] != value( cell, cohort, year, var_i, s ) )
      {
        ++bad;
        break;
      }
    ++seen[var + ":" + std::to_string( cell ) + ":" + std::to_string( cohort ) + ":" + std::to_string( year )];
  } );
  size_t expected = 0;
  const int first = size > 1 ? rank - 1 : 0, stride = size > 1 ? size - 1 : 1; // rank 0 is the I/O rank when there are several ranks
  for( int cell = first; first >= 0 && cell < NCELLS; cell += stride ) // the cells are dealt in turn to the compute ranks, as in the model
    for( int cohort = 1; cohort <= 1 + cell % 5; ++cohort )
      for( int year = 2000; year < 2000 + NYEARS; ++year )
        for( int var = 0; var < 2; ++var )
        {
          std::vector<double> v( NSTEPS );
          for( int s = 0; s < NSTEPS; ++s )
            v[s] = value( cell, cohort, year, var, s );
          tem::diag::send_data( VARS[var], cell, cell * 0.5, cohort, year, v.data(), NSTEPS );
          ++expected;
        }
  tem::diag::end();
  tem::mpi::barrier();
  int fail = 0;
  if( rank == 0 )
  {
    size_t total = 0;
    for( int cell = 0; cell < NCELLS; ++cell )
      total += static_cast<size_t>( 1 + cell % 5 ) * NYEARS * 2;
    size_t dup = 0;
    for( const auto& e : seen )
      dup += e.second != 1;
    std::printf( "diag_test backend=%s ranks=%d: %zu records expected, %zu distinct received, %zu duplicated, %d corrupt\n",
                 tem::mpi::backend(), size, total, seen.size(), dup, bad );
    fail = ( seen.size() != total || dup || bad ) ? 1 : 0;
  }
  else if( !seen.empty() )
    fail = 1; // only the root's sink may be called
  fail = static_cast<int>( tem::mpi::broadcast( fail ) );
  tem::mpi::finalize();
  return fail;
}
