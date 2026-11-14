/* *************************************************************
****************************************************************
TTEM45.CPP - Terrestrial Ecosystem Model, XTEM version 4.5
****************************************************************

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Changed: Parameters are read with tem::yaml::Reader (tem_yaml.h) instead of ProcessXML, because the
         parameter files are YAML.
20241109 Changed: The days of a step come from Timestep (ttimestep.h) and arrays are sized MAXCYCLE, so the
         step is set by run_dt and run_dt_units and is not fixed at twelve months.
20241109 Changed: Output names are lowercase, to agree with the NetCDF names and the registry in
         tem_outvars.cpp.
20241109 Updated: Nitrogen fertilizer of cropland and pasture (xnfertflag) is passed to the soil nitrogen
         budget.
20241111 Changed: Wildfire is GDSTEM-FIRE (fire.h, Tfire): the fraction of the cohort burned in the step.
20241109 Updated: Wildfire (fire.h) acts on the vegetation and the standing dead pool, with occurrence and area
         as outputs.
20241109 Updated: The ODE solver (ode_solver) and the soil respiration function (rsoil_method) are chosen at
         run time.
20241109 Updated: The state vector holds NUMTEM entries and the new output variables are filled.
20241109 Fixed: Numbers are parsed strictly: a missing key or a value that is not a number stops the run, with
         the file and key named (atoi and atof read such values as 0).
20241109 Changed: Site calibration files are opened with their own Reader (fecd) and the run configuration is
         held as config (replaces gofile, goname, goyaml).
20241109 Fixed: The irrigation test uses && where it had a bitwise &.
20241111 Changed: The fire-interval draws (rco, mco, lco) are those of GDSTEM-FIRE (rand and srand(time)).
20241109 Fixed: The year-end summary also runs on the last step of a run that ends inside a year.
20241109 Changed: Annual averages and the update of the annual parameters use CYCLE and not 12; calendar tests
         use isMONTHSTART, true on the first step of the month at any step length.
20241109 Fixed: Fertilizer is spread over CYCLE steps (1000 * CYCLE in place of 12000), as nitrogen deposition
         already was.
20241110 Changed: Yearly arrays are vectors; allocate() sizes them.

****************************************************************
************************************************************** */

//#define BORLAND_CPP

#include<cstdio>

  using std::printf;

#include<iostream>

  using std::cout;
  using std::ios;
  using std::cerr;
  using std::endl;

#include<fstream>

  using std::ifstream;
  using std::ofstream;

#ifdef BORLAND_CPP
  #include<stdlib>
#else
  #include<cstdlib>
#endif

#ifdef BORLAND_CPP

  #include<time>

  using std::time_t;

#else
  #include<ctime>

  using std::time_t;

#endif

  //using std::exit;
  using std::atof;
  using std::atoi;

#include<cmath>

  using std::exp;
  using std::fabs;
  using std::modf;
  using std::pow;
  using std::max;

#include<vector>

  using std::vector;

#include<string>

  using std::string;

#include<iomanip>

  using std::setprecision;

//#define CALIBRATE_TEM



//#define FIRE

#ifndef TEM_NO_OPENN
#define OPENN
#endif

//#define DEBUG_CTEM

//#define DEBUG_XTEM

#ifdef CALIBRATE_TEM
  #include<curses.h>
  #include<cctype>
    using std::toupper;
//  initFlag = 0;
#endif

#include "ttimestep.h"
#include "ttem_gds.h"
#if TEM_ENABLE_STORM
#include <cstdint>
#include <random>
#endif

/* **************************************************************
************************************************************** */

// Initialization of static members

int Ttem45::avlnflag = 0;
int Ttem45::nfeed = 0;
int Ttem45::rheqflag = 0;
int Ttem45::moistlim = 0;
int Ttem45::o3flag = 0;
int Ttem45::initbase = 0;
int Ttem45::baseline = 0;
int Ttem45::intflag = 0;

int Ttem45::maxnrun = 0;
int Ttem45::equil = 0;
int Ttem45::runsize = 0;
int Ttem45::maxyears = 0;
int Ttem45::strteq = 0;
int Ttem45::endeq = 0;
int Ttem45::startyr = 0;
int Ttem45::endyr = 0;
int Ttem45::diffyr = 0;
int Ttem45::wrtyr = 0;

double Ttem45::ctol = 1.0;
double Ttem45::ntol = 0.02;
double Ttem45::wtol = 0.5;

// Initialization of adaptive integrator variables

double Ttem45::inittol = 0.01;
double Ttem45::odedtmax = 0.03333;
int Ttem45::maxit = 20;
int Ttem45::maxitmon = 100;

double Ttem45::a1 =   0.115740741;

double   Ttem45::a3 =   0.548927875;
double  Ttem45::a31 =   0.09375;
double  Ttem45::a32 =   0.28125;

double   Ttem45::a4 =   0.535331384;
double  Ttem45::a41 =   0.879380974;
double  Ttem45::a42 =  -3.277196177;
double  Ttem45::a43 =   3.320892126;

double   Ttem45::a5 =  -0.20;
double  Ttem45::a51 =   2.032407407;
double  Ttem45::a52 =  -8.0;
double  Ttem45::a53 =   7.173489279;
double  Ttem45::a54 =  -0.2058966866;

double   Ttem45::b1 =   0.118518519;
double   Ttem45::b3 =   0.518986355;
double   Ttem45::b4 =   0.50613149;
double   Ttem45::b5 =  -0.18;
double   Ttem45::b6 =   0.036363636;
double  Ttem45::b61 =  -0.296296296;
double  Ttem45::b62 =   2.0;
double  Ttem45::b63 =  -1.381676413;
double  Ttem45::b64 =   0.45297271;
double  Ttem45::b65 =  -0.275;

//PCP code
int Ttem45::cutstep = -1;
int Ttem45::rcount = 0;
int Ttem45::mcount = 0;
int Ttem45::lcount = 0;


double Ttem45::rco = (rand()%2 == 1)?(double)(rand()%10+rand()%10+rand()%10)*-1/10:(double)(rand()%10+rand()%10+rand()%10)/10;
double Ttem45::mco = (rand()%2 == 1)?(double)(rand()%10+rand()%10+rand()%10)*-1/10:(double)(rand()%10+rand()%10+rand()%10)/10;
double Ttem45::lco = (rand()%2 == 1)?(double)(rand()%10+rand()%10+rand()%10)*-1/10:(double)(rand()%10+rand()%10+rand()%10)/10;
//PCP code

/* **************************************************************
************************************************************** */

Ttem45::Ttem45() : predstr( NUMTEM )
{
  tol = inittol;
  syint = 1;
  odesolver = 1;
  rsoilmethod = 2;
  totyr = -99;
  rheqflag = 0;
  
  delta_count = 0;
  
  configfile = "tem_config.yaml"; // the driver replaces it by its first argument

#ifdef CALIBRATE_TEM
  soilfile = "tsoil45.yaml";
  rootfile = "trotz45.yaml";
  vegfile = "tveg45.yaml";
  mcrvfile = "tmcrv45.yaml";
  agfile = "ag45.yaml";
  gcfile = "tconduct45.yaml";

  rheqflag = 0;
  adapttol = 0;

  sey[0] = GET_VEGC;        //1
  sey[1] = GET_SOILC;       //3
  sey[2] = GET_SOILN;       //4
  sey[3] = GET_LABILEC;     //11
  sey[4] = GET_GPP;         //30
  sey[5] = GET_NPP;         //35
  sey[6] = GET_RH;          //50
  sey[7] = GET_LAI;         //52
//  sey[8] = GET_LABILEN;     //17
  sey[8] = GET_AVALN;     //17
  sey[9] = GET_VNUP;        //66
  
  swy[0] = GET_RAIN;        //4
  swy[1] = GET_SNWFAL;      //5
  swy[2] = GET_AVLH2O;      //36
  swy[3] = GET_EET;         //13
  swy[4] = GET_WYLD;        //20
  swy[5] = GET_TAIRD;       //10
  swy[6] = GET_VPDD;        //34
  swy[7] = GET_WS10;          //14
//  swy[8] = GET_WS10;          //14
#endif

// Identify potential output variables from TEM

// Ecosystem carbon pools determined by the integrator**********

  // vegetation carbon pools

  predstr.at( I_LEAFC ) = "leafc";

  predstr.at( I_SAPWOODC ) = "sapwoodc";

  predstr.at( I_HEARTWOODC ) = "heartwoodc";

  predstr.at( I_ROOTC ) = "rootc";

  predstr.at( I_SEEDC ) = "seedc";

  predstr.at( I_LABILEC ) = "labilec";

  // reactive soil organic carbon
  predstr.at( I_SOLC ) = "soilorgc";

// Ecosystem nitrogen pools determined by the integrator********

  // vegetation structural nitrogen pools

  predstr.at( I_LEAFN ) = "leafn";

  predstr.at( I_SAPWOODN ) = "sapwoodn";

  predstr.at( I_HEARTWOODN ) = "heartwoodn";

  predstr.at( I_ROOTN ) = "rootn";

  predstr.at( I_SEEDN ) = "seedn";

  predstr.at( I_LABILEN ) = "labilen";


  // reactive soil organic nitrogen
  predstr.at( I_SOLN ) = "soilorgn";

  // soil available nitrogen
  predstr.at( I_AVLN ) = "availn";
  
  // ozone damage factor
  predstr.at( I_FOZONE ) = "fozone";

  // GPP radiation factor
  predstr.at( I_FRDL ) = "frdl";

  //  GPP CO2 factor
  predstr.at( I_FCO2 ) = "fco2";

  // GPP temp factor
  predstr.at( I_TEMP ) = "temp";

  // GPP water factor
  predstr.at( I_FH2O ) = "fh2o";

  // GPP ozone factor
  predstr.at( I_FO3 ) = "fo3";
// Ecosystem water pools determined by the integrator***********

  // soil moisture
  predstr.at( I_SM ) = "smoist";

  // volumetric soil moisture
  predstr.at( I_VSM ) = "vsm";

  // soil moisture expressed as percent total porosity
  predstr.at( I_PCTP ) = "pctp";

  // groundwater pool resulting from rainfall
  predstr.at( I_RGRW ) = "rgrndh2o";

  // groundwater pool resulting from snow melt
  predstr.at( I_SGRW ) = "sgrndh2o";

  // foliar projected cover
  predstr.at( I_FPC ) = "fpc";


// Carbon fluxes for ecosystems ********************************

  // leaf allocation
  predstr.at( I_ALLOCLC ) = "alloclc";

  // sapwood allocation
  predstr.at( I_ALLOCSC ) = "allocsc";

  // heartwood allocation
  predstr.at( I_ALLOCHC ) = "allochc";

  // root allocation
  predstr.at( I_ALLOCRC ) = "allocrc";

  // seed allocation
  predstr.at( I_ALLOCSEEDC ) = "allocseedc";
  
  // leaf investment-allocation
  predstr.at( I_ALLOCILC ) = "allocilc";

  // sapwood investment-allocation
  predstr.at( I_ALLOCISC ) = "allocisc";

  // heartwood investment-allocation
  predstr.at( I_ALLOCIHC ) = "allocihc";

  // root investment-allocation
  predstr.at( I_ALLOCIRC ) = "allocirc";

  // seed investment-allocation
  predstr.at( I_ALLOCISEEDC ) = "allociseedc";

  // GPP not limited by nutrient availability
  predstr.at( I_INGPP ) = "vegingpp";

  // gross primary production
  predstr.at( I_GPP ) = "gpp";

 // NPP not limited by nutrient availability
  predstr.at( I_INNPP ) = "veginnpp";

  // net primary production
  predstr.at( I_NPP ) = "npp";

  // gross plant respiration
  predstr.at( I_GPR ) = "gpr";

  // vegetation maintenance respiration
  predstr.at( I_RVMNT ) = "rvmaint";

  // leaf respiration
  predstr.at( I_RMLEAF ) = "rmleaf";

  // sapwood respiration
  predstr.at( I_RMSAPWOOD ) = "rmsapwood";

  // root respiration
  predstr.at( I_RMROOT ) = "rmroot";

  // seed respiration
  predstr.at( I_RMSEED ) = "rmseed";

  // labile C respiration
  predstr.at( I_RMLABILE ) = "rmlabile";


  // vegetation growth respiration
  predstr.at( I_RVGRW ) = "rvgrwth";

  // leaf litterfall carbon
  predstr.at( I_LTRLC ) = "ltrlc";

  // sapwood litterfall carbon
  predstr.at( I_LTRSC ) = "ltrsc";

  // heartwood litterfall carbon
  predstr.at( I_LTRHC ) = "ltrhc";

  // root litterfall carbon
  predstr.at( I_LTRRC ) = "ltrrc";

  // seed litterfall carbon
  predstr.at( I_LTRSEEDC ) = "ltrseedc";

  // heterotrophic respiration
  predstr.at( I_RH ) = "rh";

  // Dissolved Organic Carbon
  predstr.at( I_DOC ) = "doc";

  // Dissolved Organic Nitrgoen
  predstr.at( I_DON ) = "don";

// Nitrogen fluxes for ecosystems determined by the integrator

  // leaf nitrogen allocation
  predstr.at( I_ALLOCLN ) = "allocln";

  // sapwood nitrogen allocation
  predstr.at( I_ALLOCSN ) = "allocsn";

  // heartwood nitrogen allocation
  predstr.at( I_ALLOCHN ) = "allochn";

  // root nitrogen allocation
  predstr.at( I_ALLOCRN ) = "allocrn";

  // seed nitrogen allocation
  predstr.at( I_ALLOCSEEDN ) = "allocseedn";
  
  // leaf nitrogen investment-allocation
  predstr.at( I_ALLOCILN ) = "allociln";

  // sapwood nitrogen investment-allocation
  predstr.at( I_ALLOCISN ) = "allocisn";

  // heartwood nitrogen investment-allocation
  predstr.at( I_ALLOCIHN ) = "allocihn";

  // root nitrogen investment-llocation
  predstr.at( I_ALLOCIRN ) = "allocirn";

  // seed nitrogen investment-allocation
  predstr.at( I_ALLOCISEEDN ) = "allociseedn";

  // total nitrogen inputs into ecosystem
  predstr.at( I_NINP ) = "ninput";

  // total DOC production into ecosystem
  predstr.at( I_DOCPROD ) = "docprod";

  // total DOC leaching from ecosystem
  predstr.at( I_LCHDOC ) = "lchdoc";

  // total DON production into ecosystem
  predstr.at( I_DONPROD ) = "donprod";

  // total DON leaching from ecosystem
  predstr.at( I_LCHDON ) = "lchdon";

  // total DIN leaching from ecosystem
     predstr.at( I_LCHDIN ) = "lchdin";
  
  // nitrogen fertilization
  predstr.at( I_AGFRTN ) = "agfertn";

  // VEGNUP not limited by carbon availability
  predstr.at( I_INNUP ) = "veginnup";

  // nitrogen uptake by vegetation
  predstr.at( I_VNUP ) = "vegnup";

  // nitrogen resorption by leaves
  predstr.at( I_NRESORBL ) = "nresorbl";

  // nitrogen resorption by stems
  predstr.at( I_NRESORBS ) = "nresorbs";

  // nitrogen resorption by roots
  predstr.at( I_NRESORBR ) = "nresorbr";

  // nitrogen resorption by seeds
  predstr.at( I_NRESORBSEED ) = "nresorbseed";

  // leaf litterfall nitrogen from vegetation
  predstr.at( I_LTRLN ) = "ltrln";

  // sapwood litterfall nitrogen from vegetation
  predstr.at( I_LTRSN ) = "ltrsn";

  // heartwood litterfall nitrogen from vegetation
  predstr.at( I_LTRHN ) = "ltrhn";

  // root litterfall nitrogen from vegetation
  predstr.at( I_LTRRN ) = "ltrrn";

  // seed litterfall nitrogen from vegetation
  predstr.at( I_LTRSEEDN ) = "ltrseedn";

  // total nitrogen immobilization
  predstr.at( I_MNUP ) = "micronup";

  // net nitrogen mineralization
  predstr.at( I_NMIN ) = "netnmin";

  // Total nitrogen losses from ecosystems
  predstr.at( I_NLST ) = "nlost";

  // Symbiotic N Fixation
  predstr.at( I_NFIXS ) = "nfixs";

  // Nonsymbiotic N Fixation
  predstr.at( I_NFIXN ) = "nfixn";

// Water fluxes determined by the integrator********************

  // Irrigation
  predstr.at( I_AGIRRIG ) = "irrigate";

  // Initial estimated evapotranspiration
  predstr.at( I_INEET ) = "ineet";

  // estimated evapotranspiration
  predstr.at( I_EET ) = "eet";

  // percolation of rainwater through soil profile
  predstr.at( I_RPERC ) = "rperc";

  // percolation of snowmelt through soil profile
  predstr.at( I_SPERC ) = "sperc";

  // runoff of rainwater
  predstr.at( I_RRUN ) = "rrun";

  // runoff of snowmelt
  predstr.at( I_SRUN ) = "srun";

  // canopy conductance
  predstr.at( I_GC ) = "gc";    
  
  // stomatal conductance
  predstr.at( I_GS ) = "gs";
  
  // transpiration
  predstr.at( I_PECAN ) = "pecan";    

  // soil evaporation
  predstr.at( I_PESOIL ) = "pesoil";    

// Other ecosystem carbon pools ********************************

  // total carbon pool found in ecosystem excluding products
  predstr.at( I_TOTEC ) = "totec";

  // total carbon (including products if present)
  predstr.at( I_TOTC ) = "vegc";

// Other ecosystem nitrogen pools ******************************

  // total nitrogen stored in vegetation
  predstr.at( I_VEGN ) = "vegn";

// Other ecosystem water pools ******************************

  predstr.at( I_SNWPCK ) = "snowpack";  // snowpack
  
  predstr.at( I_AVLW ) = "availh2o";  // plant available soil water

// Other carbon fluxes for ecosystems **************************

  // net ecosystem production
  predstr.at( I_NEP ) = "nep";

  // net carbon exchange of ecosystem with atmosphere
  predstr.at( I_NCE ) = "nce";
  
  // net carbon exchange of ecosystem with atmosphere entire biome
  predstr.at( I_NBP ) = "nbp";

  // leaf area index
  predstr.at( I_LAI ) = "lai";

// Other water fluxes ******************************************

  // potential evapotranspiration
  predstr.at( I_PET ) = "pet";

  // infiltration into the soil of water from snowmelt
  predstr.at( I_SNWINF ) = "snowinf";

  // water yield
  predstr.at( I_WYLD ) = "h2oyield";

// Carbon stocks in products ***********************************

  // carbon in agricultural products
  predstr.at( I_AGPRDC ) = "agprodc";

  // carbon in lawn clippings
  predstr.at( I_CLIPPINGS ) = "clippings";

  // carbon pool of products that decompose in 10 years
  predstr.at( I_PROD10C ) = "prod10c";

  // carbon pool of products that decompose in 100 years
  predstr.at( I_PROD100C ) = "prod100c";

  // carbon in all product pools
  predstr.at( I_TOTPRDC ) = "totprodc";


// Carbon stocks in crop residue and stubble********************

  // carbon in crop residue
  predstr.at( I_RESIDC ) = "residc";

  // stubble carbon
  predstr.at( I_AGSTUBC ) = "crpstubc";


// Nitrogen stocks in products *********************************

  // nitrogen in agricultural products
  predstr.at( I_AGPRDN ) = "agprodn";

  // nitrogen pool of products that decompose in 10 years
  predstr.at( I_PROD10N ) = "prod10n";

  // nitrogen pool of products that decompose in 100 years
  predstr.at( I_PROD100N ) = "prod100n";

  // nitrogen in all product pools
  predstr.at( I_TOTPRDN ) = "totprodn";


// Nitrogen stocks in crop residue and stubble******************

  // nitrogen in crop residue
  predstr.at( I_RESIDN ) = "residn";

  // stubble nitrogen
  predstr.at( I_AGSTUBN ) = "crpstubn";


// Carbon fluxes associated with agricultural conversion *******

  // carbon loss from the ecosystem during conversion
  predstr.at( I_CNVRTC ) = "convertc";

  // carbon loss from vegetation during conversion
  predstr.at( I_VCNVRTC ) = "vconvrtc";

  // carbon loss from soils during conversion
  predstr.at( I_SCNVRTC ) = "sconvrtc";

  // carbon associated with slash left after conversion
  predstr.at( I_SLASHC ) = "slashc";

  predstr.at( I_STANDDEADC ) = "standdeadc";
  predstr.at( I_STANDDEADN ) = "standdeadn";
  predstr.at( I_VOLAC ) = "volac";
  predstr.at( I_VOLAN ) = "volan";

  // fire occurrence (0 or 1) and area burned (km2) from the fire module
  predstr.at( I_AREABURN ) = "areaburn";
  predstr.at( I_POTEVAP ) = "potevap";
  predstr.at( I_ACTEVAP ) = "actevap";

  // carbon flux from ecosystem (NEP+CONVERTC)
  predstr.at( I_CFLX ) = "cflux";


// Nitrogen fluxes associated with agricultural conversion *****

  // nitrogen loss from the ecosystem during conversion
  predstr.at( I_CNVRTN ) = "convertn";

  // nitrogen loss from vegetation during conversion
  predstr.at( I_VCNVRTN ) = "vconvrtn";

  // nitrogen loss from soils during conversion
  predstr.at( I_SCNVRTN ) = "sconvrtn";

  // nitrogen associated with slash left after conversion
  predstr.at( I_SLASHN ) = "slashn";

  // Total organic N mineralized and retained in ecosystem
  //   after disturbance
  predstr.at( I_NRETNT ) = "nretent";

  // Vegetation N mineralized and retained in ecosystem
  //   after disturbance
  predstr.at( I_NVRTNT ) = "nvretent";

  // Soil organic N mineralized and retained in ecosystem
  //   after disturbance
  predstr.at( I_NSRTNT ) = "nsretent";


// Carbon and nitrogen fluxes to/from products *****************

  // carbon loss to formation of agricultural products
  predstr.at( I_AGFPRDC ) = "agfprodc";

  // nitrogen loss to formation of agricultural products
  predstr.at( I_AGFPRDN ) = "agfprodn";

  // carbon loss to crop residue
  predstr.at( I_FRESIDC ) = "fresidc";

  // nitrogen loss to crop residue
  predstr.at( I_FRESIDN ) = "fresidn";

  // carbon loss to resulting from decomposition of agricultural
  //   products
  predstr.at( I_AGPRDFC ) = "agprodfc";

  // nitrogen loss resulting from decomposition of agricultural
  //   products
  predstr.at( I_AGPRDFN ) = "agprodfn";

  // carbon loss from crop residue
  predstr.at( I_RESIDFC ) = "residfc";

  // nitrogen loss from crop residue
  predstr.at( I_RESIDFN ) = "residfn";

  // carbon loss to formation of products that decompose in
  //  10 years
  predstr.at( I_PRDF10C ) = "prdf10c";

  // nitrogen loss to formation of products that decompose in
  //   10 years
  predstr.at( I_PRDF10N ) = "prdf10n";

  // carbon loss resulting from decomposition of PROD10C
  predstr.at( I_PRD10FC ) = "prd10fc";

  // nitrogen loss resulting from decomposition of PROD10N
  predstr.at( I_PRD10FN ) = "prd10fn";

  // carbon loss to formation of products that decompose in
  //  100 years
  predstr.at( I_PRDF100C ) = "prdf100c";

  // nitrogen loss to formation of products that decompose in
  //   100 years
  predstr.at( I_PRDF100N ) = "prdf100n";

  // carbon loss resulting from decomposition of PROD100C
  predstr.at( I_PRD100FC ) = "prd100fc";

  // nitrogen loss resulting from decomposition of PROD100N
  predstr.at( I_PRD100FN ) = "prd100fn";

  // carbon loss to the formation of all products
  predstr.at( I_TOTFPRDC ) = "totfprdc";

  // nitrogen loss to the formation of all products
  predstr.at( I_TOTFPRDN ) = "totfprdn";

  // carbon loss resulting from decomposition of all products
  predstr.at( I_TOTPRDFC ) = "totprdfc";

  // nitrogen loss resulting from decomposition of all products
  predstr.at( I_TOTPRDFN ) = "totprdfn";


// Agro-Ecosystem carbon and nitrogen pools *********************

  // crop carbon
  predstr.at( I_CROPC ) = "cropc";

  // carbon in natural vegetation
  predstr.at( I_NATVEGC ) = "natvegc";

  // crop nitrogen
  predstr.at( I_CROPN ) = "cropn";

  // nitrogen in natural vegetation
  predstr.at( I_NATVEGN ) = "natvegn";

  // crop structural N
  predstr.at( I_CSTRN ) = "cropstrn";

  // structural N in natural vegetation
  predstr.at( I_NATSTRN ) = "natstrn";

  // crop labile N
  predstr.at( I_CSTON ) = "cropston";

  // labile N stored in natural vegetation
  predstr.at( I_NATSTON ) = "natston";


// Crop phenology **********************************************

  // leaf area index (LAI) of crops
  predstr.at( I_CROPLAI ) = "croplai";

  // leaf area index (LAI) of natural vegetation
  predstr.at( I_NATLAI ) = "natlai";

  // foliar projected cover (FPC) of crops
  predstr.at( I_CROPFPC ) = "cropfpc";

  // foliar projected cover (FPC) of natural vegetation
  predstr.at( I_NATFPC ) = "natfpc";


// Additional carbon fluxes for agro-ecosystems *****************

  // GPP of crops not limited by nutrient availability
  predstr.at( I_AGINGPP ) = "crpingpp";

  // GPP of natural vegetation not limited by
  //   nutrient availability
  predstr.at( I_NATINGPP ) = "natingpp";

  // gross primary production (GPP) of crops
  predstr.at( I_AGGPP ) = "cropgpp";

  // gross primary production of natural vegetation
  predstr.at( I_NATGPP ) = "natgpp";

  // NPP of crops not limited by nutrient availability
  predstr.at( I_AGINNPP ) = "crpinnpp";

  // NPP of natural vegetation not limited by
  //   nutrient availability
  predstr.at( I_NATINNPP ) = "natinnpp";

  // net primary production (NPP) of crops
  predstr.at( I_AGNPP ) = "cropnpp";

  // net primary production (NPP) of natural vegetation
  predstr.at( I_NATNPP ) = "natnpp";

  // gross plant respiration of crops
  predstr.at( I_AGGPR ) = "cropgpr";

  // gross plant respiration of natural vegetation
  predstr.at( I_NATGPR ) = "natgpr";

  // maintenance respiration of crop plants
  predstr.at( I_AGRVMNT ) = "crprmnt";

  // maintenance respiration of natural vegetation
  predstr.at( I_NATRVMNT ) = "natrvmnt";

  // growth respiration of crop plants
  predstr.at( I_AGRVGRW ) = "crprgrw";

  // growth respiration of natural vegetation
  predstr.at( I_NATRVGRW ) = "natrgrw";

  // litterfall carbon from crops
  predstr.at( I_AGLTRC ) = "cropltrc";

  // litterfall carbon from natural vegetation
  predstr.at( I_NATLTRC ) = "natltrc";

  // Additional nitrogen fluxes for agro-ecosystems ************

  // nitrogen uptake by crops not limited by carbon availability
  predstr.at( I_AGINNUP ) = "crpinnup";

  // nitrogen uptake by natural vegetation not limited by carbon
  //   availability
  predstr.at( I_NATINNUP ) = "natinnup";

  // nitrogen uptake by crops
  predstr.at( I_AGVNUP ) = "cropnup";

  // nitrogen uptake by natural vegetation
  predstr.at( I_NATVNUP ) = "natvnup";

  // nitrogen mobilization by crops
  predstr.at( I_AGVNMBL ) = "crpnmobl";

  // nitrogen mobilization by natural vegetation
  predstr.at( I_NATVNMBL ) = "natvnmbl";

  // nitrogen resorption by crops
  predstr.at( I_AGVNRSRB ) = "crpnrsrb";

  // nitrogen resorption by natural vegetation
  predstr.at( I_NVNRSRB ) = "nvnrsrb";

  // litterfall nitrogen from crops
  predstr.at( I_AGLTRN ) = "cropltrn";

  //litterfall nitrogen from natural vegetation
  predstr.at( I_NATLTRN ) = "natltrn";


  dbugflg = 0;

};

/* **************************************************************
************************* Functions *****************************
************************************************************** */


/* *************************************************************
************************************************************* */

int Ttem45::adapt( const int& numeq,
                   double pstate[],
                   const double& ptol,
                   const int& pdm, 
                   const int& pdyr, 
                   const double& nmax_grow )
{

  int i;
  double ipart;
  double fpart;
  double time = ZERO;
  double dtmax = 1.0;
  double dt;
  int mflag = 0;
  long nintmon = 0;
  double oldstate[NUMTEM];
  double tmpdt = 0.0; // read when a step is rejected, which can happen before the first step is accepted
  int icnt;

  if( 1 == odesolver ) { dtmax = odedtmax; }
  dt = dtmax;
  for( i = 0; i < numeq; ++i ) { dz1[i] = ZERO; } // reset dz1 if starting a new month 
  for( i = 0; i < numeq; ++i ) { dz4[i] = 1.0; } // reset dz4 if starting a new month 

  blackhol = 0;
  icnt = 0;
  while( time != 1.0  )
  {
    test = REJECT;
    if( 1 == syint )
    {
      while( test != ACCEPT )
      {
        if( 1 == odesolver )
        {
          rkbs( numeq, pstate, dt, pdm, pdyr, nmax_grow );
          test = boundcon( dum2, error, ptol );
        }
        else
        {
          rkf( numeq, pstate, dt, pdm, pdyr, nmax_grow );
          test = boundcon( dum4, error, ptol );
        }

//
//   BSF add check for very small values of dt - necessary for icpx compiler
//
// if(pdyr == 1969) {cout << "dt = " << dt << " " << dtmax*pow( 0.5, maxit ) << " " << time << " " << nintmon << endl;}
//	    if( dt <= dtmax*pow( 0.5, maxit ) || dt <= 1.0e-6 )
	    if( dt <= dtmax*pow( 0.5, maxit ) )
        {
// if(pdyr == 1969) {cout << "test = " << test << " " << time << " " << nintmon << endl;}
          test = ACCEPT;
	      mflag = 1;

          if( 0 == nintmon )
          {
            for( i = 0; i < numeq; ++i )
            {
              oldstate[i] = pstate[i];
            }
          }

	      ++nintmon;
	    }

	if(icnt >= maxit) { 
		test = ACCEPT;
//                cout << "icnt trigger " << icnt << " " << time << " " << test << " " << syint << endl;
	}

	  

        if( ACCEPT == test )
        {
          if( 1 == odesolver )
          {
            for( i = 0; i < numeq; ++i ) 
            { 
              pstate[i] = dum2[i]; 
              dz1[i] = dz4[i]; // dz1 for the next step has already been calculated
            }
          }
          else
          {
            for( i = 0; i < numeq; ++i ) { pstate[i] = dum4[i]; }
          }
          time += dt;

          fpart = modf( (0.01 + (time/(2.0*dt))),&ipart );

	  tmpdt = 0;
          if ( fpart < 0.1 && 2.0*dt <= dtmax) {
        	dt *= 2.0;
		tmpdt = 1;
	 
//	 if(pdyr == 1969) {cout << "dtx2 = " << dt << " " << fpart << " " << time << " " << nintmon << endl;}
	  } 

          if ( dt > (1.0 - time) ) { dt = 1.0 - time;
//	  if(pdyr == 1969) {cout << "newdt = " << dt << " " << time << " " << nintmon << endl;}
	  } // ensure that last step doesn't go over the end time

        }
        else 
        { 
//           if(pdyr == 1969) {cout << "dtbefore = " << dt << " " << time << " " << nintmon << endl;}
          if(tmpdt == 1) {icnt = icnt +  1;
	  if(icnt > 20) {cout << "icnt = " << icnt << endl;}}
	  tmpdt = 0;
          dt *= 0.500; 
//	  if(pdyr == 1969) {cout << std::scientific << "dtafter = " << dt << " " << time << " " << nintmon << endl;}
          if ( dt > (1.0 - time) ) { dt = 1.0 - time; } // ensure that last step doesn't go over the end time

        }

        if( nintmon == maxitmon )
        {
          time = 1.0;
          blackhol = 1;

          for( i = 0; i < numeq; ++i ) { 
            pstate[i] = oldstate[i];
//if(i == 35) {cout << "pstate = " << pstate[35] << " " << oldstate[35] << " " << nintmon << endl;}
 }
        }
      } /* end while test != ACCEPT */
    }    /* end rkf integrator */
  }      /* end time while */

  return mflag;

};

/* *************************************************************
************************************************************* */


/* **************************************************************
************************************************************** */

