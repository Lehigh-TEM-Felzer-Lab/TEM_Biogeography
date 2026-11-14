// 20241110 Changed: Time in seconds: units from second to year, run lengths in seconds, rates by rateToStep.
#include "ttimestep.h"

#include <cmath>
#include <cstdio>

int main( void )
{
  using namespace tem_time;

  Timestep45 m, d;
  int bad = 0;
  long long sum = 0;

  bad += !m.setSTEP( 1, "month" ) || 12 != m.getNSTEPS() || 28 != m.getDAYS( 1 ) || SECONDS_PER_MONTH != m.getSTEPSECONDS();
  bad += !m.isMONTHSTART( 4, 4 ) || m.isMONTHSTART( 5, 4 ) || !m.isYEAREND( 11 );
  bad += !d.setSTEP( 1, "day" ) || 365 != d.getNSTEPS() || SECONDS_PER_DAY != d.getSTEPSECONDS();

  for( int i = 0; i < 365; ++i ) { sum += d.getSTEPSECONDS( i ); }

  bad += SECONDS_PER_YEAR != sum || 0 != d.getMONTH( 30 ) || 1 != d.getMONTH( 31 ) || 11 != d.getMONTH( 364 );
  bad += !d.isMONTHSTART( 31, 1 ) || d.isMONTHSTART( 32, 1 ) || m.setSTEP( 5, "month" );
  bad += !m.setSTEP( 3, "months" ) || 4 != m.getNSTEPS() || 90 != m.getDAYS( 0 );

  // units
  bad += d.setSTEP( 1, "week" ) || !d.setSTEP( 5, "days" ) || 73 != d.getNSTEPS();
  bad += !d.setSTEP( 24, "hours" ) || 365 != d.getNSTEPS() || !d.supportedStep().empty();
  bad += !d.setSTEP( 86400, "seconds" ) || 365 != d.getNSTEPS() || !d.supportedStep().empty();
  bad += !d.setSTEP( 6, "hours" ) || 1460 != d.getNSTEPS() || d.supportedStep().empty();
  bad += !d.setSTEP( 1, "year" ) || 1 != d.getNSTEPS() || d.supportedStep().empty();
  bad += d.setSTEP( 7, "second" ) || d.setSTEP( 1, "fortnight" ) || d.setSTEP( 0, "day" );

  // one rate expression for every step: a monthly rate at monthly and daily steps
  bad += !m.setSTEP( 1, "month" ) || std::fabs( m.rateToStep( 1.0, SECONDS_PER_MONTH ) - 1.0 ) > 1e-15;
  bad += !d.setSTEP( 1, "day" ) || std::fabs( d.rateToStep( 1.0, SECONDS_PER_MONTH ) - 12.0 / 365.0 ) > 1e-15;
  bad += std::fabs( d.rateToStep( 0.11, SECONDS_PER_YEAR ) - 0.11 / 365.0 ) > 1e-15;
  bad += std::fabs( d.secondsToSteps( 0.5 * SECONDS_PER_MONTH ) - 15.2083333333333 ) > 1e-9;
  bad += std::fabs( m.stepShareOfMonth( 2 ) - 1.0 ) > 0 || std::fabs( d.stepShareOfMonth( 2 ) - 1.0 / 28.0 ) > 1e-15;

  // run lengths in seconds
  bad += !m.setSTEP( 1, "month" ) || 41 != m.runSTEPS( 41, "months" ) || 24 != m.runSTEPS( 2, "years" ) ||
         -1 != m.runSTEPS( 5, "days" ) || 4 != m.runSTEPS( 120 * SECONDS_PER_DAY, "seconds" ) ||
         SECONDS_PER_YEAR + 31 * SECONDS_PER_DAY != m.runSECONDS( 13, "months" );
  bad += !d.setSTEP( 1, "day" ) || 7 != d.runSTEPS( 7, "days" ) || 7 != d.runSTEPS( 168, "hours" ) ||
         7 != d.runSTEPS( 604800, "seconds" ) || 31 != d.runSTEPS( 1, "month" ) || -1 != d.runSTEPS( 12, "hours" );

  // any count in any unit converts to seconds; the step must still divide the year
  long long sec = 0;
  bad += !unitToSeconds( 67278272, "seconds", &sec ) || 67278272LL != sec;
  bad += !unitToSeconds( 2, "yr", &sec ) || 2 * SECONDS_PER_YEAR != sec || !unitToSeconds( 3, "hr", &sec ) || 10800 != sec;
  bad += unitToSeconds( 1, "fortnight", &sec ) || unitToSeconds( 9000000000000LL, "year", &sec );
  bad += "second" != normalizeUnit( "Secs" ) || "minute" != normalizeUnit( "min" ) || "day" != normalizeUnit( "Days" );
  bad += d.setSTEP( 67278272, "seconds" ) || 67278272LL != d.runSECONDS( 67278272, "seconds" );
  bad += !d.setSTEP( 86400, "seconds" ) || -1 != d.runSTEPS( 67278272, "seconds" ) || 778 * 86400LL != d.runSECONDS( 778, "days" );

  // CF time axes
  long yy;
  int mm;
  decodeCF( parseCFUnits( "days since 1850-01-01 00:00:00", "noleap" ), 365.0 + 31.0, &yy, &mm );
  bad += 1851 != yy || 2 != mm;
  decodeCF( parseCFUnits( "hours since 2000-01-01", "standard" ), 24.0 * 60.0, &yy, &mm );
  bad += 2000 != yy || 3 != mm;  // 1 January + 60 days of a leap year is 1 March
  decodeCF( parseCFUnits( "Seconds since 1900-03-01", "360_day" ), 30.0 * 86400.0, &yy, &mm );
  bad += 1900 != yy || 4 != mm;
  decodeCF( parseCFUnits( "months since 1990-01-01", "" ), 14.0, &yy, &mm );
  bad += 1991 != yy || 3 != mm || 29 != daysInMonth( 2000, 2, "gregorian" ) || 28 != daysInMonth( 2000, 2, "365_day" );

  std::printf( "%s\n", bad ? "FAIL" : "PASS" );

  return bad;
}
