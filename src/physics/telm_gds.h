/* **************************************************************
*****************************************************************
TELM_GDS.H - Determines the interface among CLM, LCLUC and TEM
             modules

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: Output goes to the NetCDF writer (tem::nc::outputPut) instead of one text file per variable,
         so one file holds all variables.
20241109 Changed: The cohort state is read and written through tem_state_io.h, one NetCDF state file for all
         cells.
20241109 Changed: Arrays are sized MAXCYCLE for the run step.
20241109 Changed: The texture and elevation inputs are iftexture and ifelev (were fstxt and felev).
20241110 Changed: Yearly and per-step arrays are std::vector sized at run time (CYCLE, NYEARS, NCOHORTS);
         MAXCYCLE, MAXRTIME and MAXCHRTS removed.
20241110 Changed: Absorbs telm45_competition.h; one cohort class for both orders.

*****************************************************************
************************************************************** */

#ifndef TELM_GDS_H
#define TELM_GDS_H

const int TQCZEROFLAG = 31;

//Modules representing climate and TEM

#include "tem_state_io.h"
#include "tclm_gds.h"   // Telm45 uses the Tclm45 class

// Telm45 uses the Elmntcohort45
#include "telmntcohort_gds.hpp"

#include "ttem_gds.h"   // Telm45 uses the Ttem45 class

#include "tlcluc_gds.h" // Telm45 uses the Tlcluc45 class

// Modules describing the interface with spatially explicit
//   data sets

#include "tclmdat_gds.h"  //Telm45 uses the Clmdat45 class
#include "tsoldat_gds.h"  //Telm45 uses the Soildat45 class
#include "telvdat_gds.h"  //Telm45 uses the Elevdat45 class

class Telm45
{

  public:

     Telm45();

     // sizes the arrays of steps, years and cohorts once CYCLE, NYEARS and NCOHORTS are known
     void allocate( void )
     {
       atmstotyr.assign( NYEARS, 0 );
       avetair.assign( NYEARS, 0.0 );
       climate = vec3<double>( NUMATMS, CYCLE, NYEARS );
       adep = vec2<double>( 1, NYEARS );
       nfertcrop = vec2<double>( 1, NYEARS );
       nfertpasture = vec2<double>( 1, NYEARS );
       cohort.assign( NCOHORTS, ElmntCohort45() );
       for( size_t i = 0; i < cohort.size(); ++i ) { cohort[i].initPROD1.assign( CYCLE, Biomass() ); }
       mxtair.assign( NYEARS, 0.0 );
       ttotyr.assign( NYEARS, 0 );
       yrprec.assign( NYEARS, 0.0 );
       vegceq.assign( NYEARS, 0.0 );
       soilceq.assign( NYEARS, 0.0 );
       vegneq.assign( NYEARS, 0.0 );
       soilneq.assign( NYEARS, 0.0 );
       output = vec3<double>( NCOHORTS, NUMTEM, CYCLE );
       tem.allocate();
     }


/* *************************************************************
		 Public Function Declarations
************************************************************* */


     void atmswritepred( const int& pdyr,
                         const vector<string>& predname,
                         const int& atmspred );

     int coregerr( std::ostream& rflog1,
                   const string& varname1,
                   const float& lon1,
		           const float& lat1,
                   const string& varname2,
                   const float& lon2,
		           const float& lat2 );

     int equilibrateTEM( const double& ptol,
                        const int& pichrt,
                        const vector<string>& predname,
                        const int& spinoutfg,
                        int & year,
                        std::ostream& flog1 );  

     void getTEMCohortState( const int& pichrt );

     void initializeCohortTEMState( const int& pichrt );

     void outputTEMmonth( const int& pdm, const int& ichrt );

     void readCohortState( tem::state::In& ifstate,
                           const int& pichrt );

     void saveTEMCohortState( const int& pichrt );

     void setCohortTEMState( const ElmntCohort45& firstchrt,
                             ElmntCohort45& targetchrt );