int Ttem45::boundcon( double ptstate[],
                      double err[],
                      const double& ptol )
{

  test = ACCEPT;
  int i;

/// Check carbon, nitrogen, and water state variables and fluxes

  for( i = 0; i < NUMEQ; ++i )
  {
    if( err[i] > fabs( ptol * ptstate[i] ) ) { 

       return test = temkey( i )+1; }
  }

  return test;

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void Ttem45::cropDynamics( const int& pdm, const int& pdyr, const double& nmax_grow, double pstate[] )
{

  double prob;
  double surf_wetness;
  double rsoil;
  int nopen;
  double nin,nout;
  double denitrs;
  double labilec_new,labilen_new,soilc_new,soiln_new,cnveg;

  double dtmax;

  dtmax = 0.03333;

#ifndef CALIBRATE_TEM

//  Irrigated are irigated crops (ag.state = 1), irrigated pasture
//  (ag.state = 3) and irrigated turf (ag.state = 5)
//  Fertilized are all crops, pasture, and turf (so all)
//  Perennial crops (ie switchgras) are not fertilized or irrigated

  ag.fertflag = 1;
  ag.irrgflag = 0;
  if(ag.state == 1 || ag.state == 3 || ag.state == 5)
  { 
    ag.irrgflag = 1;
  }

  if(ag.getISPERENNIAL(ag.cmnt) == 1 && ag.state == 2)
  {
    ag.irrgflag = 0;
    ag.fertflag = 0;
  }

#endif


  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in cropDynamics(), month = %2d ", pdm);
    refresh();
  #endif
//----------------------------------------------//
//  transfer information from pstate to veg biomass variables
  updateVegBiomass( pstate );

//----------------------------------------------//
//  update variables used in natvegdynamics:
//    ag.fertn
//    atms.lwoutd, atms.lwoutn, atms.nirrn
//    soil.avlh2o, soil.gm, soil.kh2o, soil.ninput
//    veg.lai, veg.rmt
//
  ag.fertn = ZERO;

  if((ag.fertflag == 1) && (ag.state == 1 || ag.state == 2) ) {
    ag.fertn = ag.getFERTNCROP()/12000.;
  }
  else if (ag.fertflag == 1 && (ag.state == 3 || ag.state == 4) )
  {
    ag.fertn = ag.getFERTNPASTURE()/12000.;

  }
  else if (ag.fertflag == 1 && (ag.state == 5 || ag.state == 6))
  {
   ag.fertn = 15.0/12.;
  }
  else
  {
   ag.fertn = 0.0;
  }

/*if((ag.fertflag == 1) && (ag.state == 1 || ag.state == 3) && pdm == 4) {
#ifdef CALIBRATE_TEM
//   ag.fertn = 15.0;
   ag.fertn = 15.0/12.0;
#endif
} */

  atms.setLWOUTD( atms.lwrad( atms.getTAIRD(),
                              atms.getVPR(),
                              atms.getNIRR(),
                              atms.getGIRR() ) );
                              
  atms.setLWOUTN( atms.lwrad( atms.getTAIRN(),
                              atms.getVPR(),
                              atms.getNIRR(),
                              atms.getGIRR() ) );
  // nirrn already set

  soil.setAVLH2O( pstate[I_SM] - soil.getWILTPT() );
  if( soil.getAVLH2O() < ZERO )
  {
    soil.setAVLH2O( ZERO );
  }
  soil.setVSM( pstate[I_VSM] );
  soil.setSWP();
  soil.setMOIST( pstate[I_SM] );
  soil.setKH2O( pstate[I_VSM], moistlim );
  soil.setNINPUT( ag.getNRETENT() );
//  soil.setNINPUT( 0.0);
  soil.setNLOST( 0.0);

/*  if( pstate[I_LEAFC] > ZERO )
  {
    veg.setLAI( (pstate[I_LEAFC]*veg.getSLA(ag.cmnt)) );
  }
  else
  {
    veg.setLAI( ZERO );
  } */ 

  veg.setLAI( pstate[I_LAI] );

  veg.aerodynamics( ag.cmnt,
	                 atms.getWS10() );

  if( 1 == rsoilmethod )
  {
    prob = 1.0 - exp(-0.005*atms.getPREC() );
    if( prob < ZERO ) { prob = ZERO; }
    
    surf_wetness = soil.getAVLH2O()/soil.getAWCAPMM();
    if( surf_wetness < ZERO ) { surf_wetness = ZERO; }
    
    rsoil = 54.65/(prob*surf_wetness + 0.01);
    
    veg.pen.setR_SS( rsoil );
  }

  if( 2 == rsoilmethod )
  {
    //surf_wetness = 100.0*soil.getVSM()/soil.getPCTPOR(); // assume surface wetness is like bulk moisture content
    prob = 1.0 - exp(-0.005*atms.getPREC() );
    if( prob < ZERO ) { prob = ZERO; }
     
    surf_wetness = soil.getAVLH2O()/soil.getAWCAPMM();
    //surf_wetness = 100.0*soil.getVSM()/soil.getPCTPOR();
    if( surf_wetness > 1.0 ) { surf_wetness = 1.0; }
    if( surf_wetness < ZERO ) { surf_wetness = ZERO; }

    //if(soil.getSNOWPACK() > ZERO) { surf_wetness = 1.0; }
  
//    veg.pen.setR_SS( exp(8.206 - 4.205*surf_wetness*sqrt(prob)) ); // using modified formulation from SiB2
    veg.pen.setR_SS( exp(8.206 - 4.205*sqrt(surf_wetness*prob)) ); // using formulation from SiB2
  }
	                 
  if( (ag.getGROWDD() >= ag.getGDDSEED(ag.cmnt))
      || (1 == ag.getISPERENNIAL(ag.cmnt))) 
  // this conditional appears multiple places below where routines are not needed
  //   if crops have not started to grow, and crops are annual
  {
    veg.pen.hydraulics( ag.cmnt,
                        veg.getPHEN( ag.cmnt ),
                        veg.getLAI(),
                        veg.getSLA( ag.cmnt ),
                        veg.getSAPWOODC(),
                        veg.getROOTC(),
                        soil.getAVLH2O(),
                        soil.getAWCAPMM(),
                        atms.getPREC() );
  }                     
	                 
  veg.rxTLaRS( ag.cmnt,
                  atms.getTAIR(),
                  veg.getRATREF( ag.cmnt ) );
  if( 0 == o3flag ) { veg.setFOZONE( 1.0 ); }
  else { veg.setFOZONE( pstate[I_FOZONE] ); }
  
//----------------------------------------------//
//  determine litterfall and respiration fluxes  
  if( (ag.getGROWDD() >= ag.getGDDSEED(ag.cmnt))
      || (1 == ag.getISPERENNIAL(ag.cmnt)))
  {
    veg.litterresp( ag.cmnt,
                    atms.getNDAYS(pdm) );
  }
                  
//----------------------------------------------//
//  determine decomposition and mineralization fluxes  
  // Note: Microbes are assumed to be acting on "old" carbon
  //   (i.e. natural vegetation - veg.cmnt) rather than
  //   "new" carbon associated with crops (i.e. ag.cmnt)
 
nopen = 0; 
#ifdef OPENN
  nopen = 1;
#endif  

  microbe.updateDynamics( ag.cmnt,
                          soil.getPCTFLDCAP(),
                          soil.getPCTWILTPT(),
                          soil.getPCTPOR(),
                          pstate[I_SOLC],
                          pstate[I_SOLN],
                          pstate[I_SM],
                          pstate[I_VSM],
                          pstate[I_AVLN],
                          moistlim,
                          ag.tillflag,
                          ag.getTILLFACTOR( ag.cmnt ),
                          soil.getKH2O(),
                          veg.getRLTRC(),
                          nopen,
                          ag.getIMMBADD(),
                          ag.getVOLAC(),
                          ag.getVOLAN() );

//----------------------------------------------//
//  determine ingpp and innup; calculations for gpp are used in allocate

  if( (ag.getGROWDD() >= ag.getGDDSEED(ag.cmnt))
      || (1 == ag.getISPERENNIAL(ag.cmnt)))
  {
//
//  BSF add variables to check month when first planting crops
//
    gddinit = gddinit + 1;
    if(gddinit == 1)
    {
     idum = pdm;
    }

    veg.nupxclm( ag.cmnt,
                 nmax_grow,
                 pstate[I_SM],
                 pstate[I_AVLN],
                 veg.getRMT(),
                 soil.getKH2O(),
                 veg.getFOZONE(),
                 pstate[I_ROOTC] );
               

    veg.gppxclm( ag.cmnt,
                 atms.getNDAYS(pdm),
		 pdyr,
                 atms.getCO2(),
                 atms.getPAR(),
                 atms.getVPR(),
                 atms.getVPDD(),
                 atms.getDAYL(),
                 veg.pen.getKEXT(ag.cmnt),
                 microbe.getRH(),
                 atms.getPREC(),
                 veg.getVEGC(),
		 initFlag);

//cout << "gpp = " << ag.cmnt << " " << atms.getNDAYS(pdm) << " " << atms.getCO2() << " " << atms.getPAR() << " " << atms.getVPR() << " " << atms.getVPDD() << " " << atms.getDAYL() << " " << veg.pen.getKEXT(ag.cmnt) << " " << microbe.getRH() << " " << atms.getPREC() << " " << veg.getVEGC() << endl;
//----------------------------------------------//
//  determine allocation fluxes
  veg.allocate( ag.cmnt,
                atms.getNDAYS(pdm),
                nfeed,
                pdm,
                ag.getGROWDD(),
                ag.state,
	        pdyr );

//----------------------------------------------//
//  update vegetation dynamics

    if(idum == pdm)
    {
     ag.fertn = ag.fertn*(double)CYCLE;
    }
    else
    {
     ag.fertn = 0.0;
    }

    soil.setNINPUT( soil.getNINPUT() + ag.fertn );

    #ifdef OPENN
      soil.setNINPUT( soil.getNINPUT() + atms.getNDEP()/(1000.0*CYCLE) );
      soil.setSONINP((veg.getNNF(ag.cmnt)*0.102 * ((double)CYCLE*soil.getEET()/10.0)+ 0.524 ) /(10.0*(double)CYCLE));
      veg.setVEGNINP(((1.0-veg.getNNF(ag.cmnt))*0.102 * ((double)CYCLE*soil.getEET()/10.0)+ 0.524) /(10.0*(double)CYCLE));

  labilec_new = y[I_LABILEC]+ dtmax * (veg.getGPP() - veg.getALLOCLC() - veg.getALLOCSC() - veg.getALLOCRC() - veg.getALLOCSEEDC() - veg.getRGRWTH());
  labilen_new = y[I_LABILEN] + dtmax * (veg.getNUPTAKE() + veg.getVEGNINP() - veg.getALLOCLN() - veg.getALLOCSN() - veg.getALLOCRN() - veg.getALLOCSEEDN() + veg.getNRESORBL() + veg.getNRESORBS() + veg.getNRESORBR() + veg.getNRESORBSEED());
  soilc_new = y[I_SOLC] + dtmax * (veg.getLTRLC()+veg.getLTRSC()+veg.getLTRHC()+veg.getLTRRC()+veg.getLTRSEEDC()-microbe.getDOCPROD()-microbe.getRH());
  soiln_new = y[I_SOLN] + dtmax * (veg.getLTRLN()+veg.getLTRSN()+veg.getLTRHN()+veg.getLTRRN()+veg.getLTRSEEDN()+soil.getSONINP()-microbe.getDONPROD()-microbe.getNETNMIN()+ ag.getSLASHN() - ag.getSCONVRTFLXN() - ag.getNSRETENT());
  cnveg = (veg.getCNLEAF(veg.cmnt)+veg.getCNSAPWOOD(veg.cmnt)+veg.getCNROOT(veg.cmnt))/3.0;

//if(labilec_new/labilen_new < cnveg)
if((labilen_new/labilec_new)*100.0 > 10.0)
{
//  if(((labilec_new/veg.getCNLEAF(veg.cmnt))- labilen_new + dtmax * veg.getVEGNINP())/dtmax >= 0.0) {cout << "positive" << endl;}
//  veg.setVEGNINP(((labilec_new/veg.getCNLEAF(veg.cmnt))- labilen_new + dtmax * veg.getVEGNINP())/dtmax);
//  if(veg.getVEGNINP() < 0.0) { veg.setVEGNINP(0.0); }
  veg.setVEGNINP(0.0);
}

//if(soilc_new/soiln_new >= microbe.getCNSOIL(veg.cmnt)) { cout << "no change in nfixn" << endl; }
if(soilc_new/soiln_new < microbe.getCNSOIL(veg.cmnt)-10.0)
{
//  soil.setSONINP(((soilc_new/microbe.getCNSOIL(veg.cmnt)) - soiln_new + dtmax * soil.getSONINP())/dtmax);
//  if(soil.getSONINP() < 0.0) { soil.setSONINP(0.0); }
  soil.setSONINP(0.0);
}

//  Intermediate NFIX
//      soil.setSONINP((veg.getNNF(veg.cmnt)*0.234 * ((double)CYCLE*soil.getEET()/10.0)+ 0.172 ) /(10.0*(double)CYCLE));
//      veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.234 * ((double)CYCLE*soil.getEET()/10.0)+ 0.172) /(10.0*(double)CYCLE));
//      if(initFlag == 1) { cout << "cropveg = " << soil.getSONINP() << " " << veg.getVEGNINP() << " " << soil.getEET() << endl;}
    #endif



  veg.updateDynamics( ag.cmnt,
                     soil.getNINPUT(),
                     pstate[I_AVLN],
                     nfeed,
                     ag.state,
                     ag.getISPERENNIAL(ag.cmnt),
                     microbe.getNETNMIN(),
                     ag.fertn,
                     soil.getSONINP(),
		     initFlag,
		     pdyr );
//    soil.setSONINP(veg.getSONINPUT());

  }
  else
  {
    // No crop plants exist - set all monthly fluxes to zero

    ag.fertn = 0.0;
    soil.setNINPUT( soil.getNINPUT() + ag.fertn + atms.getNDEP()/(1000.0*CYCLE) );
    #ifdef OPENN
      veg.setVEGNINP( ZERO );
      soil.setSONINP((veg.getNNF(ag.cmnt)*0.102 * ((double)CYCLE*soil.getEET()/10.0)+ 0.524 ) /(10.0*(double)CYCLE));
    #endif
    veg.updateDynamics( ag.cmnt,
                     soil.getNINPUT(),
                     pstate[I_AVLN],
                     nfeed,
                     ag.state,
                     ag.getISPERENNIAL(ag.cmnt),
                     microbe.getNETNMIN(),
                     ag.fertn,
                     soil.getSONINP(),
		     initFlag,
		     pdyr );
//    soil.setSONINP(veg.getSONINPUT());


    veg.resetMonthlyFluxes();
  }

  veg.setESOILMMMO(0.0);
  veg.petsw( ag.cmnt,
             pdm,
             atms.getNDAYS(pdm),
             atms.getDAYL(),
             atms.getTAIRD(),
             atms.getTAIRN(),
             atms.getCO2(),
             atms.getNIRRN(),
             atms.getLWOUTD(),
             atms.getLWOUTN(),
             atms.getVPR(),
             atms.getVPDD(),
             atms.getVPDN(),
             soil.getSNOWPACK(),
             atms.getPREC(),
             veg.getESOILMMMO(),
             soil.getAVLH2O(),
             elev );
            
//cout << "eet terms = " << atms.getNDAYS(pdm) << " " << atms.getDAYL() << " " << atms.getTAIRD() << " " << atms.getTAIRN() << " " << atms.getCO2() << " " << atms.getNIRRN() << " " << atms.getLWOUTD() << " " << atms.getLWOUTN() << " " << atms.getVPR() << " " << atms.getVPDN() << " " << soil.getSNOWPACK() << " " << atms.getPREC() << " " << veg.getESOILMMMO() << " " << soil.getAVLH2O() << " " << elev << endl;

   veg.deltafo3( ag.cmnt,
		   pdyr,
              atms.getAOT40() );

   soil.updateHydrology( elev,
                atms.getTAIR(),
                atms.getPREVTAIR(),
                atms.getPREV2TAIR(),
                atms.getRAIN(),
                veg.getPET(),
                soil.getAVLH2O(),
                pstate[I_RGRW],
                pstate[I_SGRW],
                ag.irrgflag,
                ag.irrigate,
                pdm );

//cout << " water terms = " << atms.getRAIN() << " " << pstate[I_RGRW] << " " << soil.getRPERC() << " " << soil.getRRUN() << " " << soil.getAVLH2O() << " " << soil.getAWCAPMM() << " " << y[I_SM] << " " <<  soil.getWILTPT() << endl;
// if(initFlag == 1) {cout << "eet in cropveg = " << soil.getEET() << endl;}

  soil.updateDOCLEACH( pstate[I_DOC],
                 pstate[I_SM] );



  // Determine nitrogen losses from ecosystem

  if( 1 == avlnflag )
  {
#ifdef OPENN
  if(pstate[I_DOC] > 0.0)
  {
  soil.setLCHDON( soil.getLCHDOC() * pstate[I_DON]/pstate[I_DOC]);
  }
  else
  {
  soil.setLCHDON(ZERO);
  }

    soil.updateNLosses( ag.cmnt,
//                        (atms.getRAIN() + soil.getSNOWINF() - soil.getEET() ),
                        (soil.getRPERC() + soil.getSPERC()),
                        pstate[I_AVLN],
                        pstate[I_SM],
		        ag.irrigate,
		        ag.state );
//    cout << "ag.irrigate = " << ag.irrigate << endl;
    soil.setLCHDIN(soil.getNLOST());
 
/*   if(initFlag == 0)
   {
//if(nseed != 0.0 || nprod != 0.0) {cout << "nseed, nprod = " << pdm << " " << nseed << " " << nprod << endl;}
// nin = soil.getNINPUT() + soil.getSONINP() + veg.getVEGNINP();
  nin = soil.getNINPUT() + soil.getSONINP() + veg.getVEGNINP() + nseed;
// nout = soil.getNLOST() + soil.getLCHDON();
 nout = soil.getNLOST() + soil.getLCHDON() + ag.getCROPRESIDUEFLXN() + nprod;

//if(nseed != 0.0 || nprod != 0.0){ cout << "cropseedn = " << pdm << " " << nseed << " " << nprod  << " " << ag.getCROPRESIDUEFLXN() << endl;}

//if(pdm == 9) {cout << "diag = " << pdm << " " << soil.getNINPUT() << " " << soil.getSONINP() << " " << veg.getVEGNINP() << " " << nseed << " " << soil.getNLOST() << " " << soil.getLCHDON() << " " << ag.getCROPRESIDUEFLXN() << " " << nprod << " " << nin << " " << nout << endl;}

   if(nout < nin)
   {
//    if( ag.getCROPPRODN() > 0.0) {cout << "too much NIN " << nin - nout << " " << ag.getCROPPRODN() << endl;}
//    if(nseed != 0.0 || nprod != 0.0) cout << "too much NIN " << nin - nout << " " << nin << " " << nout << endl;
//    if(nseed != 0.0 || nprod != 0.0) cout << "too much NIN " << nseed << " " << nprod << endl;
      soil.setNLOST(soil.getNLOST() + (nin - nout));
//      soil.SetNINPUT(soil.getNINPUT() - (nin - nout));
   }
   if(nout > nin)
   {
//    if(nseed != 0.0 || nprod != 0.0) cout << "too much NOUT " << nout - nin << " " << nin << " " << nout << " " << soil.getNLOST() << " " << soil.getLCHDON() << " " << ag.getCROPRESIDUEFLXN() << " " << nprod << endl;
//    if(nseed != 0.0 || nprod != 0.0) cout << "too much NOUT " << nseed << " " << nprod  << endl;
      soil.setNINPUT(soil.getNINPUT() + (nout - nin));
//        soil.setNLOST(soil.getNLOST() - (nout - nin));
   }
   }
   else
   { */
//cout << "setNLOST in crops = " << soil.getNLOST() << " " << soil.getDENITR(ag.cmnt) << " " << microbe.getGMIN() << " " << veg.getDENITR() << endl;
//cout << "gmin in crops = " << microbe.getGMIN() << " " << soil.getNLOST() << " " << soil.getDENITR(ag.cmnt) << " " << veg.getDENITR() << endl;
    if((microbe.getNETNMIN() + soil.getNINPUT() - veg.getNUPTAKE() - soil.getNLOST()) > 0.0)
    {
      denitrs = 0.5 * (microbe.getNETNMIN() + soil.getNINPUT() - veg.getNUPTAKE() - soil.getNLOST());
    }
    else
    {
      denitrs = 0.0;
    }

//      setDENITR( 0.01*microbe.getGMIN() + denitrs);
      setDENITR( soil.getDENITR(ag.cmnt) * (0.01*microbe.getGMIN() + denitrs));

//    if(veg.getNUPTAKE() == 0.0)
        if(veg.getNUPTAKE() < 0.01)
    {
      setDENITR(microbe.getNETNMIN() + soil.getNINPUT() - veg.getNUPTAKE() - soil.getNLOST());
    }

//      setDENITR(soil.getDENITR(ag.cmnt) * (0.01*microbe.getGMIN() + veg.getDENITR()));
      soil.setNLOST( soil.getNLOST() +  getDENITR() );
//   }


#endif
/*  soil.setNLOST(soil.getNLOST() + soil.getLCHDON() + ag.getCONVRTFLXN() + ag.getCROPRESIDUEFLXN());

#ifdef OPENN
    if(soil.getLCHDON() > pstate[I_DON] + microbe.getDONPROD()) {
         soil.setLCHDON( pstate[I_DON] + microbe.getDONPROD() );
      }
      if(soil.getLCHDON() < ZERO ) { soil.setLCHDON( ZERO ); }

#endif */ 

/*  Ensure Nlost does not exceed the total avln pool
//
    if( soil.getNLOST()  > pstate[I_AVLN] - veg.getNUPTAKE()
         + microbe.getNETNMIN() + soil.getNINPUT() )

    {
      setDENITR(pstate[I_AVLN]- veg.getNUPTAKE()+ microbe.getNETNMIN()+ soil.getNINPUT() - soil.getLCHDIN());
      soil.setNLOST(getDENITR() + soil.getLCHDIN());
//      soil.setNLOST( (pstate[I_AVLN]
//                     - veg.getNUPTAKE()
//                     + microbe.getNETNMIN()
//                     + soil.getNINPUT()) );
    }
    if( soil.getNLOST() < ZERO )
    {
      soil.setNLOST( ZERO ); 

      microbe.setNETNMIN( (soil.getNLOST()
                          + veg.getNUPTAKE()
                          - soil.getNINPUT()
                          - pstate[I_AVLN]) );
    } */ 
  }  // end availn loop
  else
  {
    // Do not allow changes in available nitrogen in soil
    //   (used during calibration process)

    soil.setNLOST( (soil.getNINPUT()
                   + microbe.getNETNMIN()
                       - veg.getNUPTAKE()) );
  } 

if( soil.getLCHDIN() > ZERO )
  {
    // Limit ecosystem N losses to total inorganic N available for
    //   leaching after denitrification has been substracted

    if ( soil.getLCHDIN() > (pstate[I_AVLN] + atms.getNDEP()/12000.0
         + microbe.getNETNMIN() + ag.fertn - getDENITR()
         - veg.getNUPTAKE()) )
    {
      if ( (pstate[I_AVLN] + atms.getNDEP()/12000.0 + microbe.getNETNMIN()
           + ag.fertn - getDENITR() - veg.getNUPTAKE()) > ZERO )
      {
        soil.setLCHDIN( (pstate[I_AVLN]
                           + atms.getNDEP()/12000.0
                           + microbe.getNETNMIN()
                           + ag.fertn
                           - getDENITR()
                           - veg.getNUPTAKE()) );
      }
      else
      {
        soil.setLCHDIN( ZERO );

        if( (pstate[I_AVLN] + atms.getNDEP()/12000.0 + microbe.getNETNMIN()
            + ag.fertn - veg.getNUPTAKE()) > ZERO )
        {
          setDENITR((pstate[I_AVLN]
                               + atms.getNDEP()/12000.0
                               + microbe.getNETNMIN()
                               + ag.fertn
                               - veg.getNUPTAKE())) ;
        }
        else
        {
          setDENITR(ZERO);
          veg.setNUPTAKE( (pstate[I_AVLN]
                             + atms.getNDEP()/12000.0
                             + microbe.getNETNMIN())
                             + ag.fertn );
        }
      }
    }
  }
  else
  {
    // If no leaching losses, limit ecosystem N losses to
    //   total inorganic N available for denitrification

    if( getDENITR() > (pstate[I_AVLN] + atms.getNDEP()/12000.0
         + ag.fertn + microbe.getNETNMIN() - veg.getNUPTAKE()) )
    {
      if( (pstate[I_AVLN] + atms.getNDEP()/12000.0 + microbe.getNETNMIN()
          + ag.fertn - veg.getNUPTAKE()) > ZERO )
      {
        setDENITR((pstate[I_AVLN]
                             + atms.getNDEP()/12000.0
                             + microbe.getNETNMIN()
                             + ag.fertn
                             - veg.getNUPTAKE())) ;
      }
      else
      {
        setDENITR(ZERO);

        veg.setNUPTAKE( (pstate[I_AVLN]
                           + atms.getNDEP()/12000.0
                           + ag.fertn
                           + microbe.getNETNMIN()) );
      }


    }
  }

//setDENITR(soil.getDENITR(veg.cmnt) * getDENITR());
soil.setNLOST(getDENITR() + soil.getLCHDIN());

  if( 0 == avlnflag )
//  if( 0 == initFlag )
  {
    // Do not allow changes in available nitrogen in soil
    //   (used during calibration process)

    // Equilibrate inorganic N pool

    soil.setLCHDIN( (atms.getNDEP()/12000.0
                       + microbe.getNETNMIN()
                       - getDENITR()
                       - veg.getNUPTAKE()) );

    // Equilibrate DON pool

    soil.setLCHDON( microbe.getDONPROD() );
  }

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void Ttem45::delta( const int& pdm,
                    const int& pdyr,
                    const double& nmax_grow,
                    double pstate[],
                    double pdstate[] )
{
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in delta(), month = %2d ", pdm);
    refresh();
  #endif
  
//  ++delta_count;
//  if( delta_count > 1000000 ) { exit(-1); }
  if( 0 == ag.state || 7 == ag.state )
  {
    natvegDynamics( pdm, pdyr, nmax_grow, pstate );
  }
  else
  {
    cropDynamics( pdm, pdyr, nmax_grow, pstate );
  }

  // Describe monthly changes to carbon pools and fluxes for ODE
  //   state variables (i.e., pdstate)

  // Carbon pools in ecosystems


  pdstate[I_LEAFC] = veg.getALLOCLC()
                  - veg.getRMLEAF()
                  - veg.getLTRLC();

//cout << "leafc = " << pdstate[I_LEAFC] << " " << veg.getALLOCLC() << " " << veg.getRMLEAF() << " " << veg.getLTRLC() << " " << veg.getLEAFC() << " " << pstate[0] << endl;
//  if( pstate[I_LEAFC] > ZERO )
//  {
   pdstate[I_LAI] = pdstate[I_LEAFC]*veg.getSLA(veg.cmnt);
//  }
//  else
//  {
//   pdstate[I_LAI] = ZERO;
//  }

  pdstate[I_SAPWOODC] = veg.getALLOCSC()
                      - veg.getALLOCHC()
                      - veg.getRMSAPWOOD()
                      - veg.getLTRSC();

//cout << "sapwoodc = " << veg.getALLOCSC() << " " << veg.getALLOCHC() << " " << veg.getRMSAPWOOD() << " " << veg.getLTRSC() << endl;

  pdstate[I_HEARTWOODC] = veg.getALLOCHC()
                        - veg.getLTRHC();

//  cout << "heartwoodc = " << pdstate[I_HEARTWOODC] << " " << veg.getALLOCHC() << " " <<  veg.getLTRHC() << " " << veg.getHEARTWOODC()  << endl;

  pdstate[I_ROOTC] = veg.getALLOCRC()
                  - veg.getRMROOT()
                  - veg.getLTRRC();

  pdstate[I_SEEDC] = veg.getALLOCSEEDC()
                    - veg.getRMSEED()
                    - veg.getLTRSEEDC();

  pdstate[I_LABILEC] = veg.getGPP()
                  - veg.getALLOCLC()
                  - veg.getALLOCSC()
                  - veg.getALLOCRC()
                  - veg.getALLOCSEEDC()
                  - veg.getRGRWTH();

//if (pdyr > 800) {cout << "labile C = " << pdstate[I_LABILEC] << " " << veg.getGPP() << " " << veg.getALLOCLC() << " " << veg.getALLOCSC() << " " << veg.getALLOCRC() << " " << veg.getALLOCSEEDC() <<  " " << veg.getRGRWTH() << endl;}
//  pdstate[I_SOLC] = 0.865 * veg.getLTRLC()
  pdstate[I_SOLC] =  veg.getLTRLC()
//                    + 0.865 *  veg.getLTRSC()
//                    + 0.865 * veg.getLTRHC()
//                    + 0.865 * veg.getLTRRC()
//                    + 0.865 * veg.getLTRSEEDC()
                    + veg.getLTRSC()
                    + veg.getLTRHC()
                    + veg.getLTRRC()
                    + veg.getLTRSEEDC()
                    + ag.getSLASHC()
//                    + 0.865 * ag.getSLASHC()
                    - ag.getSCONVRTFLXC()
                    - microbe.getDOCPROD()
                    - microbe.getRH();
//  ag.setSLASHC(0.0);


//  cout << "soil = " << pstate[I_SOLC] << " " << pdstate[I_SOLC] << " " << veg.getLTRLC() << " " << veg.getLTRSC() << " " << veg.getLTRHC() << " " << veg.getLTRRC() << " " << veg.getLTRSEEDC() << " " << ag.getSLASHC() << " " << ag.getSCONVRTFLXC() << " " << microbe.getRH() << " " << microbe.getDOCPROD() << endl;

  pdstate[I_DOC] = microbe.getDOCPROD() - soil.getLCHDOC();

//  cout << "doc = " << pdstate[I_DOC] << " " << microbe.getDOCPROD() << " " << soil.getLCHDOC() << endl;

#ifdef OPENN
  pdstate[I_DON] = microbe.getDONPROD() - soil.getLCHDON();
#endif 

  pdstate[I_FOZONE] = veg.getDFO3();
  pdstate[I_FRDL] = veg.getFRDL();
  pdstate[I_FCO2] = veg.getFCO2();
  pdstate[I_TEMP] = veg.getTEMP();
  pdstate[I_FH2O] = veg.getFH2O();
  pdstate[I_FO3] = veg.getFOZONE(); 

  // Nitrogen pools in ecosystems

  pdstate[I_LEAFN] = veg.getALLOCLN()
                   - veg.getNRESORBL()
                   - veg.getLTRLN();

  pdstate[I_SAPWOODN] = veg.getALLOCSN()
                      - veg.getALLOCHN()
                      - veg.getNRESORBS()
                      - veg.getLTRSN();

  pdstate[I_HEARTWOODN] = veg.getALLOCHN()
                        - veg.getLTRHN();

  pdstate[I_ROOTN] = veg.getALLOCRN()
//                   + veg.getVEGNINP()
                   - veg.getNRESORBR()
                   - veg.getLTRRN();

//  cout << "pdstate = " << pdstate[I_ROOTN] << " " << veg.getALLOCRN() << " " << veg.getNRESORBR() << " " << veg.getLTRRN() << " " << y[I_ROOTN] << endl;

  pdstate[I_SEEDN] = veg.getALLOCSEEDN()
                   - veg.getNRESORBSEED()
                   - veg.getLTRSEEDN();


#ifdef OPENN
//  pdstate[I_SOLN] =   0.865 * veg.getLTRLN()
  pdstate[I_SOLN] =    veg.getLTRLN()
//                    + 0.865 * veg.getLTRSN()
//                    + 0.865 * veg.getLTRHN()
//                    + 0.865 * veg.getLTRRN()
//                    + 0.865 * veg.getLTRSEEDN()
//                    + 0.865 * ag.getSLASHN()
                    +  veg.getLTRSN()
                    +  veg.getLTRHN()
                    +  veg.getLTRRN()
                    +  veg.getLTRSEEDN()
                    +  ag.getSLASHN()
                    + soil.getSONINP()
                    - microbe.getDONPROD()
                    - ag.getSCONVRTFLXN()
                    - ag.getNSRETENT()
                    - microbe.getNETNMIN();


//if(initFlag == 1)   cout << "deltasoln = " << pdstate[I_SOLN] << " " << ag.getNSRETENT() << " " << veg.getLTRLN() << " " << veg.getLTRSN() << " " << veg.getLTRHN() << " " << veg.getLTRRN() << " " << veg.getLTRSEEDN() << " " << ag.getSLASHN() << " "  << soil.getSONINP() << " " << microbe.getDONPROD() << " " << ag.getSCONVRTFLXN() << " " << microbe.getNETNMIN() << endl;
//   if(initFlag == 1) {cout << "soninp = " << soil.getSONINP() << " " << y[I_SOLN] << endl;}

   pdstate[I_LABILEN] = veg.getNUPTAKE()
                     + veg.getVEGNINP()
                     - veg.getALLOCLN()
                     - veg.getALLOCSN()
                     - veg.getALLOCRN()
                     - veg.getALLOCSEEDN()
                     + veg.getNRESORBL()
                     + veg.getNRESORBS()
                     + veg.getNRESORBR()
                     + veg.getNRESORBSEED();

//cout << "labilen = " << pdstate[I_LABILEN] << " " << veg.getNUPTAKE() << " " << veg.getVEGNINP() << " " << veg.getNRESORBL() << " " << veg.getNRESORBS() << " " << veg.getNRESORBR() << " " << veg.getNRESORBSEED() << " " <<  veg.getALLOCLN() << " " << veg.getALLOCSN() << " " << veg.getALLOCRN() << " " << veg.getALLOCSEEDN() << endl;

#else
  pdstate[I_SOLN] =   veg.getLTRLN()
                    + veg.getLTRSN()
                    + veg.getLTRHN()
                    + veg.getLTRRN()
                    + veg.getLTRSEEDN()
                    + ag.getSLASHN()
                    - ag.getSCONVRTFLXN()
                    - ag.getNSRETENT()
                    - microbe.getNETNMIN();


  pdstate[I_LABILEN] = veg.getNUPTAKE()
                     - veg.getALLOCLN()
                     - veg.getALLOCSN()
                     - veg.getALLOCRN()
                     - veg.getALLOCSEEDN()
                     + veg.getNRESORBL()
                     + veg.getNRESORBS()
                     + veg.getNRESORBR()
                     + veg.getNRESORBSEED();

#endif 


  pdstate[I_AVLN] = soil.getNINPUT()
                    + microbe.getNETNMIN()
                    + ag.getVOLAN()
//                    + veg.getVEGNINP()
                    - veg.getNUPTAKE()
                    - soil.getNLOST();

//cout << "pdavln = " << pdstate[I_AVLN] << " " << soil.getNINPUT() << " " << microbe.getNETNMIN() << " " <<  ag.getVOLAN() << " " << veg.getNUPTAKE() << " " << soil.getNLOST() << " " << pstate[I_AVLN] << endl;
//if(veg.cmnt == 15) {cout << "pdavln = " << pdstate[I_AVLN] << " " << soil.getNINPUT() << " " << microbe.getNETNMIN() << " " <<  ag.getVOLAN() << " " << veg.getNUPTAKE() << " " << soil.getNLOST() << " " << pstate[I_AVLN] << " " << soil.getRRUN() << " " << soil.getSRUN() << " " << pstate[I_SM] << endl;}
//if(initFlag == 1) {cout << "pdavln = " << pdstate[I_AVLN] << " " << soil.getNINPUT() << " " << microbe.getNETNMIN() << " " <<  ag.getVOLAN() << " " << veg.getNUPTAKE() << " " << soil.getNLOST() << " " << pstate[I_AVLN] << " " << soil.getRRUN() << " " << soil.getSRUN() << " " << pstate[I_SM] << endl;}
//if(pstate[I_AVLN] < 0.0) {cout << "pdavln = " << pdstate[I_AVLN] << " " << soil.getNINPUT() << " " << microbe.getNETNMIN() << " " <<  ag.getVOLAN() << " " << veg.getNUPTAKE() << " " << soil.getNLOST() << " " << pstate[I_AVLN] << " " << soil.getRRUN() << " " << soil.getSRUN() << " " << pstate[I_SM] << endl;}


  // Water pools

//  if((veg.cmnt == 9 || veg.cmnt == 8 || veg.cmnt == 6) && atms.getMXTAIR() > 25.0)
//  {
//  pdstate[I_SM] = soil.getSNOWINF()
//                    + ag.irrigate
//                    - soil.getEET()
//                    - soil.getRPERC()
//                    - soil.getSPERC();
//  }
//  else
//  {
  pdstate[I_SM] = soil.getSNOWINF()
                    + atms.getRAIN()
                    + ag.irrigate
                    - soil.getEET()
                    - soil.getRPERC()
                    - soil.getSPERC();
//  }
//if (ag.irrigate > 0.0) {cout << "ag.irrigate  = " << ag.irrigate << endl;}
//if(veg.cmnt == 15) {cout << "delta sm = " << pdstate[I_SM] << " " << pstate[I_SM] << " " << soil.getEET() << " " << atms.getRAIN() << " " << soil.getRPERC() << " " << soil.getSPERC() << endl;}

  pdstate[I_VSM] = pdstate[I_SM]/(soil.getROOTZ() * 1000.0);

  if( pstate[I_VSM]+pdstate[I_VSM] <= ZERO )
  {
    pdstate[I_VSM] = 0.001 - pstate[I_VSM];
  }

  pdstate[I_PCTP] = 100.0 * pdstate[I_SM]/soil.getTOTPOR();

  pdstate[I_RGRW] = soil.getRPERC() - soil.getRRUN();

  pdstate[I_SGRW] = soil.getSPERC() - soil.getSRUN();

  // Phenology

  pdstate[I_FPC] = veg.getFPC();
//  pdstate[I_FPC] = veg.getVEGNINP();

  // Carbon fluxes in ecosystems
  pdstate[I_ALLOCLC] = veg.getALLOCLC();
  pdstate[I_ALLOCSC] = veg.getALLOCSC();
  pdstate[I_ALLOCHC] = veg.getALLOCHC();
  pdstate[I_ALLOCRC] = veg.getALLOCRC();
  pdstate[I_ALLOCSEEDC] = veg.getALLOCSEEDC();
  
  pdstate[I_ALLOCILC] = veg.getALLOCILC();
  pdstate[I_ALLOCISC] = veg.getALLOCISC();
  pdstate[I_ALLOCIHC] = veg.getALLOCIHC();
  pdstate[I_ALLOCIRC] = veg.getALLOCIRC();
  pdstate[I_ALLOCISEEDC] = veg.getALLOCISEEDC();

  pdstate[I_INGPP] = veg.getINGPP();
  pdstate[I_GPP] = veg.getGPP();


  pdstate[I_RVMNT] = veg.getRMLEAF() + veg.getRMSAPWOOD() + veg.getRMROOT()
                   + veg.getRMSEED();

  pdstate[I_RMLEAF] = veg.getRMLEAF();
  pdstate[I_RMSAPWOOD] = veg.getRMSAPWOOD();
  pdstate[I_RMROOT] = veg.getRMROOT();
  pdstate[I_RMSEED] = veg.getRMSEED();
  pdstate[I_RMLABILE] = veg.getRMLABILE();

  pdstate[I_RVGRW] = veg.getRGRWTH();

  pdstate[I_LTRLC] = veg.getLTRLC();
  pdstate[I_LTRSC] = veg.getLTRSC();
  pdstate[I_LTRHC] = veg.getLTRHC();
  pdstate[I_LTRRC] = veg.getLTRRC();
  pdstate[I_LTRSEEDC] = veg.getLTRSEEDC();
  pdstate[I_RH] = microbe.getRH();



  // Nitrogen fluxes in ecosystems

  pdstate[I_ALLOCLN] = veg.getALLOCLN();
  pdstate[I_ALLOCSN] = veg.getALLOCSN();
  pdstate[I_ALLOCHN] = veg.getALLOCHN();
  pdstate[I_ALLOCRN] = veg.getALLOCRN();
  pdstate[I_ALLOCSEEDN] = veg.getALLOCSEEDN();
  
  pdstate[I_ALLOCILN] = veg.getALLOCILN();
  pdstate[I_ALLOCISN] = veg.getALLOCISN();
  pdstate[I_ALLOCIHN] = veg.getALLOCIHN();
  pdstate[I_ALLOCIRN] = veg.getALLOCIRN();
  pdstate[I_ALLOCISEEDN] = veg.getALLOCISEEDN();

  pdstate[I_NINP] = soil.getNINPUT();

  pdstate[I_DOCPROD] = microbe.getDOCPROD();
  pdstate[I_LCHDOC] = soil.getLCHDOC();
// BSF SLOW
//#ifdef OPENN
  pdstate[I_DONPROD] = microbe.getDONPROD();
  pdstate[I_LCHDON] = soil.getLCHDON();
  pdstate[I_LCHDIN] = soil.getLCHDIN();
  pdstate[I_SLASHC] = ag.getSLASHC();
  pdstate[I_SLASHN] = ag.getSLASHN();
//#endif  

  pdstate[I_AGFRTN] = ag.fertn;

  pdstate[I_INNUP] = veg.getINUPTAKE();

  pdstate[I_VNUP] = veg.getNUPTAKE();

  pdstate[I_NRESORBL] = veg.getNRESORBL();
  pdstate[I_NRESORBS] = veg.getNRESORBS();
  pdstate[I_NRESORBR] = veg.getNRESORBR();
  pdstate[I_NRESORBSEED] = veg.getNRESORBSEED();


  pdstate[I_LTRLN] = veg.getLTRLN();
  pdstate[I_LTRSN] = veg.getLTRSN();
  pdstate[I_LTRHN] = veg.getLTRHN();
  pdstate[I_LTRRN] = veg.getLTRRN();
  pdstate[I_LTRSEEDN] = veg.getLTRSEEDN();

  pdstate[I_MNUP] = microbe.getNUPTAKE();

  pdstate[I_NMIN] = microbe.getNETNMIN();

  pdstate[I_NLST] = soil.getNLOST();
//BSF SLOW
//#ifdef OPENN
  pdstate[I_NFIXN] = soil.getSONINP();
  pdstate[I_NFIXS] = veg.getVEGNINP();
//#endif 
//  pdstate[I_NLST] = soil.getNLOST() + soil.getLCHDON() + ag.getCONVRTFLXN() + ag.getCROPRESIDUEFLXN();
  // Water fluxes

  pdstate[I_AGIRRIG] = ag.irrigate;

  pdstate[I_INEET] = soil.getINEET();
  pdstate[I_EET] = soil.getEET();
//  if(initFlag == 1) {cout << "delta eet = " << pdstate[I_EET] << " " << pstate[I_EET] << endl;}
  pdstate[I_RPERC] = soil.getRPERC();
  pdstate[I_SPERC] = soil.getSPERC();
  pdstate[I_RRUN] = soil.getRRUN();
  pdstate[I_SRUN] = soil.getSRUN();

  // Penmon model

  pdstate[I_GC] = veg.getGC();
  pdstate[I_GS] = veg.getGS();
  
  pdstate[I_PECAN] = veg.getPECANW();
  pdstate[I_PESOIL] = veg.getPESOILW();

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

#ifdef CALIBRATE_TEM

void Ttem45::displayOptionalEflx( const seykey& s )
{
  switch( s )
  {
    case GET_VEGC:    printw(" VEG. C "); break;
    case GET_STRN:    printw(" STR. N "); break;
    case GET_SOILC:   printw(" SOIL C "); break;
    case GET_SOILN:   printw(" SOIL N "); break;
    case GET_AVALN:   printw(" AVL. N "); break;
    
    case GET_LEAFC:    printw(" LEAF C "); break;
    case GET_SAPWOODC:    printw(" SAP. C "); break;
    case GET_HEARTWOODC:    printw(" HRT. C "); break;
    case GET_ROOTC:    printw(" ROOT C "); break;
    case GET_SEEDC:    printw(" SEED C "); break;
    case GET_LABILEC:    printw(" LAB. C "); break;
    case GET_LEAFN:    printw(" LEAF N "); break;
    case GET_SAPWOODN:    printw(" SAP. N "); break;
    case GET_HEARTWOODN:    printw(" HRT. N "); break;
    case GET_ROOTN:    printw(" ROOT N "); break;
    case GET_SEEDN:    printw(" SEED N "); break;
    case GET_LABILEN:    printw(" LAB. N "); break;


    case GET_DOC:     printw("   DOC  "); break;
    case GET_DON:     printw("   DON  "); break;

    case GET_LAI:     printw("   LAI  "); break;
    case GET_FPC:     printw("   FPC  "); break;

    case GET_ALLOCLC:    printw(" A-> LC "); break;
    case GET_ALLOCSC:    printw(" A-> SC "); break;
    case GET_ALLOCHC:    printw(" A-> HC "); break;
    case GET_ALLOCRC:    printw(" A-> RC "); break;
    case GET_ALLOCSEEDC:    printw(" A->SDC "); break;
    case GET_ALLOCILC:    printw(" A->ILC "); break;
    case GET_ALLOCISC:    printw(" A->ISC "); break;
    case GET_ALLOCIHC:    printw(" A->IHC "); break;
    case GET_ALLOCIRC:    printw(" A->IRC "); break;
    case GET_ALLOCISEEDC:    printw(" A->ISDC"); break;
    case GET_INGPP:    printw("  INGPP "); break;
    case GET_GPP:      printw("   GPP  "); break;
    case GET_KPLANT:   printw(" KPLANT "); break;
    case GET_INNPP:    printw("  INNPP "); break;
    case GET_NPP:      printw("   NPP  "); break;
    case GET_GPR:      printw("    RA  "); break;
    case GET_RVMNT:    printw("  RVMNT "); break;
    case GET_RMLEAF:   printw(" RMLEAF "); break;
    case GET_RMSAPWOOD:printw(" RmSAP. "); break;
    case GET_RMROOT:   printw(" RmROOT "); break;
    case GET_RMSEED:   printw(" RmSEED "); break;
    case GET_RMLABILE: printw(" RmLAB. "); break;
    case GET_RVGRW:    printw(" RGROWTH"); break;
    case GET_LTRLC:    printw(" LTRLC  "); break;
    case GET_LTRSC:    printw(" LTRSC  "); break;
    case GET_LTRHC:    printw(" LTRHC  "); break;
    case GET_LTRRC:    printw(" LTRRC  "); break;
    case GET_LTRSEEDC: printw(" LTRSDC "); break;
    case GET_AGSTUBC:  printw(" AgSTUBC"); break;
    case GET_RH:       printw("    RH  "); break;
    case GET_NEP:      printw("   NEP  "); break;

    case GET_D40:      printw(" AOT40  "); break;
    case GET_NDEP:     printw(" NDEP  "); break;
    case GET_FOZONE:   printw(" FOZONE "); break;

    case GET_ALLOCLN:  printw(" A-> LN "); break;
    case GET_ALLOCSN:  printw(" A-> SN "); break;
    case GET_ALLOCHN:  printw(" A-> HN "); break;
    case GET_ALLOCRN:  printw(" A-> RN "); break;
    case GET_ALLOCSEEDN: printw(" A->SDN "); break;
    case GET_ALLOCILN: printw(" A->ILN "); break;
    case GET_ALLOCISN: printw(" A->ISN "); break;
    case GET_ALLOCIHN: printw(" A->IHN "); break;
    case GET_ALLOCIRN: printw(" A->IRN "); break;
    case GET_ALLOCISEEDN: printw(" A->ISDN"); break;
    case GET_NINP:     printw(" NINPUT "); break;
    case GET_AGFRTN:   printw(" AgFERTN"); break;
    case GET_INNUP:    printw("  INNUP "); break;
    case GET_VNUP:     printw(" UPTAKE "); break;
    case GET_NRESORBL: printw(" NRSRBL "); break;
    case GET_NRESORBS: printw(" NRSRBS "); break;
    case GET_NRESORBR:   printw(" NRSRBR "); break;
    case GET_NRESORBSEED:   printw(" NRSRBSD"); break;
    case GET_LTRLN:     printw(" LTRLN  "); break;
    case GET_LTRSN:     printw(" LTRSN  "); break;
    case GET_LTRHN:     printw(" LTRHN  "); break;
    case GET_LTRRN:     printw(" LTRRN  "); break;
    case GET_LTRSEEDN:     printw(" LTRSDN "); break;
    case GET_AGSTUBN:  printw(" AgSTUBN"); break;
    case GET_MNUP:     printw(" NIMMOB "); break;
    case GET_NMIN:     printw(" NETNMIN"); break;
    case GET_NLST:     printw("  NLOST "); break;
    case GET_NFIXN:     printw("  NFIXN "); break;
    case GET_NFIXS:     printw("  NFIXS "); break;

    case GET_CNVRTC:   printw(" CNVRTC "); break;
    case GET_VCNVRTC:  printw(" VCNVRTC"); break;
    case GET_SCNVRTC:  printw(" SCNVRTC"); break;
    case GET_SLASHC:   printw(" SLASHC "); break;
    case GET_STANDDEADC:   printw(" STANDDEADC "); break;
    case GET_STANDDEADN:   printw(" STANDDEADN "); break;
    case GET_VOLAC:   printw(" VOLAC "); break;
    case GET_VOLAN:   printw(" VOLAN "); break;
    case GET_FRDL:   printw("  FRDL "); break;
    case GET_FCO2:   printw("  FCO2 "); break;
    case GET_FH2O:   printw("  FH2O "); break;
    case GET_TEMP:   printw("  TEMP "); break;
    case GET_FO3:   printw("  FO3 "); break;

    case GET_CFLX:     printw("  CFLUX "); break;
    case GET_NCE:      printw("   NCE  "); break;
    case GET_NBP:      printw("   NBP  "); break;

    case GET_CNVRTN:   printw(" CNVRTN "); break;
    case GET_VCNVRTN:  printw(" VCNVRTN"); break;
    case GET_SCNVRTN:  printw(" SCNVRTN"); break;
    case GET_SLASHN:   printw(" SLASHN "); break;
    case GET_NRETNT:   printw(" NRETNT "); break;
    case GET_NVRTNT:   printw(" NVRTNT "); break;
    case GET_NSRTNT:   printw(" NSRTNT "); break;

    case GET_AGPRDC:   printw(" AGPRODC "); break;
    case GET_CLIPPINGS:   printw(" CLIPPINGS "); break;
    case GET_PROD10C:  printw(" PROD10C "); break;
    case GET_PROD100C: printw(" PROD100C"); break;
    case GET_RESIDC:   printw("  RESIDC "); break;

    case GET_AGPRDN:   printw(" AGPRODN "); break;
    case GET_PROD10N:  printw(" PROD10N "); break;
    case GET_PROD100N: printw(" PROD100N"); break;
    case GET_RESIDN:   printw("  RESIDN "); break;

    case GET_AGFPRDC:  printw(" AGFPRDC "); break;
    case GET_PRDF10C:  printw(" PRDF10C "); break;
    case GET_PRDF100C: printw(" PRDF100C"); break;
    case GET_FRESIDC:  printw(" FRESIDC "); break;
    case GET_AGPRDFC:  printw(" AGPRDFC "); break;
    case GET_PRD10FC:  printw(" PRD10FC "); break;
    case GET_PRD100FC: printw(" PRD100FC"); break;
    case GET_TOTPRDFC: printw(" TOTPRDFC"); break;
    case GET_RESIDFC:  printw(" RESIDFC "); break;

    case GET_AGFPRDN:  printw(" AGFPRDN "); break;
    case GET_PRDF10N:  printw(" PRDF10N "); break;
    case GET_PRDF100N: printw(" PRDF100N"); break;
    case GET_FRESIDN:  printw(" FRESIDN "); break;
    case GET_AGPRDFN:  printw(" AGPRDFN "); break;
    case GET_PRD10FN:  printw(" PRD10FN "); break;
    case GET_PRD100FN: printw(" PRD100FN"); break;
    case GET_TOTPRDFN: printw(" TOTPRDFN"); break;
    case GET_RESIDFN:  printw(" RESIDFN "); break;

    case GET_L2SN:     printw("   LCON  "); break;
    case GET_DOCPROD:  printw(" DOCPROD "); break;
    case GET_LCHDOC:   printw(" LCHDOC  "); break;
    case GET_DONPROD:  printw(" DONPROD "); break;
    case GET_LCHDON:   printw(" LCHDON  "); break;
    case GET_LCHDIN:   printw(" LCHDIN  "); break;
    case GET_DENITR:   printw(" DENITR  "); break;
  }

};

#endif

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

#ifdef CALIBRATE_TEM

void Ttem45::displayOptionalWflx( const swykey& s )
{
  switch( s )
  {
    case GET_SH2O:    printw(" SMOIST "); break;
    case GET_PCTP:    printw("  PCTP  "); break;
    case GET_VSM:     printw("  VSM   "); break;

    case GET_RAIN:    printw("  RAIN  "); break;
    case GET_WS10:    printw("  WIND  "); break;
    case GET_SNWFAL:  printw(" SNWFAL "); break;
    case GET_GDD:     printw("  GDD   ");    break;
    case GET_SNWINF:  printw(" SNWINF "); break;
    case GET_TAIR:    printw(" TAIR   "); break;
    case GET_TAIRD:   printw(" TAIRD  "); break;
    case GET_TAIRN:   printw(" TAIRN  "); break;
    case GET_NIRR:   printw(" NIRR  "); break;
    case GET_PAR:   printw(" PAR  "); break;
    case GET_GIRR:   printw(" GIRR  "); break;
    case GET_CLDS:   printw(" CLDS  "); break;
    
    case GET_AGIRRIG: printw(" IRRIG  "); break;
    case GET_PET:     printw("  PET   "); break;
    case GET_INEET:   printw("  INEET "); break;
    case GET_EET:     printw("  EET   "); break;
    case GET_RPERC:   printw("  RPERC "); break;
    case GET_SPERC:   printw("  SPERC "); break;
    case GET_RRUN:    printw("  RRUN  "); break;
    case GET_SRUN:    printw("  SRUN  "); break;
    case GET_WYLD:    printw("  WYLD  "); break;

    case GET_GC:      printw("   GC   "); break;
    case GET_GS:      printw("   GS   "); break;
    case GET_PESOIL:  printw(" PESOIL "); break;
    case GET_PECAN:   printw("  PECAN "); break;
    case GET_SHFLUX:  printw(" SHFLUX "); break;
    case GET_SWP:     printw("   SWP  "); break;
	case GET_VEGH:    printw("  VEGH  "); break;
    case GET_USTAR:   printw(" USTAR  "); break;
	case GET_ZD:      printw("   ZD   "); break;
	case GET_ZO:      printw("   ZO   "); break;
	
	case GET_R_AA:    printw("  R_AA  "); break;
    case GET_R_AC:    printw("  R_AC  "); break;
	case GET_R_AS:    printw("  R_AS  "); break;
	case GET_R_SS:    printw("  R_SS  "); break;
	
	case GET_VAPR:    printw("  VAPR  "); break;
	case GET_VPDD:    printw("  VPDD  "); break;
	case GET_VPDN:    printw("  VPDN  "); break;
	
    case GET_AVLH2O:  printw(" AVLH2O "); break;
    case GET_RGRNDW:  printw(" RGRNDW "); break;
    case GET_SGRNDW:  printw(" SGRNDW "); break;
    case GET_SNWPCK:  printw(" SNWPCK "); break;

  }

};

#endif

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

int Ttem45::ecdqc( const int& dcmnt )
{

  int qc = ACCEPT;

  if( leafcb[dcmnt] <= -9999.99 ) { return qc = 102; }

  if( sapwoodcb[dcmnt] <= -9999.99 ) { return qc = 104; }

  if( heartwoodcb[dcmnt] <= -9999.99 ) { return qc = 106; }

  if( rootcb[dcmnt] <= -9999.99 ) { return qc = 108; }

  if( seedcb[dcmnt] <= -9999.99 ) { return qc = 110; }

  if( labilecb[dcmnt] <= -9999.99 ) { return qc = 112; }

  if( leafnb[dcmnt] <= -9999.99 ) { return qc = 114; }

  if( sapwoodnb[dcmnt] <= -9999.99 ) { return qc = 116; }

  if( heartwoodnb[dcmnt] <= -9999.99 ) { return qc = 118; }

  if( rootnb[dcmnt] <= -9999.99 ) { return qc = 120; }

  if( seednb[dcmnt] <= -9999.99 ) { return qc = 122; }

  if( labilenb[dcmnt] <= -9999.99 ) { return qc = 124; }

  if( solcb[dcmnt] <= -9999.99 ) { return qc = 126; }

  if( solnb[dcmnt] <= -9999.99 ) { return qc = 128; }

  if( avlnb[dcmnt] <= -9999.99 ) { return qc = 130; }

  if( veg.getCMAX1B( dcmnt ) <= -9999.99 ) { return qc = 135; }
  if( veg.getTAULEAF( dcmnt ) <= -99.99 ) { return qc = 138; }
  if( veg.getTAUSAPWOOD( dcmnt ) <= -99.99 ) { return qc = 139; }
  if( veg.getTAUHEARTWOOD( dcmnt ) <= -99.99 ) { return qc = 140; }
  if( veg.getTAUROOT( dcmnt ) <= -99.99 ) { return qc = 141; }
  if( veg.getTAUSEED( dcmnt ) <= -99.99 ) { return qc = 142; }

  if( veg.getKRA( dcmnt ) <= -99.99 ) { return qc = 143; }

  if( microbe.getKDB( dcmnt ) <= -99.99 ) { return qc = 146; }

  if( microbe.getLCCLNC( dcmnt ) <= -99.99 ) { return qc = 147; }
  if( microbe.getPROPFTOS( dcmnt ) <= -99.99 ) { return qc = 148; }

  if( veg.getNMAX1B( dcmnt ) <= -9999.99 ) { return qc = 153; }

  if( microbe.getNUPB( dcmnt ) <= -9999.99 ) { return qc = 157; }

  if( soil.getNLOSS( dcmnt ) <= -99.99 ) { return qc = 158; }

  if( soil.getDENITR( dcmnt ) <= -99.99 ) { return qc = 159; }

  if( veg.getCNLTR( dcmnt ) <= -99.99 ) { return qc = 164; }

  if( microbe.getCNSOIL( dcmnt ) <= -9999.99 ) { return qc = 165; }

  if( veg.getO3PARA( dcmnt ) <= -99.99 ) { return qc = 193; }
  if( veg.getO3PARB( dcmnt ) <= -99.99 ) { return qc = 194; }
  if( veg.getO3PARC( dcmnt ) <= -99.99 ) { return qc = 195; }

  return qc;

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************** */

void Ttem45::ECDsetODEstate( const int& pdcmnt,
                             const double& psiplusc )
{
  // Initialize the NUMEQ state variables used in the
  //   ODE integrator from ECD and DAT files

  y[I_LEAFC] = leafcb[pdcmnt];

  if( y[I_LEAFC] < ZERO ) { y[I_LEAFC] = ZERO; }

  y[I_LAI] = y[I_LEAFC] * veg.getSLA(pdcmnt);

  y[I_SAPWOODC] = sapwoodcb[pdcmnt];


  if( y[I_SAPWOODC] < ZERO ) { y[I_SAPWOODC] = ZERO; }



  y[I_HEARTWOODC] = heartwoodcb[pdcmnt];

  if( y[I_HEARTWOODC] < ZERO ) { y[I_HEARTWOODC] = ZERO; }

  y[I_ROOTC] = rootcb[pdcmnt];

  if( y[I_ROOTC] < ZERO ) { y[I_ROOTC] = ZERO; }

  y[I_SEEDC] = seedcb[pdcmnt];

  if( y[I_SEEDC] < ZERO ) { y[I_SEEDC] = ZERO; }

  y[I_LABILEC] =  labilecb[pdcmnt];

  if( y[I_LABILEC] < ZERO ) { y[I_LABILEC] = ZERO; }


  y[I_SOLC] = solcb[pdcmnt];

  if( y[I_SOLC] < ZERO ) { y[I_SOLC] = ZERO; }

  y[I_DOC] =  ZERO;

  y[I_DON] =  ZERO;


  y[I_FOZONE] = 1.0;


  y[I_LEAFN] = leafnb[pdcmnt];

  if( y[I_LEAFN] < ZERO ) { y[I_LEAFN] = ZERO; }

  y[I_SAPWOODN] = sapwoodnb[pdcmnt];

  if( y[I_SAPWOODN] < ZERO ) { y[I_SAPWOODN] = ZERO; }

  y[I_HEARTWOODN] =  heartwoodnb[pdcmnt];

  if( y[I_HEARTWOODN] < ZERO ) { y[I_HEARTWOODN] = ZERO; }

  y[I_ROOTN] =  rootnb[pdcmnt];

  if( y[I_ROOTN] < ZERO ) { y[I_ROOTN] = ZERO; }

  y[I_SEEDN] = seednb[pdcmnt];

  if( y[I_SEEDN] < ZERO ) { y[I_SEEDN] = ZERO; }

  y[I_LABILEN] = labilenb[pdcmnt];

  if( y[I_LABILEN] < ZERO ) { y[I_LABILEN] = ZERO; }


  y[I_SOLN] = solnb[pdcmnt];

  if( y[I_SOLN] < ZERO ) { y[I_SOLN] = ZERO; }


  y[I_AVLN] = avlnb[pdcmnt];

  if( y[I_AVLN] < ZERO ) { y[I_AVLN] = ZERO; }


  y[I_SM] = soil.getAWCAPMM() + soil.getWILTPT();


  if( y[I_SM] <= ZERO )
  {
    y[I_SM] = 0.001;
  }

  y[I_VSM] = y[I_SM] / (soil.getROOTZ() * 1000.0);

  if( y[I_VSM] <= ZERO )
  {
    y[I_VSM] = 0.001;
  }


  y[I_PCTP] = 100.0 * y[I_SM] / soil.getTOTPOR();

  y[I_RGRW] = ZERO;

  y[I_SGRW] =  ZERO;


  // Initialize all phenology and flux states to zero

  resetODEflux();

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

void Ttem45::getenviron( void )
{
  // Determine monthly potential evapotranspiration

//  atms.petjh( atms.getNIRR(), atms.getTAIR(), pdm );



  // Determine contribution of snowmelt to soil moisture

  soil.setSNOWINF( soil.snowmelt( elev,
                                  atms.getTAIR(),
                                  atms.getPREVTAIR(),
                                  soil.getPREVSPACK() ) );

  // Determine new snow pack

  soil.setSNOWPACK( (soil.getPREVSPACK()
                     + atms.getSNOWFALL()
                     - soil.getSNOWINF()) );


  if( soil.getSNOWPACK() < ZERO )
  {
    soil.setSNOWPACK( ZERO );
  }

//  ninput = (soil.getLCHDON()+soil.getNLOST())*atms.getPREC()/atms.yrprec;

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

#ifdef CALIBRATE_TEM

double Ttem45::getOptionalEflx( const int& optflx )
{

  double outflux;

  switch( optflx )
  {
    case GET_VEGC:    outflux = veg.getVEGC(); break;
//    case GET_VEGC:    outflux = y[I_LEAFC] + y[I_SAPWOODC] +  y[I_HEARTWOODC] +  y[I_SEEDC] +  y[I_ROOTC] +  y[I_LABILEC]; break;
    case GET_STRN:    outflux = veg.getSTRN(); break;
    case GET_SOILC:   outflux = y[I_SOLC]; break;
    case GET_SOILN:   outflux = y[I_SOLN]; break;
    case GET_AVALN:   outflux = y[I_AVLN]; break;
    
    case GET_LEAFC:    outflux = y[I_LEAFC]; break;
    case GET_SAPWOODC:    outflux = y[I_SAPWOODC]; break;
    case GET_HEARTWOODC:    outflux = y[I_HEARTWOODC]; break;
    case GET_ROOTC:    outflux = y[I_ROOTC]; break;
    case GET_SEEDC:    outflux = y[I_SEEDC]; break;
    case GET_LABILEC:    outflux = y[I_LABILEC]; break;
    case GET_LEAFN:    outflux = y[I_LEAFN]; break;
    case GET_SAPWOODN:    outflux = y[I_SAPWOODN]; break;
    case GET_HEARTWOODN:    outflux = y[I_HEARTWOODN]; break;
    case GET_ROOTN:    outflux = y[I_ROOTN]; break;
    case GET_SEEDN:    outflux = y[I_SEEDN]; break;
    case GET_LABILEN:    outflux = y[I_LABILEN]; break;

    case GET_LAI:     outflux = veg.getLAI(); break;
    case GET_FPC:     outflux = y[I_FPC]; break;

    case GET_ALLOCLC:    outflux = y[I_ALLOCLC]; break;
    case GET_ALLOCSC:    outflux = y[I_ALLOCSC]; break;
    case GET_ALLOCHC:    outflux = y[I_ALLOCHC]; break;
    case GET_ALLOCRC:    outflux = y[I_ALLOCRC]; break;
    case GET_ALLOCSEEDC:    outflux = y[I_ALLOCSEEDC]; break;
    case GET_ALLOCILC:    outflux = y[I_ALLOCILC]; break;
    case GET_ALLOCISC:    outflux = y[I_ALLOCISC]; break;
    case GET_ALLOCIHC:    outflux = y[I_ALLOCIHC]; break;
    case GET_ALLOCIRC:    outflux = y[I_ALLOCIRC]; break;
    case GET_ALLOCISEEDC:    outflux = y[I_ALLOCISEEDC]; break;
    case GET_INGPP:    outflux = y[I_INGPP]; break;
    case GET_GPP:      outflux = y[I_GPP]; break;
    case GET_KPLANT:   outflux = veg.pen.getKPLANT(); break;
    case GET_INNPP:    outflux = veg.getINNPP(); break;
    case GET_NPP:      outflux = veg.getNPP(); break;
    case GET_GPR:      outflux = veg.getGPR(); break;
    case GET_RVMNT:    outflux = y[I_RVMNT]; break;
    case GET_RMLEAF:   outflux = y[I_RMLEAF]; break;
    case GET_RMSAPWOOD:   outflux = y[I_RMSAPWOOD]; break;
    case GET_RMROOT:   outflux = y[I_RMROOT]; break;
    case GET_RMSEED:   outflux = y[I_RMSEED]; break;
    case GET_RMLABILE:   outflux = y[I_RMLABILE]; break;
    case GET_RVGRW:    outflux = y[I_RVGRW]; break;
    case GET_LTRLC:     outflux = y[I_LTRLC]; break;
    case GET_LTRSC:     outflux = y[I_LTRSC]; break;
    case GET_LTRHC:     outflux = y[I_LTRHC]; break;
    case GET_LTRRC:     outflux = y[I_LTRRC]; break;
    case GET_LTRSEEDC:     outflux = y[I_LTRSEEDC]; break;
    case GET_RH:       outflux = y[I_RH]; break;
    case GET_DOC:       outflux = y[I_DOC]; break;
    case GET_DON:       outflux = y[I_DON]; break;
    case GET_NEP:      outflux = nep; break;

    case GET_D40:      outflux = atms.getAOT40(); break;
    case GET_NDEP:     outflux = atms.getNDEP()/12000.0; break;
    case GET_FOZONE:   outflux = y[I_FOZONE]; break;

    case GET_ALLOCLN:    outflux = y[I_ALLOCLN]; break;
    case GET_ALLOCSN:    outflux = y[I_ALLOCSN]; break;
    case GET_ALLOCHN:    outflux = y[I_ALLOCHN]; break;
    case GET_ALLOCRN:    outflux = y[I_ALLOCRN]; break;
    case GET_ALLOCSEEDN:    outflux = y[I_ALLOCSEEDN]; break;
    case GET_ALLOCILN:    outflux = y[I_ALLOCILN]; break;
    case GET_ALLOCISN:    outflux = y[I_ALLOCISN]; break;
    case GET_ALLOCIHN:    outflux = y[I_ALLOCIHN]; break;
    case GET_ALLOCIRN:    outflux = y[I_ALLOCIRN]; break;
    case GET_ALLOCISEEDN:    outflux = y[I_ALLOCISEEDN]; break;
    case GET_NINP:     outflux = y[I_NINP]; break;
    case GET_AGFRTN:   outflux = y[I_AGFRTN]; break;
    case GET_INNUP:    outflux = y[I_INNUP]; break;
    case GET_VNUP:     outflux = y[I_VNUP]; break;
    case GET_NRESORBL:   outflux = y[I_NRESORBL]; break;
    case GET_NRESORBS:   outflux = y[I_NRESORBS]; break;
    case GET_NRESORBR:   outflux = y[I_NRESORBR]; break;
    case GET_NRESORBSEED:   outflux = y[I_NRESORBSEED]; break;
    case GET_LTRLN:     outflux = y[I_LTRLN]; break;
    case GET_LTRSN:     outflux = y[I_LTRSN]; break;
    case GET_LTRHN:     outflux = y[I_LTRHN]; break;
    case GET_LTRRN:     outflux = y[I_LTRRN]; break;
    case GET_LTRSEEDN:     outflux = y[I_LTRSEEDN]; break;
    case GET_AGSTUBN:  outflux = ag.getSTUBBLEN(); break;
    case GET_MNUP:     outflux = y[I_MNUP]; break;
    case GET_NMIN:     outflux = y[I_NMIN]; break;
    case GET_NLST:     outflux = y[I_NLST]; break;
    case GET_NFIXN:     outflux = y[I_NFIXN]; break;
    case GET_NFIXS:     outflux = y[I_NFIXS]; break;

    case GET_CNVRTC:   outflux = ag.getCONVRTFLXC();  break;
    case GET_VCNVRTC:  outflux = ag.getVCONVRTFLXC();  break;
    case GET_SCNVRTC:  outflux = ag.getSCONVRTFLXC();  break;
    case GET_SLASHC:   outflux = ag.getSLASHC();  break;
    case GET_STANDDEADC:   outflux = ag.getSTANDDEADC();  break;
    case GET_STANDDEADN:   outflux = ag.getSTANDDEADN();  break;
    case GET_VOLAC:   outflux = ag.getVOLAC();  break;
    case GET_VOLAN:   outflux = ag.getVOLAN();  break;
    case GET_FRDL:     outflux = y[I_FRDL]; break;
    case GET_FCO2:     outflux = y[I_FCO2]; break;
    case GET_FH2O:     outflux = y[I_FH2O]; break;
    case GET_TEMP:     outflux = y[I_TEMP]; break;
    case GET_FO3:     outflux = y[I_FO3]; break;
    case GET_CFLX:     outflux = ag.getCFLUX();  break;
    case GET_NCE:      outflux = nce;  break;
    case GET_NBP:      outflux = nbp;  break;

    case GET_CNVRTN:   outflux = ag.getCONVRTFLXN();  break;
    case GET_VCNVRTN:  outflux = ag.getVCONVRTFLXN();  break;
    case GET_SCNVRTN:  outflux = ag.getSCONVRTFLXN();  break;
    case GET_SLASHN:   outflux = ag.getSLASHN();  break;
    case GET_NRETNT:   outflux = ag.getNRETENT();  break;
    case GET_NVRTNT:   outflux = ag.getNVRETENT();  break;
    case GET_NSRTNT:   outflux = ag.getNSRETENT();  break;

    case GET_AGSTUBC:  outflux = ag.getSTUBBLEC(); break;
    case GET_RESIDC:   outflux = ag.getCROPRESIDUEC();  break;

    case GET_AGPRDC:   outflux = ag.getPROD1C();  break;
    case GET_CLIPPINGS: outflux = ag.getCLIPPINGS();  break;
    case GET_PROD10C:  outflux = ag.getPROD10C();  break;
    case GET_PROD100C: outflux = ag.getPROD100C();  break;

    case GET_AGPRDN:   outflux = ag.getPROD1N();  break;
    case GET_PROD10N:  outflux = ag.getPROD10N();  break;
    case GET_PROD100N: outflux = ag.getPROD100N();  break;
    case GET_RESIDN:   outflux = ag.getCROPRESIDUEN();  break;

    case GET_FRESIDC:  outflux = ag.getFORMCROPRESIDUEC();  break;

    case GET_AGFPRDC:  outflux = ag.getCROPPRODC();  break;
    case GET_PRDF10C:  outflux = ag.getFORMPROD10C();  break;
    case GET_PRDF100C: outflux = ag.getFORMPROD100C();  break;
    case GET_AGPRDFC:  outflux = ag.getPROD1DECAYC();  break;
    case GET_PRD10FC:  outflux = ag.getPROD10DECAYC();  break;
    case GET_PRD100FC: outflux = ag.getPROD100DECAYC();  break;
    case GET_TOTPRDFC: outflux = ag.getTOTPRODDECAYC();  break;
    case GET_RESIDFC:  outflux = ag.getCROPRESIDUEFLXC();  break;

    case GET_AGFPRDN:  outflux = ag.getCROPPRODN();  break;
    case GET_PRDF10N:  outflux = ag.getFORMPROD10N();  break;
    case GET_PRDF100N: outflux = ag.getFORMPROD100N();  break;
    case GET_FRESIDN:  outflux = ag.getFORMCROPRESIDUEN();  break;
    case GET_AGPRDFN:  outflux = ag.getPROD1DECAYN();  break;
    case GET_PRD10FN:  outflux = ag.getPROD10DECAYN();  break;
    case GET_PRD100FN: outflux = ag.getPROD100DECAYN();  break;
    case GET_TOTPRDFN: outflux = ag.getTOTPRODDECAYN();  break;
    case GET_RESIDFN:  outflux = ag.getCROPRESIDUEFLXN();  break;

    case GET_DOCPROD:  outflux = y[I_DOCPROD];  break;
    case GET_LCHDOC:  outflux = y[I_LCHDOC];  break;
    case GET_DONPROD:  outflux = y[I_DONPROD];  break;
    case GET_LCHDON:  outflux = y[I_LCHDON];  break;
    case GET_LCHDIN:  outflux = y[I_LCHDIN];  break;
//    case GET_DENITR:  outflux = veg.getDENITR();  break;
    case GET_DENITR:  outflux = getDENITR();  break;

    case GET_L2SN:     if ( veg.getSTRN() != ZERO )
                       {
                         outflux = y[I_LABILEN]/veg.getSTRN();
                       }
                       else { outflux = MISSING; } break;

    default:           outflux = MISSING;
  }

  return outflux;

};

#endif

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

#ifdef CALIBRATE_TEM

double Ttem45::getOptionalWflx( const int& optflx )
{

  double outflux;

  switch( optflx )
  {
    case GET_SH2O:    outflux = y[I_SM]; break;
    case GET_PCTP:    outflux = y[I_PCTP]; break;
    case GET_VSM:     outflux = 100.0*y[I_VSM]; break;

    case GET_RAIN:    outflux = atms.getRAIN(); break;
    case GET_WS10:    outflux = atms.getWS10(); break;
    case GET_SNWFAL:  outflux = atms.getSNOWFALL(); break;
    case GET_GDD:     outflux = ag.getGROWDD(); break;
    case GET_SNWINF:  outflux = soil.getSNOWINF(); break;
    case GET_AGIRRIG: outflux = y[I_AGIRRIG]; break;
    case GET_TAIR:    outflux = atms.getTAIR(); break;
    case GET_TAIRD:    outflux = atms.getTAIRD(); break;
    case GET_TAIRN:    outflux = atms.getTAIRN(); break;
    case GET_NIRR:    outflux = atms.getNIRR(); break;
    case GET_PAR:    outflux = atms.getPAR(); break;
    case GET_GIRR:    outflux = atms.getGIRR(); break;
    case GET_CLDS:    outflux = atms.getCLDS(); break;
    
    case GET_PET:     outflux = veg.getPET(); break;
    case GET_INEET:   outflux = y[I_INEET]; break;
    case GET_EET:     outflux = y[I_EET]; break;
    case GET_RPERC:   outflux = y[I_RPERC]; break;
    case GET_SPERC:   outflux = y[I_SPERC]; break;
    case GET_RRUN:    outflux = y[I_RRUN]; break;
    case GET_SRUN:    outflux = y[I_SRUN]; break;
    case GET_WYLD:    outflux = soil.getH2OYLD(); break;

    case GET_GC:      outflux = y[I_GC]; break;
    case GET_GS:   outflux = y[I_GS]; break;
    case GET_PESOIL:   outflux = veg.getPESOILW(); break;
    case GET_PECAN:   outflux = veg.getPECANW(); break;
    case GET_SHFLUX:   outflux = veg.getSHFLUXW(); break;
    case GET_SWP:     outflux = soil.getSWP(); break;
    case GET_VEGH:    outflux = veg.pen.getVEGH(); break;
	case GET_USTAR:   outflux = veg.pen.getUSTAR(); break; 
	case GET_ZD:      outflux = veg.pen.getZD(); break;
	case GET_ZO:      outflux = veg.pen.getZO(); break;
	case GET_R_AA:    outflux = veg.pen.getR_AA(); break;
    case GET_R_AC:    outflux = veg.pen.getR_AC(); break;
	case GET_R_AS:    outflux = veg.pen.getR_AS(); break;
	case GET_R_SS:    outflux = veg.pen.getR_SS(); break;
	
	case GET_VAPR:    outflux = atms.getVPR(); break;
	case GET_VPDD:    outflux = atms.getVPDD(); break;
	case GET_VPDN:    outflux = atms.getVPDN(); break;
	
//    case GET_AVLH2O:  outflux = (y[I_SM] - soil.getWILTPT()); break;
    case GET_AVLH2O:  outflux = soil.getAVLH2O(); break;
    case GET_RGRNDW:  outflux = y[I_RGRW]; break;
    case GET_SGRNDW:  outflux = y[I_SGRW]; break;
    case GET_SNWPCK:  outflux = soil.getSNOWPACK(); break;
    
    default:          outflux = MISSING;
  }

  return outflux;

};

#endif

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void Ttem45::loadSiteCalibrations( std::ostream& log )
{
  const auto& top = config.root();
  bool loaded[MAXCMNT] = {};
  int slot = 0;
  for( const auto& entry : top.items )
  {
    const string& key = entry.first;
    if( key.size() <= 4 || key.compare( key.size() - 4, 4, "_dat" ) != 0 )
      continue;

    const auto& file = entry.second;
    if( file.kind != tem::yaml::Node::Scalar || file.text.empty() )
    {
      cerr << key << " needs a site calibration file path" << endl;
      exit( -1 );
    }
    if( slot == MAXCMNT - 1 )
    {
      cerr << "Too many *_dat calibration entries. Maximum is " << MAXCMNT - 1 << endl;
      exit( -1 );
    }
    getsitecd( slot, file.text, loaded );
    ++slot;
    log << " " << key << " opened " << file.text << " for community " << veg.cmnt << endl;
  }
  if( slot == 0 )
  {
    cerr << "TEM configuration needs at least one top-level *_dat calibration entry" << endl;
    exit( -1 );
  }
}

void Ttem45::getsitecd( int dv, const string& ecd, bool (&loaded)[MAXCMNT] )
{

  fecd[dv].open( ecd );

  version = fecd[dv].getOptional( "version" );
  sitename = fecd[dv].getOptional( "site" );
  sitecol = fecd[dv].getOptional( "longitude" );
  siterow = fecd[dv].getOptional( "latitude" );
  developer = fecd[dv].getOptional( "developedBy" );
  updated = fecd[dv].getOptional( "updated" );

  veg.cmnt = fecd[dv].nextCommunity( &description );

  if( veg.cmnt < 1 || veg.cmnt >= MAXCMNT )
  {
    cerr << "Invalid community type " << veg.cmnt << " in " << ecd << endl;
    exit( -1 );
  }
  if( loaded[veg.cmnt] )
  {
    cerr << "Duplicate calibration for community " << veg.cmnt << " in " << ecd << endl;
    exit( -1 );
  }
  loaded[veg.cmnt] = true;

  leafcb[veg.cmnt] = fecd[dv].getDouble( "leafcb" );

  sapwoodcb[veg.cmnt] = fecd[dv].getDouble( "sapwoodcb" );
//  cout << "getsitecd = " << veg.cmnt << " " << sapwoodcb[veg.cmnt] << endl;

  heartwoodcb[veg.cmnt] = fecd[dv].getDouble( "heartwoodcb" );

  rootcb[veg.cmnt] = fecd[dv].getDouble( "rootcb" );

  seedcb[veg.cmnt] = fecd[dv].getDouble( "seedcb" );

  labilecb[veg.cmnt] = fecd[dv].getDouble( "labilecb" );

  leafnb[veg.cmnt] = fecd[dv].getDouble( "leafnb" );

  sapwoodnb[veg.cmnt] = fecd[dv].getDouble( "sapwoodnb" );

  heartwoodnb[veg.cmnt] = fecd[dv].getDouble( "heartwoodnb" );

  rootnb[veg.cmnt] = fecd[dv].getDouble( "rootnb" );

  seednb[veg.cmnt] = fecd[dv].getDouble( "seednb" );

  labilenb[veg.cmnt] = fecd[dv].getDouble( "labilenb" );

  solcb[veg.cmnt] = fecd[dv].getDouble( "solcb" );

  solnb[veg.cmnt] = fecd[dv].getDouble( "solnb" );

  avlnb[veg.cmnt] = fecd[dv].getDouble( "avlnb" );

  veg.setCMAX1B( fecd[dv].getDouble( "vegcmax1b" ),
                 veg.cmnt );

  veg.setTAULEAF( fecd[dv].getDouble( "vegtauleaf" ),
                veg.cmnt );

  veg.setTAUSAPWOOD( fecd[dv].getDouble( "vegtausapwood" ),
              veg.cmnt );

  veg.setTAUHEARTWOOD( fecd[dv].getDouble( "vegtauheartwood" ),
            veg.cmnt );

  veg.setTAUROOT( fecd[dv].getDouble( "vegtauroot" ),
              veg.cmnt );

  veg.setTAUSEED( fecd[dv].getDouble( "vegtauseed" ),
            veg.cmnt );

  veg.setKRA( fecd[dv].getDouble( "vegkra" ),
              veg.cmnt );

  microbe.setKDB( fecd[dv].getDouble( "microbekdb" ),
                  veg.cmnt );

  microbe.setLCCLNC( fecd[dv].getDouble( "microbelcclnc" ),
                     veg.cmnt );

  microbe.setPROPFTOS( fecd[dv].getDouble( "microbepropftos" ),
                       veg.cmnt );

  veg.setNMAX1B( fecd[dv].getDouble( "vegnmax1b" ),
                 veg.cmnt );


  microbe.setNUPB( fecd[dv].getDouble( "microbenupb" ),
                   veg.cmnt );

  soil.setNLOSS( fecd[dv].getDouble( "soilnloss" ),
                 veg.cmnt );

//  cout << "soilnloss = " << soil.getNLOSS(veg.cmnt) << " " << veg.cmnt << endl;

  soil.setDENITR( fecd[dv].getDouble( "soildenitr" ),
                 veg.cmnt );


  microbe.setCNSOIL( fecd[dv].getDouble( "microbecnsoil" ),
                     veg.cmnt );


  veg.setCNLTR( fecd[dv].getDouble( "vegcnltr" ),
          veg.cmnt );


  veg.setO3PARA( fecd[dv].getDouble( "o3para" ),
                 veg.cmnt );

  veg.setO3PARB( fecd[dv].getDouble( "o3parb" ),
                 veg.cmnt );

  veg.setO3PARC( fecd[dv].getDouble( "o3parc" ),
                 veg.cmnt );

  // ignition efficiency of GDSTEM-FIRE (site file), used when veg_fire is 1
  if( fecd[dv].hasEntry( "igef" ) )
  {
    veg.setIGEF( fecd[dv].getDouble( "igef" ), veg.cmnt );
  }

  fecd[dv].endEntry();


};

/* *************************************************************
************************************************************** */

/* *************************************************************
************************************************************** */

void Ttem45::initializeState( void )
{
  
if(ag.state == 0)
{
  setELMNTecd( veg.cmnt, soil.getPSIPLUSC() );
  ECDsetODEstate( veg.cmnt, soil.getPSIPLUSC() );
}
else
{
  setELMNTecd( ag.cmnt, soil.getPSIPLUSC() );
  ECDsetODEstate( ag.cmnt, soil.getPSIPLUSC() );
}


  // agriculture-related variables
  ag.setNATSEEDC( ZERO );
  ag.setNATSEEDSTRN( ZERO );
  ag.setNATSEEDSTON( ZERO );
  ag.setNATTOPT( veg.getTOPT() );
  ag.setCROPTOPT( veg.getTOPT() );
  ag.setPRVCROPNPP( ZERO ); 
  

  veg.setRNPP( 50.0 );
  soil.setREET( 0.0);
//  microbe.setRRH( 0.0 );
  veg.setRLTRC( 0.0 );
  veg.setRGPP( 100.0 ); 
  veg.setRINGPP( 100.0 ); 
  veg.setPHICNT( veg.getPHI() );
  veg.setRPLEAF( 1.0 );
  

  veg.setRLABILEC( y[I_LABILEC] );
  veg.setRLABILEN( y[I_LABILEN] );
  
  veg.setRDEMANDC( 100.0 );
  veg.setRDEMANDN( 10.0 );
//  veg.setRDEMANDN( 100.0/veg.getINITCNEVEN( veg.cmnt ) );
  

  ag.setPRODUCTYEAR( 0 );
  ag.resetPROD();
  ag.resetMonthlyDisturbFluxes(); 
  
  resetYrFluxes();
  resetMonthlyELMNTFluxes();
  setPrevState();


};

/* *************************************************************
************************************************************** */
void Ttem45::initializecalibState( void )
{

  ECDsetODEstate( veg.cmnt, soil.getPSIPLUSC() );

  // agriculture-related variables
  ag.setNATSEEDC( ZERO );
  ag.setNATSEEDSTRN( ZERO );
  ag.setNATSEEDSTON( ZERO );
  ag.setNATTOPT( veg.getTOPT() );
  ag.setCROPTOPT( veg.getTOPT() );
  ag.setPRVCROPNPP( ZERO );


  veg.setRNPP( 50.0 );
  soil.setREET( 0.0 );
//  microbe.setRRH( 0.0 );
  veg.setRLTRC( 0.0 );
  veg.setRGPP( 100.0 );
  veg.setRINGPP( 100.0 );
  veg.setPHICNT( veg.getPHI() );
  veg.setRPLEAF( 1.0 );

  veg.setRLABILEC( y[I_LABILEC] );
  veg.setRLABILEN( y[I_LABILEN] );

  veg.setRDEMANDC( 100.0 );
  veg.setRDEMANDN( 10.0 );
//  veg.setRDEMANDN( 100.0/veg.getINITCNEVEN( veg.cmnt ) );

  ag.setPRODUCTYEAR( 0 );
  ag.resetPROD();
  ag.resetMonthlyDisturbFluxes();

  resetYrFluxes();
  resetMonthlyELMNTFluxes();
  setPrevState();

};


/* *************************************************************
************************************************************** */

void Ttem45::massbal( void )
{


  if( (y[I_SM] - prevy[I_SM]) != (soil.getSNOWINF() + atms.getRAIN()
      + y[I_AGIRRIG] - y[I_RPERC] - y[I_EET] - y[I_SPERC]) )
  {
    y[I_SPERC] = soil.getSNOWINF()
                 + atms.getRAIN()
                 + y[I_AGIRRIG]
                 - y[I_RPERC]
                 - y[I_EET]
                 - y[I_SM]
                 + prevy[I_SM];
  }

 if( y[I_SM] <= ZERO ) { y[I_SM] = 0.001; }

  // DWK added following statement on 20020401 to keep SPERC positive
  // when agricultural croplands are abandoned

  if( y[I_SPERC] < ZERO ) { y[I_SPERC] = ZERO; }

  if( y[I_PCTP] != 100.0 * y[I_SM] / soil.getTOTPOR() )
  {
    y[I_PCTP] = 100.0 * y[I_SM] / soil.getTOTPOR();
  }

  if( y[I_PCTP] < ZERO ) { y[I_PCTP] = ZERO; }

  if( y[I_VSM] != y[I_SM] / (soil.getROOTZ() * 1000.0) )
  {
    y[I_VSM] = y[I_SM] / (soil.getROOTZ() * 1000.0);

    if( y[I_VSM] <= ZERO ) { y[I_VSM] = 0.001; }
  }


  if( (y[I_RGRW] - prevy[I_RGRW]) != (y[I_RPERC] - y[I_RRUN]) )
  {
    y[I_RRUN] = y[I_RPERC] - y[I_RGRW] + prevy[I_RGRW];
  }

if( y[I_RGRW] < ZERO ) { y[I_RGRW] = ZERO; }
  // DWK added following statement on 20020401 to keep RRUN positive
  // when agricultural croplands are abandoned

  if( y[I_RRUN] < ZERO ) { y[I_RRUN] = ZERO; }


  if( (y[I_SGRW] - prevy[I_SGRW]) != (y[I_SPERC] - y[I_SRUN]) )
  {
    y[I_SRUN] = y[I_SPERC] - y[I_SGRW] + prevy[I_SGRW];
  }

 if( y[I_SGRW] < ZERO ) { y[I_SGRW] = ZERO; }

  // DWK added following statement on 20020401 to keep SRUN positive
  // when agricultural croplands are abandoned


  if( y[I_SRUN] < ZERO ) { y[I_SRUN] = ZERO; }



/************************* Carbon Cycle Balances **************************/

//  if( y[I_INNPP] < y[I_NPP] ) { y[I_INNPP] = y[I_NPP]; }

//  if( y[I_INGPP] < y[I_GPP] ) { y[I_INGPP] = y[I_GPP]; }

/*  if( y[I_GPR] != y[I_GPP] - y[I_NPP] )
  {
    y[I_GPR] = y[I_GPP] - y[I_NPP];
//    y[I_NPP] = y[I_GPP] - y[I_GPR];
  } */

/*  if( y[I_GPR] != y[I_RVMNT] + y[I_RVGRW] )
  {
    y[I_RVGRW] = y[I_GPR] - y[I_RVMNT];
//    y[I_NPP] = y[I_GPP] - y[I_RVGRW] - y[I_RVMNT];
  } */


  if( y[I_LEAFC] - prevy[I_LEAFC]
      != y[I_ALLOCLC] - y[I_RMLEAF] - y[I_LTRLC] )
  {
    y[I_LTRLC] = y[I_ALLOCLC] - y[I_RMLEAF] - y[I_LEAFC] + prevy[I_LEAFC];
    if( y[I_LTRLC] < ZERO ) { y[I_LTRLC] = ZERO; }
  }

  if( y[I_LEAFC] < ZERO ) { y[I_LEAFC] = ZERO; }

  if( y[I_SAPWOODC] - prevy[I_SAPWOODC]
    != y[I_ALLOCSC] - y[I_ALLOCHC] - y[I_RMSAPWOOD] - y[I_LTRSC] )
  {
    y[I_LTRSC] = y[I_ALLOCSC] - y[I_ALLOCHC] - y[I_RMSAPWOOD] - y[I_SAPWOODC] + prevy[I_SAPWOODC];
    if( y[I_LTRSC] < ZERO ) { y[I_LTRSC] = ZERO; }
  }

   if( y[I_SAPWOODC] < ZERO ) { y[I_SAPWOODC] = ZERO; }

  if( y[I_HEARTWOODC] - prevy[I_HEARTWOODC]
  != y[I_ALLOCHC] -  y[I_LTRHC] )
  {
    y[I_LTRHC] = y[I_ALLOCHC] - y[I_HEARTWOODC] + prevy[I_HEARTWOODC];
    if( y[I_LTRHC] < ZERO ) { y[I_LTRHC] = ZERO; }
  }

  if( y[I_HEARTWOODC] < ZERO ) { y[I_HEARTWOODC] = ZERO; }

  if( y[I_ROOTC] - prevy[I_ROOTC]
    != y[I_ALLOCRC] - y[I_RMROOT] - y[I_LTRRC] )
  {
    y[I_LTRRC] = y[I_ALLOCRC] - y[I_RMROOT] - y[I_ROOTC] + prevy[I_ROOTC];
    if( y[I_LTRRC] < ZERO ) { y[I_LTRRC] = ZERO; }
  }

   if( y[I_ROOTC] < ZERO ) { y[I_ROOTC] = ZERO; }

  if( y[I_SEEDC] - prevy[I_SEEDC]
    != y[I_ALLOCSEEDC] - y[I_RMSEED] - y[I_LTRSEEDC] )
  {
    y[I_LTRSEEDC] = y[I_ALLOCSEEDC] - y[I_RMSEED] - y[I_SEEDC] + prevy[I_SEEDC];
    if( y[I_LTRSEEDC] < ZERO ) { y[I_LTRSEEDC] = ZERO; }
  }

  if( y[I_LABILEC] - prevy[I_LABILEC]
    != y[I_GPP] - y[I_ALLOCLC] - y[I_ALLOCSC] - y[I_ALLOCRC] -
   - y[I_ALLOCSEEDC] - y[I_RVGRW] )
  {
  y[I_RVGRW] = y[I_GPP] - y[I_ALLOCLC] - y[I_ALLOCSC] - y[I_ALLOCRC]
       - y[I_ALLOCSEEDC] - y[I_LABILEC] + prevy[I_LABILEC];
  if( y[I_RVGRW] < ZERO ) { y[I_RVGRW] = ZERO; }
  }

  if( y[I_LABILEC] < ZERO ) { y[I_LABILEC] = ZERO; }

  if( y[I_SOLC] - prevy[I_SOLC] != y[I_LTRLC] +  y[I_LTRSC] +  y[I_LTRHC] + y[I_LTRRC] + y[I_LTRSEEDC]
       + ag.getSLASHC() - ag.getSCONVRTFLXC() - y[I_RH] - y[I_DOCPROD])
  {
    y[I_RH] = y[I_LTRLC]
              + y[I_LTRSC]
              + y[I_LTRHC]
              + y[I_LTRRC]
              + y[I_LTRSEEDC]
              + ag.getSLASHC()
              - ag.getSCONVRTFLXC()
              - y[I_DOCPROD]
              - y[I_SOLC]
              + prevy[I_SOLC];
   if( y[I_RH] < ZERO ) { y[I_RH] = ZERO; }
  } 

 if( y[I_SOLC] < ZERO ) { y[I_SOLC] = ZERO; }

  if( y[I_DOC] - prevy[I_DOC] != y[I_DOCPROD] - y[I_LCHDOC])
  {
    y[I_LCHDOC] = y[I_DOCPROD] - y[I_DOC] + prevy[I_DOC];
    if( y[I_LCHDOC] < ZERO ) { y[I_LCHDOC] = ZERO; }
  }


  /*********************Nitrogen Cycle Balances**********************/

  if( y[I_VNUP] < ZERO ) { y[I_VNUP] = ZERO; }

  if( y[I_INNUP] < y[I_VNUP] ) { y[I_INNUP] = y[I_VNUP]; }


  // DWK modified the following conditions for checking the mass
  //   balance on STON on 0020401


  if( y[I_LABILEN] - prevy[I_LABILEN]
       != y[I_VNUP] + y[I_NRESORBL] +  y[I_NRESORBS] + y[I_NRESORBR] + y[I_NRESORBSEED]
       +y[I_NFIXS] - y[I_ALLOCLN] - y[I_ALLOCSN] - y[I_ALLOCRN] - y[I_ALLOCSEEDN] )
  {
    y[I_NRESORBL] = y[I_ALLOCLN]
                  + y[I_ALLOCSN]
                  + y[I_ALLOCRN]
                  + y[I_ALLOCSEEDN]
                  + y[I_NFIXS]
                  - y[I_VNUP]
                  + y[I_LABILEN]
                  - prevy[I_LABILEN]
                  - y[I_NRESORBS]
                  - y[I_NRESORBR]
                  - y[I_NRESORBSEED];
  }

//  if(veg.cmnt == 1) {y[I_NRESORBL] = 0.0;}

   if( y[I_LABILEN] < ZERO ) { y[I_LABILEN] = ZERO; }

  if(y[I_NRESORBL] < ZERO) { y[I_NRESORBL] = ZERO; }

  if( y[I_LEAFN] - prevy[I_LEAFN]
      != y[I_ALLOCLN] - y[I_NRESORBL] - y[I_LTRLN] )
  {
    y[I_LTRLN] = y[I_ALLOCLN] - y[I_NRESORBL] - y[I_LEAFN] + prevy[I_LEAFN];
     if( y[I_LTRLN] < ZERO ) { y[I_LTRLN] = ZERO; }
  }

// if(veg.cmnt == 1) { y[I_LTRLN] = 0.0;}

 if( y[I_LEAFN] < ZERO ) { y[I_LEAFN] = ZERO; }

  if( y[I_SAPWOODN] - prevy[I_SAPWOODN]
      != y[I_ALLOCSN] - y[I_ALLOCHN] - y[I_NRESORBS] - y[I_LTRSN] )
  {
    y[I_LTRSN] = y[I_ALLOCSN] -y[I_ALLOCHN] - y[I_NRESORBS] - y[I_SAPWOODN] + prevy[I_SAPWOODN];
    if( y[I_LTRSN] < ZERO ) { y[I_LTRSN] = ZERO; }
  }


   if( y[I_SAPWOODN] < ZERO ) { y[I_SAPWOODN] = ZERO; }

  if( y[I_HEARTWOODN] - prevy[I_HEARTWOODN]
      != y[I_ALLOCHN] - y[I_LTRHN] )
  {
    y[I_LTRHN] = y[I_ALLOCHN] - y[I_HEARTWOODN] + prevy[I_HEARTWOODN];
    if( y[I_LTRHN] < ZERO ) { y[I_LTRHN] = ZERO; }
  }

   if( y[I_HEARTWOODN] < ZERO ) { y[I_HEARTWOODN] = ZERO; }

  if( y[I_ROOTN] - prevy[I_ROOTN]
//      != y[I_ALLOCRN] + y[I_NFIXS] - y[I_NRESORBR] - y[I_LTRRN] )
      != y[I_ALLOCRN] - y[I_NRESORBR] - y[I_LTRRN] )
  {
//    y[I_LTRRN] = y[I_ALLOCRN] + y[I_NFIXS] - y[I_NRESORBR] - y[I_ROOTN] + prevy[I_ROOTN];
    y[I_LTRRN] = y[I_ALLOCRN] - y[I_NRESORBR] - y[I_ROOTN] + prevy[I_ROOTN];
    if( y[I_LTRRN] < ZERO ) { y[I_LTRRN] = ZERO; }
  }

 if( y[I_ROOTN] < ZERO ) { y[I_ROOTN] = ZERO; }

  if( y[I_SEEDN] - prevy[I_SEEDN]
    != y[I_ALLOCSEEDN] - y[I_NRESORBSEED] - y[I_LTRSEEDN] )
  {
    y[I_LTRSEEDN] = y[I_ALLOCSEEDN] - y[I_NRESORBSEED] - y[I_SEEDN] + prevy[I_SEEDN];
    if( y[I_LTRSEEDN] < ZERO ) { y[I_LTRSEEDN] = ZERO; }
  }

 if( y[I_SEEDN] < ZERO ) { y[I_SEEDN] = ZERO; }

#ifdef OPENN
  if( y[I_SOLN] - prevy[I_SOLN] != y[I_LTRLN] + y[I_LTRSN] + y[I_LTRHN] + y[I_LTRRN] + y[I_LTRSEEDN]
      + ag.getSLASHN() + y[I_NFIXN]
      - y[I_DONPROD] - ag.getSCONVRTFLXN() - ag.getNSRETENT() - y[I_NMIN] )
  {
    y[I_NMIN] = y[I_LTRLN]
                + y[I_LTRSN]
                + y[I_LTRHN]
                + y[I_LTRRN]
                + y[I_LTRSEEDN]
                + ag.getSLASHN()
                + y[I_NFIXN]
                - y[I_DONPROD]
                - ag.getSCONVRTFLXN()
                - ag.getNSRETENT()
                - y[I_SOLN]
                + prevy[I_SOLN];
  }

//  NOTINSHREE
  if( y[I_SOLN] < ZERO ) { y[I_SOLN] = ZERO; }

  if( y[I_DON] - prevy[I_DON] != y[I_DONPROD] - y[I_LCHDON] )
  {
   y[I_LCHDON] = y[I_DONPROD] - y[I_DON] + prevy[I_DON];
  if( y[I_LCHDON] < ZERO ) { y[I_LCHDON] = ZERO; }
  }
#else
  if( y[I_SOLN] - prevy[I_SOLN] != y[I_LTRLN] + y[I_LTRSN] + y[I_LTRHN] + y[I_LTRRN] + y[I_LTRSEEDN]
      + ag.getSLASHN()
      - ag.getSCONVRTFLXN() - ag.getNSRETENT() - y[I_NMIN] )
  {
    y[I_NMIN] = y[I_LTRLN]
                + y[I_LTRSN]
                + y[I_LTRHN]
                + y[I_LTRRN]
                + y[I_LTRSEEDN]
                + ag.getSLASHN()
                - ag.getSCONVRTFLXN()
                - ag.getNSRETENT()
                - y[I_SOLN]
                + prevy[I_SOLN];
  }

    if( y[I_SOLN] < ZERO ) { y[I_SOLN] = ZERO; }
#endif 

  if( y[I_AGFRTN] < ZERO ) { y[I_AGFRTN] = ZERO; }

  if ( y[I_NINP] != y[I_AGFRTN] + ag.getNRETENT() )
  {
    y[I_NINP] = y[I_AGFRTN] + ag.getNRETENT();
  }

  if ( y[I_NINP] < ZERO ) { y[I_NINP] = ZERO; }

  if ( y[I_NLST] < ZERO ) { y[I_NLST] = ZERO; }


  if( y[I_AVLN] - prevy[I_AVLN]
//      > y[I_NINP] + y[I_NMIN] - y[I_VNUP] - y[I_NLST] )
      > y[I_NINP] + y[I_NMIN] + ag.getVOLAN() - y[I_VNUP] - y[I_NLST] )
  {
//    y[I_NLST] = ZERO;
    y[I_NINP] =  y[I_NLST]
                   - y[I_NMIN]
                   - ag.getVOLAN()
                   + y[I_VNUP]
                   + y[I_AVLN]
                   - prevy[I_AVLN];
    if( y[I_NINP] < ZERO ) { y[I_NINP] = ZERO; }

  }
  else if( y[I_AVLN] - prevy[I_AVLN]
//      < y[I_NINP] + y[I_NMIN] - y[I_VNUP] - y[I_NLST] )
      < y[I_NINP] + y[I_NMIN] + ag.getVOLAN() - y[I_VNUP] - y[I_NLST] )
  {
//    y[I_NINP] = ZERO;
    y[I_NLST] =  y[I_NINP]
                 + y[I_NMIN]
                 + ag.getVOLAN()
                 - y[I_VNUP]
                 - y[I_AVLN]
                 + prevy[I_AVLN];
   if( y[I_NLST] < ZERO ) { y[I_NLST] = ZERO; }
  }

//  BSF make sure AVLN not negative
  if( y[I_AVLN] < ZERO ) { y[I_AVLN] = ZERO; }

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

int Ttem45::monthlyTransient( const int& pdyr,
                              const int& pdm,
                              const double& ptol,
                              const int& ichrt,
                              std::ostream& rflog1 )
{
  endeq = 0;
  initFlag = 1;

  // Reset texture-dependent parameters for element (e.g. grid
  //   cell) each month (Note: required if simulating a vector
  //   of elements month-to-month)

//  Kick's Fix 09272007
//  if ( ag.prvstate == 0 )

  if ( ag.state == 0
        || (ag.state > 0
        && disturbflag > 0
        && getTimestep45().getMONTH( pdm ) < (disturbmonth - 1)) )
  {
    y[I_VSM] = soil.updateRootZ( veg.cmnt,
                                 y[I_SM],
                                 y[I_ROOTC] );

    veg.resetEcds( veg.cmnt, soil.getPSIPLUSC() );
  }
  else
  {
    y[I_VSM] = soil.updateRootZ( ag.cmnt,
                                 y[I_SM],
                                 y[I_ROOTC] );


    veg.resetEcds( ag.cmnt, soil.getPSIPLUSC() );
  }

    microbe.resetEcds( veg.cmnt, soil.getPSIPLUSC() );




  // Update monthly carbon, nitrogen and water dynamics of
  //   terrestrial ecosystem

//  if(getTimestep45().isYEARSTART( pdm )) {cout << "cohort = " << ichrt << endl;}
  stepmonth( pdyr, pdm, intflag, ptol, ichrt, rflog1, totyr >= startyr );


  if( totyr == startyr ) { wrtyr = 0; }

  if( (CYCLE-1) == pdm )
  {
    if( totyr > startyr ) { ++wrtyr; }
  }

  return wrtyr;

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void Ttem45::natvegDynamics( const int& pdm, const int& pdyr, const double& nmax_grow, double pstate[] )
{
  int agstate = 0;
  int perennial = 1;
  double prob;
  double surf_wetness;
  double rsoil;
  int nopen;
  double denitrs;
  double labilec_new,labilen_new,soilc_new,soiln_new,cnveg;

  double dtmax;

  dtmax = 0.03333;

  ag.fertflag = 0;
  ag.tillflag = 0;
  ag.irrgflag = 0;

//  initFlag = 0;
  
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in natvegDynamics(), month = %2d ", pdm);
    refresh();
  #endif

  #ifdef DEBUG_XTEM
    printf(" entering natvegDynamics(), month = %2d \n", pdm);
  #endif

//----------------------------------------------//
//  transfer information from pstate to veg biomass variables
  updateVegBiomass( pstate );

//----------------------------------------------//
//  update variables used in natvegdynamics:
//    ag.fertn
//    atms.lwout, atms.nirrn
//    soil.avlh2o, soil.gm, soil.kh2o, soil.ninput
//    veg.lai, veg.rmt
//
  ag.fertn = ZERO;

  atms.setLWOUTD( atms.lwrad( atms.getTAIRD(),
                              atms.getVPR(),
                              atms.getNIRR(),
                              atms.getGIRR() ) );
                              
  atms.setLWOUTN( atms.lwrad( atms.getTAIRN(),
                              atms.getVPR(),
                              atms.getNIRR(),
                              atms.getGIRR() ) );

  // nirrn already set

  soil.setAVLH2O( pstate[I_SM] - soil.getWILTPT() );
  if( soil.getAVLH2O() < ZERO )
  {
    soil.setAVLH2O( ZERO );
  }
  soil.setVSM( pstate[I_VSM] );
  soil.setSWP();
  soil.setMOIST( pstate[I_SM] );
  soil.setKH2O( pstate[I_VSM], moistlim );
//  soil.setNINPUT( 0.0);
  soil.setNLOST( 0.0);
  soil.setNINPUT( ag.getNRETENT() );
//  cout << "avalh2o = " << soil.getAVLH2O() << " " << soil.getVSM() << " " << soil.getSWP() << " " << soil.getMOIST() << " " << soil.getKH2O() << endl;
//  cout << "water terms = " << soil.getSNOWINF() << " " << atms.getRAIN() <<  " " << ag.irrigate << " " << soil.getEET() << " " << soil.getRPERC() << " " << soil.getSPERC() << endl;

/*  if( pstate[I_LEAFC] > ZERO )
  {
    veg.setLAI( (pstate[I_LEAFC]*veg.getSLA(veg.cmnt)) );
  }
  else
  {
    veg.setLAI( ZERO );
  } */ 
//  cout << "lai = " << veg.getLAI() << " " << veg.getSLA(veg.cmnt) << " " << veg.cmnt << endl;
  
  veg.setLAI( pstate[I_LAI] );

  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in natvegDynamics() before aerodynamics(), month = %2d ", pdm);
    refresh();
  #endif
  
  #ifdef DEBUG_XTEM
    printf(" in natvegDynamics(), before aerodynamics() \n");
  #endif
  
  veg.aerodynamics( veg.cmnt,
	                 atms.getWS10() );

  if( 1 == rsoilmethod )
  {
    prob = 1.0 - exp(-0.005*atms.getPREC() );
    if( prob < ZERO ) { prob = ZERO; }
    
    surf_wetness = soil.getAVLH2O()/soil.getAWCAPMM();
    if( surf_wetness < ZERO ) { surf_wetness = ZERO; }
    
    rsoil = 54.65/(prob*surf_wetness + 0.01);
    
    veg.pen.setR_SS( rsoil );
  }
  
  if( 2 == rsoilmethod )
  {
    //surf_wetness = 100.0*soil.getVSM()/soil.getPCTPOR(); // assume surface wetness is like bulk moisture content
    prob = 1.0 - exp(-0.005*atms.getPREC() );
    if( prob < ZERO ) { prob = ZERO; }
     
//    surf_wetness = soil.getAVLH2O()/soil.getAWCAPMM();
    surf_wetness = 100.0*soil.getVSM()/soil.getPCTPOR();
//    if(atms.getPREC() > 10.0) {
//     surf_wetness = soil.getAVLH2O()/soil.getAWCAPMM();
//    }
    if( surf_wetness > 1.0 ) { surf_wetness = 1.0; }
    if( surf_wetness < ZERO ) { surf_wetness = ZERO; }

    //if(soil.getSNOWPACK() > ZERO) { surf_wetness = 1.0; }
  
    //veg.pen.setR_SS( exp(8.206 - 4.205*surf_wetness*sqrt(prob)) ); // using modified formulation from SiB2
//    if(atms.getPREC() > 10.0) {
//    veg.pen.setR_SS(54.65/(prob*surf_wetness + 0.01));
//    }
//    else
//    {
    veg.pen.setR_SS(exp(8.206 - 4.205*sqrt(surf_wetness*prob)) ); // using formulation from SiB2
//    }
//    veg.pen.setR_SS((11.489*exp(-11.67*soil.getVSM()))* exp(8.206 - 4.205*sqrt(surf_wetness*prob)) ); // using formulation from SiB2
//    veg.pen.setR_SS((78.935*exp(-20.7*soil.getVSM()))* exp(8.206 - 4.205*sqrt(surf_wetness*prob)) ); // using formulation from SiB2
//    veg.pen.setR_SS((-4.64*log(soil.getVSM())-6.5404)* exp(8.206 - 4.205*sqrt(surf_wetness*prob)) ); // using formulation from SiB2
  }
	                 
  veg.pen.hydraulics( veg.cmnt,
                      veg.getPHEN( veg.cmnt ),
                      veg.getLAI(),
                      veg.getSLA( veg.cmnt ),
                      veg.getSAPWOODC(),
                      veg.getROOTC(),
                      soil.getAVLH2O(),
                      soil.getAWCAPMM(),
                      atms.getPREC() );

  veg.rxTLaRS( veg.cmnt,
                  atms.getTAIR(),
                  veg.getRATREF( veg.cmnt ) );
  if( 0 == o3flag ) { veg.setFOZONE( 1.0 ); }
  else { veg.setFOZONE( pstate[I_FOZONE] ); }

//----------------------------------------------//
//  determine litterfall and respiration fluxes  
  veg.litterresp( veg.cmnt,
                  atms.getNDAYS(pdm) );
                  
//----------------------------------------------//
//  determine decomposition and mineralization fluxes

nopen = 0;
#ifdef OPENN
  nopen = 1;
#endif

  microbe.updateDynamics( veg.cmnt,
                          soil.getPCTFLDCAP(),
                          soil.getPCTWILTPT(),
                          soil.getPCTPOR(),
                          pstate[I_SOLC],
                          pstate[I_SOLN],
                          pstate[I_SM],
                          pstate[I_VSM],
                          pstate[I_AVLN],
                          moistlim,
                          ag.tillflag,
                          ag.getTILLFACTOR( veg.cmnt ),
                          soil.getKH2O(),
                          veg.getRLTRC(),
                          nopen,
                          ag.getIMMBADD(),
                          ag.getVOLAC(),
                          ag.getVOLAN());

//   microbe.setRH( microbe.getRH() + ag.getCONVRTFLXC());
                 

//----------------------------------------------//
//  determine ingpp and innup; calculations for gpp are used in allocate
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in natvegDynamics() before gppxclm(), month = %2d ", pdm);
    move(DEBUG_ROW+1,1);
    printw(" aerodynamic resistances (raa,rac) = %8.21f  %8.21f, lai = %8.21f ", veg.pen.getR_AA(), veg.pen.getR_AC(), veg.getLAI() );
    refresh();
  #endif
  
  #ifdef DEBUG_XTEM
    printf(" in natvegDynamics(), before gppxclm() \n");
  #endif
  
  veg.nupxclm( veg.cmnt,
               nmax_grow,
               pstate[I_SM],
               pstate[I_AVLN],
               veg.getRMT(),
               soil.getKH2O(),
               veg.getFOZONE(),
               pstate[I_ROOTC] );
               

  veg.gppxclm( veg.cmnt,
               atms.getNDAYS(pdm),
	       pdyr,
               atms.getCO2(),
               atms.getPAR(),
               atms.getVPR(),
               atms.getVPDD(),
               atms.getDAYL(),
               veg.pen.getKEXT(veg.cmnt),
               microbe.getRH(),
               atms.getPREC(),
               veg.getVEGC(),
	       initFlag);

//  if(atms.getPREC() > 10 && veg.pen.getFWS() < .9) {cout << "fws = " << veg.pen.getFWS() << " " << atms.getPREC() << endl;}

//----------------------------------------------//
//  determine allocation fluxes
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in natvegDynamics() before allocate(), month = %2d ", pdm);
    refresh();
  #endif
  
  #ifdef DEBUG_XTEM
    printf(" in natvegDynamics(), before allocate() \n");
  #endif
  
  veg.allocate( veg.cmnt,
                atms.getNDAYS(pdm),
                nfeed,
                pdm,
                ag.getGROWDD(),
                ag.state,
	        pdyr );

//  cout << "diag = " << veg.getALLOCLC() << " " << veg.getALLOCSC() << " " << veg.getALLOCRC() << " " << veg.getALLOCSEEDC() << " " << veg.getRGRWTH() << endl;


//----------------------------------------------//
//  update vegetation dynamics

#ifdef OPENN
//  soil.setNINPUT((0.102 * (12.0*soil.getEET()/10.0)+ 0.524 )/(10.0*12.0));
  soil.setNINPUT(soil.getNINPUT() + atms.getNDEP()/(1000.0*CYCLE));
  soil.setSONINP((veg.getNNF(veg.cmnt)*0.102 * ((double)CYCLE*soil.getEET()/10.0)+ 0.524 ) /(10.0*(double)CYCLE));
  veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.102 * ((double)CYCLE*soil.getEET()/10.0)+ 0.524) /(10.0*(double)CYCLE));

  labilec_new = y[I_LABILEC]+ dtmax * (veg.getGPP() - veg.getALLOCLC() - veg.getALLOCSC() - veg.getALLOCRC() - veg.getALLOCSEEDC() - veg.getRGRWTH());
  labilen_new = y[I_LABILEN] + dtmax * (veg.getNUPTAKE() + veg.getVEGNINP() - veg.getALLOCLN() - veg.getALLOCSN() - veg.getALLOCRN() - veg.getALLOCSEEDN() + veg.getNRESORBL() + veg.getNRESORBS() + veg.getNRESORBR() + veg.getNRESORBSEED());
  soilc_new = y[I_SOLC] + dtmax * (veg.getLTRLC()+veg.getLTRSC()+veg.getLTRHC()+veg.getLTRRC()+veg.getLTRSEEDC()-microbe.getDOCPROD()-microbe.getRH());
  soiln_new = y[I_SOLN] + dtmax * (veg.getLTRLN()+veg.getLTRSN()+veg.getLTRHN()+veg.getLTRRN()+veg.getLTRSEEDN()+soil.getSONINP()-microbe.getDONPROD()-microbe.getNETNMIN());
  cnveg = (veg.getCNLEAF(veg.cmnt)+veg.getCNSAPWOOD(veg.cmnt)+veg.getCNROOT(veg.cmnt))/3.0;

//if(labilec_new/labilen_new < cnveg)
if((labilen_new/labilec_new)*100.0 > 10.0)
{
//  if(((labilec_new/veg.getCNLEAF(veg.cmnt))- labilen_new + dtmax * veg.getVEGNINP())/dtmax >= 0.0) {cout << "positive" << endl;}
//  veg.setVEGNINP(((labilec_new/veg.getCNLEAF(veg.cmnt))- labilen_new + dtmax * veg.getVEGNINP())/dtmax);
//  if(veg.getVEGNINP() < 0.0) { veg.setVEGNINP(0.0); }
  veg.setVEGNINP(0.0);
}

//if(soilc_new/soiln_new >= microbe.getCNSOIL(veg.cmnt)) { cout << "no change in nfixn" << endl; }
if(soilc_new/soiln_new < microbe.getCNSOIL(veg.cmnt)-10.0)
{
//  soil.setSONINP(((soilc_new/microbe.getCNSOIL(veg.cmnt)) - soiln_new + dtmax * soil.getSONINP())/dtmax);
//  if(soil.getSONINP() < 0.0) { soil.setSONINP(0.0); }
  soil.setSONINP(0.0);
}
//if(y[I_LABILEC]/y[I_LABILEN] < veg.getCNLEAF(veg.cmnt)) {
//   veg.setVEGNINP(0.0);





// intermediate Nfix
//  soil.setSONINP((veg.getNNF(veg.cmnt)*0.234 * ((double)CYCLE*soil.getEET()/10.0)+ 0.172 ) /(10.0*(double)CYCLE));
//  veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.234 * ((double)CYCLE*soil.getEET()/10.0)+ 0.172) /(10.0*(double)CYCLE));

//  if(veg.cmnt == 1 || veg.cmnt == 15) { veg.setVEGNINP(0.0); }
  if(veg.cmnt == 1 || ag.state == 7) { veg.setVEGNINP(0.0); }
//  if(initFlag == 1) { cout << "natveg = " << soil.getSONINP() << " " << veg.getVEGNINP() << " " << soil.getEET() << endl; } 
#endif
//
//if(initFlag == 1) {cout << "ori = " << soil.getSONINP() << endl;}
  if ( disturbflag > 1 && getTimestep45().isMONTHSTART( pdm, disturbmonth - 1 ) )
  {
    // set monthly vegetation fluxes to zero
    veg.resetMonthlyFluxes();
  }
  else
  {

    veg.updateDynamics( veg.cmnt,
                        soil.getNINPUT(),
                        pstate[I_AVLN],
                        nfeed,
                        agstate,
                        perennial,
                        microbe.getNETNMIN(),
                        ag.fertn,
                        soil.getSONINP(),
		        initFlag,
		        pdyr );
//    soil.setSONINP(veg.getSONINPUT());

//if(initFlag == 1) {cout << "post = " << soil.getSONINP() << endl;}
//if(initFlag == 1) {cout << "diag = " << soil.getSONINP() << " " << veg.getVEGNINP() << " " << veg.getDENITR() << endl;}
//    soil.setNLOST(veg.getDENITR());

 //  cout << "nfix = " << soil.getSONINP() << " " << veg.getVEGNINP() << endl;
  }

//   soil.setSONINP(0.0*soil.getSONINP());
//   veg.setVEGNINP(0.0*veg.getVEGNINP());
  
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" in natvegdynamics, before petsw(), month = %2d ", pdm);
    refresh();
  #endif
  
  #ifdef DEBUG_XTEM
    printf(" in natvegDynamics(), before petsw() \n");
    for( int i = 0; i < NUMEQ; ++i ) { printf(" pstate[%d] = %4.1lf \n", i, pstate[i]); }
  #endif
  
  veg.setESOILMMMO(0.0);
  veg.petsw( veg.cmnt,
                 pdm,
                 atms.getNDAYS(pdm),
                 atms.getDAYL(),
                 atms.getTAIRD(),
                 atms.getTAIRN(),
                 atms.getCO2(),
                 atms.getNIRRN(),
                 atms.getLWOUTD(),
                 atms.getLWOUTN(),
                 atms.getVPR(),
                 atms.getVPDD(),
                 atms.getVPDN(),
                 soil.getSNOWPACK(),
                 atms.getPREC(),
                 veg.getESOILMMMO(),
                 soil.getAVLH2O(),
                 elev );
                

 if((soil.getAVLH2O() < .5 * soil.getAWCAPMM()) && (veg.getPESOILW() > 0.5 * atms.getPREC()))
{
//  cout << "met condition " << soil.getAVLH2O() << " " << soil.getAWCAPMM() << " " << veg.getPESOILW() << " " << atms.getPREC() << endl;
  veg.setESOILMMMO(0.5 * atms.getPREC());
 if(atms.getPREC() == 0.0) { veg.setESOILMMMO(0.01);}

//if(veg.getPESOILW() >  max(.5*atms.getPREC(),.1*soil.getAVLH2O()))
//{
//  veg.setESOILMMMO( max(.5*atms.getPREC(),.1*soil.getAVLH2O()) );
  veg.petsw( veg.cmnt,
                 pdm,
                 atms.getNDAYS(pdm),
                 atms.getDAYL(),
                 atms.getTAIRD(),
                 atms.getTAIRN(),
                 atms.getCO2(),
                 atms.getNIRRN(),
                 atms.getLWOUTD(),
                 atms.getLWOUTN(),
                 atms.getVPR(),
                 atms.getVPDD(),
                 atms.getVPDN(),
                 soil.getSNOWPACK(),
                 atms.getPREC(),
                 veg.getESOILMMMO(),
                 soil.getAVLH2O(),
                 elev );
}  

//cout << "petsw = " << veg.cmnt << " " << atms.getNDAYS(pdm) << " " << atms.getDAYL() << " " << atms.getTAIRD() << " " << atms.getTAIRN() << " " << atms.getCO2() << " " << atms.getNIRRN() << " " << atms.getLWOUTD() << " " << atms.getLWOUTN() << " " << atms.getVPR() << " " << atms.getVPDD() << " " << atms.getVPDN() << " " << soil.getSNOWPACK() << " " << atms.getPREC() << " " << veg.getESOILMMMO() << " " << elev << endl;
/* while(veg.getPESOILW() >  max(.9*atms.getPREC(),.5*soil.getAVLH2O()))
  {
    count = count+1;
    pesoilcor = 2.0; 
    veg.pen.setR_SS(pesoilcor*veg.pen.getR_SS());
    veg.petsw( ag.cmnt,
             pdm,
             atms.getNDAYS(pdm),
             atms.getDAYL(),
             atms.getTAIRD(),
             atms.getTAIRN(),
             atms.getCO2(),
             atms.getNIRRN(),
             atms.getLWOUTD(),
             atms.getLWOUTN(),
             atms.getVPR(),
             atms.getVPDD(),
             atms.getVPDN(),
             soil.getSNOWPACK(),
             atms.getPREC(),
             soil.getAVLH2O() );
  } */    
//  cout << "pesoil = " << veg.getPESOILW() << endl;

  veg.deltafo3( veg.cmnt,
		pdyr,
                atms.getAOT40() );

// set ET zero for impervious surface
  if(ag.state == 7)
  {
    veg.setPET( 0.0);
    soil.setSONINP(0.0);
  }


  soil.updateHydrology( elev,
                     atms.getTAIR(),
                     atms.getPREVTAIR(),
                     atms.getPREV2TAIR(),
                     atms.getRAIN(),
                     veg.getPET(),
                     soil.getAVLH2O(),
                     pstate[I_RGRW],
                     pstate[I_SGRW],
                     ag.irrgflag,
                     ag.irrigate,
                     pdm );

// if(initFlag == 1) {cout << "eet in natveg = " << soil.getEET() << endl;}

  soil.updateDOCLEACH( pstate[I_DOC],
                 pstate[I_SM] );


/* if(veg.cmnt == 5)
{
  soil.setSONINP((veg.getNNF(veg.cmnt)*0.102 * (12.0*((soil.getEET()/soil.getREET())*90.0)/10.0)+ 0.524 )/(10.0*12.0));
  veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.102 * (12.0*((soil.getEET()/soil.getREET())*90.0)/10.0)+ 0.524 )/(10.0*12.0));
}
if(veg.cmnt == 6)
{
  soil.setSONINP((veg.getNNF(veg.cmnt)*0.102 * (12.0*((soil.getEET()/(1.0+soil.yreet))*140.0/12.0)/10.0)+ 0.524 )/(10.0*12.0));
  veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.102 * (12.0*((soil.getEET()/(1.0+soil.yreet))*140.0/12.0)/10.0)+ 0.524 )/(10.0*12.0));  

//  soil.setSONINP((veg.getNNF(veg.cmnt)*0.102 * (12.0*soil.getEET()/10.0)+ 0.524 ) /(10.0*12.0));
//  veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.102 * (12.0*soil.getEET()/10.0)+ 0.524) /(10.0*12.0));

//  soil.setSONINP((veg.getNNF(veg.cmnt)*0.102 * (12.0*(soil.getEET()*627/787)/10.0)+ 0.524 ) /(10.0*12.0));
//  veg.setVEGNINP(((1.0-veg.getNNF(veg.cmnt))*0.102 * (12.0*(soil.getEET()*627/787)/10.0)+ 0.524) /(10.0*12.0));
} */

//  soil.setSONINP(soil.getSONINP()/2.0);
//  veg.setVEGNINP(veg.getVEGNINP()/2.0);
//  soil.setNINPUT((0.102 * (12.0*pstate[I_EET]/10.0)+ 0.524 )/(10.0*12.0));
//  cout << "EET = " << soil.getEET() << " " << pstate[I_EET] << endl;
//  if(veg.cmnt == 9) {
 
/*  cout << "evapcorb = " << soil.getRRUN() << " " << soil.getSRUN() << endl;

  wevap = pen.watev( atms.getNIRRN(),
                     atms.getLWOUTD(),    
                     atms.getTAIRD(),
                     atms.getVPDD(),
                     atms.getWS10(),
                     soil.getRRUN(),
                     soil.getSRUN(),
                     pdm );

  if( soil.getRRUN()+soil.getSRUN() > 0.0) 
  {
  rfrac = soil.getRRUN()/(soil.getRRUN()+soil.getSRUN());
 

  soil.setRRUN(soil.getRRUN() - (rfrac*wevap));
  soil.setSRUN(soil.getSRUN() - ((1-rfrac)*wevap));
  if(soil.getRRUN() <= 0.0) {soil.setRRUN( 0.0 );}
  if(soil.getSRUN() <= 0.0) {soil.setSRUN( 0.0 );}
if (rfrac*wevap < 0.0) {cout << "diag is negative " << endl;}
//  cout << "evapcora = " << wevap <<  " " << " " << rfrac << " " << soil.getRRUN() << " " << soil.getSRUN() << endl;
  }
//  }  */ 

  // Determine nitrogen losses from ecosystem

  if( 1 == avlnflag )
  {

#ifdef OPENN
  if(pstate[I_DOC] > 0.0)
  {
  soil.setLCHDON( soil.getLCHDOC() * pstate[I_DON]/pstate[I_DOC]);
  }
  else
  {
  soil.setLCHDON(ZERO);
  }
//cout << "lchdon = " << soil.getLCHDON() << " " << y[I_LCHDON] << " " << pstate[I_LCHDON] << endl;

    soil.updateNLosses( veg.cmnt,
//                        (atms.getRAIN() + soil.getSNOWINF() - soil.getEET() ),
                        (soil.getRPERC() + soil.getSPERC()),
                        pstate[I_AVLN],
                        pstate[I_SM],
		        ag.irrigate,
		        ag.state );

    soil.setLCHDIN(soil.getNLOST());

    if((microbe.getNETNMIN() + soil.getNINPUT() - veg.getNUPTAKE() - soil.getNLOST()) > 0.0)
    {
      denitrs = 0.5 * (microbe.getNETNMIN() + soil.getNINPUT() - veg.getNUPTAKE() - soil.getNLOST());
    }
    else
    {
      denitrs = 0.0;
    }
//    setDENITR( 0.01*microbe.getGMIN() + denitrs);
    setDENITR( soil.getDENITR(veg.cmnt) * (0.01*microbe.getGMIN() + denitrs));

//    if(veg.getNUPTAKE() == 0.0)
    if(veg.getNUPTAKE() < 0.01)
    {
      setDENITR(microbe.getNETNMIN() + soil.getNINPUT() - veg.getNUPTAKE() - soil.getNLOST());
    }

//    setDENITR(soil.getDENITR(ag.cmnt) * (0.01*microbe.getGMIN() + veg.getDENITR()));

    soil.setNLOST( soil.getNLOST() +  getDENITR() );

//    cout << "denitr = " << 0.01*microbe.getGMIN() << " " << veg.getDENITR() << " " << soil.getLCHDIN() << " " << atms.getNDEP()/12000.0 << " " << soil.getSONINP() + veg.getVEGNINP() << endl;
//#endif
//  Ensure Nlost does not exceed the total avln pool
//
/*    if( soil.getNLOST()  > pstate[I_AVLN] - veg.getNUPTAKE()
         + microbe.getNETNMIN() + soil.getNINPUT() )

    {
      setDENITR(pstate[I_AVLN]- veg.getNUPTAKE()+ microbe.getNETNMIN()+ soil.getNINPUT() - soil.getLCHDIN());
      soil.setNLOST(getDENITR() + soil.getLCHDIN());
//      soil.setNLOST( (pstate[I_AVLN]
//                     - veg.getNUPTAKE()
//                     + microbe.getNETNMIN()
//                     + soil.getNINPUT()) );
    }
    if( soil.getNLOST() < ZERO )
    {
      soil.setNLOST( ZERO );

      microbe.setNETNMIN( (soil.getNLOST()
                          + veg.getNUPTAKE()
                          - soil.getNINPUT()
                          - pstate[I_AVLN]) );
    }*/ 

//  Ensure Nin = Nout in all cases - calibration, equilibration, and transient
/*   if(soil.getNLOST() + soil.getLCHDON() < soil.getNINPUT() + soil.getSONINP() + veg.getVEGNINP())
   {
      soil.setNLOST(soil.getNLOST() + (soil.getNINPUT() + soil.getSONINP() + veg.getVEGNINP()) - (soil.getNLOST() + soil.getLCHDON()));
   }
   if(soil.getNLOST() + soil.getLCHDON() > soil.getNINPUT() + soil.getSONINP() + veg.getVEGNINP())
   {
      soil.setNINPUT(soil.getNINPUT() + ((soil.getNLOST() + soil.getLCHDON()) - ((soil.getNINPUT() + soil.getSONINP() + veg.getVEGNINP()))));
   } */   


#endif

  } // and avlnflag = 1 loop
  else

  {
    soil.setNLOST( (soil.getNINPUT()
                   + microbe.getNETNMIN()
                       - veg.getNUPTAKE()) );
  } 

if( soil.getLCHDIN() > ZERO )
  {
    // Limit ecosystem N losses to total inorganic N available for
    //   leaching after denitrification has been substracted

    if ( soil.getLCHDIN() > (pstate[I_AVLN] + atms.getNDEP()/12000.0
         + microbe.getNETNMIN() - getDENITR()
         - veg.getNUPTAKE()) )
    {
      if ( (pstate[I_AVLN] + atms.getNDEP()/12000.0 + microbe.getNETNMIN()
           - getDENITR() - veg.getNUPTAKE()) > ZERO )
      {
        soil.setLCHDIN( (pstate[I_AVLN]
                           + atms.getNDEP()/12000.0
                           + microbe.getNETNMIN()
                           - getDENITR()
                           - veg.getNUPTAKE()) );
      }
      else
      {
        soil.setLCHDIN( ZERO );

        if( (pstate[I_AVLN] + atms.getNDEP()/12000.0 + microbe.getNETNMIN()
            - veg.getNUPTAKE()) > ZERO )
        {
          setDENITR((pstate[I_AVLN]
                               + atms.getNDEP()/12000.0
                               + microbe.getNETNMIN()
                               - veg.getNUPTAKE())) ;
        }
        else
        {
          setDENITR(ZERO);
          veg.setNUPTAKE( (pstate[I_AVLN]
                             + atms.getNDEP()/12000.0
                             + microbe.getNETNMIN()) );
        }
      }
    }
  }
  else
  {
    // If no leaching losses, limit ecosystem N losses to
    //   total inorganic N available for denitrification

    if( getDENITR() > (pstate[I_AVLN] + atms.getNDEP()/12000.0
         + microbe.getNETNMIN() - veg.getNUPTAKE()) )
    {
      if( (pstate[I_AVLN] + atms.getNDEP()/12000.0 + microbe.getNETNMIN()
          - veg.getNUPTAKE()) > ZERO )
      {
        setDENITR((pstate[I_AVLN]
                             + atms.getNDEP()/12000.0
                             + microbe.getNETNMIN()
                             - veg.getNUPTAKE())) ;
      }
      else
      {
        setDENITR(ZERO);

        veg.setNUPTAKE( (pstate[I_AVLN]
                           + atms.getNDEP()/12000.0
                           + microbe.getNETNMIN()) );
      }


    }
  }

//setDENITR(soil.getDENITR(veg.cmnt) * getDENITR());
if(getDENITR() < 0.0) {setDENITR(0.0);}
soil.setNLOST(getDENITR() + soil.getLCHDIN());
if(y[I_LABILEC]/y[I_LABILEN] < veg.getCNLEAF(veg.cmnt)) { 
//   veg.setNUPTAKE(0.0);
//   veg.setNRESORBL(0.0);
//   veg.setVEGNINP(0.0);
 }

  if( 0 == avlnflag )
//  if( 0 == initFlag )
  {
    // Do not allow changes in available nitrogen in soil
    //   (used during calibration process)

    // Equilibrate inorganic N pool

    soil.setLCHDIN( (atms.getNDEP()/12000.0
                       + microbe.getNETNMIN()
                       - getDENITR()
                       - veg.getNUPTAKE()) );

    // Equilibrate DON pool

    soil.setLCHDON( microbe.getDONPROD() );
  }


  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" at end of natvegDynamics(), month = %2d ", pdm);
    refresh();
  #endif
  
  #ifdef DEBUG_XTEM
    printf(" at end of natvegDynamics() \n");
  #endif

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

#ifdef CALIBRATE_TEM

void Ttem45::pcdisplayMonth( const int& dyr, const int& pdm )
{
  int iout, iopt;

  double outevar[ESY];
  double outwvar[WSY];
  
  if ( 0 == topwind )
  {
    for( iopt = 0; iopt < ESY; ++iopt )
    {
      move(1,8+8*iopt);
      displayOptionalEflx( sey[iopt] );
    }

    for( iopt = 0; iopt < WSY; ++iopt )
    {
      move(1,8+8*ESY+8*iopt);
      displayOptionalWflx( swy[iopt] );
    }  
    
    topwind = 1;
  }

  // Assign values of optional variables to outflux? for later
  //   screen display

  for( iout = 0; iout < ESY; ++iout ) 
  { 
    outevar[iout] = getOptionalEflx( sey[iout] );
    if(getTimestep45().isYEARSTART( pdm ))
    {
      outevarsum[iout] = 0.0;
      outevaravg[iout] = 0.0;
      outevarmax[iout] = -1000000.0;
      outevarmin[iout] = 1000000.0;
    }
    
    outevarsum[iout] += outevar[iout];
    outevaravg[iout] += outevar[iout]/(double)CYCLE;
    if( outevar[iout] > outevarmax[iout] ) { outevarmax[iout] = outevar[iout]; }
    if( outevar[iout] < outevarmin[iout] ) { outevarmin[iout] = outevar[iout]; }
  }
  
  for( iout = 0; iout < WSY; ++iout ) 
  { 
    outwvar[iout] = getOptionalWflx( swy[iout] );
    if(getTimestep45().isYEARSTART( pdm ))
    {
      outwvarsum[iout] = 0.0;
      outwvaravg[iout] = 0.0;
      outwvarmax[iout] = -1000000.0;
      outwvarmin[iout] = 1000000.0;
    }
    
    outwvarsum[iout] += outwvar[iout];
    outwvaravg[iout] += outwvar[iout]/(double)CYCLE;
    if( outwvar[iout] > outwvarmax[iout] ) { outwvarmax[iout] = outwvar[iout]; }
    if( outwvar[iout] < outwvarmin[iout] ) { outwvarmin[iout] = outwvar[iout]; }
  }
  // Display monthly values for selected C and N pools and fluxes
  
  move(1,1);

  for( iopt = 0; iopt < ESY; ++iopt )
  {
    move(1,8+8*iopt);
    displayOptionalEflx( sey[iopt] );
  }

  for( iopt = 0; iopt < WSY; ++iopt )
  {
    move(1,8+8*ESY+8*iopt);
    displayOptionalWflx( swy[iopt] );
  }
  
  move(VPARAMS_ROW-1, 1);
  printw("CALIBRATION INPUT: ");
  
  move(VPARAMS_ROW, 1);
  printw("SETTINGS:          ");
  
  move(VPARAMS_ROW+1, 1);
  printw("AGFILE:            ");
  
  move(VPARAMS_ROW+2, 1);
  printw("DATFILE:           ");
  
  move(VPARAMS_ROW+3, 1);
  printw("VEGFILE:           ");
  
  move(VPARAMS_ROW+4, 1);
  printw("CONDUCTFILE:       ");
  
  move(VPARAMS_ROW+5, 1);
  printw("OPTIONAL C/N:      ");
  
  move(VPARAMS_ROW+6, 1);
  printw("OPTIONAL H2O:      ");
  
  move(pdm+2,1);
  
  printw( "%3d-%2d %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf %7.3lf %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf",
           dyr,
               (pdm+1),
                   outevar[0],
                          outevar[1],
                                 outevar[2],
                                        outevar[3],
                                               outevar[4],
                                                      outevar[5],
                                                             outevar[6],
                                                                    outevar[7],
                                                                           outevar[8],
                                                                                  outevar[9],
                                                                                         outwvar[0],
                                                                                                outwvar[1],
                                                                                                       outwvar[2],
                                                                                                              outwvar[3],
                                                                                                                     outwvar[4],
                                                                                                                            outwvar[5],
                                                                                                                                   outwvar[6],
                                                                                                                                          outwvar[7] );
                                                                                                                                          
  if( getTimestep45().isYEAREND( pdm ) )
  {
    move(15,1);
    printw( " SUM   %7.0lf %7.0lf %7.1lf %7.1lf %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.0lf %7.0lf %7.0lf %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf",
                   outevarsum[0],
                          outevarsum[1],
                                 outevarsum[2],
                                        outevarsum[3],
                                               outevarsum[4],
                                                      outevarsum[5],
                                                             outevarsum[6],
                                                                    outevarsum[7],
                                                                           outevarsum[8],
                                                                                  outevarsum[9],
                                                                                         outwvarsum[0],
                                                                                                outwvarsum[1],
                                                                                                       outwvarsum[2],
                                                                                                              outwvarsum[3],
                                                                                                                     outwvarsum[4],
                                                                                                                            outwvarsum[5],
                                                                                                                                   outwvarsum[6],
                                                                                                                                          outwvarsum[7] );
    
    move(16,1);
    printw( " MAX   %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf %7.3lf %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf",
                   outevarmax[0],
                          outevarmax[1],
                                 outevarmax[2],
                                        outevarmax[3],
                                               outevarmax[4],
                                                      outevarmax[5],
                                                             outevarmax[6],
                                                                    outevarmax[7],
                                                                           outevarmax[8],
                                                                                  outevarmax[9],
                                                                                         outwvarmax[0],
                                                                                                outwvarmax[1],
                                                                                                       outwvarmax[2],
                                                                                                              outwvarmax[3],
                                                                                                                     outwvarmax[4],
                                                                                                                            outwvarmax[5],
                                                                                                                                   outwvarmax[6],
                                                                                                                                          outwvarmax[7] );
    
    move(17,1);                                                                                                                                      
    printw( " AVG   %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf %7.3lf %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf",
                   outevaravg[0],
                          outevaravg[1],
                                 outevaravg[2],
                                        outevaravg[3],
                                               outevaravg[4],
                                                      outevaravg[5],
                                                             outevaravg[6],
                                                                    outevaravg[7],
                                                                           outevaravg[8],
                                                                                  outevaravg[9],
                                                                                         outwvaravg[0],
                                                                                                outwvaravg[1],
                                                                                                       outwvaravg[2],
                                                                                                              outwvaravg[3],
                                                                                                                     outwvaravg[4],
                                                                                                                            outwvaravg[5],
                                                                                                                                   outwvaravg[6],
                                                                                                                                          outwvaravg[7] );
   
    move(18,1);
    printw( " MIN   %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf %7.3lf %7.1lf %7.1lf %7.1lf %7.2lf %7.2lf %7.2lf %7.3lf %7.3lf",
                   outevarmin[0],
                          outevarmin[1],
                                 outevarmin[2],
                                        outevarmin[3],
                                               outevarmin[4],
                                                      outevarmin[5],
                                                             outevarmin[6],
                                                                    outevarmin[7],
                                                                           outevarmin[8],
                                                                                  outevarmin[9],
                                                                                         outwvarmin[0],
                                                                                                outwvarmin[1],
                                                                                                       outwvarmin[2],
                                                                                                              outwvarmin[3],
                                                                                                                     outwvarmin[4],
                                                                                                                            outwvarmin[5],
                                                                                                                                   outwvarmin[6],
                                                                                                                                          outwvarmin[7] );
  }
  refresh();
  
};

#endif

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************** */

void Ttem45::resetMonthlyELMNTFluxes( void )
{

  // Reset all monthly fluxes to zero

  atms.resetMonthlyFluxes();

  veg.resetMonthlyFluxes();

  microbe.resetMonthlyFluxes();

  soil.resetMonthlyFluxes();

  ag.resetMonthlyFluxes();

  // Carbon fluxes

  nep = ZERO;
  nce = ZERO;
  nbp = ZERO;

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************** */

void Ttem45::resetODEflux( void )
{
  int i;

  for ( i = MAXSTATE; i < NUMEQ; ++i ) { y[i] = ZERO; }

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************** */

void Ttem45::resetYrFluxes( void )
{

  atms.resetYrFluxes();

  veg.resetYrFluxes();

  microbe.resetYrFluxes();

  soil.resetYrFluxes();

  ag.resetYrFluxes();

  yrtotalc = ZERO;

  // Annual carbon fluxes

  yrnep = ZERO;
  yrnce = ZERO;
  yrnbp = ZERO;


};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

void Ttem45::rkbs( const int& numeq,
                  double pstate[],
                  double& pdt,
                  const int& pdm,
                  const int& pdyr,
                  const double& nmax_grow )
{

// Runge-Kutta-Bogacki-Shampine method: compares 2nd and 3rd order solutions

  int i;
  double ptdt = ZERO;
  int calc_dz1_flag = 0;

  for( i = 0; i < numeq; ++i )
  {
    dum2[i] = dum3[i] = pstate[i];
    
    if( dz1[i] != dz4[i] ) { ++calc_dz1_flag; }
    // if previous step has been accepted, these arrays will be equal
    zstate[i] = ZERO;
    error[i] = ZERO;
    dz2[i] = dz3[i] = ZERO;
  }
  
//cout << "calcflag = " << calc_dz1_flag << " " << dz1[35] << " " << dz4[35] << endl;

  if( calc_dz1_flag != 0 ) { delta( pdm, pdyr, nmax_grow, pstate, dz1 );
//       cout << "delta1 = " << pstate[35] << " " << dz1[35] << " " << sapwoodcb[11] << " " << sapwoodcb[4] << endl;
 }

  //   since the 4th calculation from the last step should equal the first from the new step, 
  //   there is no need to recalculate
  ptdt = pdt * 0.5;   
  step( numeq, pstate, dz1, zstate, ptdt );
  
  delta( pdm, pdyr, nmax_grow, zstate, dz2 );
//  cout << "delta2 = " << zstate[35] << " " << dz2[35] << " " << sapwoodcb[11] << " " << sapwoodcb[4] << endl;

  ptdt = pdt * 0.75;
  step( numeq, pstate, dz2, zstate, ptdt );
  
  delta( pdm, pdyr, nmax_grow, zstate, dz3 );
//  cout << "delta3 = " << zstate[35] << " " << dz3[35] << " " <<  sapwoodcb[11] << " " << sapwoodcb[4] << endl;

  for( i = 0; i < numeq; ++i )
  {
    dp3[i] = 2.0/9.0 * dz1[i] + 1.0/3.0 * dz2[i] + 4.0/9.0 * dz3[i];
  }
  step( numeq, pstate, dp3, dum3, pdt );
  
  delta( pdm, pdyr, nmax_grow, dum3, dz4 );
// cout << "delta4 = " << dum3[35] << " " << dz4[35] << " " << sapwoodcb[11] << " " << sapwoodcb[4] << endl;

  for( i = 0; i < numeq; ++i )
  {
    dp2[i] = 7.0/24.0 * dz1[i] + 1.0/4.0 * dz2[i] + 1.0/3.0 * dz3[i] + 1.0/8.0 * dz4[i];
  }
  step( numeq, pstate, dp2, dum2, pdt );
  
  for ( i = 0; i < numeq; ++i )
  {
    error[i] = fabs( dum2[i] - dum3[i] );
  }
//  cout << "error = " << error[35] << " " << dum2[35] << " " << dum3[35] << " " << veg.cmnt << " " << sapwoodcb[veg.cmnt] << " " << sapwoodcb[11] << " " << sapwoodcb[4] << endl;

};

/***************************************************************
 ***************************************************************/


/* *************************************************************
************************************************************* */

void Ttem45::rkf( const int& numeq,
                  double pstate[],
                  double& pdt,
                  const int& pdm,
                  const int& pdyr,
                  const double& nmax_grow )
{
// Runge-Kutta-Fehlberg method: compares 4th and 5th order solutions

  int i;
  double ptdt = ZERO;

  for( i = 0; i < numeq; ++i )
  {
    dum4[i] = dum5[i] = pstate[i];
    yprime[i] = rk45[i] = error[i] = ZERO;
  }

  ptdt = pdt * 0.25;

  delta( pdm,pdyr,nmax_grow,dum4,f11 );
  step( numeq,yprime,f11,yprime,a1 );
  step( numeq,rk45,f11,rk45,b1 );
  step( numeq,dum4,f11,ydum,ptdt );
  delta( pdm,pdyr,nmax_grow,ydum,f2 );

  for( i = 0; i < numeq; ++i )
  {
    f13[i] = a31*f11[i] + a32*f2[i];
  }

  step( numeq,dum4,f13,ydum,pdt );
  delta( pdm,pdyr,nmax_grow,ydum,f3 );
  step( numeq,yprime,f3,yprime,a3 );
  step( numeq,rk45,f3,rk45,b3 );

  for( i = 0; i < numeq; ++i )
  {
    f14[i] = a41*f11[i] + a42*f2[i] + a43*f3[i];
  }

  step( numeq,dum4,f14,ydum,pdt );
  delta( pdm,pdyr,nmax_grow,ydum,f4 );
  step( numeq,yprime,f4,yprime,a4 );
  step( numeq,rk45,f4,rk45,b4 );

  for( i = 0; i < numeq; ++i )
  {
    f15[i] = a51*f11[i] + a52*f2[i] + a53*f3[i] + a54*f4[i];
  }

  step( numeq,dum4,f15,ydum,pdt );
  delta( pdm,pdyr,nmax_grow,ydum,f5 );
  step( numeq,yprime,f5,yprime,a5 );
  step( numeq,rk45,f5,rk45,b5 );

  for( i = 0; i < numeq; ++i )
  {
    f16[i] = b61*f11[i] + b62*f2[i] + b63*f3[i] + b64*f4[i] + b65*f5[i];
  }

  step( numeq,dum4,f16,ydum,pdt );
  delta( pdm,pdyr,nmax_grow,ydum,f6 );
  step( numeq,rk45,f6,rk45,b6 );
  step( numeq,dum4,yprime,dum4,pdt );
  step( numeq,dum5,rk45,dum5,pdt );

  for ( i = 0; i < numeq; ++i )
  {
    error[i] = fabs( dum4[i] - dum5[i] );
  }

};

/***************************************************************
 ***************************************************************/


/* *************************************************************
************************************************************** */

void Ttem45::setELMNTecd( const int& pdcmnt,
                          const double& psiplusc )
{
  // Initialize TEM parameters dependent upon a grid cell's
  //   soil texture

//  cout << "setELMNTecd " << pdcmnt << " " << psiplusc << endl;
  veg.resetEcds( pdcmnt, psiplusc );

  microbe.resetEcds( pdcmnt, psiplusc );

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

void Ttem45::setPrevState( void )
{
  for( int i = 0; i < MAXSTATE; ++i ) { prevy[i] = y[i]; }

};

/* *************************************************************
************************************************************* */


/***************************************************************
 ***************************************************************/

void Ttem45::step( const int& numeq,
                   double pstate[],
                   double pdstate[],
                   double ptstate[],
	               double& pdt )
{

  for( int i = 0; i < numeq; ++i )
  {
    ptstate[i] = pstate[i] + (pdt * pdstate[i]);
//    if(initFlag == 1) {if(i == 92) {cout << "diag = " << ptstate[i] << endl;}}
//  {if(i == 37 && initFlag == 1) {cout << "diag = " << ptstate[i] << " " << pstate[i] << " " << pdstate[i] << " " << pdt << endl;}}
  }
//  cout << "sm in step = " << pstate[19] << " " << ptstate[19] << " " << pdstate[19] << endl;
//cout << "leafc in step = " << pstate[0] << " " << pdstate[0] << endl;
//   cout << "diag = " << ptstate[0] << " " << ptstate[1] << " " << ptstate[2] << " " << ptstate[3] << " " << ptstate[4] << " " << ptstate[5] << " " << ptstate[6] << " " << ptstate[7] << " " << ptstate[8] << " " << ptstate[9] << " " << ptstate[10] << " " << ptstate[11] << " " << ptstate[12] << " " << ptstate[13] << " " << ptstate[14] << " " << ptstate[15] << " " << ptstate[16] << " " << ptstate[17] << " " << ptstate[18] << " " << ptstate[19] << " " << ptstate[20] << " " << ptstate[21] << " " << ptstate[22] << " " << ptstate[23] << " " << ptstate[24] << " " << ptstate[25] << " " << ptstate[26] << " " << ptstate[27] << " " << ptstate[28] << " " << ptstate[29] << " " << ptstate[30] << " " << ptstate[31] << " " << ptstate[32] << " " << ptstate[33] << " " << ptstate[34] << " " << ptstate[35] << endl;

};

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************** */

#if TEM_ENABLE_STORM
void Ttem45::setStormReturnIntervals( double lon, double lat, double storm, double hurricane )
{
  for( double years : { storm, hurricane } )
  {
    if( !std::isfinite( years ) || ( years != 0.0 && years < 1.0 ) )
    {
      std::cerr << "Storm return intervals must be zero or at least one year at "
                << lon << ", " << lat << std::endl;
      std::exit( -1 );
    }
  }
  stormLon = lon;
  stormLat = lat;
  stormYears = storm;
  hurricaneYears = hurricane;
}

void Ttem45::applyStormDisturbance( int cohort, std::ostream& log )
{
  if( ( stormYears == 0.0 && hurricaneYears == 0.0 ) || ag.state != 0 )
    return;

  const int community = veg.cmnt;
  if( community != 4 && community != 5 && community != 6 &&
      community != 10 && community != 11 && community != 12 )
    return;

  // Use cell, year and cohort so rank assignment and restart do not change the draws.
  std::seed_seq seed{ static_cast<std::uint32_t>( std::lround( ( stormLon + 180.0 ) * 1e6 ) ),
                      static_cast<std::uint32_t>( std::lround( ( stormLat + 90.0 ) * 1e6 ) ),
                      static_cast<std::uint32_t>( totyr ),
                      static_cast<std::uint32_t>( cohort ) };
  std::mt19937 rng( seed );
  const double stormDraw = std::generate_canonical<double, 53>( rng );
  const double hurricaneDraw = std::generate_canonical<double, 53>( rng );

  const auto damage = [&]( const char* event, double leaf, double wood )
  {
    log << event << " in " << totyr << " at " << stormLon << ", " << stormLat
        << " cohort " << cohort + 1 << '\n';
    ag.setVCONVERT( 0.0 );
    ag.setSCONVERT( 0.0 );
    ag.setPROD10PAR( 0.0 );
    ag.setPROD100PAR( 0.0 );
    stormoccur = 1;
    ag.setNATSEEDC( y[I_SEEDC] );
    ag.setNATSEEDSTON( y[I_SEEDN] );
    ag.standingdead( community,
                     ag.getSLASHC() + leaf * y[I_LEAFC] + wood * y[I_ROOTC],
                     ag.getSLASHN() + leaf * y[I_LEAFN] + wood * y[I_ROOTN],
                     wood * ( y[I_SAPWOODC] + y[I_HEARTWOODC] + y[I_LABILEC] ),
                     wood * ( y[I_SAPWOODN] + y[I_HEARTWOODN] + y[I_LABILEN] ), 0.0 );
    for( int i : { I_LEAFC, I_LEAFN } )
      prevy[i] = y[i] *= 1.0 - leaf;
    for( int i : { I_LABILEC, I_LABILEN, I_SAPWOODC, I_SAPWOODN,
                   I_HEARTWOODC, I_HEARTWOODN, I_ROOTC, I_ROOTN } )
      prevy[i] = y[i] *= 1.0 - wood;
  };

  if( stormYears > 0.0 && stormDraw < 1.0 / stormYears )
    damage( "Tropical storm", 0.07, 0.04 );
  if( hurricaneYears > 0.0 && hurricaneDraw < 1.0 / hurricaneYears )
    damage( "Hurricane", 0.20, 0.11 );
}
#endif

int Ttem45::stepmonth( const int& pdyr,
                       const int& pdm,
                       int& intflag,
                       const double& ptol,
                       const int& ichrt,
                       std::ostream& rflog1,
                       bool transient )
{

  int mintflag;
  double avgfac;
  double mdemandc, mdemandn;
  double numprec, magprec;
  double cabove, nabove, frcab;
  double dleaf, dwood;
  double sonp,availnp,vegnp,donpout,dinpout,ndif;

  double rhmoist,dq10;
  double areaburn = 0.0;

  double cwdloss=0.0;

   disturbmonth = 1;
//  cout << "entering stepmonth " << pdyr << " " << pdm << " " << veg.cmnt << " " << ag.cmnt << " " << ag.state << " " << y[I_LEAFC] << " " << y[I_LAI] << endl;
//cout << "entering stepmonth" << " " << y[I_SAPWOODC] << " " << veg.cmnt << " " << ag.cmnt << " " << sapwoodcb[11] << " " << sapwoodcb[4] << endl;

  srand(time(NULL)*rand()); //PCP code (as in GDSTEM-FIRE)

if((pdyr == 0 || pdyr == 1) and getTimestep45().isYEARSTART( pdm )) {
 rco = (rand()%2 == 1)?(double)(rand()%10+rand()%10+rand()%10)*-1/10:(double)(rand()%10+rand()%10+rand()%10)/10;
 mco = (rand()%2 == 1)?(double)(rand()%10+rand()%10+rand()%10)*-1/10:(double)(rand()%10+rand()%10+rand()%10)/10;
 lco = (rand()%2 == 1)?(double)(rand()%10+rand()%10+rand()%10)*-1/10:(double)(rand()%10+rand()%10+rand()%10)/10;
}

  ag.setSLASHC(0.0);
  ag.setSLASHN(0.0);
 
//cout << "entering stepmonth " << veg.cmnt << " " << pdyr << " " << pdm << " " << y[I_GPP] << endl;

  avgfac = 1.0/((double)CYCLE*(veg.getTAULEAF( veg.cmnt ) + veg.getTAUSAPWOOD( veg.cmnt ) 
           + veg.getTAUROOT( veg.cmnt ))/3.0);
  avgfac = exp(-avgfac);
  if(getTimestep45().isYEARSTART( pdm )) { mxeet = 0.0; }

  if((pdyr == 1 || pdyr == 0) && getTimestep45().isYEARSTART( pdm ) && initFlag == 0)
  {
  ag.setSTANDDEADC(0.0);
  ag.setSTANDDEADN(0.0);
  ag.setVOLAC(0.0);
  ag.setVOLAN(0.0);
  fireoccur = 0;
  stormoccur = 0;
 }
  
  // Wildfire (GDSTEM-FIRE): the fraction of the cohort burned in the step; no fire during the equilibration
  fire.setBURNED_AREA( 0.0 );

#if TEM_ENABLE_FIRE
  if( 1 == fire.enabled && 1 == initFlag )
  {
    if( veg.getIGEF( veg.cmnt ) <= MISSING + 1.0 || veg.getFSR( veg.cmnt ) <= MISSING + 1.0 || veg.getFD( veg.cmnt ) <= MISSING + 1.0 )
    {
      cerr << endl << "veg_fire is 1, but community " << veg.cmnt << " of the vegetation file (vegecd) has no igef, fsr and fd" << endl;
      exit( -1 );
    }

    // the year of the land cover (the first year of the transient run is pdyr 1)
    const int iy = std::max( 0, std::min( (int) ag.lwft.size() - 1, pdyr - 1 ) );

    ni = fire.ignitions( ig[pdm], pop, lon, veg.getIGEF( veg.cmnt ), ag.lwft[iy], ag.forarea[iy] );

    const double rh = atms.getVPR() / (atms.getVPR() + atms.getVPDD()) * 100.0;

    areaburn = fire.fireburn( veg.getPOTVEG(), veg.getVEGC(), y[I_VSM], atms.getWS10(), atms.getTAIR(),
                              y[I_ROOTC], y[I_RH], y[I_RMROOT], rh, ni, gdp, pop, elev,
                              veg.getFSR( veg.cmnt ), veg.getFD( veg.cmnt ), ag.lwft[iy] );
  }
#endif // TEM_ENABLE_FIRE

  // avgfac = exp(-1 / average lifetime of leaves, stem, and roots, in months)
  
 // Reset all monthly fluxes to zero
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" entering resetMonthlyELMNTFluxes() ");
    refresh();
  #endif

  #ifdef DEBUG_XTEM
    printf(" beginning stepmonth, month = %2d, year = %4d \n", pdm+1, pdyr );
    printf(" cmnt = %2d, yrnep = %8.21f, tauavg = %4.11f \n", veg.cmnt, yrnep, tauavg );
    printf(" labilec = %8.21f, yrnpp = %8.21f \n", y[I_LABILEC] , veg.yrnpp );
  #endif

  resetMonthlyELMNTFluxes();

  // Reset ODE state variables representing monthly
  //   fluxes to zero
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" entering resetODEflux() ");
    refresh();
  #endif
  resetODEflux();

  // If 12 months have passed since disturbance, reset
  //   all immediate fluxes associated with disturbance
  //   (i.e., slash, convrtflx, nretent) to zero

  if ( CYCLE == distmnthcnt )
  {
//    cout << "resetting fluxes" << " " << CYCLE << " " << distmnthcnt << " " << pdm << endl;
    distmnthcnt = 0;
    ag.resetMonthlyDisturbFluxes();
  }

//  cout << "fireoccur, stormoccur = " << fireoccur << " " << stormoccur << endl;
  if (fireoccur == 1 || stormoccur == 1)
  {
    ag.resetMonthlyDisturbFluxes();
  }
//
//   BSF remove resetting convrtflx to 0
//
//    ag.setCONVRTFLXC(0.0);
    fireoccur = 0;
    stormoccur = 0;
//  cout << "convertcflx = " << ag.getCONVRTFLXC() << endl;

  // Count the number of months since disturbance

  if( distmnthcnt > 0 ) { ++distmnthcnt; }

  if( firemnthcnt > 0 ) { ++firemnthcnt; }

//  if( 0 == ag.state && 2 == distmnthcnt)
//  {
    // Establish natural vegetation the month
    //   following disturbance if cohort is not in agriculture
    
//    prevy[I_LABILEC] = y[I_LABILEC] = 0.29*y[I_LABILEC] + ag.getNATSEEDC();
//    prevy[I_LABILEN] = y[I_LABILEN] = 0.2785*y[I_LABILEN] + ag.getNATSEEDSTON();

//    prevy[I_LABILEC] = y[I_LABILEC] =  ag.getNATSEEDC();
//    prevy[I_LABILEN] = y[I_LABILEN] =  ag.getNATSEEDSTON();

//    prevy[I_SAPWOODC] = y[I_SAPWOODC] = ZERO;
//    prevy[I_SAPWOODN] = y[I_SAPWOODN] = ZERO;
//    prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = ZERO;
//    prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = ZERO;


//  }

//  if( 2 == ag.state && 2 == distmnthcnt)
//  {
//    prevy[I_AVLN] = y[I_AVLN] = y[I_AVLN]/10.0; 
//    prevy[I_SOLN] = y[I_SOLN] = y[I_SOLN]/10.0; 
//  }

  if( 0 == pdm )
  {
    if( 0 == pdyr )
    {
      microbe.setKD( microbe.getKDC() );
      ag.setKD( microbe.getKD() );
      ag.setNATSOIL( y[I_SOLC] );
    }
    else
    {
      if( 0 == ag.state && 0 == ag.prvstate )
      {
      microbe.setKD( microbe.yrkd( nfeed,
                                     veg.yrltrc,
                                     veg.yrltrn,
                                     veg.cmnt ) );

        ag.setKD( microbe.getKD() );
        ag.setNATSOIL( y[I_SOLC] );
      }
      else
      {
        if( y[I_SOLC] < ag.getNATSOIL() )
        {
          microbe.setKD( (ag.getKD()
                         * y[I_SOLC]
                         / ag.getNATSOIL()) );
        }
        else { microbe.setKD( ag.getKD() ); }
      }
    }
  }

  // Implement disturbance effects

 if( disturbflag ==  1 && getTimestep45().isMONTHSTART( pdm, disturbmonth - 1 ) ) // agriculture
  {
    distmnthcnt = 1;
 
   ag.setVCONVERT(0.6);
   ag.setPROD10PAR(0.3);
   ag.setPROD100PAR(0.1);

//  set temperate and boreal forest values from McGuire et al. 01
// if(veg.cmnt == 4 || veg.cmnt == 5 || veg.cmnt == 6 || veg.cmnt == 7 )
/* if(veg.srccmnt == 4 || veg.srccmnt == 5 || veg.srccmnt == 6 || veg.srccmnt == 7 )
 {
   ag.setVCONVERT(0.6);
//   ag.setVCONVERT(1.0);
   ag.setPROD10PAR(0.3);
//   ag.setPROD10PAR(0.0);
   ag.setPROD100PAR(0.1);
//   ag.setPROD100PAR(0.0);
 }
// set tropical forest values
// if(veg.cmnt == 13 || veg.cmnt == 14)
 if(veg.srccmnt == 13 || veg.srccmnt == 14)
  {
   ag.setVCONVERT(0.6);
   ag.setPROD10PAR(0.4);
   ag.setPROD100PAR(0.0);
  }
// set grassland/tundra values
//  if(veg.cmnt == 2 || veg.cmnt == 8 || veg.cmnt == 9)
  if(veg.srccmnt == 2 || veg.srccmnt == 8 || veg.srccmnt == 9)
  {
   ag.setVCONVERT(1.0);
   ag.setPROD10PAR(0.0);
   ag.setPROD100PAR(0.0);
  }
// set shrubland/woodland/savanna values
//  if(veg.cmnt == 10 || veg.cmnt == 11 || veg.cmnt == 12)
  if(veg.srccmnt == 10 || veg.srccmnt == 11 || veg.srccmnt == 12)
{
   ag.setVCONVERT(0.8);
   ag.setPROD10PAR(0.2);
   ag.setPROD100PAR(0.0);
  } */

//  ag.setVCONVERT(1.0);
//  ag.setSCONVERT(0.025);
  ag.setSCONVERT(0.0);
  cwdloss = 1.0;

    ag.setNATSEEDC (y[I_SEEDC] );
    ag.setNATSEEDSTON (y[I_SEEDN] );
/*    ag.conversion( veg.cmnt,
        y[I_ROOTC] + y[I_LEAFC],
        y[I_ROOTN]+y[I_LEAFN],
        y[I_SAPWOODC]+y[I_HEARTWOODC],
        y[I_SAPWOODN]+y[I_HEARTWOODN],
        0.0,
        y[I_SOLC],
        y[I_SOLN],
        cwdloss,
        CYCLE ); */

//
//  NEW Approach
//
//         cout << "entering disturbance " << pdyr << " " << pdm << " " << ichrt << " " << y[I_LEAFC]/ag.getVCONVERT()+y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC] << " " << ag.getVCONVERT() << " " << y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC] << " " << veg.srccmnt << endl;

    ag.conversion( veg.cmnt,
//        y[I_ROOTC] + y[I_LABILEC],
        y[I_ROOTC] + y[I_LABILEC]*(1-ag.getVCONVERT()),
//        y[I_ROOTN],
        y[I_ROOTN] + y[I_LABILEN]*(1-ag.getVCONVERT()),
        y[I_LEAFC]/ag.getVCONVERT()+y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC],
        y[I_LEAFN]/ag.getVCONVERT()+y[I_SAPWOODN]+y[I_HEARTWOODN],
        y[I_LABILEN],
        y[I_SOLC],
        y[I_SOLN],
        cwdloss,
        CYCLE ); 


    ag.createWoodProducts( pdyr,
//                           (y[I_HEARTWOODC]+y[I_SAPWOODC]+ y[I_LABILEC]),
                           (y[I_HEARTWOODC]+y[I_SAPWOODC]),
//                           (y[I_HEARTWOODN]+y[I_SAPWOODN]+ y[I_LABILEN]) );
                           (y[I_HEARTWOODN]+y[I_SAPWOODN] ));
    ag.standingdead( veg.cmnt,
                   (y[I_ROOTC]),
                   (y[I_ROOTN]),
                   0.0,
                   0.0,
                   cwdloss );

    prevy[I_LEAFC] = y[I_LEAFC] = ZERO;
    y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
    prevy[I_LEAFN] = y[I_LEAFN] = ZERO;
    prevy[I_ROOTC] = y[I_ROOTC] = ZERO;
    prevy[I_ROOTN] = y[I_ROOTN] = ZERO;
    prevy[I_SEEDC] = y[I_SEEDC] = ZERO;
    prevy[I_SEEDN] = y[I_SEEDN] = ZERO;
    prevy[I_SAPWOODC] = y[I_SAPWOODC] = ZERO;
    prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = ZERO;
    prevy[I_LABILEC] = y[I_LABILEC] = ZERO;
    prevy[I_SAPWOODN] = y[I_SAPWOODN] = ZERO;
    prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = ZERO;
    prevy[I_LABILEN] = y[I_LABILEN] = ZERO;
// for Chris only
//    if(ichrt > 6) {
//       prevy[I_SOLC] = y[I_SOLC] = ZERO;
//       prevy[I_SOLN] = y[I_SOLN] = ZERO;
//    }
  }

else if ( disturbflag ==  2 && getTimestep45().isMONTHSTART( pdm, disturbmonth - 1 )) // timber harvest clearcut
  {
    distmnthcnt = 1;
    timber = 1.0;

//
//  Timber harvest product values from conservatree.org based on Weyerhauser
//
 ag.setVCONVERT(0.21);
//  ag.setVCONVERT(0.73);
  ag.setSCONVERT(0.0);
  ag.setPROD10PAR(0.32);
//  ag.setPROD10PAR(0.20);
  ag.setPROD100PAR(0.47);
//  ag.setPROD100PAR(0.07);
  cwdloss = 0.0;

    ag.setNATSEEDC (y[I_SEEDC] );
    ag.setNATSEEDSTON (y[I_SEEDN] );
    ag.createWoodProducts( pdyr,
//                           timber*(y[I_HEARTWOODC]+y[I_SAPWOODC]+ y[I_LABILEC]),
                           timber*(y[I_HEARTWOODC]+y[I_SAPWOODC]),
//                           timber*(y[I_HEARTWOODN]+y[I_SAPWOODN]+ y[I_LABILEN]) );
                           timber*(y[I_HEARTWOODN]+y[I_SAPWOODN] ));

/*    ag.conversion( veg.cmnt,
        timber*(y[I_LEAFC]+y[I_ROOTC]),
//        timber*(y[I_LEAFC]+y[I_ROOTC]+y[I_LABILEC]),
        timber*(y[I_LEAFN]+y[I_ROOTN]),
//        timber*(y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC]),
        timber*(y[I_SAPWOODC]+y[I_HEARTWOODC]),
//        timber*(1-(ag.getPROD10PAR() + ag.getPROD100PAR()))*(y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC]),
        timber*(y[I_SAPWOODN]+y[I_HEARTWOODN]),
//        timber*(1-(ag.getPROD10PAR() + ag.getPROD100PAR()))*(y[I_SAPWOODN]+y[I_HEARTWOODN]),
//        timber*y[I_LABILEN],
      0.0,
//        timber*(1-(ag.getPROD10PAR() + ag.getPROD100PAR()))*y[I_LABILEN],
        timber*y[I_SOLC],
        timber*y[I_SOLN],
        cwdloss,
        CYCLE ); */ 
//
//   Original Approach
//
     ag.conversion( veg.cmnt,
       timber*(y[I_LEAFC]+y[I_ROOTC]+y[I_LABILEC]),
       timber*(y[I_LEAFN]+y[I_ROOTN]),
       timber*(y[I_SAPWOODC]+y[I_HEARTWOODC]),
       timber*(y[I_SAPWOODN]+y[I_HEARTWOODN]),
       timber*y[I_LABILEN],
       timber*y[I_SOLC],
       timber*y[I_SOLN],
       cwdloss,
       CYCLE ); 


    prevy[I_LEAFC] = y[I_LEAFC] = (1. - timber)*y[I_LEAFC];
    y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
    prevy[I_LEAFN] = y[I_LEAFN] = (1. -  timber)*y[I_LEAFN];
    prevy[I_ROOTC] = y[I_ROOTC] = (1. - timber)*y[I_ROOTC];
    prevy[I_ROOTN] = y[I_ROOTN] = (1. -  timber)*y[I_ROOTN];
    prevy[I_LABILEC] = y[I_LABILEC] = (1. - timber)* y[I_LABILEC];
    prevy[I_LABILEN] = y[I_LABILEN] = (1. -  timber)*y[I_LABILEN];
//    prevy[I_LABILEC] = y[I_LABILEC] = y[I_SEEDC];
//    prevy[I_LABILEN] = y[I_LABILEN] = y[I_SEEDN];
    prevy[I_SAPWOODC] = y[I_SAPWOODC] = (1. - timber)*y[I_SAPWOODC];
    prevy[I_SAPWOODN] = y[I_SAPWOODN] = (1. - timber)*y[I_SAPWOODN];
    prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = (1. - timber)*y[I_HEARTWOODC];
    prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = (1. - timber)*y[I_HEARTWOODN];
   }

else if( ( 3 == disturbflag && getTimestep45().isMONTHSTART( pdm, disturbmonth - 1 ) ) || areaburn > 0.0 ) // fire, mid intensity
  {
    // Prescribed fire (disturbflag 3) burns the whole cohort; wildfire (GDSTEM-FIRE) burns the fraction areaburn of it
    if( 3 == disturbflag && getTimestep45().isMONTHSTART( pdm, disturbmonth - 1 ) ) { areaburn = 1.0; }

    ag.setVCONVERT(0.0124 * areaburn);
    ag.setSCONVERT(0.0);
//  cwdloss midrange value from Meigs et al. 2009
    cwdloss = 0.24 * areaburn;
//
       distmnthcnt = 1;
       firemnthcnt = 1;
       ag.setNATSEEDC (y[I_SEEDC] );
       ag.setNATSEEDSTON (y[I_SEEDN] );

       ag.conversion( veg.cmnt,
              y[I_ROOTC],
              y[I_ROOTN],
              (y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC])+areaburn *y[I_LEAFC]/ag.getVCONVERT(),
              (y[I_SAPWOODN]+y[I_HEARTWOODN])+areaburn*y[I_LEAFN]/ag.getVCONVERT(),
              y[I_LABILEN],
              y[I_SOLC],
              y[I_SOLN],
              cwdloss,
              1 );
//
////  overwrite the slash with same as above but add to standing dead
////
      ag.standingdead( veg.cmnt,
                      y[I_ROOTC],
                      y[I_ROOTN],
                      (areaburn-ag.getVCONVERT())*(y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC]),
                      (areaburn-ag.getVCONVERT())*(y[I_SAPWOODN]+y[I_HEARTWOODN]+y[I_LABILEN]),
                      cwdloss);

      prevy[I_LABILEC] = y[I_LABILEC] = (1 - areaburn)*y[I_LABILEC];
      prevy[I_LABILEN] = y[I_LABILEN] =  (1 - areaburn)*y[I_LABILEN];
      prevy[I_LEAFC] = y[I_LEAFC] = (1 - areaburn)*y[I_LEAFC];
      y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
      prevy[I_LEAFN] = y[I_LEAFN] = (1 - areaburn)*y[I_LEAFN];
      prevy[I_SAPWOODC] = y[I_SAPWOODC] = (1 - areaburn)*y[I_SAPWOODC];
      prevy[I_SAPWOODN] = y[I_SAPWOODN] = (1 - areaburn)*y[I_SAPWOODN];
      prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = (1 - areaburn)*y[I_HEARTWOODC];
      prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = (1 - areaburn)*y[I_HEARTWOODN];
      prevy[I_ROOTC] = y[I_ROOTC] =  (1 - areaburn)*y[I_ROOTC];
      prevy[I_ROOTN] = y[I_ROOTN] =  (1 - areaburn)*y[I_ROOTN];
  }

else if ( disturbflag ==  4 && getTimestep45().isMONTHSTART( pdm, disturbmonth - 1 )) //hurricane-strength storm
  {
//  ADDED PCP
    ag.setVCONVERT(0.0);
    ag.setSCONVERT(0.0);
    stormoccur = 1;
    cwdloss = 0.0;
//
       distmnthcnt = 1;
       ag.setNATSEEDC (y[I_SEEDC] );
       ag.setNATSEEDSTON (y[I_SEEDN] );
       ag.standingdead( veg.cmnt,
//     Specifically for Harvard Forest
                       0.99*y[I_LEAFC]+0.99*(y[I_ROOTC]),
                       0.99*y[I_LEAFN]+0.99*(y[I_ROOTN]),
                       0.99*(y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC]),
                       0.99*(y[I_SAPWOODN]+y[I_HEARTWOODN]+y[I_LABILEN]),
                       cwdloss );
//
/*                       0.2*y[I_LEAFC]+0.11*(y[I_ROOTC]),
                       0.2*y[I_LEAFN]+0.11*(y[I_ROOTN]),
                       0.11*(y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_LABILEC]),
                       0.11*(y[I_SAPWOODN]+y[I_HEARTWOODN]+y[I_LABILEN]),
                       cwdloss );*/ 


/*      prevy[I_LABILEC] = y[I_LABILEC] = (1. - 0.11)*y[I_LABILEC];   //destruction values from Gresham et al.
      prevy[I_LABILEN] = y[I_LABILEN] = (1. - 0.11)*y[I_LABILEN];
      prevy[I_LEAFC] = y[I_LEAFC] = (1. - 0.2)*y[I_LEAFC];
      prevy[I_LEAFN] = y[I_LEAFN] = (1. - 0.2)*y[I_LEAFN];
      prevy[I_SAPWOODC] = y[I_SAPWOODC] = (1. - 0.11)*y[I_SAPWOODC];
      prevy[I_SAPWOODN] = y[I_SAPWOODN] = (1. - 0.11)*y[I_SAPWOODN];
      prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = (1. - 0.11)*y[I_HEARTWOODC];
      prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = (1. - 0.11)*y[I_HEARTWOODN];
      prevy[I_ROOTC] = y[I_ROOTC] = (1. - 0.11)*y[I_ROOTC];
      prevy[I_ROOTN] = y[I_ROOTN] = (1. - 0.11)*y[I_ROOTN]; */ 

//  Havard Forest
      prevy[I_LABILEC] = y[I_LABILEC] = 0.01*y[I_LABILEC]; 
      prevy[I_LABILEN] = y[I_LABILEN] = 0.01*y[I_LABILEN];
      prevy[I_LEAFC] = y[I_LEAFC] = 0.01*y[I_LEAFC];
      y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
      prevy[I_LEAFN] = y[I_LEAFN] = 0.01* y[I_LEAFN];
      prevy[I_SAPWOODC] = y[I_SAPWOODC] = 0.01*y[I_SAPWOODC];
      prevy[I_SAPWOODN] = y[I_SAPWOODN] = 0.01*y[I_SAPWOODN];
      prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = 0.01*y[I_HEARTWOODC];
      prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = 0.01*y[I_HEARTWOODN];
      prevy[I_ROOTC] = y[I_ROOTC] = 0.01*y[I_ROOTC];
      prevy[I_ROOTN] = y[I_ROOTN] = 0.01*y[I_ROOTN]; 
  }




#if TEM_ENABLE_STORM
  if( transient && getTimestep45().isMONTHSTART( pdm, 7 ) )
    applyStormDisturbance( ichrt, rflog1 );
#endif


//cout << "diag = " << veg.getVEGC() << " " << y[I_SOLC] << " " << ag.getSLASHC() << " " << ag.getVOLAC() << " " << ag.getCONVRTFLXC() << " " << ag.getSTANDDEADC() << endl;
  dq10 = microbe.setDQ10LT( veg.cmnt,
                       atms.getTAIR(),
                       veg.getTOPTMIC() );

  rhmoist = microbe.setRHMOIST( veg.cmnt, soil.getPCTFLDCAP(), soil.getPCTWILTPT(), soil.getPCTPOR(), y[I_VSM], moistlim );

//  BSF uncommented updatestanddead for fire
  ag.updatestanddead(pdyr,rhmoist,dq10,veg.getCNLTR( veg.cmnt ));
//  ag.updatestanddead(pdyr,rhmoist,dq10,microbe.getCNSOIL( veg.cmnt ));


    if ( 1 == disturbflag && ag.state >= 1 )
    {
      // Update rooting depth to be appropriate to crops

    y[I_VSM] = soil.updateRootZ( ag.cmnt,
                                y[I_SM],
                                y[I_ROOTC] );



      // Establish crops

      prevy[I_LABILEC] = y[I_LABILEC] = ag.getCROPSEEDC( ag.cmnt );
      prevy[I_LABILEN] = y[I_LABILEN] = ag.getCROPSEEDSTON( ag.cmnt );

      prevy[I_LEAFC] = y[I_LEAFC] = ZERO;
      y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
      prevy[I_LEAFN] = y[I_LEAFN] = ZERO;
      prevy[I_SAPWOODC] = y[I_SAPWOODC] = ZERO;
      prevy[I_SAPWOODN] = y[I_SAPWOODN] = ZERO;
      prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = ZERO;
      prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = ZERO;
      prevy[I_ROOTC] = y[I_ROOTC] = ZERO;
      prevy[I_ROOTN] = y[I_ROOTN] = ZERO;
      prevy[I_SEEDC] = y[I_SEEDC] = ZERO;
      prevy[I_SEEDN] = y[I_SEEDN] = ZERO;

      // Update soil texture-dependent vegetation parameters for crops

      veg.resetEcds( ag.cmnt, soil.getPSIPLUSC() );

      // Update other adaptive parameters

      atms.yrpet = 1.0;
      soil.yreet = 1.0;


      veg.setTOPT( ag.getCROPTOPT() );

      ag.setPRVCROPNPP( ZERO );
    }
//  else { ag.setNoWoodProducts( pdyr ); }

  // Revert to natural vegetation after cropland abandonment

  if( 0 == ag.state && 1 == ag.prvstate )
  {
    // Update rooting depth to be appropriate to natural vegetation

   y[I_VSM] = soil.updateRootZ( veg.cmnt,
                                y[I_SM],
                                y[I_ROOTC] );




    // Establish natural vegetation

//  BSF change
//  Return carbon from agriculture to the atmosphere and nitrogen to the inorganic N pool in the ssoil

    prevy[I_LABILEC] = y[I_LABILEC] = ag.getNATSEEDC();
    prevy[I_LABILEN] = y[I_LABILEN] = ag.getNATSEEDSTON();

    prevy[I_LEAFC] = y[I_LEAFC] = ZERO;
    y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
    prevy[I_LEAFN] = y[I_LEAFN] = ZERO;
    prevy[I_SAPWOODC] = y[I_SAPWOODC] = ZERO;
    prevy[I_SAPWOODN] = y[I_SAPWOODN] = ZERO;
    prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = ZERO;
    prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = ZERO;
    prevy[I_ROOTC] = y[I_ROOTC] = ZERO;
    prevy[I_ROOTN] = y[I_ROOTN] = ZERO;
    prevy[I_SEEDC] = y[I_SEEDC] = ZERO;
    prevy[I_SEEDN] = y[I_SEEDN] = ZERO;

//  BSF
//  give a shot of nitrogen to allow forest to grow back better
    prevy[I_AVLN] = y[I_AVLN] = y[I_AVLN] + 15.0;

    // Update soil texture-dependent vegetation parameters
    //   for natural vegetation

    veg.resetEcds( veg.cmnt, soil.getPSIPLUSC() );


    // Update other adaptive parameters

    veg.setTOPT( ag.getNATTOPT() );
    ag.setPRVCROPNPP( ZERO );
  }

  if( 0 == pdyr && 0 == pdm )
  {
    //initialize rootz if first year, first month
    y[I_VSM] = soil.updateRootZ( veg.cmnt,
                               y[I_SM],
                               100.0 );
    soil.setAVLH2O(y[I_SM] - soil.getWILTPT());
    if(soil.getAVLH2O() < ZERO) { soil.setAVLH2O( ZERO );}
  }
  else
  {
    //  update awcapmm every month
    y[I_VSM] = soil.updateRootZ( veg.cmnt,
                               y[I_SM],
                               y[I_ROOTC] );
  }


  // Get environmental conditions for month "dm"
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" entering getenviron() ");
    refresh();
  #endif
  getenviron();


  // Determine effect of air temperature on GPP (i.e. temp)

  if( 1 == ag.state || 2 == ag.state )
  {
    veg.setTEMP( ag.cmnt, atms.getTAIRD(),pdyr );
    veg.phenology(ag.cmnt,
                  atms.getTAIR(),
                  atms.getTAIRD(),
                  atms.getTAIRN(),
                  soil.getAVLH2O(),
                  soil.getAWCAPMM(),
                  atms.getPREC());


//    veg.setRESPQ10( ag.cmnt, atms.getTAIR() );
  }
  else
  {
    veg.setTEMP( veg.cmnt, atms.getTAIRD(),pdyr );
    veg.phenology(veg.cmnt,
                  atms.getTAIR(),
                  atms.getTAIRD(),
                  atms.getTAIRN(),
                  soil.getAVLH2O(),
                  soil.getAWCAPMM(),
                  atms.getPREC());

//    veg.setRESPQ10( veg.cmnt, atms.getTAIR() );
  }

  // Determine effect of temperature on decomposition
  //   (i.e. dq10)

//  microbe.setDQ10( veg.cmnt,
//                   atms.getTAIR(),
//                   soil.getSNOWPACK() );

//  microbe.setDQ10LT( veg.cmnt,
//                       atms.getTAIR(),
//                       veg.getTOPTMIC() );

  // Update growing degree days (GDD)

cseed = 0.0;

  if( (atms.getTAIR() >= ag.getGDDMIN(ag.cmnt)) 
    && (atms.getTAIRN() > ag.getTKILL(ag.cmnt)) )
  {
    ag.setGROWDD( ag.getGROWDD()
                  + ((atms.getTAIR() - ag.getGDDMIN(ag.cmnt))
                  * atms.getNDAYS( pdm )) );
  }
  else
  {
    // If "cold snap" (i.e. TAIRN < TKILL) hits after crops
    //   begin to grow, crops are assumed to die and resulting
    //   detritus is added to soil organic carbon and nitrogen

    if( (1 == ag.state || 2 == ag.state)
        && ag.getGROWDD() > ag.getGDDSEED(ag.cmnt)
        && atms.getTAIRN() <= ag.getTKILL(ag.cmnt) )
    {
      #ifdef DEBUG_CTEM
        move(DEBUG_ROW,1);
        printw(" entering frostdamage() ");
        refresh();
      #endif
      ag.frostDamage( y[I_LEAFC] + y[I_SAPWOODC] + y[I_HEARTWOODC] + y[I_ROOTC] + y[I_SEEDC] + y[I_LABILEC] - ag.getCROPSEEDC( ag.cmnt ), 
                      y[I_LEAFN] + y[I_SAPWOODN] + y[I_HEARTWOODN] + y[I_ROOTN] + y[I_SEEDN] + y[I_LABILEN] - ag.getCROPSEEDSTON( ag.cmnt ) );
      
      y[I_LABILEC] = ag.getCROPSEEDC( ag.cmnt );
      prevy[I_LABILEC] = ag.getCROPSEEDC( ag.cmnt );
      y[I_LABILEN] = ag.getCROPSEEDSTON( ag.cmnt );
      prevy[I_LABILEN] = ag.getCROPSEEDSTON( ag.cmnt );

      cseed = ag.getCROPSEEDC( ag.cmnt );
      nseed = ag.getCROPSEEDSTON( ag.cmnt );

      prevy[I_LEAFC] = y[I_LEAFC] = ZERO;
      y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
      prevy[I_LEAFN] = y[I_LEAFN] = ZERO;
      prevy[I_SAPWOODC] = y[I_SAPWOODC] = ZERO;
      prevy[I_SAPWOODN] = y[I_SAPWOODN] = ZERO;
      prevy[I_HEARTWOODC] = y[I_HEARTWOODC] = ZERO;
      prevy[I_HEARTWOODN] = y[I_HEARTWOODN] = ZERO;
      prevy[I_ROOTC] = y[I_ROOTC] = ZERO;
      prevy[I_ROOTN] = y[I_ROOTN] = ZERO;
	  prevy[I_SEEDC] = y[I_SEEDC] = ZERO;
      prevy[I_SEEDN] = y[I_SEEDN] = ZERO;
      y[I_SOLC] += ag.getSTUBBLEC();
      prevy[I_SOLC] = y[I_SOLC];
      y[I_SOLN] += ag.getSTUBBLEN();
      prevy[I_SOLN] = y[I_SOLN];
    }

    if(0 == ag.getISPERENNIAL( ag.cmnt )
    && atms.getTAIRN() <= ag.getTKILL(ag.cmnt))
    {
      ag.setGROWDD( ZERO );
    }
  }

//   set irrigation
 if( ag.irrgflag == 1 &&  ag.getGROWDD() >= ag.getGDDSEED(ag.cmnt) && ag.getGROWDD() <=ag.getGDDHARVST(ag.cmnt)) {
   ag.irrigate = soil.getAVLH2O() - soil.getAWCAPMM();
  }
  else
  {
   ag.irrigate = 0.0;
  }



  // Run TEM for a monthly time step

  veg.setERRCNT( ZERO );
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" entering adapt( NUMEQ, y, ptol, pdm ) ");
    refresh();
  #endif
  
  #ifdef DEBUG_XTEM
    printf(" entering adaptive integrator \n");
//    for( i = 0; i < NUMEQ; ++i ) { printf("y[%2d] = %4.1lf \n", i, y[i]); }
  #endif
 
//BSF COMBO START
//  numprec = 1;
//  magprec = atms.getPREC()/numprec;

//  if(veg.cmnt == 9 && atms.getMXTAIR() > 30.0)
//  if((veg.cmnt == 9 || veg.cmnt == 8 || veg.cmnt == 6) && atms.getMXTAIR() > 25.0)
//  {
//    y[I_SM] = y[I_SM] + magprec;
//  }
// BSF COMBO END
  
    if(pdyr >= 97 && veg.cmnt == 6 && initFlag == 1)
    {
//   growth rate is 20% increase from 1993 to 2005 which is 12 years, so increse from 70% of deciduous to 84%, with corresponding
//   decrease from 30% to 16% of the rest; GPP goes from 1380 to 1493, so nmax goes from 525 to 680 in 12 year, which is 12.9/yr
//   assume increase continues beyond 2005 for now
//        nmax_grow[ichrt] += 12.9/12.0;
//        nmax_grow[ichrt] += 7.75/12.0;
    if(ag.state == 0)
       {
        nmax_grow[ichrt] = veg.getNMAX1B( veg.cmnt );
//          nmax_grow[ichrt] += 12.9/12.0;
       }
    else
       {
        nmax_grow[ichrt] = veg.getNMAX1B( ag.cmnt );
       }
    }
    else
    {
    if(ag.state == 0)
    {
    nmax_grow[ichrt] = veg.getNMAX1B( veg.cmnt );
    }
    else
    {
    nmax_grow[ichrt] = veg.getNMAX1B( ag.cmnt );
    }
    }


    #ifdef CALIBRATE_TEM
        nmax_grow[ichrt] = veg.getNMAX();
    #endif

//cout << "diag = " << veg.getVEGC() << " " << y[I_SOLC] << " " << y[I_SLASHC] << " " << y[I_VOLAC] << " " << ag.getSCONVRTFLXC() << " " << y[I_STANDDEADC] << endl;
//  cout << "nmax_grow = " << veg.cmnt << "  " << nmax_grow[ichrt] << " " << pdyr << " " << pdm << " " << initFlag << endl;
//cout << "time = " << pdyr << " " << pdm << " " << y[I_SAPWOODC] << " " << disturbflag << " " << veg.cmnt << " " << sapwoodcb[veg.cmnt] << " " << sapwoodcb[11] << " " << sapwoodcb[4] << " " << sapwoodcb[1] << " " << sapwoodcb[2] << " " << sapwoodcb[3] << " " << sapwoodcb[5] << " " << sapwoodcb[6] << " " << sapwoodcb[7] << " " << sapwoodcb[8] << " " << sapwoodcb[9] << " " << sapwoodcb[10] << " " << sapwoodcb[12] << endl;

//cout << "time = " << pdyr << " " << pdm  << " " << disturbflag << " " << veg.cmnt  << " " << veg.getGPP() << endl;
//cout << "time = " << pdyr << " " << pdm  <<  endl;

//cout << "time = " << pdyr << " " << pdm << " " << sapwoodcb[1] << " " << sapwoodcb[2] << " " << sapwoodcb[3] << " " << sapwoodcb[4] << " " << sapwoodcb[5] << " " << sapwoodcb[6] << " " << sapwoodcb[7] << " " << sapwoodcb[8] << " " << sapwoodcb[9] << " " << sapwoodcb[10] << " " << sapwoodcb[11] << " " << sapwoodcb[12] << " " << sapwoodcb[13] << " " << sapwoodcb[14] << " " << sapwoodcb[15] << " " << sapwoodcb[16] << " " << sapwoodcb[17] << " " << sapwoodcb[18] << " " << sapwoodcb[19] << " " << sapwoodcb[20] << endl;
//if(initFlag == 1) {cout << "time = " << startyr + pdyr << " " << pdm << " " << veg.cmnt << " " << ag.cmnt << endl;}
//{cout << "time = " << startyr + pdyr << " " << pdm << endl;}
//cout << "diag = " << pdyr << " " << pdm << " " << veg.getVEGC() << " " << atms.getTAIR() << " " <<  atms.getCO2() << " " << atms.getNDEP() << " " << veg.getFOZONE() << " " << veg.cmnt << " " << ag.cmnt << " " << ag.state << " " << soil.getPCTPOR() << " " <<  initFlag << endl;
//  if(pdyr == 1969) {cout << "entering adapt" << endl;}
  mintflag = adapt( NUMEQ, y, ptol, pdm, pdyr, nmax_grow[ichrt] );
//  if(pdyr == 1969) {cout << "leaving adapt" << endl;}



  if( 1 == mintflag ) { intflag = 1; }

  if ( blackhol != 0 )
  {
    if( 0 == initFlag || pdyr < 0 || pdyr >= NYEARS ) { qualcon[0] = 10; }
    else { qualcon[pdyr] += 10; }
  }

  // Check mass balance
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" entering massbal( y, prevy ) ");
    refresh();
  #endif
  //massbal( y, prevy );
  massbal();  //BSF COMBO FIX

