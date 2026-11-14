/* *************************************************************
****************************************************************
TCLM_GDS.H - describes climate module used by TEM

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Updated: Adds tws10flag, the switch between transient and long-term average wind (xtws10flag).
20241109 Changed: Inputs come from NetCDF through the drivers: the file-name and extension members of every
         climate variable are removed.

****************************************************************
************************************************************* */

#ifndef TCLM_GDS_H
#define TCLM_GDS_H

// Tclm45 uses the global constants CYCLE, MISSING, NUMATMS
//   and NUMATMS
#include "temconsts_gds.hpp"

// Tclm45 inherits class Atms45
#include "atms_gds.h"


class Tclm45 : public Atms45
{

  public:

     Tclm45();

     enum clmkey { I_GIRR,   I_NIRR,   I_PAR,    I_CLDS,  I_TAIR,
                   I_PREC,   I_RAIN,   I_SNWFAL, I_CO2,   I_VPR,
                   I_TRANGE, I_TAIRD,  I_TAIRN,  I_VPDD,  I_VPDN,   
                   I_WS10,   I_AOT40,  I_DAYL };

/* *************************************************************
		 Public Functions
************************************************************* */

     /* Determine cloudiness based on solar radiation at the top
        of the atmosphere and solar radiation at the top of the
        vegetation canopy */

     double mkclds( const double& girr, const double& nirr );

     double mkd40( const double& lon,
                   const double& lat,
	  	   const string& contnent,
                   const double& o3,
	  	   const int& pdm );

     /* Determine solar radiation at the top of the atmosphere (i.e.,
        "gross irradiance" or GIRR) based on the solar constant, time
        of year and latitude as described by S. M. Turton (1986)
        Solar radiation under cloudless skies.  Weatherwise 39:
        223-224.  */

     double xgirr( const float& plat,
                   const int& pdm,
                   double& psumday );

     /* Determine solar radiation at the top of the vegetation canopy
        (i.e., "net irradiance" or NIRR) based on GIRR and cloudiness */

     double xnirr( const double& clds, const double& girr );

     /* Determine phototsynthetically active radiation (PAR) based on
        NIRR and cloudiness.  Algorithm determined by Jim Raich from
        a variety of studies [e.g., McCree (1966) Agricul. Meteorol.
        3: 353-366; Stigter and Musabilha (1982) Journal of Applied
        Ecology 19: 853-858] that indicate that cloud cover increases
        the proportion of PAR. */

     double xpar( const double& clds, const double& nirr );


/* *************************************************************
		 Public Variables
************************************************************* */


     // "Do data sets have cloudiness data or NIRR data?" flag
     int cldflag;

//     int co2year[MAXRTIME];       // year of CO2 data

     int parflag;     // Read in PAR from spatially explicit data sets?

     int predflag;    // Write climate module output to files?

     // Names of climate output variables

     vector<string> predstr;

     int sradflag;    // Run solar radiation module?

     // Initial year of transient portion of simulation

     int startyr;

     // Flag for transient cloudiness data
     //   ( =0 for long-term mean data
     //      1 for transient data)

     int tcldsflag;

     // transient CO2 concentration (ppmv)

//     double tco2[MAXRTIME][MAXCYCLE];

     // Flag for transient atmospheric CO2 data
     //   ( =0 for long-term mean data
     //      1 for transient data)
     int tco2flag;

     // Flag for transient ozone (AOT40) data
     //   ( =0 for long-term mean data
     //      1 for transient data)
     int to3flag;

     // Flag for transient ozone (AOT40) data
     //  ( =0 for long-term mean data
     //    1 for transient data)
     int tndepflag;
   
     // Flag for transient precipitation data
     //   ( =0 for long-term mean data
     //      1 for transient data)
     int tprecflag;


     // Flag for transient air temperature data
     //   ( =0 for long-term mean data
     //      1 for transient data)
     int ttairflag;

     // Flag for transient temperature range
     //   ( =0 for long-term mean data
     //      1 for transient data)	
     int ttrangeflag;

     // Flag for transient vapor pressure
     //   ( =0 for long-term mean data
     //      1 for transient data)
     int tvprflag;

     // Flag for transient 10 m wind speed
     //   ( =0 for long-term mean data
     //      1 for transient data)
     int tws10flag;

     // Year represented by the climate data

     int year;

     // Number of days since beginning of year

     double yrsumday;


};

#endif
