/* **************************************************************
TEM_TIME.H - time in seconds: units, calendars and CF time axes

All user times (run_dt, run_length, the time axes of the NetCDF
inputs) are converted here to seconds on the noleap model calendar
or to a calendar date, as the FMS time_manager of SHiELD does
(set_time takes seconds and days; the calendar types are NOLEAP,
JULIAN, GREGORIAN, THIRTY_DAY_MONTHS; a time axis carries
"<unit> since <date>" and a calendar attribute, as in CF).

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Created: Unit conversion to seconds (second, minute, hour, day, week, month, year, with the CF
         spellings s, sec, min, h, hr, d, yr), noleap month lengths, and the CF "<unit> since <date>" decoding
         that was in tem_netcdf_io.cpp (standard, noleap and 360_day calendars).
**************************************************************** */

#ifndef TEM_TIME_H
#define TEM_TIME_H

#include <string>

namespace tem_time
{
  // Base unit: the second. The model calendar is noleap.
  const long long SECONDS_PER_MINUTE = 60LL;
  const long long SECONDS_PER_HOUR = 3600LL;
  const long long SECONDS_PER_DAY = 86400LL;
  const long long SECONDS_PER_WEEK = 7LL * SECONDS_PER_DAY;
  const long long SECONDS_PER_YEAR = 365LL * SECONDS_PER_DAY;
  const long long SECONDS_PER_MONTH = SECONDS_PER_YEAR / 12LL;  // mean month, 365/12 days

  extern const int MDAYS[12];  // days of the months of the noleap year

  // Seconds of calendar month m (0 to 11) of the noleap year
  inline long long monthSeconds( const int& m ) { return MDAYS[m] * SECONDS_PER_DAY; }

  // Canonical singular unit (second, minute, hour, day, week, month or year) of a spelling such as
  //   "Seconds", "sec", "hr", "days" or "yr"; empty when the unit is not known.
  std::string normalizeUnit( const std::string& unit );

  // Seconds in n units (month: the mean month, year: 365 days). Returns false for a unit that is not
  //   known or a count that does not fit in 63 bits of seconds.
  bool unitToSeconds( const long long& n, const std::string& unit, long long* seconds );

  // Calendars of the time axes of the inputs (CF): standard (gregorian), noleap (365_day), 360_day
  std::string normalizeCalendar( const std::string& calendar );
  bool leapYear( const long& y );
  int daysInMonth( const long& year, const int& month, const std::string& calendar );  // month 1 to 12

  // "<unit> since <date>" of a time axis
  struct CFBase
  {
    std::string unit;  // days, hours, minutes, seconds, months, years
    long y;
    int m, d;
    double secs;  // seconds into the base day
    std::string cal;
  };

  // Stops the run with a message when the units or the calendar are not understood
  CFBase parseCFUnits( const std::string& units, const std::string& calendar );

  // Year and month (1 to 12) of a time value of an axis
  void decodeCF( const CFBase& base, const double& value, long* year, int* month );

  // Seconds of one unit of the axis (months and years: the mean month and year); for the checks of the
  //   resolution of the time axis of an input against the step of the run
  double cfUnitSeconds( const CFBase& base );
}

#endif