//ag.setSLASHC(0.0);
//ag.setSLASHN(0.0);
//cout << "year, month = " << pdyr << " " << pdm << " " << ag.fert1950flag << endl;
//if(initFlag == 1) {cout << "year, month = " << pdyr << " " << pdm << " " << soil.getSONINP()  <<  " " << veg.getVEGNINP() << endl;}
//cout << "year, month = " << pdyr << " " << pdm << " " << soil.getSONINP()  <<  " " << veg.getVEGNINP() << endl;
//cout << "year, month = " << pdyr << " " << pdm << " " << y[I_LEAFC] << " " << y[I_AVLN] << " " << ag.getIMMBADD() << endl;
//cout << "year, month = " << pdyr << " " << pdm << " " << veg.getVEGC() << " " << veg.getGPP() << " " << veg.getVEGN() << " " << y[I_SOLC] << " " << y[I_SOLN] << " " << y[I_AVLN] <<  " " << veg.getSTRN() << " " <<  y[I_LABILEN] <<  endl;
//  cout << "veggpp = " << veg.cmnt <<  " " << atms.getNDAYS(pdm) << "  " << atms.getCO2() << "  " << atms.getPAR() << " " << atms.getVPR() << " " << atms.getVPDD() << " " << atms.getDAYL() << " " << veg.pen.getKEXT(veg.cmnt) << " " <<   atms.getPREC() << " " << veg.getCMAX() << endl;
//  cout << "veggpp = " << veg.cmnt <<  " " <<  veg.getLAI() << " " <<  veg.getALLOCLC() << " " << veg.getRMLEAF() << " " << veg.getLTRLC() <<  " " << pen.getLSCMIN(veg.cmnt) << endl;
//`cout << "year, month = " << pdyr << " " << pdm << " " <<veg.getGPP() << " " << y[I_INGPP] << endl;
//cout << "ROOTN = " << y[I_ROOTN] << " " << veg.getALLOCRN() << " " << veg.getNRESORBR() << " " << veg.getLTRRN() << endl;

