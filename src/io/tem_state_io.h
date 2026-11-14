// tem_state_io.h - TEM state (equilibrium or a chosen year) as one NetCDF file.
//
// One record per cell and cohort: lon, lat, cohort and the state vector `state(record, item)`, double
// precision. The state of a cohort is the sequence of numbers that Telm45::writeCohortState streams into a
// state::Out and Telm45::readCohortState streams out of a state::In. Under MPI the compute
// ranks send their records to rank 0 through the diag manager (the way the output is collected) and rank 0 writes the one
// file; there are no files per rank and no merge. Any rank can read the file.
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Updated: state of the cohorts as one NetCDF file.
#ifndef TEM_STATE_IO_H
#define TEM_STATE_IO_H

#include <string>
#include <type_traits>
#include <vector>

namespace tem
{
namespace state
{

// Collects the numeric state vector of a cohort.
struct Out
{
  std::vector<double> v;
  template <class T>
  typename std::enable_if<std::is_arithmetic<T>::value, Out&>::type operator<<( T x )
  {
    v.push_back( static_cast<double>( x ) );
    return *this;
  }
};

// Hands the numbers of a cohort back in the order they were written.
struct In
{
  const std::vector<double>* v;
  size_t pos;
  In() : v( 0 ), pos( 0 ) {}
  template <class T>
  typename std::enable_if<std::is_arithmetic<T>::value, In&>::type operator>>( T& x )
  {
    x = static_cast<T>( ( *v )[pos++] );
    return *this;
  }
};

class Writer
{
public:
  Writer() : ncid_( -1 ), nitem_( 0 ), nrec_( 0 ) {}
  void open( const std::string& path ); // the file is created on rank 0 at the first record
  void put( double lon, double lat, int cohort, const Out& s );
  void close(); // all ranks must call it: rank 0 receives what is still on its way, then closes the file
private:
  void create( size_t nitem );
  void write( double lon, double lat, int cohort, const double* v, int n ); // rank 0 only
  std::string path_;
  int ncid_, vlon_, vlat_, vcoh_, vstate_;
  size_t nitem_, nrec_;
};

class Reader
{
public:
  Reader() : ncid_( -1 ), nitem_( 0 ) {}
  bool open( const std::string& path );
  bool get( double lon, double lat, int cohort, In& s ); // false if the file has no such record
  size_t items() const { return nitem_; }
  void close();
private:
  struct Key
  {
    long lon, lat;
    int cohort;
    bool operator<( const Key& o ) const;
  };
  int ncid_, vstate_;
  size_t nitem_;
  std::vector<double> row_;
  std::vector<std::pair<Key, size_t> > index_;
};

} // namespace state
} // namespace tem

#endif
