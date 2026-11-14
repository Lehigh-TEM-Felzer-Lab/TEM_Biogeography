// tem_log.cpp: see tem_log.h
#include "tem_log.h"

#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <vector>

#include "mpi_layer.h"

namespace tem
{
namespace log
{
namespace
{
int fd = -1;    // tem_<id>.log
int fdOut = -1; // tem_<id>.out
std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

std::string stamp()
{
  const std::time_t t = std::time( nullptr );
  std::tm tm;
  localtime_r( &t, &tm );
  char b[24];
  std::strftime( b, sizeof b, "%Y-%m-%d %H:%M:%S", &tm );
  return b;
}

std::vector<std::string> lines( const std::string& text )
{
  std::vector<std::string> out;
  std::istringstream in( text );
  std::string l;
  while( std::getline( in, l ) )
  {
    while( !l.empty() && ( '\r' == l.back() || ' ' == l.back() ) ) { l.pop_back(); }
    if( l.size() > 1 && ' ' == l[0] && ' ' != l[1] ) { l.erase( 0, 1 ); } // the setup text starts every line with one space
    if( !l.empty() ) { out.push_back( l ); }
  }
  return out;
}

// date time | rank | level | text: one write() per line, so lines of different ranks stay whole
void emit( const char* level, const std::string& text, const int to = fd )
{
  if( to < 0 ) { return; }
  for( const std::string& l : lines( text ) )
  {
    char head[64];
    if( level ) { std::snprintf( head, sizeof head, " | rank %04d | %-5s | ", mpi::rank(), level ); }
    else { std::snprintf( head, sizeof head, " | rank %04d | ", mpi::rank() ); }
    const std::string line = stamp() + head + l + "\n";
    const ssize_t n = ::write( to, line.data(), line.size() );
    (void) n;
  }
}

std::string runId()
{
  const char* env = std::getenv( "TEM_RUN_ID" );
  if( env && *env ) { return env; }
  const std::time_t t = static_cast<std::time_t>( mpi::broadcast( static_cast<long>( std::time( nullptr ) ) ) );
  std::tm tm;
  localtime_r( &t, &tm );
  char b[24];
  std::strftime( b, sizeof b, "%Y%m%d_%H%M%S", &tm );
  return b;
}

int openAppend( const std::string& path )
{
  if( 0 == mpi::rank() )
  {
    const int f = ::open( path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644 );
    if( f >= 0 ) { ::close( f ); }
  }
  mpi::barrier(); // the file exists and is empty before any rank appends
  return ::open( path.c_str(), O_WRONLY | O_APPEND );
}

void toStderr( const char* level, const std::string& text )
{
  // the log file has the message; stderr gets a copy only at a terminal or when there is no log file
  if( fd >= 0 && !::isatty( 2 ) ) { return; }
  const std::vector<std::string> l = lines( text );
  std::ostringstream o;
  for( size_t i = 0; i < l.size(); ++i )
  {
    if( 0 == i ) { o << "tem " << level << " (rank " << mpi::rank() << "): " << l[i] << "\n"; }
    else { o << "    " << l[i] << "\n"; }
  }
  const std::string s = o.str();
  const ssize_t n = ::write( 2, s.data(), s.size() );
  (void) n;
}

// stream of the setup text: whole lines from rank 0, nothing from the other ranks
class RootBuf : public std::streambuf
{
  protected:
    int overflow( int c ) override
    {
      if( EOF != c ) { buf_.push_back( static_cast<char>( c ) ); }
      if( '\n' == c ) { flush(); }
      return c;
    }
    std::streamsize xsputn( const char* s, std::streamsize n ) override
    {
      buf_.append( s, static_cast<size_t>( n ) );
      if( buf_.find( '\n' ) != std::string::npos ) { flush(); }
      return n;
    }
    int sync() override { return 0; }

  private:
    void flush()
    {
      const size_t last = buf_.rfind( '\n' );
      if( std::string::npos == last ) { return; }
      if( 0 == mpi::rank() ) { emit( "INFO", buf_.substr( 0, last ) ); }
      buf_.erase( 0, last + 1 );
    }
    std::string buf_;
};

// std::cerr and std::cout of the model code (about 150 places, many before an exit) become log lines of the same format:
// cerr is level ERROR, cout is level INFO. Whole lines are written as they end, so nothing is lost when a process exits.
class StreamBuf : public std::streambuf
{
  public:
    explicit StreamBuf( const char* level ) : level_( level ) {}
    void flushRest() { if( !buf_.empty() ) { emit( level_, buf_ ); buf_.clear(); } }