//cout << "diag = " << pdyr << " " << pdm << " " << soil.getAVLH2O() << " " <<  soil.getAWCAPMM() << " " << veg.getPESOILW() << " " <<  atms.getPREC() << endl;

//   if(getTimestep45().isYEAREND( pdm ))
//   {

//   vegceq[pdyr] = veg.getVEGC();
//   vegceq[pdyr] = y[I_LEAFC]+y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_ROOTC]+y[I_LABILEC];
//   soilceq[pdyr] = y[I_SOLC];
//   vegneq[pdyr] = veg.getVEGN();
//   vegneq[pdyr] = y[I_LEAFN]+y[I_SAPWOODN]+y[I_HEARTWOODN]+y[I_ROOTN]+y[I_LABILEN];
//   soilneq[pdyr] = y[I_SOLN];
//   cout << "diag = " << pdyr << " " << vegceq[pdyr] << " " << soilceq[pdyr] << " " << vegneq[pdyr] << " " << soilneq[pdyr] << endl;

//   }
//cout << "time = " << pdyr << " " << pdm << " " << veg.cmnt << " " << pen.getLSCMIN(veg.cmnt) << " " << veg.getCMAX() << endl;
//cout << "water = " << pdyr << " " << pdm << " " << y[I_SM] <<  " " << soil.getAVLH2O() << " " << soil.getSNOWINF() << " " << atms.getRAIN() << " " << y[I_EET] << " " << y[I_RPERC] << " " << y[I_SPERC] << " " << soil.getSNOWINF()+atms.getRAIN()-y[I_EET]-y[I_RPERC]-y[I_SPERC] << " " << y[I_SM] - prevy[I_SM] << " " << y[I_PECAN] << " " << y[I_PESOIL] << " " << y[I_VSM] << endl;