     int setGIStopography( std::ostream& flog1,
                           int& ftlerr,
                           tem::nc::Stream* iftexture,
                           tem::nc::Stream* ifelev );

     void setTEMequilState( std::ostream& rflog1,
                            const int& equil,
                            const int& totsptime,
                            const int& pichrt,
                            const vector<string>& predname,
                            const int& spinoutfg
      );

     void setTEMmiss( const int& pdyr,
                      const int& equil,
                      const int& totsptime,
                      const int& pichrt );

     void temwritepred( const vector<string>& predname,
                        const int& pdyr,
                        const int& pichrt,
                        const int& ntempred,
                        const int& spinoutfg,
                        const int& equilflg );

     void updateTEMmonth( const int& equil,
                          const int& totsptime,
                          const int& outyr,
                          const int& pdm,
                          const int& pichrt,
                          std::ostream& flog1 );

     void writeCohortState( tem::state::Out& ofstate,
                            const int& pichrt );

/* *************************************************************
		 Public Variables
************************************************************* */

     std::vector<int> atmstotyr = std::vector<int>( 1 );

     // Mean annual air temperature
     std::vector<double> avetair = std::vector<double>( 1 );

//     long carea;
     float carea;

     std::vector<std::vector<std::vector<double>>> climate = vec3<double>( NUMATMS, 12, 1 );
     std::vector<std::vector<double>> adep = vec2<double>( 1, 1 );
     std::vector<std::vector<double>> nfertcrop = vec2<double>( 1, 1 );
     std::vector<std::vector<double>> nfertpasture = vec2<double>( 1, 1 );

     Tclm45 clm;

     std::vector<ElmntCohort45> cohort = std::vector<ElmntCohort45>( 1 );

     float lon;

     string contnent;
     
     int de_nyears;
     int de_startyear;
     int spinflag;

     int fatalerr;

     Tlcluc45 lcluc;




     // Maximum number of cohorts in an element
     //   during the current year
     int maxcohorts;

     // Maximum monthly air temperature during the year
     std::vector<double> mxtair = std::vector<double>( 1 );

     // Maximum number of "natural" cohorts in an element
     int natcohorts;

     // Number of climate predicted variables
     int natmspred;

     // Number of TEM predicted variables
     int ntempred;

//     vector<string> predstr;

     std::vector<std::vector<std::vector<double>>> output = vec3<double>( 1, NUMTEM, 12 );

     int outyr;

     // Maximum number of cohorts in an element
     //   during the previous year
     int prvmxcohrts;

     string region;

     float lat;

//     long subarea;
     float subarea;

     Ttem45 tem;

     int totpred;

     std::vector<int> ttotyr = std::vector<int>( 1 );

     int wrtyr;

     // Annual precipitation during the year
     std::vector<double> yrprec = std::vector<double>( 1 );

     int year;

     std::vector<double> avlneq = std::vector<double>( 1 );
     std::vector<double> vegceq = std::vector<double>( 1 );
     std::vector<double> soilceq = std::vector<double>( 1 );
     std::vector<double> vegneq = std::vector<double>( 1 );
     std::vector<double> soilneq = std::vector<double>( 1 );
     std::vector<double> smoisteq = std::vector<double>( 1 );


/* *************************************************************
		 Private Function Declarations
************************************************************* */

  private:

//     int temgisqc( const long& subarea,
     int temgisqc( const float& subarea,
		           const double& pctsand,
                   const double& pctsilt,
                   const double& pctclay,
                   const int& cmnt,
                   const double& elev,
                   const double& nirr,
                   const double& par,
		           const double& tair,
                   const double& mxtair,
                   const double& avtair,
                   const double& yrprec,
                   const double& rain,
                   const double& snowfall,
                   const double& co2,
                   const double& aot40,
                   const double& ndep,
				   const double& trange,
				   const double& vpd );

     int transqc( int& maxyears,
                  int& totyr,
                  double* plantc );

};

#endif
