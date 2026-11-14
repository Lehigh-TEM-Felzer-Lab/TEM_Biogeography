/* *************************************************************
TELMNTCOHORT_GDS.HPP - Container class to hold land cover cohort
                      characteristics

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Updated: Sizes the monthly arrays MAXCYCLE.
20241110 Changed: Yearly and per-step arrays are std::vector sized at run time (CYCLE, NYEARS, NCOHORTS);
         MAXCYCLE, MAXRTIME and MAXCHRTS removed.

************************************************************* */

#ifndef ELMNTCOHORT_GDS_H
#define ELMNTCOHORT_GDS_H

#include "temconsts_gds.hpp"
#include "bioms_gds.hpp"

struct ElmntCohort45
{
  // Index for agricultural crop type used for
  //   parameterization
  int agcmnt;

  // Number of growing degree days in croplands
  double aggrowdd;

  // Current value of decomposition parameter in croplands
  double agkd;

  // Index to indicate whether cohort was in
  //   agriculture during the previous year
//  int agprevstate;

  int agprvstate;

  // Index to indicate whether cohort is in
  //   agriculture during the current year
  int agstate;
//  BSF added alloclc
  double alloclc;

  double avlh2o;
  double awcapmm;

  double c2n;

  // Area covered by cohort (square kilometers)
  //   during current year
  float chrtarea;

  // Index for vegetation community type used for
  //   parameterization
  int cmnt;

  double cneven;

  Biomass convrtflx;
  Biomass standdead;
  Biomass vola;

  double cropprvleafmx;

  Biomass cropResidue;

  double croptopt;

  // Index for current vegetation type
  int currentveg;

  int distmnthcnt;
  int disturbflag;
  int disturbmonth;
  int retint;

  //  BSF added dgppdrootc
  double dgppdrootc;

  // Index to indicate if crops are fertilized (= 1)
  //   or not (= 0)
  int fertflag;

  int firemnthcnt;

  Biomass formPROD10;
  Biomass formPROD100;

  // Fire return interval
  int FRI;

  std::vector<Biomass> initPROD1 = std::vector<Biomass>( 12 );
  Biomass initPROD10[10];
  Biomass initPROD100[100];

 // Index to indicate if crops are irrigated (= 1)
  //   or not (= 0)
  int irrgflag;

  // Current value of decomposition parameter in
  //   natural ecosystems
  double kd;

  double natprvleafmx;
  double natseedC;
  double natseedSTRN;
  double natseedSTON;
  double natsoil;
  double nattopt;

  double newleafmx;

  double newtopt;

  double nretent;
  double nsretent;
  double nvretent;

  int potveg;

  // Air temperature that occurred 2 months ago
  double prev2tair;

  // Atmospheric CO2 concentration during the previous month
  double prevco2;

  Biomass prevCropResidue;

  Biomass prevPROD1;
  Biomass prevPROD10;
  Biomass prevPROD100;

//  Previous P-E
  double prvpme;
  // Snowpack during the previous month
  double prevspack;

  // Air temperature of the previous month
  double prevtair;

  // Value of y[] during previous time step
  double prevy[MAXSTATE];

  double prod10par;

  double prod100par;

  int productYear;

  // Area covered by cohort (square kilometers)
  //   during previous year
  float prvchrtarea;

  // Crop net primary production during the previous month
  double prvcropnpp;

  double rprec;
  double rdemandc;
  double rdemandn;
  double rlabilec;
  double rlabilen;
  
  double rgpp;
  double ringpp;
  
  double rnpp;
  double rtair;
  double rtairphi;
  double rphi;
  double phicnt;
  double rltrc;
  double rrh;
  double rpleaf;
  double reet;

  int qc;

  double sconvert;

  Biomass sconvrtflx;

  Biomass slash;

  double slashpar;

  //  BSF added soninput
  double soninput;

  // Source cohort for current cohort
  int srcCohort;

  // Age of cohort
  int standage;

  int subtype;

  int tillflag;

  double topt;
  double toptmic;

  int tqc;

  double vconvert;

  Biomass vconvrtflx;

  double vrespar;

  // Wilting point
//  double wiltpt;

  double y[MAXSTATE];

  double yrltrc;
  double yrltrn;
  
  double yrpleaf;
  double yrpsapwood;
  double yrproot;
  //  BSF added yrpseed
  double yrpseed;
  
  double yralloclc;
  double yrallocsc;
  double yrallocrc;
  //  BSF added yrallocseedc
  double yrallocseedc;
  
  double yrrmleaf;
  double yrrmsapwood;
  double yrrmroot;

};

#endif