//cout << "leafc = " << y[I_LEAFC] << " " << y[I_ALLOCLC] << " " << y[I_RMLEAF] << " " << y[I_LTRLC] << " " << y[I_LEAFC] - prevy[I_LEAFC] << endl;

//cout <<"vegc = " << y[I_SAPWOODC] << " " << y[I_HEARTWOODC] << " " << y[I_ROOTC] << " " << y[I_SEEDC] << " " << y[I_LABILEC] << " " << y[I_INGPP] << " " << y[I_GPP] << endl;

  wevapd[pdm] = pen.watev( atms.getNIRRN(),
                     atms.getLWOUTD(),
                     atms.getTAIRD(),
                     atms.getVPDD(),
                     atms.getWS10(),
                     soil.getRRUN(),
                     soil.getSRUN(),
                     pdm );

  wevapn[pdm] = pen.watev( 0.0,
                     atms.getLWOUTN(),
                     atms.getTAIRN(),
                     atms.getVPDN(),
                     atms.getWS10(),
                     soil.getRRUN(),
                     soil.getSRUN(),
                     pdm );

  if( soil.getRRUN()+soil.getSRUN() > 0.0)
  {
  rfrac[pdm] = soil.getRRUN()/(soil.getRRUN()+soil.getSRUN());


//  soil.setRRUN(soil.getRRUN() - (rfrac*wevap));
//  soil.setSRUN(soil.getSRUN() - ((1-rfrac)*wevap));
//  if(soil.getRRUN() <= 0.0) {soil.setRRUN( 0.0 );}
//  if(soil.getSRUN() <= 0.0) {soil.setSRUN( 0.0 );}
  } 

