/* **************************************************************
*****************************************************************
TTIMESTEP45.H - object describes the calendar and the length of
                the model step in seconds

Time is kept in seconds (long long), as in the FMS time_manager of
SHiELD (set_time normalizes seconds and days; the run length and
dt_atmos of its namelist are converted to seconds), on the noleap
calendar of the model (365 days a year, the month lengths below).
Units are second, minute, hour, day, week, month and year (singular
or plural). A step is either a calendar month (run_dt of n months)
or a fixed number of seconds that divides the year; every
rate, span or count that depends on the step is converted from
its own time unit with rateToStep, secondsToSteps or
stepShareOfMonth, so no formula has a monthly and a daily branch.

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Created: The calendar of a model step (noleap), set by run_dt and run_dt_units; runSTEPS converts a
         run length in days, months or years to steps.
20241109 Updated: isMONTHLY, monthRATE and getSTEPDAYS give the step-dependent rates at run time.
20241110 Changed: monthRATE is 12 / steps per year (1 for monthly steps, 12/365 for daily), so one formula serves every
         step; monthsToSteps and shareOfMonth are added. The daily rates no longer use the divisor 30.5 of TEM_Daily.
20241110 Changed: All times are seconds. run_dt and run_length accept seconds, minutes, hours, days, weeks, months and
         years; runSECONDS converts a run length (calendar months from 1 January). monthRATE, monthsToSteps,
         shareOfMonth and getSTEPDAYS are replaced by rateToStep, secondsToSteps, stepShareOfMonth and
         getSTEPSECONDS; supportedStep reports the steps the model physics can run.
20241110 Changed: Header: time in seconds (tem_time); rateToStep, secondsToSteps, stepShareOfMonth.

*****************************************************************
************************************************************** */

#ifndef TTIMESTEP45_H
#define TTIMESTEP45_H

#include "tem_time.h"

#include <cmath>
#include <string>

class Timestep45
{

  public:

     Timestep45() : dtsec( tem_time::SECONDS_PER_MONTH ), nsteps( 12 ), monthly( true ) {};

/* *************************************************************
		 Public Functions
************************************************************* */

     // A step of dt units: n calendar months (n divides 12) or a number of seconds that divides the
     //   year. Returns false for another unit or a step that does not divide the year evenly.

     bool setSTEP( const long long& dt, std::string units )
     {
       using namespace tem_time;

       if( dt < 1 ) { return false; }

       units = normalizeUnit( units );

       monthly = ( "month" == units );

       if( monthly )
       {
         if( 0 != 12 % dt ) { return false; }

         nsteps = 12 / dt;
         dtsec = SECONDS_PER_YEAR / nsteps;
         return true;
       }

       long long s;

       if( !unitToSeconds( dt, units, &s ) || s > SECONDS_PER_YEAR ) { return false; }

       dtsec = s;
       nsteps = static_cast<int>( SECONDS_PER_YEAR / s );

       return 0 == SECONDS_PER_YEAR % s;
     };

     // Empty when the model physics can run this step: calendar months, or a whole number of days of at most
     //   28 (the forcing, the day length and the daily rates are per day). Otherwise the reason.

     std::string supportedStep( void ) const
     {
       using namespace tem_time;

       if( monthly ) { return ""; }

       if( 0 != dtsec % SECONDS_PER_DAY )
         return "steps shorter than a day need sub-daily forcing, which the model does not read";

       if( dtsec > monthSeconds( 1 ) )
         return "steps longer than 28 days must be calendar months (run_dt_units: month)";

       return "";
     };

     inline int getNSTEPS( void ) const { return nsteps; };

     // True for steps of calendar months

     inline bool isMONTHLY( void ) const { return monthly; };

     // Nominal seconds of a step: the year divided by the steps of a year (a mean month for monthly steps)

     inline long long getSTEPSECONDS( void ) const { return dtsec; };

     // Seconds of step i of the year (the days of its months for monthly steps)