  protected:
    int overflow( int c ) override
    {
      if( EOF == c ) { return c; }
      if( '\n' == c ) { flushRest(); } else { buf_.push_back( static_cast<char>( c ) ); }
      return c;
    }
    std::streamsize xsputn( const char* s, std::streamsize n ) override
    {
      for( std::streamsize i = 0; i < n; ++i ) { overflow( static_cast<unsigned char>( s[i] ) ); }
      return n;
    }

  private:
    const char* level_;
    std::string buf_;
};

// allocated once and never freed: cerr and cout still point to them when the process exits
StreamBuf* const errbuf = new StreamBuf( "ERROR" );
StreamBuf* const outbuf = new StreamBuf( "INFO" );
std::streambuf* oldErr = nullptr;
std::streambuf* oldOut = nullptr;

RootBuf rootbuf;
std::ostream rootstream( &rootbuf );
} // namespace

std::string open( const std::string& prefix )
{
  t0 = std::chrono::steady_clock::now();
  const std::string id = runId();
  const std::string base = prefix + "_" + id;
  fdOut = openAppend( base + ".out" );
  fd = openAppend( base + ".log" );
  if( fd >= 0 && nullptr == oldErr )
  {
    oldErr = std::cerr.rdbuf( errbuf );
    oldOut = std::cout.rdbuf( outbuf );
  }
  if( ( fd < 0 || fdOut < 0 ) && 0 == mpi::rank() ) { std::cerr << "tem WARN: cannot open " << base << ".log or .out; messages go to stderr only\n"; }
  if( 0 == mpi::rank() )
  {
    emit( "INFO", "run " + id + ", " + std::to_string( mpi::size() ) + " rank(s), backend " + mpi::backend() );
    emit( nullptr, "run " + id + ": progress of " + std::to_string( mpi::size() ) + " rank(s); details are in " + base + ".log", fdOut );
  }
  return id;
}

void close()
{
  errbuf->flushRest();
  outbuf->flushRest();
  if( oldErr ) { std::cerr.rdbuf( oldErr ); std::cout.rdbuf( oldOut ); oldErr = oldOut = nullptr; }
  if( fd >= 0 ) { ::close( fd ); fd = -1; }
  if( fdOut >= 0 ) { ::close( fdOut ); fdOut = -1; }
}

std::ostream& root() { return rootstream; }

void note( const std::string& text ) { emit( "INFO", text ); }

void progress( const std::string& text ) { emit( nullptr, text, fdOut ); }

void warn( const std::string& text )
{
  emit( "WARN", text );
  toStderr( "WARN", text );
}

void cell( const long& index, const double& lon, const double& lat, const std::string& detail )
{
  char b[160];
  std::snprintf( b, sizeof b, "cell %8ld  lon %8.2f  lat %7.2f  %s  (%s)", index, lon, lat, detail.c_str(), elapsed().c_str() );
  emit( nullptr, b, fdOut );
}

std::string elapsed()
{
  const double s = std::chrono::duration<double>( std::chrono::steady_clock::now() - t0 ).count();
  char b[40];
  if( s < 60.0 ) { std::snprintf( b, sizeof b, "%.1f s", s ); }
  else if( s < 3600.0 ) { std::snprintf( b, sizeof b, "%d min %02d s", static_cast<int>( s / 60 ), static_cast<int>( s ) % 60 ); }
  else { std::snprintf( b, sizeof b, "%d h %02d min", static_cast<int>( s / 3600 ), static_cast<int>( s / 60 ) % 60 ); }
  return b;
}

void fatal( const std::string& text )
{
  emit( "ERROR", text );
  toStderr( "ERROR", text );
  mpi::abort( 1 );
  std::_Exit( 1 ); // not reached
}

void fatalRoot( const std::string& text )
{
  if( 0 != mpi::rank() )
  {
    // rank 0 finds the same error and aborts everyone; wait for it so that its message is written first
    for( int i = 0; i < 300; ++i ) { ::usleep( 100000 ); }
    mpi::abort( 1 );
    std::_Exit( 1 );
  }
  fatal( text );
}

} // namespace log
} // namespace tem