//if(disturbflag == 2) {cout << "heartwoodc = " << y[I_HEARTWOODC] << endl;}
  // Determine vegetation total carbon and nitrogen stocks
  veg.setVEGC( y[I_LEAFC] + y[I_SAPWOODC] + y[I_HEARTWOODC] + y[I_ROOTC] + y[I_SEEDC] + y[I_LABILEC] );

  veg.setSTRN( y[I_LEAFN] + y[I_SAPWOODN] + y[I_HEARTWOODN] + y[I_ROOTN] + y[I_SEEDN] );

  veg.setVEGN( (veg.getSTRN() + y[I_LABILEN]) );

//cout << "diag = " << veg.getVEGC() << " " << y[I_SOLC] << " " << ag.getSLASHC() << " " << ag.getVOLAC() << " " << ag.getCONVRTFLXC() << " " << ag.getSTANDDEADC() << endl;
//cout << "diag = " << veg.getVEGC() << " " << y[I_SOLC] << " " << y[I_SLASHC] << " " << y[I_VOLAC] << " " << ag.getSCONVRTFLXC() << " " << y[I_STANDDEADC] << endl;

  // Determine water yield (soil.h2oyld)

  soil.setH2OYLD( (y[I_RRUN] + y[I_SRUN]) );


  // Determine Net Ecosystem Production (nep)

