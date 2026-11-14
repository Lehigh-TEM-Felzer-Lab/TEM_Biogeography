// tem_time.cpp - see tem_time.h
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Created: Conversion of time units to seconds and the decoding of CF time axes (moved from
//          tem_netcdf_io.cpp, with the unit spellings and the calendars normalized in one place).

#include "tem_time.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

namespace tem_time
{

const int MDAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

namespace
{

[[noreturn]] void fail( const std::string& msg )
{
  std::cerr << std::endl << "time: " << msg << std::endl;
  std::exit( -1 );
}

std::string lower( std::string s )
{
  for( size_t i = 0; i < s.size(); ++i ) { s[i] = static_cast<char>( std::tolower( static_cast<unsigned char>( s[i] ) ) ); }
  return s;
}

// days since 1970-01-01 of a proleptic Gregorian date (H. Hinnant)
long days_from_civil( long y, int m, int d )
{
  y -= m <= 2;
  const long era = ( y >= 0 ? y : y - 399 ) / 400;
  const long yoe = y - era * 400;
  const long doy = ( 153 * ( m + ( m > 2 ? -3 : 9 ) ) + 2 ) / 5 + d - 1;
  const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

void civil_from_days( long z, long* y, int* m )
{
  z += 719468;
  const long era = ( z >= 0 ? z : z - 146096 ) / 146097;
  const long doe = z - era * 146097;
  const long yoe = ( doe - doe / 1460 + doe / 36524 - doe / 146096 ) / 365;
  const long yy = yoe + era * 400;
  const long doy = doe - ( 365 * yoe + yoe / 4 - yoe / 100 );
  const long mp = ( 5 * doy + 2 ) / 153;
  *m = static_cast<int>( mp < 10 ? mp + 3 : mp - 9 );
  *y = yy + ( *m <= 2 );
}

}  // namespace

std::string normalizeUnit( const std::string& unit )
{
  std::string u = lower( unit );

  while( !u.empty() && ' ' == u[0] ) { u.erase( 0, 1 ); }
  while( !u.empty() && ' ' == u[u.size() - 1] ) { u.erase( u.size() - 1 ); }

  if( "s" == u || "sec" == u || "secs" == u || "second" == u || "seconds" == u ) { return "second"; }
  if( "min" == u || "mins" == u || "minute" == u || "minutes" == u ) { return "minute"; }
  if( "h" == u || "hr" == u || "hrs" == u || "hour" == u || "hours" == u ) { return "hour"; }
  if( "d" == u || "day" == u || "days" == u ) { return "day"; }
  if( "week" == u || "weeks" == u ) { return "week"; }
  if( "month" == u || "months" == u ) { return "month"; }
  if( "yr" == u || "yrs" == u || "year" == u || "years" == u ) { return "year"; }

  return "";
}

bool unitToSeconds( const long long& n, const std::string& unit, long long* seconds )
{
  const std::string u = normalizeUnit( unit );
  long long per;

  if( "second" == u ) { per = 1; }
  else if( "minute" == u ) { per = SECONDS_PER_MINUTE; }
  else if( "hour" == u ) { per = SECONDS_PER_HOUR; }
  else if( "day" == u ) { per = SECONDS_PER_DAY; }
  else if( "week" == u ) { per = SECONDS_PER_WEEK; }
  else if( "month" == u ) { per = SECONDS_PER_MONTH; }
  else if( "year" == u ) { per = SECONDS_PER_YEAR; }
  else { return false; }

  const long long big = std::numeric_limits<long long>::max();

  if( n > big / per || n < -( big / per ) ) { return false; }

  *seconds = n * per;

  return true;
}

std::string normalizeCalendar( const std::string& calendar )
{
  const std::string c = calendar.empty() ? "standard" : lower( calendar );

  if( "noleap" == c || "365_day" == c ) { return "noleap"; }
  if( "360_day" == c ) { return "360_day"; }
  if( "standard" == c || "gregorian" == c || "proleptic_gregorian" == c ) { return "standard"; }

  fail( "calendar '" + calendar + "' is not supported (use standard, noleap or 360_day)" );
}

bool leapYear( const long& y )
{
  return ( y % 4 == 0 && y % 100 != 0 ) || y % 400 == 0;
}

int daysInMonth( const long& year, const int& month, const std::string& calendar )
{ // month 1..12
  const std::string cal = normalizeCalendar( calendar );

  if( "360_day" == cal ) { return 30; }
  if( "noleap" == cal ) { return MDAYS[month - 1]; }

  return MDAYS[month - 1] + ( month == 2 && leapYear( year ) ? 1 : 0 );
}

CFBase parseCFUnits( const std::string& units, const std::string& calendar )
{
  CFBase tb;
  tb.cal = normalizeCalendar( calendar );

  const std::string u = lower( units );
  const size_t p = u.find( " since " );

  if( p == std::string::npos ) { fail( "time units '" + units + "' are not of the form '<unit> since <date>'" ); }

  const std::string unit = normalizeUnit( u.substr( 0, p ) );

  if( unit.empty() ) { fail( "time unit '" + u.substr( 0, p ) + "' is not supported" ); }

  tb.unit = unit + "s";

  std::string rest = u.substr( p + 7 );
  long y = 0;
  int m = 1, d = 1, hh = 0, mm = 0;
  double ss = 0;
  char c1, c2, c3;
  std::istringstream is( rest );

  is >> y >> c1 >> m >> c2 >> d;
  (void) c1;
  (void) c2;

  while( is.peek() == ' ' || is.peek() == 't' ) { is.get(); }

  if( is.good() && std::isdigit( is.peek() ) ) { is >> hh >> c3 >> mm >> c3 >> ss; }

  tb.y = y;
  tb.m = m;
  tb.d = d;
  tb.secs = hh * 3600.0 + mm * 60.0 + ss;

  return tb;
}

double cfUnitSeconds( const CFBase& base )
{
  long long s = 0;

  unitToSeconds( 1, base.unit, &s );

  return static_cast<double>( s );
}

// (year, month 1..12) of a time value
void decodeCF( const CFBase& tb, const double& v, long* year, int* month )
{
  if( tb.unit == "years" )
  {
    *year = tb.y + static_cast<long>( std::floor( v + 1e-9 ) );
    *month = 1;
    return;
  }

  if( tb.unit == "months" )
  {
    const long n = ( tb.y * 12 + ( tb.m - 1 ) ) + static_cast<long>( std::floor( v + 1e-9 ) );
    *year = n / 12;
    *month = static_cast<int>( n % 12 ) + 1;
    return;
  }

  // other units in days: seconds of the unit over the seconds of a day (week included)
  double days = v * cfUnitSeconds( tb ) / static_cast<double>( SECONDS_PER_DAY );

  days += tb.secs / 86400.0;

  if( tb.cal == "360_day" )
  {
    const long n = ( tb.y * 360 + ( tb.m - 1 ) * 30 + ( tb.d - 1 ) ) + static_cast<long>( std::floor( days + 1e-9 ) );
    *year = n / 360;
    *month = static_cast<int>( ( n % 360 ) / 30 ) + 1;
  }
  else if( tb.cal == "noleap" )
  {
    int doy0 = tb.d - 1;

    for( int i = 0; i < tb.m - 1; ++i ) { doy0 += MDAYS[i]; }

    const long n = tb.y * 365 + doy0 + static_cast<long>( std::floor( days + 1e-9 ) );
    long yy = n / 365;
    int doy = static_cast<int>( n % 365 ), mo = 0;

    while( doy >= MDAYS[mo] )
    {
      doy -= MDAYS[mo];
      ++mo;
    }

    *year = yy;
    *month = mo + 1;
  }
  else
  {
    civil_from_days( days_from_civil( tb.y, tb.m, tb.d ) + static_cast<long>( std::floor( days + 1e-9 ) ), year, month );
  }
}

}  // namespace tem_time