     inline long long getSTEPSECONDS( const int& i ) const
     {
       if( !monthly ) { return dtsec; }

       const int n = 12 / nsteps;
       long long s = 0;

       for( int m = i * n; m < (i + 1) * n; ++m ) { s += tem_time::monthSeconds( m ); }

       return s;
     };

     // A rate (or a threshold) given per unitseconds, for one step: rate * step / unitseconds. The constants
     //   of the model are per month (SECONDS_PER_MONTH) or per year (SECONDS_PER_YEAR); a monthly step is
     //   one month, so a monthly rate is unchanged at monthly steps.

     inline double rateToStep( const double& rate, const long long& unitseconds ) const
     {
       return rate * ( static_cast<double>( dtsec ) / static_cast<double>( unitseconds ) );
     };

     // A span of the given seconds in steps of the run (half a month is 0.5 monthly steps, 15.2 daily steps)

     inline double secondsToSteps( const double& seconds ) const { return seconds / static_cast<double>( dtsec ); };

     // Share of the given month (1 to 12) that a step covers; 1 for monthly steps

     inline double stepShareOfMonth( const int& month ) const
     {
       return monthly ? 1.0 : static_cast<double>( dtsec ) / static_cast<double>( tem_time::monthSeconds( month - 1 ) );
     };

     // Seconds in a run of n units from 1 January (months are the calendar months of the noleap year).
     //   Returns -1 for another unit or a length below one.

     long long runSECONDS( const long long& n, const std::string& units ) const
     {
       using namespace tem_time;

       if( n < 1 ) { return -1; }

       const std::string u = normalizeUnit( units );

       if( "month" == u )
       {
         long long s = SECONDS_PER_YEAR * ( n / 12 );

         for( long long m = 0; m < n % 12; ++m ) { s += monthSeconds( static_cast<int>( m ) ); }

         return s;
       }

       long long s;

       return unitToSeconds( n, u, &s ) ? s : -1;
     };

     // Number of steps in a run of n units from 1 January. Returns -1 when the run is not a whole number
     //   of steps, for example 5 days at a monthly step.

     long runSTEPS( const long long& n, const std::string& units ) const
     {
       const long long total = runSECONDS( n, units );

       if( total < 1 ) { return -1; }

       long long sum = 0;
       long steps = 0;

       while( sum < total )
       {
         sum += getSTEPSECONDS( static_cast<int>( steps % nsteps ) );
         ++steps;
       }

       return ( sum == total ) ? steps : -1;
     };

     // Days in step i of the year

     inline double getDAYS( const int& i ) const
     {
       return static_cast<double>( getSTEPSECONDS( i ) ) / static_cast<double>( tem_time::SECONDS_PER_DAY );
     };

     // Calendar month (0-11) containing the start of step i

     inline int getMONTH( const int& i ) const
     {
       if( monthly ) { return i * (12 / nsteps); }

       long long s = static_cast<long long>( i ) * dtsec;
       int m = 0;

       while( m < 11 && s >= tem_time::monthSeconds( m ) ) { s -= tem_time::monthSeconds( m ); ++m; }

       return m;
     };

     inline bool isYEARSTART( const int& i ) const { return 0 == i; };

     inline bool isYEAREND( const int& i ) const { return nsteps - 1 == i; };

     // True when step i starts calendar month m

     inline bool isMONTHSTART( const int& i, const int& m ) const
     {
       return getMONTH( i ) == m && (0 == i || getMONTH( i - 1 ) != m);
     };

     // True when step i is in calendar month m

     inline bool isMONTH( const int& i, const int& m ) const { return getMONTH( i ) == m; };

     inline static int getMONTHDAYS( const int& m )
     {
       return static_cast<int>( tem_time::monthSeconds( m ) / tem_time::SECONDS_PER_DAY );
     };


  private:

     long long dtsec;  // nominal seconds of a step
     int nsteps;       // steps per year
     bool monthly;     // steps are calendar months

};

// The step of the run, set once from run_dt and run_dt_units

inline Timestep45& getTimestep45( void )
{
  static Timestep45 ts;

  return ts;
};

#endif