//  nep = y[I_NPP] - y[I_RH];
//  nep = y[I_NPP] - y[I_RH] - ag.getVOLAC();

//cout << "year, month = " << pdyr << " " << pdm << " " << nep << " " << y[I_NPP] << " " << y[I_RH] << endl;
  // Detemine total monthly N inputs to ecosystem

//  soil.setNINPUT( (soil.getNINPUT() + ag.getNRETENT() + y[I_AGFRTN]) + y[I_NFIXN] + y[I_NFIXS] );
#ifdef OPENN
//  soil.setNINPUT( soil.getNINPUT() + ag.getNRETENT() + y[I_NFIXN] + y[I_NFIXS] + ag.getSTUBBLEN() );
//  soil.setNINPUT( soil.getNINPUT() +  y[I_NFIXN] + y[I_NFIXS] + nseed );
//  soil.setNINPUT( soil.getNINPUT() +  y[I_NFIXN] + y[I_NFIXS] );
//  soil.setNINPUT( soil.getNINPUT() + ag.getNRETENT() + soil.getSONINP() + veg.getVEGNINP() );
//  soil.setNLOST( soil.getNLOST() + soil.getLCHDON() );
//  soil.setNINPUT( soil.getNINPUT() + y[I_NFIXN] + y[I_NFIXS] );
//  if(disturbflag == 2) {cout << "NRETENT = " << ag.getNRETENT() << " " << soil.getNINPUT() << endl;}
#else
//   soil.setNINPUT( (ag.getNRETENT() + y[I_AGFRTN]) );
//   soil.setNINPUT( (ag.getNRETENT() + ag.fertn) );
//   soil.setNINPUT( ( ag.fertn) );
#endif 

//  soil.setNINPUT( y[I_NINP] + y[I_NFIXN] + y[I_NFIXS] );
//  soil.setNLOST( (y[I_NLST]
//                  + y[I_LCHDON]
//                  + ag.getCONVRTFLXN()
//                  + ag.getCROPRESIDUEFLXN()) );


  // Determine fluxes from crop residues

//  cout << "diag = " <<  ag.getCROPRESIDUEFLXC() << endl;
//  ag.updateCropResidueFluxes();


  // Determine fluxes from decay of products

//  ag.decayProducts();


  // Harvest crops after a specific number of growing degree
  //   days; reset growing degree days to zero if crops were
  // harvested this month
//   cout << "harvest = " << ag.getGROWDD() << " " << ag.getGDDHARVST(ag.cmnt) << " " << y[I_NPP] << " " << y[I_SEEDC] << " " << y[I_ALLOCSEEDC] << " " << y[I_RMSEED] << " " << y[I_LTRSEEDC] << endl;
//cout << "gdd " << ag.getGROWDD() << endl;
  if(getTimestep45().isYEARSTART( pdm )) {harcnt = 0;}
  nprod = 0.0;
  nseed = 0.0;

//if(ag.state == 1 && initFlag == 1) {
//cout << "harvest = " << ag.state << " " << ag.getGROWDD() << " " << ag.getGDDHARVST(ag.cmnt) << " " << pdyr << " " << pdm << endl;}
  if( (1 == ag.state || 2 == ag.state) && ((ag.getGROWDD() >= ag.getGDDHARVST(ag.cmnt)) || (harcnt == 0 && getTimestep45().isMONTHSTART( pdm, 9 )))    && (0 == ag.getFROSTFLAG() ) )
//  if( (1 == ag.state) && (ag.getGROWDD() >= ag.getGDDHARVST(ag.cmnt)) && (0 == ag.getFROSTFLAG() ) )
  {
//    ag.harvest( pdm, y[I_SEEDC], y[I_SEEDN], veg.getVEGC() - ag.getCROPSEEDC( ag.cmnt ), veg.getVEGN() - ag.getCROPSEEDSTON( ag.cmnt ) );
//    cout << "entering crop harvest" << endl;
    ag.harvest( pdm, y[I_SEEDC], y[I_SEEDN], veg.getVEGC(), veg.getVEGN());
//if (ag.getCROPPRODN() > 0.0)
//{
//  cout << " cropprod = " << ag.getCROPPRODN() << endl;
//
   nprod = ag.getCROPPRODN();
//   cout << "nprod = " << nprod << endl;

    if( 0 == ag.getISPERENNIAL( ag.cmnt )) // harvesting an annual kills the crop
    {
      y[I_LABILEC] = ag.getCROPSEEDC( ag.cmnt );
      y[I_LABILEN] = ag.getCROPSEEDSTON( ag.cmnt );

      cseed =  ag.getCROPSEEDC( ag.cmnt );
      nseed = ag.getCROPSEEDSTON( ag.cmnt );   

      y[I_LEAFC] = ZERO;
      y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
      y[I_LEAFN] = ZERO;
      y[I_SAPWOODC] = ZERO;
      y[I_SAPWOODN] = ZERO;
      y[I_HEARTWOODC] = ZERO;
      y[I_HEARTWOODN] = ZERO;
      y[I_ROOTC] = ZERO;
      y[I_ROOTN] = ZERO;
      y[I_SEEDC] = ZERO;
      y[I_SEEDN] = ZERO;
    }
    else // harvesting a perennial just removes the "seed" pool
    {
      y[I_SEEDC] = ZERO;
      y[I_SEEDN] = ZERO;
    }
    y[I_SOLC] += ag.getSTUBBLEC();
    y[I_SOLN] += ag.getSTUBBLEN();

//    cout << "adding stubble " << ag.getSTUBBLEC() << endl;
    ag.setGROWDD( ZERO );
    harcnt = harcnt + 1; 
//cout << "diag = " << ag.getCROPPRODC() << " " << veg.getVEGC() << endl;
//ag.updateCropResidueFluxes();
  }
  else { ag.setNoCrops( pdm ); } 


/*  if( 2 == ag.state & (ag.getGROWDD() >= ag.getGDDHARVST(ag.cmnt))) // pasture
  {
      frcab =(y[I_LEAFC]+y[I_SAPWOODC])/(y[I_LEAFC]+y[I_SAPWOODC]+y[I_ROOTC]);
      cabove = 0.05*(y[I_LABILEC]*frcab + y[I_LEAFC] + y[I_SAPWOODC] + y[I_HEARTWOODC]);
	  nabove = 0.05*(y[I_LABILEN]*frcab + y[I_LEAFN] + y[I_SAPWOODN] + y[I_HEARTWOODN]);
      y[I_LABILEC] = y[I_LABILEC]*frcab*0.95 + (y[I_LABILEC]*(1-frcab));
      y[I_LABILEN] = y[I_LABILEN]*frcab*0.95 + (y[I_LABILEN]*(1-frcab));
      y[I_LEAFC] *= 0.95;
      y[I_LEAFN] *= 0.95;
      y[I_SAPWOODC] *= 0.95;
      y[I_SAPWOODN] *= 0.95;
      y[I_HEARTWOODC] *= 0.95;
      y[I_HEARTWOODN] *= 0.95;
      ag.setPREVPROD1C( ag.getPREVPROD1C()+ 0.83 * cabove );
      y[I_SOLC] += 0.17 * cabove;
      y[I_SOLN] += 0.5 * nabove;
      y[I_AVLN] += 0.5 * nabove;
  } 

 if( 3 == ag.state & (ag.getGROWDD() >= ag.getGDDHARVST(ag.cmnt))) // turflawn
  {
      frcab =(y[I_LEAFC]+y[I_SAPWOODC])/(y[I_LEAFC]+y[I_SAPWOODC]+y[I_ROOTC]);
      cabove = 0.16*(y[I_LABILEC]*frcab + y[I_LEAFC] + y[I_SAPWOODC] + y[I_HEARTWOODC]);
      y[I_LABILEC] = y[I_LABILEC]*frcab*0.84 + (y[I_LABILEC]*(1-frcab));
      y[I_LABILEN] = y[I_LABILEN]*frcab*0.84 + (y[I_LABILEN]*(1-frcab));
      y[I_LEAFC] *= 0.84;
      y[I_LEAFN] *= 0.84;
      y[I_SAPWOODC] *= 0.84;
      y[I_SAPWOODN] *= 0.84;
      y[I_HEARTWOODC] *= 0.84;
      y[I_HEARTWOODN] *= 0.84;
      ag.setPREVPROD1C( ag.getPREVPROD1C()+  cabove );
 } */

 if( 3 == ag.state || 4 == ag.state ) // pasture
  {
   ag.grazing(  y[I_LEAFC], y[I_SAPWOODC], y[I_HEARTWOODC], y[I_LABILEC],
                y[I_LEAFN], y[I_SAPWOODN], y[I_HEARTWOODN], y[I_LABILEN],
                y[I_ROOTC] );
    y[I_LEAFC] -= ag.getFORAGECLEAF();
    y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
    y[I_SAPWOODC] -= ag.getFORAGECSAP();
    y[I_HEARTWOODC] -= ag.getFORAGECHEART();
    y[I_LABILEC] -= ag.getFORAGECLABILE();
    y[I_LEAFN] -= ag.getFORAGENLEAF();
    y[I_SAPWOODN] -= ag.getFORAGENSAP();
    y[I_HEARTWOODN] -= ag.getFORAGENHEART();
    y[I_LABILEN] -= ag.getFORAGENLABILE();
    y[I_SOLC] += ag.getMANUREC();
    y[I_SOLN] += ag.getMANUREN();
    y[I_AVLN] += ag.getURINE();
  }
  else { ag.setNoGrazing();
        }     
//ag.setNoGrazing();

 if( (5 == ag.state || 6 == ag.state) && (ag.getGROWDD() >= ag.getGDDSEED(ag.cmnt) && ag.getGROWDD() <= ag.getGDDHARVST(ag.cmnt))) // turflawn
  {
      if((y[I_LEAFC]+y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_ROOTC]) == 0.0)
      {
        frcab = 0.0;
      }
      else
      {
      frcab =(y[I_LEAFC]+y[I_SAPWOODC]+y[I_HEARTWOODC])/(y[I_LEAFC]+y[I_SAPWOODC]+y[I_HEARTWOODC]+y[I_ROOTC]);
      }
      cabove = 0.16*(y[I_LABILEC]*frcab + y[I_LEAFC] + y[I_SAPWOODC] + y[I_HEARTWOODC]);
      nabove = 0.16*(y[I_LABILEN]*frcab + y[I_LEAFN] + y[I_SAPWOODN] + y[I_HEARTWOODN]);
      y[I_LABILEC] = y[I_LABILEC]*frcab*0.84 + (y[I_LABILEC]*(1-frcab));
      y[I_LABILEN] = y[I_LABILEN]*frcab*0.84 + (y[I_LABILEN]*(1-frcab));
      y[I_LEAFC] *= 0.84;
      y[I_LAI] = y[I_LEAFC] * veg.getSLA(veg.cmnt);
      y[I_LEAFN] *= 0.84;
      y[I_SAPWOODC] *= 0.84;
      y[I_SAPWOODN] *= 0.84;
      y[I_HEARTWOODC] *= 0.84;
      y[I_HEARTWOODN] *= 0.84;
    ag.setCLIPPINGS(cabove);
    y[I_SOLC] += cabove;
    y[I_SOLN] += nabove;
 }  
//
//  BSF move calculation of updateCropResidueFluxes and decayProducts here
//
// Determine fluxes from crop residues
//
//   nep = y[I_NPP] - y[I_RH] - ag.getVOLAC() + cseed - soil.getLCHDOC();
//   nep = y[I_NPP] - y[I_RH] - ag.getVOLAC();
     
     veg.setGPR( y[I_RVMNT] + y[I_RVGRW] );
     veg.setINNPP( y[I_INGPP] - veg.getGPR() );
     veg.setNPP( y[I_GPP] - veg.getGPR() ) ;
     nep = veg.getNPP() - y[I_RH];

//     nep = y[I_GPP] - y[I_RVMNT] -y[I_RVGRW] - y[I_RH];

//     cout << "nep = " << nep << " " << y[I_NPP] << " " << y[I_RH] << endl;


//  cout << "diag = " <<  ag.getCROPRESIDUEFLXC() << endl;
   ag.updateCropResidueFluxes();


// Determine fluxes from decay of products

   ag.decayProducts();


  // Determine crop residue

  ag.updateCropResidue();


  // Determine standing stock of products

  ag.updateProducts();


  // Determine total amount of products formed
  //   this month

  ag.updateTotalProductFormation();

  // Determine CFLUX from ecosystem from NEP plus
  //   fluxes from burning associated with agricultural
  //   conversion or

  ag.setCFLUX( (nep
                - ag.getCONVRTFLXC()
                - ag.getCROPRESIDUEFLXC()) );

  // Determine Net Carbon Exchange (NCE) with atmosphere
  //   (CFLUX plus decay of agricultural and wood products)

  nce = ag.getCFLUX() - ag.getTOTPRODDECAYC();

  nbp = nce - ag.getVOLAC() + cseed - soil.getLCHDOC();


//  if(initFlag == 1 && nce < nbp && (veg.cmnt == 5 || veg.cmnt == 6)) { cout << "nce = " << nce << " " << nbp << " " << ag.getVOLAC() << " " << cseed << " " << soil.getLCHDOC() << " " << veg.cmnt << " " << ag.cmnt << " " << pdyr << " " << pdm << " " << ichrt << endl;}
  // Determine carbon storage in ecosystem

  ag.setTOTEC( (veg.getVEGC()
             + y[I_SOLC]) );


//  veg.setTOTC( veg.getVEGC() );


  // Determine total carbon in ecosystems plus
  //   agricultural and wood products

  totalc = ag.getTOTEC() + ag.getTOTPRODC();

  // Update total loss of nitrogen from ecosystem for flux
  //   associated with crop residue


//  soil.setNLOST( (soil.getNLOST()
//                  + soil.getLCHDON()
//                  + ag.getCONVRTFLXN()
//                  + ag.getCROPRESIDUEFLXN()) );

  // Detemine total monthly N inputs to ecosystem

//  soil.setNINPUT( (soil.getNINPUT() + ag.getNRETENT() + y[I_AGFRTN]) + y[I_NFIXN] + y[I_NFIXS]);
//  soil.setLCHDIN( (y[I_NLST]
//                  + ag.getCONVRTFLXN()
//                  + ag.getCROPRESIDUEFLXN()) );

//  favg = 1.0 - exp(-1.0 / (12.0*tauavg));
//NOTINSHREE
#ifdef OPENN
//  soil.yrlchdin = soil.yrlchdin*(1.0 - favg) + 365.0*y[I_NLST]*favg;
//  soil.yrlchdin = soil.yrlchdin*(1.0 - favg) + 12.0*y[I_NLST]*favg;
//  soil.yrlchdin = soil.yrlchdin*(1.0 - favg) + 12.0*soil.getLCHDIN()*favg;

//  soil.setNLOST( (y[I_NLST]
//cout << "nlost = " << soil.getNLOST() << " " << soil.getLCHDON() << " " << ag.getCONVRTFLXN() << " " << ag.getCROPRESIDUEFLXN() << endl;
  soil.setNLOST( (soil.getNLOST()
//                  + nprod
//                  + y[I_LCHDON]
                  + soil.getLCHDON()
                  + ag.getCONVRTFLXN()
                  + ag.getCROPRESIDUEFLXN()) );
//cout << "nlost = " << soil.getNLOST() <<  endl;
//  soil.setNINPUT( soil.getNINPUT() +  y[I_NFIXN] + y[I_NFIXS] + nseed );
//cout << "NINPUT = " << soil.getNINPUT() << " " << y[I_NFIXN] << " " << y[I_NFIXS] << endl;
  soil.setNINPUT( soil.getNINPUT() +  y[I_NFIXN] + y[I_NFIXS] );
#else
  soil.setNLOST( (soil.getNLOST()
                  + ag.getCONVRTFLXN()
                  + ag.getCROPRESIDUEFLXN()) );
  soil.setNINPUT( ( ag.fertn) );
#endif 

//cout << "final Nvalues = " << soil.getNLOST() << " " << soil.getNINPUT() << endl;

//  y[I_NLST] = soil.getNLOST();
//  y[I_NLST] += y[I_LCHDON] + ag.getCONVRTFLXN() + ag.getCROPRESIDUEFLXN();

  

//  cout << "din = " << soil.getLCHDON() << " " << soil.getNLOST() << " " << y[I_NLST] << " " << soil.getLCHDOC() << " " << microbe.getDOCDON(veg.cmnt) << endl;
  // Update ANNUAL carbon, nitrogen and water pools and fluxes
  //   from integrator results

/*  if( 0 == baseline )
    {
      sonp = soil.getSONINP()/soil.getNINPUT();
      availnp = ((atms.getNDEP()/(1000.0*CYCLE))+ag.fertn)/soil.getNINPUT();
      donpout = soil.getLCHDON()/(soil.getNLOST());
      dinpout = 1-donpout;

      vegnp = 1-(sonp+availnp);
      if(soil.getNINPUT() > soil.getNLOST())
      {

        y[I_SOLN] = y[I_SOLN] - (soil.getNINPUT() - soil.getNLOST())*sonp;
        y[I_AVLN] = y[I_AVLN] - (soil.getNINPUT() - soil.getNLOST())*availnp;
        y[I_LABILEN] = y[I_LABILEN] - (soil.getNINPUT() - soil.getNLOST())*vegnp;
        veg.setVEGN( (veg.getSTRN() + y[I_LABILEN]) );
        soil.setNINPUT(soil.getNLOST());
      }
      if(soil.getNLOST() > soil.getNINPUT())
      {
        y[I_DON] = y[I_DON] + (soil.getNLOST() - soil.getNINPUT())*donpout;
        y[I_AVLN] = y[I_AVLN] + (soil.getNLOST() - soil.getNINPUT())*dinpout;
        soil.setNLOST(soil.getNINPUT());
      }
    } */


  updateYearSummary( pdm );
  if(mxeet < soil.getEET()) { mxeet = soil.getEET(); }

  if(pdyr == (int)tauavg && ( getTimestep45().isYEAREND( pdm ) || isCutStep( pdm ) )) { yearSummaryExtrapolate(); }

  if( getTimestep45().isYEAREND( pdm ) || isCutStep( pdm ) )
  {
    if( (veg.yralloclc+veg.yrallocsc+veg.yrallocrc+veg.yrallocseedc
        -veg.yrrmleaf-veg.yrrmsapwood-veg.yrrmroot) != ZERO )
    { 
      veg.yrpleaf = (veg.yralloclc-veg.yrrmleaf)/
        (veg.yralloclc+veg.yrallocsc+veg.yrallocrc+veg.yrallocseedc
        -veg.yrrmleaf-veg.yrrmsapwood-veg.yrrmroot);

      veg.yrpsapwood = (veg.yrallocsc-veg.yrrmsapwood)/
        (veg.yralloclc+veg.yrallocsc+veg.yrallocrc+veg.yrallocseedc
        -veg.yrrmleaf-veg.yrrmsapwood-veg.yrrmroot);

      veg.yrproot = (veg.yrallocrc-veg.yrrmroot)/
        (veg.yralloclc+veg.yrallocsc+veg.yrallocrc+veg.yrallocseedc
        -veg.yrrmleaf-veg.yrrmsapwood-veg.yrrmroot);

      veg.yrpseed = (veg.yrallocseedc)/
        (veg.yralloclc+veg.yrallocsc+veg.yrallocrc+veg.yrallocseedc
        -veg.yrrmleaf-veg.yrrmsapwood-veg.yrrmroot);
    }
    else
    {
      veg.yrpleaf = ZERO;
      veg.yrpsapwood = ZERO;
      veg.yrproot = ZERO;
      veg.yrpseed = ZERO;
    }
    
    if( (veg.yrallocilc+veg.yrallocisc+veg.yrallocirc+veg.yrallociseedc) != ZERO )
    { 
      veg.yrpleafi = veg.yrallocilc/
        (veg.yrallocilc+veg.yrallocisc+veg.yrallocirc+veg.yrallociseedc);

      veg.yrpsapwoodi = veg.yrallocisc/
        (veg.yrallocilc+veg.yrallocisc+veg.yrallocirc+veg.yrallociseedc);

      veg.yrprooti = veg.yrallocirc/
        (veg.yrallocilc+veg.yrallocisc+veg.yrallocirc+veg.yrallociseedc);

      veg.yrpseedi = veg.yrallociseedc/
        (veg.yrallocilc+veg.yrallocisc+veg.yrallocirc+veg.yrallociseedc);
    }
    else
    {
      veg.yrpleafi = ZERO;
      veg.yrpsapwoodi = ZERO;
      veg.yrprooti = ZERO;
      veg.yrpseedi = ZERO;
    }
    
    veg.setRPLEAF( veg.getRPLEAF()*pow(avgfac,CYCLE) + veg.yrpleafi*(1.0 - pow(avgfac,CYCLE)));
  }



  #ifdef CALIBRATE_TEM
    // Display monthly results to DOS screen
    pcdisplayMonth( pdyr, pdm );
  #endif


  if( (1 == ag.state || 2 == ag.state) && ag.getPRVCROPNPP() < veg.getNPP() )
  {
     ag.setPRVCROPNPP( veg.getNPP() );
  }
  else { ag.setPRVCROPNPP( ZERO ); }

// Reset growing degree days to zero if crops were
// harvested this month

  if( (1 == ag.state || 2 == ag.state) && ag.getGROWDD() >= ag.getGDDHARVST(ag.cmnt) )
  {
    ag.setGROWDD( ZERO );
  }

  if( atms.getTAIR() < ag.getGDDMIN(ag.cmnt) ) { ag.setGROWDD( ZERO ); }
  if( atms.getTAIRN() < ag.getTKILL(ag.cmnt) ) { ag.setGROWDD( ZERO ); }
  ag.setFROSTFLAG( 0 );

  veg.setRPREC( veg.getRPREC()*avgfac+atms.getPREC()*(1.0-avgfac));
  veg.setRNPP( veg.getRNPP()*avgfac+veg.getNPP()*(1.0-avgfac));
  if( getTimestep45().isYEAREND( pdm ) || isCutStep( pdm ) )
  {
//  cout << "mxeet = " << mxeet << endl;
  soil.setREET( mxeet*avgfac+mxeet*(1.0-avgfac));
//  soil.setREET( soil.getREET()*avgfac+y[I_EET]*(1.0-avgfac));
  }
  microbe.setRRH( microbe.getRRH()*avgfac+y[I_RH]*(1.0-avgfac));
  veg.setRLTRC( veg.getRLTRC()*avgfac+(y[I_LTRLC]+ y[I_LTRSC] + y[I_LTRHC] + y[I_LTRRC] + y[I_LTRSEEDC])*(1.0-avgfac));
    
  veg.setRGPP( veg.getRGPP()*avgfac+y[I_GPP]*(1.0-avgfac));
  veg.setRINGPP( veg.getRINGPP()*avgfac+y[I_INGPP]*(1.0-avgfac));
  veg.setRTAIR( veg.getRTAIR()*avgfac+atms.getTAIRD()*(1.0-avgfac));
  veg.setRTAIRPHI( veg.getRTAIRPHI()*avgfac+atms.getTAIRD()* veg.getPHI() *(1.0-avgfac));
  veg.setRPHI( veg.getRPHI()*avgfac+veg.getPHI()*(1.0-avgfac));
    
  veg.setRLABILEC( veg.getRLABILEC()*avgfac+y[I_LABILEC]*(1.0-avgfac));
  veg.setRLABILEN( veg.getRLABILEN()*avgfac+y[I_LABILEN]*(1.0-avgfac));
    
  mdemandc = y[I_ALLOCLC] + y[I_ALLOCSC] + y[I_ALLOCRC] + y[I_ALLOCSEEDC] + y[I_RVGRW];
  mdemandn = y[I_ALLOCLN] + y[I_ALLOCSN] + y[I_ALLOCRN] + y[I_ALLOCSEEDN]
              -y[I_NRESORBL] -y[I_NRESORBS] -y[I_NRESORBR] -y[I_NRESORBSEED];
//  if(pdyr > 800) {cout << "mdemandn = " << mdemandn << " " << y[I_ALLOCLN] << " " << y[I_NRESORBL] << endl;}  
  veg.setRDEMANDC( veg.getRDEMANDC()*avgfac+mdemandc*(1.0-avgfac));
  veg.setRDEMANDN( veg.getRDEMANDN()*avgfac+mdemandn*(1.0-avgfac));

  // Update atms.prevco2 for next month

  atms.setPREVCO2( atms.getCO2() );

  // Update atms.prevtair and atms.prev2tair for next month

  atms.setPREV2TAIR( atms.getPREVTAIR() );
  atms.setPREVTAIR( atms.getTAIR() );

  // Update previous snowpack for next month

  soil.setPREVSPACK( soil.getSNOWPACK() );

  // Update ag.prevPROD1, ag.prevPROD10 and ag.prevPROD100
  // for next month

  ag.setPREVPROD1C( ag.getPROD1C() );
  ag.setPREVPROD1N( ag.getPROD1N() );

  ag.setPREVPROD10C( ag.getPROD10C() );
  ag.setPREVPROD10N( ag.getPROD10N() );

  ag.setPREVPROD100C( ag.getPROD100C() );
  ag.setPREVPROD100N( ag.getPROD100N() );

  // Update ag.prevCropResidue for next month

  ag.setPREVCROPRESIDUEC( ag.getCROPRESIDUEC() );
  ag.setPREVCROPRESIDUEN( ag.getCROPRESIDUEN() );

  //  Update maximum EET, maximum PET, GPP optimum temperature
  //    (veg.topt), and maximum leaf cover (veg.prvleafmx) for
  //    the current year

  if( 0 == pdm )
  {
    veg.setNEWTOPT( atms.getTAIRD() );
  }
  else
  {
    if( 0 == ag.state )
    {
      veg.resetNEWTOPT( veg.cmnt,
                        atms.getTAIRD(),
                        veg.getRTAIRPHI(),
                        veg.getRPHI(),
			pdyr);
    }
    else
    {
      veg.resetNEWTOPT( ag.cmnt,
                        atms.getTAIRD(),
                        veg.getRTAIRPHI(),
                        veg.getRPHI(),
			pdyr);
    }
  }

  // Save state of all the ODE state variables
  //   representing pools to allow checking
  //   of mass balance

  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" entering setPrevState() ");
    refresh();
  #endif
  setPrevState();





