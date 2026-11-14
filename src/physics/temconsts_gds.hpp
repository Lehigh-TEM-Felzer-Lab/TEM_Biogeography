/*
  TWC/BSF 2007-2009

 MAXCHRTS: change from 650 to 300
 MAXCMNT: from 13 to 17
 MAXRTIME: from 1100 to 900
 MAXWSTAT: from 3 to 5
 NUMATMS: from 10 to 18

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241111 Changed: MAXCMNT from 23 to 24: the parameter files of GDSTEM-FIRE (params/gdstem_fire) have 23 communities.
20241109 Updated: The number of steps per year is a build option (TEM_MAXCYCLE) with CYCLE set at run time,
         and NUMTEM grows by 32 for the new output variables.
20241109 Updated: TEM_ENABLE_STORM switches the storm and hurricane code on (make STORM=1, the default) or
         off.
20241109 Changed: MAXCYCLE is 366 and not a compile-time option; CYCLE is set at run time, so one build runs
         monthly or daily steps.
20241110 Changed: MAXCYCLE, MAXRTIME and MAXCHRTS are removed. The arrays are sized at run time from CYCLE (steps
         per year), NYEARS (years held per cell) and NCOHORTS (cohort dimension of the land-cover input).

*/

#include <cstddef>
#include <vector>

// TEM_ENABLE_FIRE switches the wildfire code on (make FIRE=1, the default) or leaves it out (make FIRE=0). A build without
// fire stops with a message when the configuration sets veg_fire to 1, and does not open the fire inputs.
#ifndef TEM_ENABLE_FIRE
  #define TEM_ENABLE_FIRE 1
#endif

#ifndef TEM_ENABLE_STORM
  #define TEM_ENABLE_STORM 1
#endif

#ifndef ACCEPT_CONST
#define ACCEPT_CONST
  const int ACCEPT = 0;
#endif

// TEMVEG index of crops
#ifndef CROPVEG_CONST
#define CROPVEG_CONST
  const int CROPVEG = 50;
#endif

// Sizes set at run time. CYCLE: steps in a year, from run_dt and run_dt_units (tem_config.yaml). NYEARS: years
// held per cell (spin-up, transient and equilibrium years). NCOHORTS: cohort dimension of the land-cover input.
// Arrays are vectors sized from these once they are known (allocate() of the classes).
#ifndef CYCLE_CONST
#define CYCLE_CONST
  inline int CYCLE = 12;
  inline int NYEARS = 1;
  inline int NCOHORTS = 1;
#endif

#ifndef TEM_VEC_HELPERS
#define TEM_VEC_HELPERS
// Zero-filled vectors of 1, 2 and 3 dimensions: the indexing of the arrays they replace is unchanged.
template <class T> std::vector<T> vec1( std::size_t n ) { return std::vector<T>( n ); }
template <class T> std::vector<std::vector<T>> vec2( std::size_t n, std::size_t m ) { return std::vector<std::vector<T>>( n, std::vector<T>( m ) ); }
template <class T> std::vector<std::vector<std::vector<T>>> vec3( std::size_t n, std::size_t m, std::size_t k )
{ return std::vector<std::vector<std::vector<T>>>( n, std::vector<std::vector<T>>( m, std::vector<T>( k ) ) ); }
#endif

// Maximum number of community types for TEM parameters
#ifndef MAXCMNT_CONST
#define MAXCMNT_CONST
  const int MAXCMNT = 24;
#endif

// Maximum number of ecosystem (i.e. carbon and nitrogen)
//   pool variables in TEM
#ifndef MAXESTAT_CONST
#define MAXESTAT_CONST
  const int MAXESTAT = 19;
#endif

// Maximum value of Fire Return Interval (years)
#ifndef MAXFRI_CONST
#define MAXFRI_CONST
  const int MAXFRI = 2000;
#endif

#ifndef MAXGRID_CONST
#define MAXGRID_CONST
  const int MAXGRID = 1;
#endif

// Maximum number of water pool variables
#ifndef MAXWSTAT_CONST
#define MAXWSTAT_CONST
  const int MAXWSTAT = 5;
#endif

// Default value for missing data
#ifndef MISSING_CONST
#define MISSING_CONST
  const double MISSING = -999999.9;
#endif

// Total number of TEMCLM output variables
#ifndef NUMATMS_CONST
#define NUMATMS_CONST
const int NUMATMS = 18;
#endif

// Maximum number of vegetation subtypes in a mosaic
#ifndef NUMMSAC_CONST
#define NUMMSAC_CONST
  const int NUMMSAC = 5;
#endif

// Maximum number of vegetation types
#ifndef NUMVEG_CONST
#define NUMVEG_CONST
  const int NUMVEG = 62;
#endif

// TEMVEG index of pasture
#ifndef PASTURE_CONST
#define PASTURE_CONST
  const int PASTURE = 51;
#endif

#ifndef REJECT_CONST
#define REJECT_CONST
  const int REJECT = 1;
#endif

// TEMVEG index of urban areas
#ifndef URBAN_CONST
#define URBAN_CONST
  const int URBAN = 40;
#endif

// constant indicating lat number for display purposes in ctem
#ifndef VPARAMS_ROW_CONST
#define VPARAMS_ROW_CONST
  const int VPARAMS_ROW = 32;
#endif

#ifndef ZERO_CONST
#define ZERO_CONST
  const double ZERO = 0.000000;
#endif

/* *************************************************************
     Constants that are combinations of other constants
************************************************************* */

// Maximum number of pool variables in simulation
#ifndef MAXSTATE_CONST
#define MAXSTATE_CONST
  const int MAXSTATE = MAXWSTAT + MAXESTAT;
#endif

// Maximum number of state (pools plus fluxes) variables
//  used to simulate hydrology in TEM
#ifndef NUMWEQ_CONST
#define NUMWEQ_CONST
  const int NUMWEQ = MAXWSTAT + 11;
#endif

// Maximum number of state (pools plus fluxes) variables
//  used to simulate carbon and nitrogen dynamics in TEM
#ifndef NUMEEQ_CONST
#define NUMEEQ_CONST
  const int NUMEEQ = MAXESTAT + 66; 
#endif

// Total number of state (pools plus fluxes variables
//  (carbon, nitrogen and water) used in TEM
#ifndef NUMEQ_CONST
#define NUMEQ_CONST
  const int NUMEQ = NUMWEQ + NUMEEQ;
#endif

// Total number of output variables from TEM
#ifndef NUMTEM_CONST
#define NUMTEM_CONST
  const int NUMTEM = NUMEQ + 130;
#endif