// Update annual parameters for next year

  if( (CYCLE-1) == pdm )
  {
    veg.setTOPT( veg.getTOPT()*pow(avgfac,CYCLE)+veg.getNEWTOPT()*(1.0 - pow(avgfac,CYCLE)));
    veg.setTOPTMIC( veg.getTOPTMIC()*pow(avgfac,CYCLE)+veg.getNEWTOPT()*(1.0 - pow(avgfac,CYCLE)));
//    cout << "newTOPT = "  << veg.getTOPT() << " " << pow(avgfac,CYCLE) << " " << veg.getNEWTOPT() << endl;
    // annual number, so use avgfac^12 instead of just avgfac

    // Update optimum temperature parameters for GPP

    if( 0 == ag.state )
    {
      veg.boundTOPT( veg.cmnt );

    // Update adaptive parameters

      ag.setNATTOPT( veg.getTOPT() );

      // Determine vegetation C/N parameter as a function
      // of vegetation type, annual PET, annual EET,
      // CO2 concentration


    }
    else
    {
      veg.boundTOPT( ag.cmnt );

      ag.setCROPTOPT( veg.getTOPT() );

     // Determine vegetation C/N parameter as a function of
     //   vegetation type, annual PET, annual EET, CO2
     //   concentration


    }

    // Update next year ag.prvstate with current year ag.state

    ag.prvstate = ag.state;

    if( veg.yrstructn != ZERO )
    {
      veg.yrc2n  = veg.yrcarbon / veg.yrstructn;
    }

    if( soil.yrorgn != ZERO )
    {
      soil.yrc2n = soil.yrorgc / soil.yrorgn;
    }
//    if( 1 == baseline || microbe.getDOCFR( veg.cmnt ) == 0 )
    if( 1 == baseline )
    {
      soil.yrnin = ZERO;
      soil.yrnlost = ZERO;

      if( (soil.yrorgc/microbe.getCNSOIL( veg.cmnt ) > soil.yrorgn)
          &&  (y[I_SOLC]/microbe.getCNSOIL( veg.cmnt ) > y[I_SOLN]) )
      {
        soil.yrnin = (soil.yrorgc / microbe.getCNSOIL( veg.cmnt )) - soil.yrorgn;
        soil.yrnin += (y[I_SOLC]/microbe.getCNSOIL( veg.cmnt )) - y[I_SOLN];
        soil.yrnin /= 2.0;
      }
      else if( (soil.yrorgc/microbe.getCNSOIL( veg.cmnt ) < soil.yrorgn)
               &&  (y[I_SOLC]/microbe.getCNSOIL( veg.cmnt ) < y[I_SOLN]) ) 
      {
        soil.yrnlost = soil.yrorgn - (soil.yrorgc / microbe.getCNSOIL( veg.cmnt ));
        soil.yrnlost += y[I_SOLN] - (y[I_SOLC]/microbe.getCNSOIL( veg.cmnt ));
        soil.yrnlost /= 2.0;
      }

      y[I_SOLN] = y[I_SOLN] + soil.yrnin - soil.yrnlost;
    }

//
//  BSF NEW CODE FOR OPEN N EQUILIBRATION
//

/*    if( 0 == baseline )
    {
      ntot = soil.getSONINP()+veg.getVEGNINP()+atms.getNDEP()/(1000.0*CYCLE)+ag.fertn;
      sonp = soil.getSONINP()/ntot;
      availnp = (ag.fertn+atms.getNDEP()/(1000.0*CYCLE))/ntot;
      donpout = soil.getLCHDON()/(soil.getLCHDON() + soil.getLCHDIN());
      dinpout = soil.getLCHDIN()/(soil.getLCHDON() + soil.getLCHDIN());
//      availnp = (ag.fertn+veg.getVEGNINP()+atms.getNDEP()/(1000.0*CYCLE))/ntot;
      vegnp = veg.getVEGNINP()/ntot;
      if(soil.yrnin > soil.yrnlost)
      {
//        cout << "nin > nout " << soil.yrnin - soil.yrnlost << " " << sonp << " " << availnp << " " << vegnp << " " << ntot<< endl;
//        cout << "compare = " << soil.getSONINP() << " " << y[I_NFIXN] << " " << veg.getVEGNINP() << " " << y[I_NFIXS] << endl;
//      cout << "removing N = " << soil.yrnin << " " << soil.yrnlost << " " << sonp << " " << availnp << " " << vegnp << endl;
        y[I_SOLN] = y[I_SOLN] - (soil.yrnin - soil.yrnlost)*sonp;
        y[I_AVLN] = y[I_AVLN] - (soil.yrnin - soil.yrnlost)*availnp;
        y[I_LABILEN] = y[I_LABILEN] - (soil.yrnin - soil.yrnlost)*vegnp;
        veg.setVEGN( (veg.getSTRN() + y[I_LABILEN]) );
//        soil.yrnlost = soil.yrnlost + (soil.yrnin - soil.yrnlost);
      }
      if(soil.yrnlost > soil.yrnin)
      {
//       cout << "adding N = " << soil.yrnin << " " << soil.yrnlost << endl;
        y[I_DON] = y[I_DON] + (soil.yrnlost - soil.yrnin)*donpout;
        y[I_AVLN] = y[I_AVLN] + (soil.yrnlost - soil.yrnin)*dinpout;
//       soil.yrnin = soil.yrnin + (soil.yrnlost - soil.yrnin);
      } 
    } */      
         

/*    if( 0 == baseline )
    {
      ntot = soil.getSONINP()+veg.getVEGNINP()+atms.getNDEP()/(1000.0*CYCLE)+ag.fertn;
      sonp = soil.getSONINP()/ntot;
      availnp = (ag.fertn+atms.getNDEP()/(1000.0*CYCLE))/ntot;
      donpout = soil.getLCHDON()/(soil.getLCHDON() + soil.getLCHDIN());
      dinpout = soil.getLCHDIN()/(soil.getLCHDON() + soil.getLCHDIN());
      vegnp = veg.getVEGNINP()/ntot;
      if(soil.yrnin > soil.yrnlost)
      {
        y[I_DON] = y[I_DON] - (soil.yrnin - soil.yrnlost)*(sonp+vegnp);
        y[I_AVLN] = y[I_AVLN] - (soil.yrnin - soil.yrnlost)*availnp;
      }
      if(soil.yrnlost > soil.yrnin)
      {
        y[I_SOLN] = y[I_SOLN] + (soil.yrnlost - soil.yrnin)*sonp;
        y[I_AVLN] = y[I_AVLN] + (soil.yrnlost - soil.yrnin)*availnp;
        y[I_LABILEN] = y[I_LABILEN] + (soil.yrnlost - soil.yrnin)*vegnp;
        veg.setVEGN( (veg.getSTRN() + y[I_LABILEN]) );
      }
    } */


      if ( endeq > 0 )
    {
      ++endeq;
    }
  }
  
  #ifdef DEBUG_CTEM
    move(DEBUG_ROW,1);
    printw(" at end of stepmonth, month %2d ", pdm);
    refresh();
  #endif
  
  return endeq;

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

int Ttem45::testEquilibrium( const int& pdyr,
                             const int& nyears,
			     const double& avlneq,
                             const double& vegceq, 
                             const double& soilceq,
                             const double& vegneq,
                             const double& soilneq,
	                     const double& smoisteq)
{

//cout << "testEquilibrium " << pdyr <<  " " << rheqflag << " " << pdyr << " " << nyears+strteq << endl;
 if (pdyr >= nyears+strteq)
 {
  if( 0 == nfeed && 0 == rheqflag
      && wtol >= fabs( atms.yrrain + atms.yrsnowfall
      - soil.yreet - soil.yrrrun - soil.yrsrun )
      && (ctol >= fabs( veg.yrnpp - veg.yrltrc )) )
  {
    return 1;
  }

  if( 0 == nfeed && 1 == rheqflag
//      && wtol >= fabs( atms.yrrain
//                       + atms.yrsnowfall
//                       - soil.yreet
//                       - soil.yrrrun
//                       - soil.yrsrun )

        && wtol >= fabs(y[I_SM]-smoisteq)

//      && (ctol >= fabs( yrnep ))
//      && (ctol >= fabs( veg.yrnpp - veg.yrltrc ))
//      && (ctol >= fabs( veg.yrltrc - microbe.yrrh )) )
        && (ctol >= fabs((veg.getVEGC()+y[I_SOLC])-
                    (vegceq+soilceq)))
        && (ctol >= fabs((veg.getVEGC()-vegceq)))
        && (ctol >= fabs((y[I_SOLC]-soilceq))) )
  {
    return 1;
  }

//cout << "equilibrium = " << fabs( soil.yrnin - soil.yrnlost ) << " " << (veg.getVEGN()+y[I_SOLN])-(vegneq+soilneq) << " " << (veg.getVEGN()-vegneq) << " " << (y[I_SOLN]-soilneq) << endl;
//cout << "equil2 = " << veg.getVEGC() << " " << vegceq  << endl;
if(ag.state == 0)
{
//cout << "vegeq = " << veg.getVEGC() << " " << vegceq << " " << ctol << " " << veg.getVEGC()-vegceq << endl;
//cout << "soleq = " << y[I_SOLC] << " " << soilceq << " " << ctol << " " << y[I_SOLC]-soilceq << endl;
//cout << "diag = " << y[I_SM] << " " << smoisteq << " " <<  soil.yrnin - soil.yrnlost << " " << (veg.getVEGN()+y[I_SOLN]) - (vegneq+soilneq) << " " << (veg.getVEGC()+y[I_SOLC])-(vegceq+soilceq) <<  endl;
  if( 1 == nfeed && 1 == rheqflag
//      && wtol >= fabs( atms.yrrain
//                       + atms.yrsnowfall
//                       - soil.yreet
//                       - soil.yrrrun
//                       - soil.yrsrun )
//        && wtol >= fabs(y[I_SM]-smoisteq)

//       && wtol >= fabs( atms.yrrain + atms.yrsnowfall
//      - soil.yreet - soil.yrrrun - soil.yrsrun )
//      && (ntol >= fabs( soil.yrnin - soil.yrnlost ))
//      && (ntol >= fabs( veg.yrnup - veg.yrltrn ))
//      && (ntol >= fabs( veg.yrnup - microbe.yrnmin ))
//      && (ntol >= fabs( veg.yrltrn - microbe.yrnmin ))

//      && (ctol >= fabs( yrnep ))
//      && (ctol >= fabs( veg.yrnpp - veg.yrltrc ))
//      && (ctol >= fabs( veg.yrltrc - microbe.yrrh )) )


/*        && wtol >= fabs(y[I_SM]-smoisteq)
        && (ntol >= fabs( soil.yrnin - soil.yrnlost ))
        && (ntol >= fabs((veg.getVEGN()+y[I_SOLN])-
                    (vegneq+soilneq)))
        && (ntol >= fabs((veg.getVEGN()-vegneq)))
        && (ntol >= fabs((y[I_SOLN]-soilneq)))

        && (ctol >= fabs((veg.getVEGC()+y[I_SOLC])-
                    (vegceq+soilceq)))
        && (ctol >= fabs((veg.getVEGC()-vegceq)))
        && (ctol >= fabs((y[I_SOLC]-soilceq))) ) */

        && wtol >= fabs(soil.yrsmoist-smoisteq)
//        && (ntol >= fabs( soil.yrnin - soil.yrnlost ))
        && (ntol >= fabs(soil.yravln - avlneq ))
        && (ntol >= fabs((veg.yrnitrogen+soil.yrorgn)-
                    (vegneq+soilneq)))
        && (ntol >= fabs((veg.yrnitrogen-vegneq)))
        && (ntol >= fabs((soil.yrorgn-soilneq)))

        && (ctol >= fabs((veg.yrcarbon+soil.yrorgc)-
                    (vegceq+soilceq)))
        && (ctol >= fabs((veg.yrcarbon-vegceq)))
        && (ctol >= fabs((soil.yrorgc-soilceq))) )
  {
      return 1;
  }
}
else if (ag.state == 1 || ag.state == 2 || ag.state == 3 || ag.state == 4 || ag.state == 5 || ag.state == 6)
{
//          cout << "diag = " << (veg.getVEGN()+y[I_SOLN]) - ( vegneq+soilneq ) << " " << veg.getVEGN()-vegneq << " " << y[I_SOLN]-soilneq << " " << (veg.getVEGC()+y[I_SOLC]) - (vegceq+soilceq ) << " " << veg.getVEGC()-vegceq << " " << y[I_SOLC]-soilceq << " " << atms.yrrain + atms.yrsnowfall - soil.yreet -  soil.yrrrun - soil.yrsrun + ag.yrirrig << " " << fabs(yrnep - ag.yrformPROD1C) << endl;
  if( 1 == nfeed && 1 == rheqflag
//      && wtol >= fabs( atms.yrrain
//                       + atms.yrsnowfall
//                       - soil.yreet
//                       - soil.yrrrun
//                       - soil.yrsrun
//	               + ag.yrirrig )

/*        && wtol >= fabs(y[I_SM]-smoisteq)

        && (ntol >= (veg.getVEGN()+y[I_SOLN])-
                    (vegneq+soilneq))
        && (ntol >= (veg.getVEGN()-vegneq))
        && (ntol >= (y[I_SOLN]-soilneq))

        && (ctol >= (veg.getVEGC()+y[I_SOLC])-
                    (vegceq+soilceq))
        && (ctol >= (veg.getVEGC()-vegceq))
        && (ctol >= (y[I_SOLC]-soilceq))) */
//        && ( fabs(yrnep - ag.yrformPROD1C) <= 2.0 )) 
 
        && wtol >= fabs(soil.yrsmoist-smoisteq)
        && (ntol >= fabs(soil.yravln - avlneq ))
        && (ntol >= fabs((veg.yrnitrogen+soil.yrorgn)-
                    (vegneq+soilneq)))
        && (ntol >= fabs((veg.yrnitrogen-vegneq)))
        && (ntol >= fabs((soil.yrorgn-soilneq)))

        && (ctol >= fabs((veg.yrcarbon+soil.yrorgc)-
                    (vegceq+soilceq)))
        && (ctol >= fabs((veg.yrcarbon-vegceq)))
        && (ctol >= fabs((soil.yrorgc-soilceq))) )
  {
      return 1;
  }
}
else
{
  if( 1 == nfeed && 1 == rheqflag
//      && wtol >= fabs( atms.yrrain
//                       + atms.yrsnowfall
//                       - soil.yreet
//                       - soil.yrrrun
//                       - soil.yrsrun )

/*        && wtol >= fabs(y[I_SM]-smoisteq)

        && (ntol >= (veg.getVEGN()+y[I_SOLN])-
                    (vegneq+soilneq))
        && (ntol >= (veg.getVEGN()-vegneq))
        && (ntol >= (y[I_SOLN]-soilneq))

        && (ctol >= (veg.getVEGC()+y[I_SOLC])-
                    (vegceq+soilceq))
        && (ctol >= (veg.getVEGC()-vegceq))
        && (ctol >= (y[I_SOLC]-soilceq)) ) */

        && wtol >= fabs(soil.yrsmoist-smoisteq)
        && (ntol >= fabs(soil.yravln - avlneq ))
        && (ntol >= fabs((veg.yrnitrogen+soil.yrorgn)-
                    (vegneq+soilneq)))
        && (ntol >= fabs((veg.yrnitrogen-vegneq)))
        && (ntol >= fabs((soil.yrorgn-soilneq)))

        && (ctol >= fabs((veg.yrcarbon+soil.yrorgc)-
                    (vegceq+soilceq)))
        && (ctol >= fabs((veg.yrcarbon-vegceq)))
        && (ctol >= fabs((soil.yrorgc-soilceq))) )
  {
      return 1;
  }
}

  }

  return 0;
  cout << "exiting it" << endl;

};


/* *************************************************************
************************************************************** */

/* *************************************************************
************************************************************** */
void Ttem45::updateVegBiomass( double pstate[] )
{
  // Update veg object's biomass variables, which are used in veg.updateDynamics
  veg.setLEAFC( pstate[I_LEAFC] );
  veg.setLEAFN( pstate[I_LEAFN] );

  veg.setSAPWOODC( pstate[I_SAPWOODC] );
  veg.setSAPWOODN( pstate[I_SAPWOODN] );

  veg.setHEARTWOODC( pstate[I_HEARTWOODC] );
  veg.setHEARTWOODN( pstate[I_HEARTWOODN] );

  veg.setROOTC( pstate[I_ROOTC] );
  veg.setROOTN( pstate[I_ROOTN] );

  veg.setSEEDC( pstate[I_SEEDC] );
  veg.setSEEDN( pstate[I_SEEDN] );

  veg.setLABILEC( pstate[I_LABILEC] );
  veg.setLABILEN( pstate[I_LABILEN] );

  veg.setSTRC( pstate[I_LEAFC] + pstate[I_SAPWOODC] + pstate[I_HEARTWOODC] + pstate[I_ROOTC] + pstate[I_SEEDC] );
  veg.setSTRN( pstate[I_LEAFN] + pstate[I_SAPWOODN] + pstate[I_HEARTWOODN] + pstate[I_ROOTN] + pstate[I_SEEDN] );

  veg.setVEGC( veg.getSTRC() + veg.getLABILEC() );
  veg.setVEGN( veg.getSTRN() + veg.getLABILEN() );



};
/* *************************************************************
************************************************************** */

/* *************************************************************
************************************************************** */

void Ttem45::updateYearSummary( const int& pdm )
{

  double favg = 1.0 - exp(-1.0 / ((double)CYCLE*tauavg));

  // Update sum of annual carbon storage in ecosystems
 
//  if(getTimestep45().isYEARSTART( pdm ))
//  {
//    soil.yrnin = 0.0;
//    soil.yrnlost = 0.0;
//  } 

  veg.yrcarbon  = veg.yrcarbon*(1.0 - favg) + veg.getVEGC()*favg;
  soil.yrorgc = soil.yrorgc*(1.0 - favg) + y[I_SOLC]*favg;
  yrtotalc = yrtotalc*(1.0 - favg) + totalc*favg;

  // Update sum of annual nitrogen storage in ecosystems

  veg.yrstructn = veg.yrstructn*(1.0 - favg) + veg.getSTRN()*favg;
  veg.yrstoren = veg.yrstoren*(1.0 - favg) + y[I_LABILEN]*favg;
  soil.yrorgn = soil.yrorgn*(1.0 - favg) + y[I_SOLN]*favg;
  soil.yravln = soil.yravln*(1.0 - favg) + y[I_AVLN]*favg;

  veg.yrnitrogen = veg.yrnitrogen*(1.0 - favg) + veg.getVEGN()*favg;

  // Update sum of annual water storage in ecosystems

  soil.yravlh2o = soil.yravlh2o*(1.0 - favg) + (y[I_SM] - soil.getWILTPT())*favg;
  soil.yrsmoist = soil.yrsmoist*(1.0 - favg) + y[I_SM]*favg;
  soil.yrvsm = soil.yrvsm*(1.0 - favg) + y[I_VSM]*favg;
  soil.yrpctp = soil.yrpctp*(1.0 - favg) + y[I_PCTP]*favg;
  soil.yrsnowpack = soil.yrsnowpack*(1.0 - favg) + soil.getSNOWPACK()*favg;
  soil.yrrgrndh2o = soil.yrrgrndh2o*(1.0 - favg) + y[I_RGRW]*favg;
  soil.yrsgrndh2o = soil.yrsgrndh2o*(1.0 - favg) + y[I_SGRW]*favg;

  // Update sum of annual phenology in natural ecosystems

  veg.yrfpc = veg.yrfpc*(1.0 - favg) + y[I_FPC]*favg;

//  Penman variables

  veg.yrgc = veg.yrgc*(1.0 - favg) + y[I_GC]*favg;
  veg.yrgs = veg.yrgs*(1.0 - favg) + y[I_GS]*favg;

  // Update sum of annual carbon fluxes in ecosystems

  veg.yringpp = veg.yringpp*(1.0 - favg) + (double)CYCLE*y[I_INGPP]*favg;
  veg.yrgpp   = veg.yrgpp*(1.0 - favg) + (double)CYCLE*y[I_GPP]*favg;
  veg.yrinnpp = veg.yrinnpp*(1.0 - favg) + (double)CYCLE*veg.getINNPP()*favg;
  veg.yrnpp   = veg.yrnpp*(1.0 - favg) + (double)CYCLE*veg.getNPP()*favg;
  veg.yrgpr   = veg.yrgpr*(1.0 - favg) + (double)CYCLE*veg.getGPR()*favg;
  veg.yrrmaint = veg.yrrmaint*(1.0 - favg) + (double)CYCLE*(y[I_RMLEAF] + y[I_RMSAPWOOD] + y[I_RMROOT] + y[I_RMSEED])*favg;
  veg.yrrmleaf = veg.yrrmleaf*(1.0 - favg) + (double)CYCLE*y[I_RMLEAF]*favg;
  veg.yrrmsapwood = veg.yrrmsapwood*(1.0 - favg) + (double)CYCLE*y[I_RMSAPWOOD]*favg;
  veg.yrrmroot = veg.yrrmroot*(1.0 - favg) + (double)CYCLE*y[I_RMROOT]*favg;
  veg.yrrgrowth = veg.yrrgrowth*(1.0 - favg) + (double)CYCLE*y[I_RVGRW]*favg;


  veg.yrltrc  = veg.yrltrc*(1.0 - favg) + (double)CYCLE*(y[I_LTRLC] + y[I_LTRSC] + y[I_LTRHC] + y[I_LTRRC] + y[I_LTRSEEDC])*favg;
  microbe.yrrh = microbe.yrrh*(1.0 - favg) + (double)CYCLE*y[I_RH]*favg;

  veg.yralloclc = veg.yralloclc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCLC]*favg;
  veg.yrallocsc = veg.yrallocsc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCSC]*favg;
  veg.yrallocrc = veg.yrallocrc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCRC]*favg;
  
  veg.yrallocilc = veg.yrallocilc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCILC]*favg;
  veg.yrallocisc = veg.yrallocisc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCISC]*favg;
  veg.yrallocirc = veg.yrallocirc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCIRC]*favg;
  veg.yrallociseedc = veg.yrallociseedc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCISEEDC]*favg;
  
  veg.yrallocseedc = veg.yrallocseedc*(1.0 - favg) + (double)CYCLE*y[I_ALLOCSEEDC]*favg;
  veg.yrallocseedn = veg.yrallocseedn*(1.0 - favg) + (double)CYCLE*y[I_ALLOCSEEDN]*favg;

  yrnep = yrnep*(1.0 - favg) + (double)CYCLE*nep*favg;
  yrnce = yrnce*(1.0 - favg) + (double)CYCLE*nce*favg;
  yrnbp = yrnbp*(1.0 - favg) + (double)CYCLE*nbp*favg;


 // Update sum of annual nitrogen fluxes in ecosystems

//  soil.yrnin = soil.yrnin*(1.0 - favg) + (double)CYCLE*(y[I_NINP] + y[I_NFIXN] + y[I_NFIXS])*favg;
  soil.yrnin = soil.yrnin*(1.0 - favg) + (double)CYCLE*soil.getNINPUT()*favg;
//  soil.yrnin = soil.yrnin + soil.getNINPUT();

  ag.yrfertn = ag.yrfertn*(1.0 - favg) + (double)CYCLE*y[I_AGFRTN]*favg;

  veg.yrinnup = veg.yrinnup*(1.0 - favg) + (double)CYCLE*y[I_INNUP]*favg;
  veg.yrnup   = veg.yrnup*(1.0 - favg) + (double)CYCLE*y[I_VNUP]*favg;
  veg.yrnrsorb = veg.yrnrsorb*(1.0 - favg) + (double)CYCLE*(y[I_NRESORBL] + y[I_NRESORBS] + y[I_NRESORBR] + y[I_NRESORBSEED])*favg;

  veg.yrltrn = veg.yrltrn*(1.0 - favg) + (double)CYCLE*(y[I_LTRLN] + y[I_LTRSN] + y[I_LTRHN] + y[I_LTRRN] + y[I_LTRSEEDN])*favg;

  microbe.yrnuptake = microbe.yrnuptake*(1.0 - favg) + (double)CYCLE*y[I_MNUP]*favg;
  microbe.yrnmin = microbe.yrnmin*(1.0 - favg)  + (double)CYCLE*y[I_NMIN]*favg;

//  soil.yrnlost = soil.yrnlost*(1.0 - favg) + (double)CYCLE*(y[I_NLST])*favg;
  soil.yrnlost = soil.yrnlost*(1.0 - favg) + (double)CYCLE*(soil.getNLOST())*favg;
//  soil.yrnlost = soil.yrnlost + soil.getNLOST();
  soil.yrlchdin = soil.yrlchdin*(1.0 - favg) + (double)CYCLE*(soil.getLCHDIN())*favg;

//cout << "nvals = " << soil.yrnin << " " << soil.yrnlost << " " << soil.getNINPUT() << " " << soil.getNLOST() << endl;

   // Update sum of annual water fluxes in ecosystems

  ag.yrirrig = ag.yrirrig*(1.0 - favg) + (double)CYCLE*y[I_AGIRRIG]*favg;
  soil.yrineet = soil.yrineet*(1.0 - favg) + (double)CYCLE*y[I_INEET]*favg;
  soil.yreet = soil.yreet*(1.0 - favg) + (double)CYCLE*y[I_EET]*favg;
  soil.yrrperc = soil.yrrperc*(1.0 - favg) + (double)CYCLE*y[I_RPERC]*favg;
  soil.yrsperc = soil.yrsperc*(1.0 - favg) + (double)CYCLE*y[I_SPERC]*favg;
  soil.yrrrun = soil.yrrrun*(1.0 - favg) + (double)CYCLE*y[I_RRUN]*favg;
  soil.yrsrun = soil.yrsrun*(1.0 - favg) + (double)CYCLE*y[I_SRUN]*favg;

  atms.yrrain = atms.yrrain*(1.0 - favg) + (double)CYCLE*atms.getRAIN()*favg;
  atms.yrsnowfall = atms.yrsnowfall*(1.0 - favg) + (double)CYCLE*atms.getSNOWFALL()*favg;
//  atms.yrpet += atms.getPET();
  atms.yrpet = atms.yrpet*(1.0 - favg) + (double)CYCLE*veg.getPET()*favg;
  soil.yrsnowinf = soil.yrsnowinf*(1.0 - favg) + (double)CYCLE*soil.getSNOWINF()*favg;
  soil.yrh2oyld = soil.yrh2oyld*(1.0 - favg) + (double)CYCLE*soil.getH2OYLD()*favg;

  if( (atms.getTAIR() >= ag.getGDDMIN(ag.cmnt))&&(atms.getTAIRN() > ag.getTKILL(ag.cmnt)) )
  {
    ag.yrgrowdd = ag.yrgrowdd*(1.0 - favg) + (double)CYCLE*(atms.getTAIR() - ag.getGDDMIN(ag.cmnt))*atms.getNDAYS( pdm )*favg;
  }
  else
  {
    ag.yrgrowdd = ag.yrgrowdd*(1.0 - favg);
  }
  ag.yrfrost = ag.yrfrost*(1.0 - favg) + (double)CYCLE*ag.getFROSTFLAG()*favg;

  ag.yrstubC = ag.yrstubC*(1.0 - favg) + (double)CYCLE*ag.getSTUBBLEC()*favg;
  ag.yrstubN = ag.yrstubN*(1.0 - favg) + (double)CYCLE*ag.getSTUBBLEN()*favg;

 // Update sum of annual carbon and nitrogen fluxes from
 //   agricultural conversion

  ag.yrconvrtC = ag.yrconvrtC*(1.0 - favg) + (double)CYCLE*ag.getCONVRTFLXC()*favg;
  ag.yrvconvrtC = ag.yrvconvrtC*(1.0 - favg) + (double)CYCLE*ag.getVCONVRTFLXC()*favg;
  ag.yrsconvrtC = ag.yrsconvrtC*(1.0 - favg) + (double)CYCLE*ag.getSCONVRTFLXC()*favg;
  ag.yrslashC = ag.yrslashC*(1.0 - favg) + (double)CYCLE*ag.getSLASHC()*favg;
  ag.yrcflux = ag.yrcflux*(1.0 - favg) + (double)CYCLE*ag.getCFLUX()*favg;

  ag.yrconvrtN = ag.yrconvrtN*(1.0 - favg) + (double)CYCLE*ag.getCONVRTFLXN()*favg;
  ag.yrvconvrtN = ag.yrvconvrtN*(1.0 - favg) + (double)CYCLE*ag.getVCONVRTFLXN()*favg;
  ag.yrsconvrtN = ag.yrsconvrtN*(1.0 - favg) + (double)CYCLE*ag.getSCONVRTFLXN()*favg;
  ag.yrslashN = ag.yrslashN*(1.0 - favg) + (double)CYCLE*ag.getSLASHN()*favg;

  ag.yrnrent = ag.yrnrent*(1.0 - favg) + (double)CYCLE*ag.getNRETENT()*favg;
  ag.yrnvrent = ag.yrnvrent*(1.0 - favg) + (double)CYCLE*ag.getNVRETENT()*favg;
  ag.yrnsrent = ag.yrnsrent*(1.0 - favg) + (double)CYCLE*ag.getNSRETENT()*favg;

  ag.yrformResidueC = ag.yrformResidueC*(1.0 - favg) + (double)CYCLE*ag.getFORMCROPRESIDUEC()*favg;
  ag.yrformResidueN = ag.yrformResidueN*(1.0 - favg) + (double)CYCLE*ag.getFORMCROPRESIDUEN()*favg;

  ag.yrfluxResidueC = ag.yrfluxResidueC*(1.0 - favg) + (double)CYCLE*ag.getCROPRESIDUEFLXC()*favg;
  ag.yrfluxResidueN = ag.yrfluxResidueN*(1.0 - favg) + (double)CYCLE*ag.getCROPRESIDUEFLXN()*favg;


 // Update sum of annual carbon and nitrogen fluxes from
 //   products

  ag.yrformPROD1C = ag.yrformPROD1C*(1.0 - favg) + (double)CYCLE*ag.getCROPPRODC()*favg;
  ag.yrformPROD1N = ag.yrformPROD1N*(1.0 - favg) + (double)CYCLE*ag.getCROPPRODN()*favg;

  ag.yrdecayPROD1C = ag.yrdecayPROD1C*(1.0 - favg) + (double)CYCLE*ag.getPROD1DECAYC()*favg;
  ag.yrdecayPROD1N = ag.yrdecayPROD1N*(1.0 - favg) + (double)CYCLE*ag.getPROD1DECAYN()*favg;

  ag.yrformPROD10C = ag.yrformPROD10C*(1.0 - favg) + (double)CYCLE*ag.getFORMPROD10C()*favg;
  ag.yrformPROD10N = ag.yrformPROD10N*(1.0 - favg) + (double)CYCLE*ag.getFORMPROD10N()*favg;

  ag.yrdecayPROD10C = ag.yrdecayPROD10C*(1.0 - favg) + (double)CYCLE*ag.getPROD10DECAYC()*favg;
  ag.yrdecayPROD10N = ag.yrdecayPROD10N*(1.0 - favg) + (double)CYCLE*ag.getPROD10DECAYN()*favg;

  ag.yrformPROD100C = ag.yrformPROD100C*(1.0 - favg) + (double)CYCLE*ag.getFORMPROD100C()*favg;
  ag.yrformPROD100N = ag.yrformPROD100N*(1.0 - favg) + (double)CYCLE*ag.getFORMPROD100N()*favg;

  ag.yrdecayPROD100C = ag.yrdecayPROD100C*(1.0 - favg) + (double)CYCLE*ag.getPROD100DECAYC()*favg;
  ag.yrdecayPROD100N = ag.yrdecayPROD100N*(1.0 - favg) + (double)CYCLE*ag.getPROD100DECAYN()*favg;

  ag.yrformTOTPRODC = ag.yrformTOTPRODC*(1.0 - favg) + (double)CYCLE*ag.getFORMTOTPRODC()*favg;
  ag.yrformTOTPRODN = ag.yrformTOTPRODN*(1.0 - favg) + (double)CYCLE*ag.getFORMTOTPRODN()*favg;

  ag.yrdecayTOTPRODC = ag.yrdecayTOTPRODC*(1.0 - favg) + (double)CYCLE*ag.getTOTPRODDECAYC()*favg;
  ag.yrdecayTOTPRODN = ag.yrdecayTOTPRODN*(1.0 - favg) + (double)CYCLE*ag.getTOTPRODDECAYN()*favg;

};

/* *************************************************************
************************************************************* */

/* *************************************************************
************************************************************** */

void Ttem45::yearSummaryExtrapolate( void )
{
  double fxtra = 1.0/(1.0 - exp(-1.0));
  
  // Extrapolate values at year = tauavg mark based on 
  //   expectation of exponential relaxation from initial value of 0 to equilibrium value,
  //   with characteristic time tauavg 
  
  // Update sum of annual carbon storage in ecosystems

  veg.yrcarbon  *= fxtra;
  soil.yrorgc *= fxtra;
  yrtotalc *= fxtra;

  // Update sum of annual nitrogen storage in ecosystems

  veg.yrstructn *= fxtra;
  veg.yrstoren *= fxtra;
  soil.yrorgn *= fxtra;
  soil.yravln *= fxtra;

  veg.yrnitrogen *= fxtra;

  // Update sum of annual water storage in ecosystems

  soil.yravlh2o *= fxtra;
  soil.yrsmoist *= fxtra;
  soil.yrvsm *= fxtra;
  soil.yrpctp *= fxtra;
  soil.yrsnowpack *= fxtra;
  soil.yrrgrndh2o *= fxtra;
  soil.yrsgrndh2o *= fxtra;

//  Penman variables

  veg.yrgc *= fxtra;
  veg.yrgs *= fxtra;

  // Update sum of annual carbon fluxes in ecosystems

  veg.yringpp *= fxtra;
  veg.yrgpp   *= fxtra;
  veg.yrinnpp *= fxtra;
  veg.yrnpp   *= fxtra;
  veg.yrgpr   *= fxtra;
  veg.yrrmaint *= fxtra;
  veg.yrrmleaf *= fxtra;
  veg.yrrmsapwood *= fxtra;
  veg.yrrmroot *= fxtra;
  veg.yrrgrowth *= fxtra;


  veg.yrltrc  *= fxtra;
  microbe.yrrh *= fxtra;

  veg.yralloclc *= fxtra;
  veg.yrallocsc *= fxtra;
  veg.yrallocrc *= fxtra;
  
  veg.yrallocilc *= fxtra;
  veg.yrallocisc *= fxtra;
  veg.yrallocirc *= fxtra;
  
  veg.yrallocseedc *= fxtra;
  veg.yrallocseedn *= fxtra;

 // Update sum of annual nitrogen fluxes in ecosystems

  veg.yrinnup *= fxtra;
  veg.yrnup   *= fxtra;
  veg.yrnrsorb *= fxtra;

  veg.yrltrn *= fxtra;

  microbe.yrnuptake *= fxtra;
  microbe.yrnmin *= fxtra;

   // Update sum of annual water fluxes in ecosystems

  ag.yrirrig *= fxtra;
  soil.yrineet *= fxtra;
  soil.yreet *= fxtra;
  soil.yrrperc *= fxtra;
  soil.yrsperc *= fxtra;
  soil.yrrrun *= fxtra;
  soil.yrsrun *= fxtra;

  atms.yrrain *= fxtra;
  atms.yrsnowfall *= fxtra;
  atms.yrpet *= fxtra;
  soil.yrsnowinf *= fxtra;
  soil.yrh2oyld *= fxtra;
  
  ag.yrgrowdd *= fxtra;
  ag.yrfrost *= fxtra;

  ag.yrstubC *= fxtra;
  ag.yrstubN *= fxtra;
  
  ag.yrformPROD1C *= fxtra;
  ag.yrformPROD1N *= fxtra;

};

/* *************************************************************
************************************************************* */
