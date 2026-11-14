/* *************************************************************
XTEM45.CPP - Extrapolation version of the Terrestrial Ecosystem
               Model, XTEM version 4.5
****************************************************************

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241110 Changed: File renamed from xtem45_disturb.cpp to xtem45.cpp.
20241109 Changed: Inputs are NetCDF, one file key per group with the varmap configuration.
20241109 Updated: Cells are shared between MPI tasks (mpi_layer.h); every task writes its own output files and
         the first task merges them.
20241109 Changed: The state is one NetCDF file; nitrogen fertilizer and wildfire inputs are opened when their
         switches are on.
20241109 Changed: The step of the run comes from run_dt and run_dt_units.
20241109 Changed: Cell order and the end of the data are taken from the files, so a run covers the cells in
         the order of the input.
20241109 Changed: Configuration values are read with tem.config.getInt, getDouble and getString, which are
         strict (see tem_yaml.h).
20241109 Fixed: The output-year filter is parenthesised so that its precedence is explicit; the result is
         unchanged.
20241109 Updated: The run length is run_length in run_length_units (days, months or years from 1 January of
         clmstartyr), so a run can stop after any number of steps; only the steps run are written. transtime
         and transsteps are derived from it.
20241109 Updated: The last step of a run that ends inside a
         year is passed to Ttem45 (setCutStep).
20241109 Changed: Annual means of climate use CYCLE steps and not 12; the capacity message no longer names
         MAXCYCLE.
20241109 Changed: veg_fire allowed for any run_dt (step-scaled ignition risk)
20241110 Changed: Arrays are sized from the input dimensions and the run.
20241111 Changed: veg_fire is the fire of GDSTEM-FIRE; lightning_file, population_file and gdp_file are its inputs.
20241110 Changed: One executable: competition key replaces xtem45_competition; run_dt and run_length in any
         unit (seconds); veg_fire on or off; setRunStart check.

*****************************************************************
************************************************************** */

#define ANSI_CPP
#define EXTRAPOLATE_TEM

//#define BORLAND_CPP

//#define DEBUGX


#include <algorithm>
#include "ttimestep.h"
#include<cstdio>

  using std::printf;
  using std::sprintf;

#include<iostream>

  using std::cout;
  using std::ios;
  using std::cerr;
  using std::endl;

#include<fstream>

  using std::ifstream;
  using std::ofstream;

#include<iomanip>

  using std::setiosflags;
  using std::setw;
  using std::setprecision;

#include<cstdlib>

  using std::exit;

#include<cmath>

  using std::exp;
  using std::sin;

#include<vector>

  using std::vector;

#include<cctype>

  using std::toupper;

#include<cstring>

#include<string>

  using std::string;

#include<sstream>

  using std::ostringstream;

#ifdef ANSI_CPP

  #include<ctime>

  using std::time_t;
  using std::ctime;

#endif


#ifdef BORLAND_CPP

  #include<time>

  using std::time_t;
  using std::ctime;

#endif

#include "tem_netcdf_io.h"
#include "netcdf_io.h"

#include "mpi_layer.h"
#include "tem_log.h"
#include <vector>
#include "temconsts_gds.hpp"   // Global constants
#include "tclmdat_gds.h"       // Clmdat45 class
#include "elmnt_gds.h"         // Elmnt45 Class
#include "atmdepdat_gds.h"
#include "tem_invars.h"
#include "telm_gds.h"          // Telm45 Class
#include "nfertdat_gds.h"                // Nfertdat class

void initializeCLMGridCell( void );
void initializeLCLUCGridCell( void );
void initializeTEMGridCell( void );
void initRun( void );
void updateTCLMGridCell( const int& pdyr );
void updateTLCLUCGridCell( const int& pdyr );
void updateTTEMGridCell( const int& pdyr,
                         std::ostream& flog1 );

std::ostream& flog1 = tem::log::root();

Elmnt45 elmnt;
Telm45 telmnt[MAXGRID];

// An optional file key that is set (not null, not empty)
[[maybe_unused]] static bool fileGiven( const char* key )
{
  if( !telmnt[0].tem.config.has( key ) ) { return false; }

  for( const std::string& p : telmnt[0].tem.config.getStringList( key ) )
  {
    if( !p.empty() ) { return true; }
  }

  return false;
}

static tem::nc::Input input( const char* label )
{
  auto spec = tem::nc::inputSpec( telmnt[0].tem.config, label );
  flog1 << label << ": " << spec.path << ':' << spec.variables << endl;
  return spec;
}

static tem::nc::Stream* openInput( const char* label )
{
  return tem::nc::open( input( label ), label );
}

int equil;
int RTIME;

long mxnumgrid;

//int spinflag;
int spinoutfg;
int spinoutyrs;
int numspin;
int spintime;
int totsptime;
int transtime;
int transsteps;   // steps of the last transient year that are run (1 to CYCLE), derived from run_length

int temflag;
int istateflag;
int istateyear;
int ostateflag;
int ostateyear;

vector<string> clmpredmap;
vector<string> tempredmap;

int fatalerr;
int end1;
int icount;
int glob_count;

std::vector<Clmdat45> girrdat;
std::vector<Clmdat45> cldsdat;
std::vector<Clmdat45> nirrdat;
std::vector<Clmdat45> pardat;
std::vector<Clmdat45> tairdat;
std::vector<Clmdat45> precdat;
std::vector<Clmdat45> trangedat;
std::vector<Clmdat45> vprdat;
std::vector<Clmdat45> ws10dat;

std::vector<double> co2dat;
std::vector<Clmdat45> o3dat;

std::vector<MaxCohortdat45> mxcohrtdat;
std::vector<std::vector<Lulcdat45>> lulcdat;
std::vector<Atmdepdat> ndepdat;
std::vector<Nfertdat> nfertcropdat;
std::vector<Nfertdat> nfertpasturedat;


tem::nc::Stream* ifgirr;
tem::nc::Stream* ifnirr;
tem::nc::Stream* ifpar;
tem::nc::Stream* ifclds;
tem::nc::Stream* iftair;
tem::nc::Stream* ifprec;
tem::nc::Stream* iftrange;
tem::nc::Stream* ifvpr;
tem::nc::Stream* ifws10;
std::vector<int> co2year;
std::vector<double> co2val;
tem::nc::Stream* ifo3;

tem::nc::Stream* ifnumchrts;
tem::nc::Stream* iflulc;
tem::nc::Stream* ifndep;
tem::nc::Stream* ifnfertcrop;
tem::nc::Stream* ifnfertpasture;
int nfertflag;
bool nfertused;
tem::nc::Stream* iffireflash;
tem::nc::Stream* iffirepop;
tem::nc::Stream* iffiregdp = nullptr;   // GDP per capita (gdp_file)
#if TEM_ENABLE_STORM
tem::nc::Stream* ifstorm;
#endif
int vegfire;
tem::nc::Stream* iftexture;
tem::nc::Stream* ifelev;

tem::state::Reader ifstate; // TEM state (NetCDF) to start from
tem::state::Writer ofstate; // TEM state (NetCDF) to save

int assignCO2 = 0;


// File name for this process: the name itself in a serial run, name.NNNN.ext under MPI
string rankname( const string& name )
{
  if( tem::mpi::size() < 2 ) { return name; }

  return tem::mpi::rankPath( name, tem::mpi::rank() );
}

/* *************************************************************
**********************START MAIN PROGRAM************************
************************************************************* */

int main( int argc, char** argv )
{
  if( argc > 1 ) { telmnt[0].tem.configfile = argv[1]; } // run configuration, default tem_config.yaml

  tem::mpi::init();
  // With two or more ranks rank 0 runs no cells: it reads the inputs, checks them, writes the log and the output file; ranks 1 to N-1 compute.
  // A run with one rank computes and writes.
  const bool ioRank = tem::mpi::size() > 1 && 0 == tem::mpi::rank();
  tem::nc::setPartition( tem::mpi::size() > 1 ? tem::mpi::rank() - 1 : 0, tem::mpi::size() > 1 ? tem::mpi::size() - 1 : 1 );

  int xdyr;
  int i;


  long grdcnt;

  /// Master input routine initRun reads in all relevant information for the specific run 
  /// from the YAML file named by the first argument (default tem_config.yaml)
   
  initRun();

  // Sizes that depend on the data and the run: years held per cell (spin-up, transient, equilibrium) and the
  // cohort dimension of the land-cover input.
  // The output files start with the first year that can be written: climate output from clmstartyr, TEM output from startyr, and the spin-up
  // years that spinoutfg asks for (spinoutyrs before the start)
  tem::nc::setOutputStart( std::min( telmnt[0].clm.startyr, telmnt[0].tem.startyr ) - ( spinoutfg > 0 ? spinoutyrs : 0 ) );
  NCOHORTS = tem::nc::cohortCount();
  NYEARS = std::max( RTIME, telmnt[0].tem.maxyears ) + 2;
  flog1 << " arrays: " << CYCLE << " steps per year, " << NYEARS << " years, " << NCOHORTS << " cohorts " << endl;

  for( int ig = 0; ig < MAXGRID; ++ig ) { telmnt[ig].allocate(); }

  girrdat.assign( NYEARS, Clmdat45() );
  cldsdat.assign( NYEARS, Clmdat45() );
  nirrdat.assign( NYEARS, Clmdat45() );
  pardat.assign( NYEARS, Clmdat45() );
  tairdat.assign( NYEARS, Clmdat45() );
  precdat.assign( NYEARS, Clmdat45() );
  trangedat.assign( NYEARS, Clmdat45() );
  vprdat.assign( NYEARS, Clmdat45() );
  ws10dat.assign( NYEARS, Clmdat45() );
  o3dat.assign( NYEARS, Clmdat45() );
  co2dat.assign( NYEARS + 1, 0.0 );
  mxcohrtdat.assign( NYEARS, MaxCohortdat45() );
  lulcdat.assign( NYEARS, std::vector<Lulcdat45>( NCOHORTS ) );
  ndepdat.assign( NYEARS, Atmdepdat() );
  nfertcropdat.assign( NYEARS, Nfertdat() );
  nfertpasturedat.assign( NYEARS, Nfertdat() );

  
  telmnt[0].lon = MISSING;
  telmnt[0].lat = MISSING;
  telmnt[0].tem.totyr = -99;

//  cout << endl;
//  flog1 << endl << endl;

  #ifdef DEBUGX
    cout << " tem initialized " << endl;
  #endif  

  elmnt.show( flog1,
              telmnt[0].lon,
              telmnt[0].lat,
              telmnt[0].tem.totyr,
              telmnt[0].tem.inittol,
              telmnt[0].tem.veg.getERRCNT() );


  // Extrapolate TEM across region

  grdcnt = 0;

  while( grdcnt < mxnumgrid && 0 == fatalerr && !( 1 == temflag && tem::nc::atEnd( iftair ) ) )   // Grid cell loop
  {

    // Load grid cell climate data into one node of CLM linked list

    for( xdyr = 0; xdyr < RTIME; ++xdyr )
    {

//  cout << "year = " << xdyr << endl;
//      xdyr = 0;
      updateTCLMGridCell( xdyr ); 
  // Copy TEMclm results to output variables

  if( 1 == telmnt[0].clm.predflag )
  {
    if ( (1 == spinoutfg
         && telmnt[0].year < telmnt[0].clm.startyr)
         || (2 == spinoutfg
         && telmnt[0].year >= (telmnt[0].clm.startyr-spinoutyrs))
         || (telmnt[0].year >= telmnt[0].clm.startyr) )
    {
      if( 1 == telmnt[0].clm.cldflag )
      {
        telmnt[0].carea = cldsdat[0].carea;
        telmnt[0].contnent = cldsdat[0].contnent;
      }
      else
      {
        telmnt[0].carea = nirrdat[0].carea;
        telmnt[0].contnent = nirrdat[0].contnent;
      }

      telmnt[0].atmswritepred( xdyr,
                               clmpredmap,
                               telmnt[0].natmspred );
    }
  }  

      // load all years of climate data into telm.climate[][][] variable
    }

    #ifdef DEBUGX
      cout << " after updateTCLMGridCell " << endl;
    #endif  

    // Determine number of land use/land cover cohorts in a
    //   grid cell and load land cover data into cohorts of
    //   LULC linked list

    updateTLCLUCGridCell( 0 );

    #ifdef DEBUGX
      cout << " after updateTLCLUCGridCell " << endl;
    #endif  

    // Initialize TEM to equilibrium conditions for all cohorts
    // using the baseline climate if starting from calibration data
    // (i.e. istateflag == 0) or read in initial conditions from
    // temstate file

    initializeTEMGridCell();
    #ifdef DEBUGX
      cout << " after initializeTEMGridCell " << endl;
    #endif  

    // Begin simulation of transient climate and terrestrial
    //   ecosystem response

    if( 0 == equil )
    {

      for( xdyr = 1; xdyr < RTIME; ++xdyr )
//      for( xdyr = 2; xdyr < RTIME; ++xdyr )
      {

//   BSF do not need to call updateTCLMGridCell again
//        updateTCLMGridCell( xdyr );
//  set year        
  telmnt[0].year = telmnt[0].clm.startyr
                  - totsptime
                  - 1
                   + xdyr;

        // Run land cover module or read in land cover data from file
        // to update land cover characteristics for grid cell during year "dyr"

        updateTLCLUCGridCell( xdyr );


        // Run TEM for grid cell during year "dyr"

        updateTTEMGridCell( xdyr, flog1 );

      }
    }

    elmnt.show( flog1,
                telmnt[0].lon,
                telmnt[0].lat,
                telmnt[0].tem.totyr,
                telmnt[0].tem.tol,
                telmnt[0].tem.veg.getERRCNT() );

    ++grdcnt;
  }


  tem::nc::outputFlush(); // before any barrier: rank 0 must be receiving while the others finish
  if( 0 == fatalerr )
  {
    if( ioRank ) { tem::log::progress( "I/O rank: output of " + std::to_string( tem::mpi::size() - 1 ) + " compute rank(s) written in " + tem::log::elapsed() ); }
    else { tem::log::progress( "finished " + std::to_string( elmnt.count - 1 ) + " cells in " + tem::log::elapsed() ); }
  }
  else
  {
    tem::log::fatal( "run stopped: a cell reported a fatal error (see the lines above in tem_<id>.log)" );
  }

  tem::nc::closeInputs();
  if( 1 == temflag )
  {
    if( istateflag != 0 ) { ifstate.close(); }
    if( ostateflag != 0 ) { ofstate.close(); }
  }

  

  tem::nc::outputClose();
  tem::mpi::barrier();
  if( 0 == tem::mpi::rank() )
  {
    flog1 << "all ranks finished, output written and closed; run time " << tem::log::elapsed() << endl;
    tem::log::progress( "all ranks finished, output written and closed; run time " + tem::log::elapsed() );
  }
  tem::mpi::finalize();

  tem::log::close();

  return 0;

};

/* *************************************************************
******************** End of Main *******************************
************************************************************* */


/* *************************************************************
************************************************************* */

void initializeCLMGridCell()
{
  // xnfertflag (int): fertilizer of crops and pasture is read from annual input data, as in GDSTEM: data
  //   without a time axis (1, long-term average) or transient data (2). There is no built-in series.
  // The data are read only when agfertflag is 1, because with agfertflag 0 no fertilizer is applied.

  nfertflag = telmnt[0].tem.config.getInt( "xnfertflag" );
  if( 1 != nfertflag && 2 != nfertflag )
  {
    cerr << "xnfertflag must be 1 (long-term average) or 2 (transient): nitrogen fertilizer is read from the "
         << "annual input files, as in GDSTEM, and there is no built-in series" << endl;
    exit( -1 );
  }
  nfertused = ( 1 == telmnt[0].tem.config.getInt( "agfertflag" ) );
  flog1 << " nitrogen fertilizer " << ( ( 1 == nfertflag ) ? "long-term average" : "transient" )
        << " input data " << ( nfertused ? "(used)" : "(not used: agfertflag is 0)" ) << endl;

  // veg_fire (int): wildfire of GDSTEM-FIRE, off (0) or on (1). Reads lightning_flashes (one record per run step),
  //   population_density and gdp from the NetCDF files lightning_file, population_file and gdp_file

  vegfire = telmnt[0].tem.config.getInt( "veg_fire" );
  if( 0 != vegfire && 1 != vegfire )
  {
    flog1 << "veg_fire must be 0 (no fire) or 1 (fire), not " << vegfire << endl;
    exit( -1 );
  }
#if !TEM_ENABLE_FIRE
  if( 1 == vegfire )
  {
    flog1 << "veg_fire is 1, but this executable was built without fire (make FIRE=0). Rebuild with FIRE=1 or set veg_fire to 0." << endl;
    cerr << "veg_fire is 1, but this executable was built without fire (make FIRE=0)." << endl;
    exit( -1 );
  }
#endif
  // The competition key was removed. Cohorts of a cell never interact in this model, so the order of the
  // cohort and step loops has no effect on the result. A value of 1 requested a competition that does not exist.
  if( telmnt[0].tem.config.has( "competition" ) )
  {
    if( 0 != telmnt[0].tem.config.getInt( "competition" ) )
    {
      flog1 << "The competition key has been removed because competition among cohorts was never implemented. "
            << "Delete the key from the configuration." << endl;
      cerr << "The competition key has been removed. Delete it from the configuration." << endl;
      exit( -1 );
    }
    flog1 << " warning: the competition key is ignored and will be an error in a later release" << endl;
  }
  flog1 << " wildfire " << ( ( 1 == vegfire ) ? "on" : "off" ) << endl;

  for( int i = 0; i < MAXGRID; ++i )
  {
    telmnt[i].tem.fire.enabled = vegfire;
  }

  tem::nc::configureOutput( telmnt[0].tem.config.getString( "output_file" ),
                            telmnt[0].tem.config.getInt( "output_new_file_freq" ),
                            telmnt[0].tem.config.getString( "output_new_file_freq_units" ),
                            telmnt[0].tem.config.getString( "region" ) );


  if( 0 == telmnt[0].clm.sradflag )
    ifgirr = openInput( "GIRR" );
  if( 1 == telmnt[0].clm.cldflag )
    ifclds = openInput( "CLDINESS" );
  else
    ifnirr = openInput( "NIRR" );
  if( 1 == telmnt[0].clm.parflag )
    ifpar = openInput( "PAR" );

  iftair = openInput( "TAIR" );
  ifprec = openInput( "PREC" );
  iftrange = openInput( "DTAIRRNG" );
  ifvpr = openInput( "VPRPRESS" );
  ifws10 = openInput( "WIND" );
  if( 1 == telmnt[0].clm.tco2flag )
    tem::nc::series( input( "CO2" ), co2year, co2val );
  ifo3 = openInput( "AOT40" );
  ifndep = openInput( "NDEP" );

  if( 1 == vegfire )
  {
    iffireflash = openInput( "LIGHTNING" );
    iffirepop = openInput( "POPDENS" );
    iffiregdp = openInput( "GDP" );
  }

  auto& tem = telmnt[0].tem;
  if( tem.config.has( "storm_disturbance" ) &&
      tem.config.getBool( "storm_disturbance" ) )
  {
#if TEM_ENABLE_STORM
    ifstorm = openInput( "STORM" );
#else
    std::cerr << "storm_disturbance requires a build with STORM=1" << endl;
    exit( -1 );
#endif
  }

  if( nfertused )
  {
    ifnfertcrop = openInput( "NFERTCROP" );
    ifnfertpasture = openInput( "NFERTPASTURE" );
  }
}

/* *************************************************************
************************************************************** */


/* *************************************************************
************************************************************* */

void initializeLCLUCGridCell( void )
{
  ifnumchrts = openInput( "MXCOHRTS" );
  iflulc = openInput( "LULCCHRT" );
  if( 1 == telmnt[0].lcluc.tlulcflag && !tem::nc::hasTime( iflulc ) )
  {
    cerr << endl << "tlulcflag is 1 (transient land cover), but the land-cover input has no time axis: it needs (time, cohort, lat, lon) or a ragged ncohort(time, lat, lon) "
         << "with one record per year, or set tlulcflag to 0" << endl;
    tem::mpi::abort( 1 );
  }
}

/* *************************************************************
************************************************************* */


/* **************************************************************
************************************************************** */

void initializeTEMGridCell( void )
{
  const int dyr = 0;
  int ichrt;

  // Set elevation and soil texture for TEM


  telmnt[0].setGIStopography( flog1, fatalerr, iftexture, ifelev );


/* *************************************************************
		BEGIN VEGETATION MOSAIC LOOP
************************************************************* */

  for( ichrt = 0; ichrt < telmnt[0].maxcohorts; ++ichrt )
  {
    if( istateflag > 0 )
    {
      // Read in initial TEM state determined in a previous
      //  TEM simulation to telmnt[0].cohort

      tem::state::In sin;
      if( !ifstate.get( telmnt[0].lon, telmnt[0].lat, ichrt + 1, sin ) )
      {
        flog1 << "the state file has no record for cell " << telmnt[0].lon << " " << telmnt[0].lat << " cohort " << ichrt + 1 << endl;
        exit( -1 );
      }
      telmnt[0].readCohortState( sin, ichrt );

      // Pass telmnt[0].cohort information to TEM

      telmnt[0].getTEMCohortState( ichrt );
    }
    else
    {
      telmnt[0].setTEMequilState( flog1,
                                  equil,
                                  totsptime,
                                  ichrt,
                                  tempredmap,
                                  spinoutfg );
//
//  BSF if intflag = 1, that means integration is solved ok
//
      if( telmnt[0].tem.intflag == 0 )
      {
        if( elmnt.count < elmnt.grdcnt )
        {
          cout << "Integration terminated before attaining ";
          cout << "tolerance level" << endl;
        }

        flog1 << "Integration terminated before attaining ";
        flog1 << "tolerance level" << endl;

      }
//  BSF already set to 0 within equilibrateTEM
//      if( telmnt[0].tem.intflag > 0 )
//      {
//        telmnt[0].tem.intflag = 0;
//      }

      // Write out telmnt[0].cohort to output file for
      //   potential use in a future TEM simulation

      if( 1 == ostateflag )
      {
        { tem::state::Out sout; telmnt[0].writeCohortState( sout, ichrt ); ofstate.put( telmnt[0].lon, telmnt[0].lat, ichrt + 1, sout ); }
      }

      // Write selected TEM variables from telmnt[0].output to

//      if( 1 == spinoutfg || 1 == equil )
      if( 1 == equil )
      {
        telmnt[0].temwritepred( tempredmap,
                                dyr,
                                ichrt,
                                telmnt[0].ntempred,
                                spinoutfg,
                                0 );
      }
    } // End of istateflag else
  } // End of cohort loop

//  elmnt.show( flog1,
//              telmnt[0].lon,
//              telmnt[0].lat,
//              telmnt[0].tem.totyr,
//              telmnt[0].tem.tol );

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void initRun( void )
{
  
  int i;

  time_t starttime;
  
  double initco2;
  double co2level;

  string ifilename;
  string cmntecd;


  
  temflag = 1; 

  starttime = time( NULL );
  
  // tem_<id>.out (progress) and tem_<id>.log (all other messages), shared by all ranks (tem_log.h)
  tem::log::open( "tem" );

  telmnt[0].tem.config.open( telmnt[0].tem.configfile );
  flog1 << "using " << telmnt[0].tem.configfile << " for information on this run " << endl;

  // NetCDF parameters (netcdf_io.h): the keys are optional and have the defaults of the settings
  {
    tem::netcdf_io::NetcdfSettings ns;
    auto& c = telmnt[0].tem.config;
    if( c.has( "ncchksz" ) ) { ns.ncchksz = static_cast<size_t>( c.getInt( "ncchksz" ) ); }
    if( c.has( "header_buffer_val" ) ) { ns.header_buffer_val = c.getInt( "header_buffer_val" ); }
    if( c.has( "deflate_level" ) ) { ns.deflate_level = c.getInt( "deflate_level" ); }
    if( c.has( "shuffle" ) ) { ns.shuffle = c.getBool( "shuffle" ); }
    if( c.has( "output_chunk_lonlat" ) ) { ns.output_chunk_lonlat = c.getInt( "output_chunk_lonlat" ); }
    if( c.has( "num_io_buffers" ) ) { ns.num_io_buffers = c.getInt( "num_io_buffers" ); }
    if( c.has( "io_buffer_mb" ) ) { ns.io_buffer_mb = static_cast<size_t>( c.getInt( "io_buffer_mb" ) ); }
    if( c.has( "netcdf_read_broadcast" ) ) { ns.netcdf_read_broadcast = c.getBool( "netcdf_read_broadcast" ); }
    if( c.has( "use_netcdf_mpi" ) ) { ns.use_netcdf_mpi = c.getBool( "use_netcdf_mpi" ); }
    if( c.has( "use_collective" ) ) { ns.use_collective = c.getBool( "use_collective" ); }
    if( ns.num_io_buffers == 0 || ns.num_io_buffers < -1 || ns.deflate_level < 0 || ns.deflate_level > 9 || ns.io_buffer_mb < 1 )
    {
      cerr << endl << "num_io_buffers must be at least 1 or -1, deflate_level 0 to 9 and io_buffer_mb at least 1" << endl;
      tem::mpi::abort( 1 );
    }
    tem::netcdf_io::netcdf_io_init( ns );
    flog1 << " NetCDF: num_io_buffers " << ns.num_io_buffers << " of " << ns.io_buffer_mb << " MiB, netcdf_read_broadcast "
          << ns.netcdf_read_broadcast << ", use_netcdf_mpi " << ns.use_netcdf_mpi << ", use_collective " << ns.use_collective
          << ", deflate_level " << ns.deflate_level << ", shuffle " << ns.shuffle << ", output_chunk_lonlat " << ns.output_chunk_lonlat << ", ncchksz " << ns.ncchksz
          << ", header_buffer_val " << ns.header_buffer_val << endl;
  }

/// Runmode and runtime information

  const int run_dt = telmnt[0].tem.config.getInt( "run_dt" );
  const string run_dt_units = telmnt[0].tem.config.getString( "run_dt_units" );

  // run_dt is converted to seconds (second, minute, hour, day, week, month or year); the year is 365 days
  if( !getTimestep45().setSTEP( run_dt, run_dt_units ) || getTimestep45().getNSTEPS() < 1 )
  {
    cerr << endl << "run_dt = " << run_dt << " " << run_dt_units << " is not a step of the 365-day year: "
         << "units are second, minute, hour, day, week, month or year, and the step must divide the year evenly" << endl;
    exit( -1 );
  }
  if( !getTimestep45().supportedStep().empty() )
  {
    cerr << endl << "run_dt = " << run_dt << " " << run_dt_units << " (" << getTimestep45().getSTEPSECONDS() << " s) is not supported: "
         << getTimestep45().supportedStep() << endl;
    exit( -1 );
  }

  CYCLE = getTimestep45().getNSTEPS();
  {
    std::vector<double> sdays( CYCLE );
    std::vector<int> smonths( CYCLE );

    for( int i = 0; i < CYCLE; ++i )
    {
      sdays[i] = getTimestep45().getDAYS( i );
      smonths[i] = getTimestep45().getMONTH( i );
    }

    tem::nc::setSteps( CYCLE, &sdays[0], &smonths[0] );
  }
  telmnt[0].clm.setNDAYS();
  telmnt[0].tem.atms.setNDAYS();
  telmnt[0].tem.veg.pen.setNDAYS();
  telmnt[0].tem.pen.setNDAYS();
  telmnt[0].tem.soil.setNDAYS();

  equil = telmnt[0].tem.config.getInt( "xequil" );
  flog1 << " running tem in " << ( ( equil == 0 ) ? "transient" : "equilibrium" ) << " mode " << endl;
  
  RTIME = 1;

  telmnt[0].de_startyear = telmnt[0].clm.startyr;
  telmnt[0].de_nyears = 1;

  if( 0 == equil )
  {
    telmnt[0].clm.startyr = telmnt[0].tem.config.getInt( "clmstartyr" );
    flog1 << " start transient run in " << telmnt[0].clm.startyr << endl;
    
    telmnt[0].spinflag = telmnt[0].tem.config.getInt( "spinflag" );
    flog1 << " initialize model using " << ((telmnt[0].spinflag == 0)?"equilibration to long-term average climate":((telmnt[0].spinflag==1)?"equilibration then spinup":"dynamic equilibration")) << endl;
    
    numspin = telmnt[0].tem.config.getInt( "numspin" );
    if (1 == telmnt[0].spinflag ) { flog1 << " model will be 'spun up' " << numspin << " times " << endl; }
    
    spintime = telmnt[0].tem.config.getInt( "spintime" );
    if (1 == telmnt[0].spinflag ) { flog1 << " each 'spin' will last " << spintime << " years " << endl; }
    
    telmnt[0].de_startyear = telmnt[0].tem.config.getInt( "de_startyear" );
    if (2 == telmnt[0].spinflag ) { flog1 << " startyear for dynamic equilibration: " << telmnt[0].de_startyear << endl; }
    
    telmnt[0].de_nyears = telmnt[0].tem.config.getInt( "de_nyears" );
    if (2 == telmnt[0].spinflag ) { flog1 << " number of years for dynamic equilibration loop: " << telmnt[0].de_nyears << endl; }
    
    const long runlen = telmnt[0].tem.config.getInt( "run_length" );
    const string rununits = telmnt[0].tem.config.getString( "run_length_units" );
    const long runsteps = getTimestep45().runSTEPS( runlen, rununits );
    if( runsteps < 1 )
    {
      cerr << "run_length = " << runlen << " " << rununits << " (" << getTimestep45().runSECONDS( runlen, rununits ) << " s) is not a whole number of "
           << run_dt << " " << run_dt_units << " steps of " << getTimestep45().getSTEPSECONDS() << " s (run_length_units: second, minute, hour, day, week, month or year, counted from 1 January of clmstartyr)" << endl;
      exit( EXIT_FAILURE );
    }
    // transtime: years touched by the run; transsteps: steps run in the last of them
    transtime = static_cast<int>( ( runsteps + CYCLE - 1 ) / CYCLE );
    transsteps = static_cast<int>( runsteps - static_cast<long>( transtime - 1 ) * CYCLE );
    flog1 << " model will run for " << runlen << " " << rununits << " = " << getTimestep45().runSECONDS( runlen, rununits ) << " s ("
          << runsteps << " steps of " << run_dt << " " << run_dt_units << ", " << getTimestep45().getSTEPSECONDS() << " s nominal) in transient mode following initialization" << endl;
    flog1 << " the last transient year runs " << transsteps << " of " << CYCLE << " steps" << endl;
    tem::nc::setTransientYears( transtime );
    tem::nc::setRunStart( telmnt[0].clm.startyr );
    tem::nc::setRunLength( runlen, rununits, runsteps );
    tem::nc::setRunEnd( telmnt[0].clm.startyr + transtime - 1, transsteps );
      
    telmnt[0].tem.startyr = telmnt[0].clm.startyr;
    totsptime = 0;

    if( 0 == telmnt[0].spinflag )
    {
      telmnt[0].de_startyear = telmnt[0].clm.startyr;
      telmnt[0].de_nyears = 1;
    }
    
    if( 1 == telmnt[0].spinflag )
    {
      telmnt[0].de_startyear = telmnt[0].clm.startyr;
      telmnt[0].de_nyears = 1;
      totsptime = spintime * numspin;
      RTIME += totsptime;
    }
    
    if( 2 == telmnt[0].spinflag )
    {
      numspin = 0;
      spintime = 0;
      totsptime = 0;
      RTIME += totsptime;
    }

    RTIME += transtime;
  }
  else
  {
    totsptime = RTIME;
    spintime = 1;
  }
  
/// number of grid cells and what GIS data to use 
  mxnumgrid = telmnt[0].tem.config.getInt( "mxnumgrid" );
  flog1 << " the model will be run for " << mxnumgrid << " grid cells " << endl;
  
  elmnt.grdcnt = telmnt[0].tem.config.getLong( "grdcnt" );
  flog1 << " the logfile will include a timestamp after each of the first " << elmnt.grdcnt << " grid cells " << endl;
  
  elmnt.strtflag = telmnt[0].tem.config.getInt( "strtflag" );
  flog1 << " the model will start " << ((elmnt.strtflag==1)?"at the beginning of":"partway through") << " the GIS files " << endl;
  
  elmnt.numskip = telmnt[0].tem.config.getLong( "numskip" );
  if( elmnt.strtflag == 0 ) { flog1 << elmnt.numskip  << " grid cells will be skipped " << endl; }
  
  elmnt.endflag = telmnt[0].tem.config.getInt( "endflag" );
  flog1 << " the model will end " << ((elmnt.endflag==1)?"at the end of":"partway through") << " the GIS files " << endl;
  
  elmnt.numgrids = telmnt[0].tem.config.getLong( "numgrids" );
  if( elmnt.endflag == 0 ) { flog1 << " the model will run until " << elmnt.numgrids << " grid cells have been run " << endl; }

/// Climate inputs  


  telmnt[0].clm.sradflag = telmnt[0].tem.config.getInt( "sradflag" );
  telmnt[0].clm.cldflag = telmnt[0].tem.config.getInt( "cldflag" );
  telmnt[0].clm.parflag = telmnt[0].tem.config.getInt( "parflag" );
  telmnt[0].clm.tcldsflag = telmnt[0].tem.config.getInt( "xtcldsflag" );
  
  flog1 << " the TEMclm model " << ((telmnt[0].clm.sradflag==1)?"will":"will not") << " be run for solar radiation variables " << endl;
  flog1 << ((telmnt[0].clm.tcldsflag==1)?"transient ":"static ") << ((telmnt[0].clm.cldflag==1)?"cloudiness":"surface radiation") << " data will be used" << endl;
  if( telmnt[0].clm.sradflag == 0 ) { flog1 << " spatially explicit PAR data " << ((telmnt[0].clm.parflag==1)?"will":"will not") << " be used " << endl; }
  
  telmnt[0].clm.ttairflag = telmnt[0].tem.config.getInt( "xttairflag" );
  
  

  telmnt[0].clm.tprecflag = telmnt[0].tem.config.getInt( "xtprecflag" );

  

  telmnt[0].clm.ttrangeflag = telmnt[0].tem.config.getInt( "xttrangeflag" );


  
  telmnt[0].clm.tvprflag = telmnt[0].tem.config.getInt( "xtvprflag" );
  


  initco2 = telmnt[0].tem.config.getDouble( "initco2" );
  flog1 << " initial co2 concentration (ppmv) : " << initco2 << endl;   
  telmnt[0].clm.setINITCO2( initco2 );

  co2level = telmnt[0].tem.config.getDouble( "xco2level" );
  flog1 << " final equilibrium co2 concentration (ppmv) : " << co2level << endl;
  telmnt[0].clm.setCO2LEVEL( co2level );
  
  telmnt[0].clm.tco2flag = telmnt[0].tem.config.getInt( "xtco2flag" );

    
  
  telmnt[0].clm.to3flag = telmnt[0].tem.config.getInt( "xto3flag" );

  
//
//  Input name of Ndep file
//
  telmnt[0].clm.tndepflag = telmnt[0].tem.config.getInt( "xtndepflag" );
  
  telmnt[0].clm.tws10flag = telmnt[0].tem.config.getInt( "xtws10flag" );

 
  telmnt[0].clm.predflag = telmnt[0].tem.config.getInt( "clmoutflag" );
  flog1 << " climate data " << ((telmnt[0].clm.predflag == 1)?"will":"will not") << " be output" << endl;
  
  clmpredmap = telmnt[0].tem.config.getStringList( "clmoutvars", false  );
  telmnt[0].natmspred = static_cast<int>( clmpredmap.size() );
  telmnt[0].totpred = telmnt[0].natmspred;
  for( const auto& variable : clmpredmap )
    flog1 << "      " << variable << " will be written to the NetCDF output" << endl;
  if( 1 == telmnt[0].clm.predflag ) { tem::nc::declareOutput( clmpredmap, false ); }

  initializeCLMGridCell();
  
/// Land use/land cover inputs

  telmnt[0].lcluc.startyr = telmnt[0].clm.startyr;

  cmntecd = telmnt[0].tem.config.getString( "cmntecd" );
  telmnt[0].lcluc.getvtype( cmntecd );
  flog1 << " the file describing vegetation mosaics is: " << cmntecd << endl;

  telmnt[0].lcluc.tlulcflag = telmnt[0].tem.config.getInt( "tlulcflag" );
  flog1 << " transient land use " << ((telmnt[0].lcluc.tlulcflag==1)?"will":"will not") << " be used " << endl;
  
  
  
  telmnt[0].lcluc.lastyr = telmnt[0].tem.config.getInt( "lulclastyr" );
  if( telmnt[0].lcluc.tlulcflag == 1 ) { flog1 << " the last year of transient cohort data is: " << telmnt[0].lcluc.lastyr << endl; }

  initializeLCLUCGridCell();

/// Vegetation inputs and output selection

  telmnt[0].ntempred = 0;

  if( 1 == temflag )
  {
    telmnt[0].tem.startyr = telmnt[0].clm.startyr;
    telmnt[0].tem.atms.setINITCO2( telmnt[0].clm.getINITCO2() );
    telmnt[0].tem.atms.setCO2LEVEL( telmnt[0].clm.getCO2LEVEL() );

    telmnt[0].tem.ag.tlulcflag = telmnt[0].lcluc.tlulcflag;

    
    //telmnt[0].tem.initrun( flog1 );
    
    telmnt[0].tem.avlnflag = 0; 
    telmnt[0].tem.nfeed = 0; 
    telmnt[0].tem.rheqflag = 0;
    
    telmnt[0].tem.avlnflag = telmnt[0].tem.config.getInt( "avlnflag" );
    flog1 << " " << ((telmnt[0].tem.avlnflag==1)?"allowing":"not allowing") << " available N to fluctuate " << endl;
  
    telmnt[0].tem.nfeed = telmnt[0].tem.config.getInt( "nfeed" );
    flog1 << " running TEM " << ((telmnt[0].tem.nfeed==1)?"with":"without") << " N feedback on GPP " << endl;
  
    telmnt[0].tem.baseline = telmnt[0].tem.config.getInt( "baseline" );
    flog1 << " " << ((telmnt[0].tem.baseline==1)?"solving":"not solving") << " for baseline soil nitrogen" << endl;
    telmnt[0].tem.initbase = telmnt[0].tem.baseline;
  
    telmnt[0].tem.moistlim = telmnt[0].tem.config.getInt( "moistlim" );
    flog1 << " moisture limitation is " << ((telmnt[0].tem.moistlim==1)?"on":"off") << endl;
  
    telmnt[0].tem.o3flag = telmnt[0].tem.config.getInt( "o3flag" );
    flog1 << " ozone damage to gpp is " << ((telmnt[0].tem.o3flag==1)?"on":"off") << endl;
  
    //telmnt[0].tem.equil = telmnt[0].tem.config.getInt( "equil" );
    //rflog1 << " equil = " << equil << endl;
  
    telmnt[0].tem.strteq = telmnt[0].tem.config.getInt( "equil_min_years" );
    flog1 << " model will check for equilibrium after running for " << telmnt[0].tem.strteq << " years " << endl;
  
    telmnt[0].tem.rheqflag = telmnt[0].tem.config.getInt( "equil_rh" );
    flog1 << " decomposition " << ((telmnt[0].tem.rheqflag==1)?"must":"is not required to") << " equilibrate " << endl;
  
    telmnt[0].tem.odesolver = telmnt[0].tem.config.getInt( "ode_solver" );
    flog1 << " ode solver ( 1 -> rkbs; 2 -> rkf ) " << telmnt[0].tem.odesolver << endl;

    telmnt[0].tem.rsoilmethod = telmnt[0].tem.config.getInt( "rsoil_method" );
    flog1 << " soil respiration method ( 1 -> tim; 2 -> sib ) " << telmnt[0].tem.rsoilmethod << endl;

    telmnt[0].tem.microbe.rhmoistmethod = telmnt[0].tem.config.getInt( "rhmoist_method" );
    flog1 << " rh moisture response ( 1 -> wfps; 2 -> dndc; 3 -> sech ) " << telmnt[0].tem.microbe.rhmoistmethod << endl;

    telmnt[0].tem.wtol = telmnt[0].tem.config.getDouble( "wtol" );
    flog1 << " absolute tolerance for the water cycle: " << telmnt[0].tem.wtol << endl;
  
    telmnt[0].tem.ctol = telmnt[0].tem.config.getDouble( "ctol" );
    flog1 << " absolute tolerance for the carbon cycle: " << telmnt[0].tem.ctol << endl;
  
    telmnt[0].tem.ntol = telmnt[0].tem.config.getDouble( "ntol" );
    flog1 << " absolute tolerance for the nitrogen cycle: " << telmnt[0].tem.ntol << endl;
    
    telmnt[0].tem.maxyears = telmnt[0].tem.config.getInt( "equil_max_years" );
    flog1 << " maximum number of years for the model to run before initialization is complete: " << telmnt[0].tem.maxyears << " years " << endl;
    telmnt[0].tem.runsize = telmnt[0].tem.maxyears;
  
    telmnt[0].tem.maxnrun = telmnt[0].tem.config.getInt( "equil_max_attempts" );
    flog1 << " maximum number of attempts to reach a solution: " << telmnt[0].tem.maxnrun << endl;
  
    telmnt[0].tem.tauavg = telmnt[0].tem.config.getDouble( "equil_tau_years" );
    flog1 << " averaging time for equilibration: " << telmnt[0].tem.tauavg << " years " << endl;
  
    telmnt[0].tem.veg.setDC2N( telmnt[0].tem.config.getDouble( "dc2n" ) );
    flog1 << " factor for changing relative c:n per change in co2 concentration: " << telmnt[0].tem.veg.getDC2N() << endl;
      
    telmnt[0].tem.startyr = telmnt[0].tem.config.getInt( "startyr" );
    flog1 << " first year for collection of output data: " << telmnt[0].tem.startyr << endl;
  
    telmnt[0].tem.endyr = telmnt[0].tem.config.getInt( "endyr" );
    flog1 << " last year for collection of output data: " << telmnt[0].tem.endyr << endl;
  
    telmnt[0].tem.diffyr = telmnt[0].tem.config.getInt( "diffyr" );
    flog1 << " time between years for output data (1 = annual): " << telmnt[0].tem.diffyr << endl;


    telmnt[0].tem.inittol = telmnt[0].tem.config.getDouble( "inittol" );
    flog1 << " proportional tolerance of the integrator: " << telmnt[0].tem.inittol << endl;
  
    telmnt[0].tem.maxit = telmnt[0].tem.config.getInt( "maxit" );
    flog1 << " maximum number of tolerance-reductions in a month: " << telmnt[0].tem.maxit << endl;
  
    telmnt[0].tem.maxitmon = telmnt[0].tem.config.getInt( "maxitmon" );
    flog1 << " maximum number of iterations in a month: " << telmnt[0].tem.maxitmon << endl;

    // the adaptive solver keeps its error tolerance; the cap only limits the largest sub-step: 1/30 of a monthly step (as before), 1/4 of a step shorter than a week
    telmnt[0].tem.odedtmax = getTimestep45().getSTEPSECONDS() < 7 * 86400 ? 0.25 : 0.03333;
    if( telmnt[0].tem.config.has( "ode_dtmax" ) && telmnt[0].tem.config.getDouble( "ode_dtmax" ) > 0.0 )
      telmnt[0].tem.odedtmax = telmnt[0].tem.config.getDouble( "ode_dtmax" );
    flog1 << " largest integrator sub-step (fraction of the model step): " << telmnt[0].tem.odedtmax << endl;
 
    
    //telmnt[0].tem.askODE( flog1 );


    // Get vegetation type dependent parameters

    ifilename = telmnt[0].tem.config.getString( "rootzecd" );
    telmnt[0].tem.soil.getrootz( ifilename );
    flog1 << " root zone parameters found in " << ifilename << endl;
    
    ifilename = telmnt[0].tem.config.getString( "vegecd" );
    telmnt[0].tem.veg.getecd( ifilename );
    flog1 << " vegetation parameters found in " << ifilename << endl;
    
    ifilename = telmnt[0].tem.config.getString( "conductecd" );
    telmnt[0].tem.veg.pen.getecd( ifilename );
	flog1 << " hydraulic and conductance-related parameters found in " << ifilename << endl;
	
	ifilename = telmnt[0].tem.config.getString( "mcrbecd" );
    telmnt[0].tem.microbe.getvegecd( ifilename );
    flog1 << " microbial parameters found in " << ifilename << endl;


    // Get parameters associated with human disturbance
    //   activities
    
    ifilename = telmnt[0].tem.config.getString( "agecd" );
    telmnt[0].tem.ag.getecd( ifilename );
    flog1 << " agricultural parameters found in " << ifilename << endl;
    
    telmnt[0].tem.ag.tillflag = telmnt[0].tem.config.getInt( "agtillflag" );
    flog1 << " agricultural tilling is " << ((telmnt[0].tem.ag.tillflag==1)?"on":"off") << endl;
    telmnt[0].tem.ag.irrgflag = telmnt[0].tem.config.getInt( "agirrgflag" );
    flog1 << " agricultural irrigation is " << ((telmnt[0].tem.ag.irrgflag==1)?"on":"off") << endl;
    telmnt[0].tem.ag.fertflag = telmnt[0].tem.config.getInt( "agfertflag" );
    flog1 << " agricultural fertilization is " << ((telmnt[0].tem.ag.fertflag==1)?"on":"off") << endl;
        

    telmnt[0].tem.loadSiteCalibrations( flog1 );

    iftexture = openInput( "TEXTURE" );
    ifelev = openInput( "ELEV" );
    if( telmnt[0].tem.config.has( "mask" ) )
      openInput( "MASK" );

    telmnt[0].tem.predflag = telmnt[0].tem.config.getInt( "temoutflag" );
    flog1 << " output from TEM " << ((telmnt[0].tem.predflag==1)?"will":"will not be") << " written" << endl;
  
    tempredmap = telmnt[0].tem.config.getStringList( "temoutvars", false  );
    telmnt[0].ntempred = static_cast<int>( tempredmap.size() );
    for( const auto& variable : tempredmap )
      flog1 << "      " << variable << " will be written to the NetCDF output" << endl;
    if( 1 == telmnt[0].tem.predflag && !tempredmap.empty() )
    {
      tem::nc::declareOutput( { "currentveg", "potveg", "agstate", "standage", "subtype", "cmnt", "carea", "subarea" }, true );
      tem::nc::declareOutput( tempredmap, true );
    }

    spinoutfg = telmnt[0].tem.config.getInt( "spinoutflag" );
    spinoutyrs = telmnt[0].tem.config.getInt( "spinoutyrs" );
    if( telmnt[0].tem.predflag > 0 ) { flog1 << " output from spinup period " << ((spinoutfg==1)?"will":"will not be") << " written " << endl; }
    if( (telmnt[0].tem.predflag > 0) && ( spinoutfg == 1 )) { flog1 << "    for " << spinoutyrs << " years " << endl; }
    
    
    istateflag = telmnt[0].tem.config.getInt( "istateflag" );
    flog1 << " input state ( 0 -> require initizlization; 1 -> file input at equilibrium; 2 -> file input for specific year) " << istateflag << endl;
    ifilename = telmnt[0].tem.config.getString( "istatefile" );
    if( istateflag > 0 ) { flog1 << " input state file: " << ifilename << endl; }
    istateyear = telmnt[0].tem.config.getInt( "istateyear" );
    if( istateflag == 2 ) { flog1 << " input state year: " << istateyear << endl; }
    
    if( istateflag > 0 && !ifstate.open( ifilename ) )
    {
      flog1 << "cannot open the state file " << ifilename << endl;
      exit( -1 );
    }
    
    ostateflag = telmnt[0].tem.config.getInt( "ostateflag" );
    flog1 << " output state ( 0 -> do not write TEMSTATE file; 1-> save TEMSTATE at equilibrium; 2 -> save TEMSTATE at a specific year) " << ostateflag << endl;
    ifilename = telmnt[0].tem.config.getString( "ostatefile" );
    if( ostateflag > 0 ) { flog1 << " output state file: " << ifilename << endl; }
    ostateyear = telmnt[0].tem.config.getInt( "ostateyear" );
    if( ostateflag == 2 ) { flog1 << " output state year: " << ostateyear << endl; }
    
    if( ostateflag > 0 ) { ofstate.open( ifilename ); }
    
  }
  

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void updateTCLMGridCell( const int& pdyr )
{

  const double Watts2cal = 1.0 / 0.4845;
  int dyr;
  int dm;
  int gisend;

  double avgtair, avgprec, avgtaird, avgphi;

  avgtair = ZERO;
  avgprec = ZERO;
  avgtaird = ZERO;
  avgphi = ZERO;

  float lat = -999.9;

  string SRADname;

  fatalerr = 0;

  if( 0 == pdyr )
  {
    if( 1 == telmnt[0].clm.tcldsflag )
    {
      if( 0 == telmnt[0].clm.sradflag )
      {
        for( dyr = 0; dyr < (transtime); ++dyr )
        {
          gisend = girrdat[dyr].read( ifgirr );

          if( -1 == gisend )
          {
            cout << "Ran out of GIRR data" << endl << endl;
            flog1 << "Ran out of GIRR data" << endl << endl;

            exit( -1 );
          }
        }
      }


      if( 1 == telmnt[0].clm.cldflag )
      {
        for( dyr = 0; dyr < (transtime); ++dyr )
        {
          gisend = cldsdat[dyr].read( ifclds );

          if( -1 == gisend )
          {
            cout << "Ran out of Cloudiness data" << endl << endl;
            flog1 << "Ran out of Cloudiness data" << endl << endl;

            exit( -1 );
          }
        }

        telmnt[0].lon = cldsdat[0].lon;
        telmnt[0].lat = cldsdat[0].lat;
        SRADname = "CLDINESS";
      }
      else
      {
        for( dyr = 0; dyr < (transtime); ++dyr )
        {
          gisend = nirrdat[dyr].read( ifnirr );

          if( -1 == gisend )
          {
            cout << "Ran out of NIRR data" << endl << endl;
            flog1 << "Ran out of NIRR data" << endl << endl;

            exit( -1 );
          }
        }

        telmnt[0].lon = nirrdat[0].lon;
        telmnt[0].lat = nirrdat[0].lat;
        SRADname = "NIRR";
      }

      if( 1 == telmnt[0].clm.parflag )
      {
        for( dyr = 0; dyr < (transtime); ++dyr )
        {
          gisend = pardat[dyr].read( ifpar );

          if( -1 == gisend )
          {
            cout << "Ran out of PAR data" << endl << endl;
            flog1 << "Ran out of PAR data" << endl << endl;

            exit( -1 );
          }
        }
      }
    }
    else  // 0 == telmnt[0].clm.tcldsflag
    {
      if( 0 == telmnt[0].clm.sradflag )
      {
        gisend = girrdat[0].read( ifgirr );

        if( -1 == gisend )
        {
          cout << "Ran out of GIRR data" << endl << endl;
          flog1 << "Ran out of GIRR data" << endl << endl;

          exit( -1 );
        }
      }

      if( 1 == telmnt[0].clm.cldflag )
      {
        gisend = cldsdat[0].read( ifclds );

        if( -1 == gisend )
        {
          cout << "Ran out of Cloudiness data" << endl << endl;
          flog1 << "Ran out of Cloudiness data" << endl << endl;

          exit( -1 );
        }

        telmnt[0].lon = cldsdat[0].lon;
        telmnt[0].lat = cldsdat[0].lat;
        SRADname = "CLDINESS";
      }
      else
      {
        gisend = nirrdat[0].read( ifnirr );

        if( -1 == gisend )
        {
          cout << "Ran out of NIRR data" << endl << endl;
          flog1 << "Ran out of NIRR data" << endl << endl;

          exit( -1 );
        }

        telmnt[0].lon = nirrdat[0].lon;
        telmnt[0].lat = nirrdat[0].lat;
        SRADname = "NIRR";
      }

      if( 1 == telmnt[0].clm.parflag )
      {
        gisend = pardat[0].read( ifpar );

        if( -1 == gisend )
        {
          cout << "Ran out of PAR data" << endl << endl;
          flog1 << "Ran out of PAR data" << endl << endl;

          exit( -1 );
        }
      }
    }

    // Look for spatial co-registration problems between
    //   cloudiness or net irradiance (NIRR) spatially explicit
    //   data and gross irradiance (GIRR) spatially explicit
    //   data

    if( 0 == telmnt[0].clm.sradflag )
    {
      fatalerr = telmnt[0].coregerr( flog1,
                                     SRADname,
                                     telmnt[0].lon,
                                     telmnt[0].lat,
                                     "GIRR",
                                     girrdat[0].lon,
                                     girrdat[0].lat );

      if( fatalerr != 0 ) { exit( -1 ); }
    }

    // Look for spatial co-registration problems between
    //   cloudiness or net irradiance (NIRR) spatially explicit
    //   data and photosynthetically active radiation (PAR)
    //   spatially explicit data

    if( 1 == telmnt[0].clm.parflag )
    {
      fatalerr = telmnt[0].coregerr( flog1,
                                     SRADname,
                                     telmnt[0].lon,
                                     telmnt[0].lat,
                                     "PAR",
                                     pardat[0].lon,
                                     pardat[0].lat );

      if( fatalerr != 0 ) { exit( -1 ); }
    }

    // Read in historical monthly air temperatures for grid cell

    if( 1 == telmnt[0].clm.ttairflag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        gisend = tairdat[dyr].read( iftair );

        if( -1 == gisend )
        {
          cout << "Ran out of Air Temperature data" << endl << endl;
          flog1 << "Ran out of Air Temperature data" << endl << endl;

          exit( -1 );
        }
      }
    }
    else
    {
      gisend = tairdat[0].read( iftair );

      if( -1 == gisend )
      {
        cout << "Ran out of Air Temperature data" << endl << endl;
        flog1 << "Ran out of Air Temperature data" << endl << endl;

        exit( -1 );
      }
    }


    // Look for spatial co-registration problems between cloudiness and
    // air temperature spatially explicit data sets

    fatalerr = telmnt[0].coregerr( flog1,
                                   SRADname,
                                   telmnt[0].lon,
                                   telmnt[0].lat,
                                   "TAIR",
                                   tairdat[0].lon,
                                   tairdat[0].lat );

    if( fatalerr != 0 ) { exit( -1 ); }


    // Read in historical monthly precipitation for grid cell

    if( 1 == telmnt[0].clm.tprecflag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        gisend = precdat[dyr].read( ifprec );

        if( -1 == gisend )
        {
          cout << "Ran out of Precipitation data" << endl << endl;
          flog1 << "Ran out of Precipitation data" << endl << endl;

          exit( -1 );
        }

      }
    }
    else
    {
      gisend = precdat[0].read( ifprec );

      if( -1 == gisend )
      {
        cout << "Ran out of Precipitation data" << endl << endl;
        flog1 << "Ran out of Precipitation data" << endl << endl;

        exit( -1 );
      }
    }

	// Look for spatial co-registration problems between cloudiness and
    // precipitation spatially explicit data sets

    fatalerr = telmnt[0].coregerr( flog1,
                                   SRADname,
                                   telmnt[0].lon,
                                   telmnt[0].lat,
                                   "PREC",
                                   precdat[0].lon,
                                   precdat[0].lat );

    if( fatalerr != 0 ) { exit( -1 ); }


	// Read in historical monthly daily temperature range for grid cell

    if( 1 == telmnt[0].clm.ttrangeflag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        gisend = trangedat[dyr].read( iftrange );

        if( -1 == gisend )
        {
          cout << "Ran out of Temp. range data" << endl << endl;
          flog1 << "Ran out of Temp. range data" << endl << endl;

          exit( -1 );
        }

      }
    }
    else
    {
      gisend = trangedat[0].read( iftrange );

      if( -1 == gisend )
      {
        cout << "Ran out of Temp. range data" << endl << endl;
        flog1 << "Ran out of Temp. range data" << endl << endl;

        exit( -1 );
      }
    }

	// Look for spatial co-registration problems between cloudiness and
    // trange spatially explicit data sets

    fatalerr = telmnt[0].coregerr( flog1,
                                   SRADname,
                                   telmnt[0].lon,
                                   telmnt[0].lat,
                                   "TRANGE",
                                   trangedat[0].lon,
                                   trangedat[0].lat );

    if( fatalerr != 0 ) { exit( -1 ); }

	// Read in historical monthly vapor pressure for grid cell

    if( 1 == telmnt[0].clm.tvprflag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        gisend = vprdat[dyr].read( ifvpr );

        if( -1 == gisend )
        {
          cout << "Ran out of Vapor Pressure data" << endl << endl;
          flog1 << "Ran out of Vapor Pressure data" << endl << endl;

          exit( -1 );
        }

      }
//    cout << "vprlonlat = " << vprdat[0].lon << " " << vprdat[0].lat << endl;
    }
    else
    {
      gisend = vprdat[0].read( ifvpr );

      if( -1 == gisend )
      {
        cout << "Ran out of Vapor Pressure data" << endl << endl;
        flog1 << "Ran out of Vapor Pressure data" << endl << endl;

        exit( -1 );
      }
    }


    // Look for spatial co-registration problems between cloudiness and
    // vapor pressure spatially explicit data sets

    fatalerr = telmnt[0].coregerr( flog1,
                                   SRADname,
                                   telmnt[0].lon,
                                   telmnt[0].lat,
                                   "VPD",
                                   vprdat[0].lon,
                                   vprdat[0].lat );

    if( fatalerr != 0 ) { exit( -1 ); }


    // Read in historical annual atmospheric CO2 data for
    //   globe

    if( 1 == telmnt[0].clm.tco2flag && 0 == assignCO2 )
    {
//  BSF CO2 Fix
      for( dyr = 0; dyr < (transtime+1); ++dyr )
      {
        const size_t k = (size_t) dyr < co2val.size() ? dyr : co2val.size() - 1;
        co2dat[dyr] = co2val[k];
      }

      assignCO2 = 1;
    }

    // Read in historical monthly atmospheric CO2 data for
    //   grid cell

    if( 2 == telmnt[0].clm.tco2flag )
    {
      cout << "This feature has not been implemented yet ";
      cout << "in this TEM version!"  << endl;

      flog1 << "This feature has not been implemented yet ";
      flog1 << "in this TEM version!"  << endl;

      exit( -1 );
    }


    // Read in historical monthly ozone data for grid cell

    if( 1 == telmnt[0].clm.to3flag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        gisend = o3dat[dyr].read( ifo3 );
//      cout << "transient ozone = " << o3dat[0].lon << " " << o3dat[0].lat << endl;

        if( -1 == gisend )
        {
          cout << "Ran out of Ozone data" << endl << endl;
          flog1 << "Ran out of Ozone data" << endl << endl;

          exit( -1 );
        }
      }
    }
    else
    {
      gisend = o3dat[0].read( ifo3 );

      if( -1 == gisend )
      {
        cout << "Ran out of Ozone data" << endl << endl;
        flog1 << "Ran out of Ozone data" << endl << endl;

        exit( -1 );
      }
    }

    // Look for spatial co-registration problems between cloudiness and
    // ozone spatially explicit data sets

    fatalerr = telmnt[0].coregerr( flog1,
                                   SRADname,
                                   telmnt[0].lon,
                                   telmnt[0].lat,
                                   "AOT40",
                                   o3dat[0].lon,
                                   o3dat[0].lat );

    if( fatalerr != 0 ) { exit( -1 ); }

//
//  Nitrogen Deposition
//
//
    if( 1 == telmnt[0].clm.tndepflag )
   { 
   for( dyr = 0; dyr < (transtime); ++dyr )
   {
   gisend = ndepdat[dyr].read( ifndep );
   if( -1 == gisend )
   {
     cout << "Ran out of Ndep data";
     cout << endl << endl;
     flog1 << "Ran out of Ndep data";
     flog1 << endl << endl;

     exit( -1 );
   }

//   cout << "ndepdat = " << dyr << " " << ndepdat[dyr].lon <<  " " << ndepdat[dyr].lat << " " << ndepdat[dyr].adepval << endl;
   }
   }
    else
  {
      gisend = ndepdat[0].read( ifndep );

   if( -1 == gisend )
   {
     cout << "Ran out of Ndep data";
     cout << endl << endl;
     flog1 << "Ran out of Ndep data";
     flog1 << endl << endl;

     exit( -1 );
   }
  }

// Check data for spatial coregistration errors
//
   fatalerr = telmnt[0].coregerr( flog1,
                                  "Climate",
                                  telmnt[0].lon,
                                  telmnt[0].lat,
                                  "NDEP",
                                  ndepdat[0].lon,
                                  ndepdat[0].lat );

   if( fatalerr != 0 ) { exit( -1 ); }

    // Nitrogen fertilizer of crops and pasture

    if( nfertused )
    {
      for( int ift = 0; ift < 2; ++ift )
      {
        std::vector<Nfertdat>& fdat = ( 0 == ift ) ? nfertcropdat : nfertpasturedat;
        tem::nc::Stream* iffert = ( 0 == ift ) ? ifnfertcrop : ifnfertpasture;

        if( 2 == nfertflag )
        {
          for( dyr = 0; dyr < (transtime); ++dyr )
          {
            gisend = fdat[dyr].read( iffert );

            if( -1 == gisend )
            {
              cout << "Ran out of nitrogen fertilizer data" << endl << endl;
              flog1 << "Ran out of nitrogen fertilizer data" << endl << endl;

              exit( -1 );
            }
          }
        }
        else
        {
          gisend = fdat[0].read( iffert );

          if( -1 == gisend )
          {
            cout << "Ran out of nitrogen fertilizer data" << endl << endl;
            flog1 << "Ran out of nitrogen fertilizer data" << endl << endl;

            exit( -1 );
          }
        }
      }
    }

    // Lightning and population density for the wildfire module

    if( 1 == vegfire )
    {
      tem::nc::Record fr;

      if( -1 == tem::nc::next( iffireflash, fr ) )
      {
        cout << "Ran out of lightning data" << endl << endl;
        flog1 << "Ran out of lightning data" << endl << endl;

        exit( -1 );
      }

      for( int im = 0; im < CYCLE; ++im ) { telmnt[0].tem.ig[im] = fr.mon[im]; }

      telmnt[0].tem.lon = fr.lon;

      if( -1 == tem::nc::next( iffirepop, fr ) )
      {
        cout << "Ran out of population density data" << endl << endl;
        flog1 << "Ran out of population density data" << endl << endl;

        exit( -1 );
      }

      telmnt[0].tem.pop = fr.v[0];

      if( -1 == tem::nc::next( iffiregdp, fr ) )
      {
        cout << "Ran out of GDP data" << endl << endl;
        flog1 << "Ran out of GDP data" << endl << endl;

        exit( -1 );
      }

      telmnt[0].tem.gdp = fr.v[0];
    }

#if TEM_ENABLE_STORM
    if( ifstorm )
    {
      tem::nc::Record record;
      if( tem::nc::next( ifstorm, record ) != 1 )
      {
        flog1 << "Ran out of storm return-interval data" << endl;
        exit( -1 );
      }
      telmnt[0].tem.setStormReturnIntervals( record.lon, record.lat, record.v[0], record.v[1] );
    }
#endif

    // Read in wind speed, transient or climatology

    for( dyr = 0; dyr < ( ( 1 == telmnt[0].clm.tws10flag ) ? transtime : 1 ); ++dyr )
    {
      gisend = ws10dat[dyr].read( ifws10 );

      if( -1 == gisend )
      {
        cout << "Ran out of windspeed data" << endl << endl;
        flog1 << "Ran out of windspeed data" << endl << endl;

        exit( -1 );
      }
    }

    // Look for spatial co-registration problems between cloudiness and
    // windspeed spatially explicit data sets
    
    fatalerr = telmnt[0].coregerr( flog1,
                                   SRADname,
                                   telmnt[0].lon,
                                   telmnt[0].lat,
                                   "WS10",
                                   ws10dat[0].lon,            
                                   ws10dat[0].lat );

    tem::nc::checkAligned();

    if( fatalerr != 0 ) { exit( -1 ); }

  }  // end if pdyr = 0 loop


  if( 0 == pdyr ) { dyr = 0; }
  else if( istateflag < 2 && pdyr < (totsptime + 1) && telmnt[0].spinflag == 1 )
  {
    dyr = (pdyr-1)%spintime + 1;
  }
  else
  {
//   BSF fixes to dyr
//    if( istateflag < 2 ) { dyr = pdyr - totsptime; }
    if( istateflag < 2 ) { dyr = pdyr - totsptime -1; }
//    else{ dyr = pdyr; }
    else{ dyr = pdyr-1; }
  }


  if( 0 == telmnt[0].clm.tcldsflag )
  {
    if( 0 == telmnt[0].clm.sradflag )
    {
      for( dm = 0; dm < CYCLE; ++dm )
      {
        girrdat[dyr].mon[dm] = girrdat[0].mon[dm];
      }
    }

    if( 1 == telmnt[0].clm.cldflag )
    {
      for( dm = 0; dm < CYCLE; ++dm )
      {
        cldsdat[dyr].mon[dm] = cldsdat[0].mon[dm];
      }
    }
    else
    {
      for( dm = 0; dm < CYCLE; ++dm )
      {
        nirrdat[dyr].mon[dm] = nirrdat[0].mon[dm];
      }
    }

    if( 1 == telmnt[0].clm.parflag )
    {
      for( dm = 0; dm < CYCLE; ++dm )
      {
        pardat[dyr].mon[dm] = pardat[0].mon[dm];
      }
    }
  }

  if( 0 == telmnt[0].clm.ttairflag )
  {
    tairdat[dyr].max = tairdat[0].max;
    tairdat[dyr].ave = tairdat[0].ave;

    for( dm = 0; dm < CYCLE; ++dm )
    {
      tairdat[dyr].mon[dm] = tairdat[0].mon[dm];
    }
  }

  if( 0 == telmnt[0].clm.tprecflag )
  {
    precdat[dyr].total = precdat[0].total;

    for( dm = 0; dm < CYCLE; ++dm )
    {
      precdat[dyr].mon[dm] = precdat[0].mon[dm];
    }
  }

  if( 0 == telmnt[0].clm.ttrangeflag )
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      trangedat[dyr].mon[dm] = trangedat[0].mon[dm];
    }
  }

  if( 0 == telmnt[0].clm.tvprflag )
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      vprdat[dyr].mon[dm] = vprdat[0].mon[dm];
    }
  }
  
  if( 0 == telmnt[0].clm.tws10flag )
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      ws10dat[dyr].mon[dm] = ws10dat[0].mon[dm];
    }
  }

  if( 0 == telmnt[0].clm.to3flag )
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      o3dat[dyr].mon[dm] = o3dat[0].mon[dm];
    }
  }

  if( 0 == telmnt[0].clm.tndepflag )
  {
      ndepdat[dyr] = ndepdat[0];
  }

  if( nfertused && 1 == nfertflag )
  {
    nfertcropdat[dyr] = nfertcropdat[0];
    nfertpasturedat[dyr] = nfertpasturedat[0];
  }


  // Determine annual mean and maximum monthly air
  //   temperature for year for each grid cell

  telmnt[0].mxtair[pdyr] = tairdat[dyr].max;
  telmnt[0].avetair[pdyr] = tairdat[dyr].ave;


  // Determine annual precipitation for each grid cell

  telmnt[0].yrprec[pdyr] = precdat[dyr].total;


  // Interpolate annual atmospheric CO2 concentrations
  //   to a monthly temporal resolution for grid cell
  //   Value in co2dat assumed to represent July atmospheric
  //   CO2 concentrations

  if( 1 == telmnt[0].clm.tco2flag && dyr > 0 )
  {
    if( pdyr < (totsptime+1) )
    {
      for( dm = 0; dm < CYCLE; ++dm )
      {
        telmnt[0].climate[telmnt[0].clm.I_CO2][dm][pdyr] = co2dat[0];
      }
    }
    else
    {
      for( dm = 0; dm < CYCLE; ++dm )
      {
        if( dm < 6 )
        {
          telmnt[0].climate[telmnt[0].clm.I_CO2][dm][pdyr] = co2dat[dyr-1]
                                                       + ((dm + 6)
                                                       * (co2dat[dyr]
                                                       - co2dat[dyr-1])
                                                       / (double) CYCLE);
        }
        else
        {
          if(dyr == transtime) {co2dat[dyr+1] = co2dat[dyr];}
          telmnt[0].climate[telmnt[0].clm.I_CO2][dm][pdyr] = co2dat[dyr]
                                                       + ((dm - 6)
                                                       * (co2dat[dyr+1]
                                                       - co2dat[dyr])
                                                       / (double) CYCLE);

        }
      }
    }

  }
  else // 0 == telmnt[0].clm.tco2flag || 0 == dyr
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      telmnt[0].climate[telmnt[0].clm.I_CO2][dm][pdyr] = telmnt[0].clm.getCO2LEVEL();
    }
  }

 // Assign information from files to telmnt.climate

  for( dm = 0; dm < CYCLE; ++dm )
  {
    // Air temperature

    telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr] = tairdat[dyr].mon[dm];
  }

   //  Ndep
        
   
     telmnt[0].adep[0][pdyr]= ndepdat[dyr].adepval;

   if( nfertused )
   {
     telmnt[0].nfertcrop[0][pdyr] = nfertcropdat[dyr].nfertval;
     telmnt[0].nfertpasture[0][pdyr] = nfertpasturedat[dyr].nfertval;
   }
   //    cout << "ndepval = " << telmnt[0].tem.ndepval << endl;
   //
  for( dm = 0; dm < CYCLE; ++dm )
  {
    // Daylength (hours)
    
    telmnt[0].climate[telmnt[0].clm.I_DAYL][dm][pdyr] = telmnt[0].tem.atms.daylength( telmnt[0].lat, dm );
    
    // Temperature Range

    telmnt[0].climate[telmnt[0].clm.I_TRANGE][dm][pdyr] = trangedat[dyr].mon[dm];

    // Daytime air temperature

    telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][pdyr] = telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr]
                            + 12.0*telmnt[0].climate[telmnt[0].clm.I_TRANGE][dm][pdyr]/telmnt[0].climate[telmnt[0].clm.I_DAYL][dm][pdyr]
                            * 1.732 / (2.0*3.14159) * sin( 3.14159 * telmnt[0].climate[telmnt[0].clm.I_DAYL][dm][pdyr] / 24.0 );

//cout << "taird = " << dm << " " << pdyr << " " << telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][pdyr] << " " << telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr] << " " << telmnt[0].climate[telmnt[0].clm.I_TRANGE][dm][pdyr] << " " << telmnt[0].climate[telmnt[0].clm.I_DAYL][dm][pdyr] << endl;

    #ifdef DEBUGX
      cout << " sin(1.57) test " << sin(1.57) << endl;
      printf( " tair = %8.21f, trange = %8.21f, taird = %8.21f ", telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr],
              telmnt[0].climate[telmnt[0].clm.I_TRANGE][dm][pdyr], telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][pdyr] );
      cout << endl;
    #endif

    // Nighttime air temperature

    telmnt[0].climate[telmnt[0].clm.I_TAIRN][dm][pdyr] = telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr]
                            - 12.0*telmnt[0].climate[telmnt[0].clm.I_TRANGE][dm][pdyr]/(24.0 - telmnt[0].climate[telmnt[0].clm.I_DAYL][dm][pdyr])
                            * 1.732 / (2.0*3.14159) * sin( 3.14159 * telmnt[0].climate[telmnt[0].clm.I_DAYL][dm][pdyr] / 24.0 );
    
    // Precipitation

    telmnt[0].climate[telmnt[0].clm.I_PREC][dm][pdyr] = precdat[dyr].mon[dm];

    // Rain and Snowfall

    telmnt[0].clm.precsplt( telmnt[0].climate[telmnt[0].clm.I_PREC][dm][pdyr],
                            telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr],
                            telmnt[0].climate[telmnt[0].clm.I_RAIN][dm][pdyr],
                            telmnt[0].climate[telmnt[0].clm.I_SNWFAL][dm][pdyr] );

//
//  Calculate vapor pressure deficit from vapor pressure
//

    telmnt[0].climate[telmnt[0].clm.I_VPR][dm][pdyr] = vprdat[dyr].mon[dm];

    telmnt[0].climate[telmnt[0].clm.I_VPDD][dm][pdyr] = (10.*0.61078*exp((17.27*telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][pdyr])
                                                  /(telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][pdyr]+237.3)))-vprdat[dyr].mon[dm];
    if(telmnt[0].climate[telmnt[0].clm.I_VPDD][dm][pdyr] < ZERO) { telmnt[0].climate[telmnt[0].clm.I_VPDD][dm][pdyr] = ZERO; }


    telmnt[0].climate[telmnt[0].clm.I_VPDN][dm][pdyr] = (10.*0.61078*exp((17.27*telmnt[0].climate[telmnt[0].clm.I_TAIRN][dm][pdyr])
                                                  /(telmnt[0].climate[telmnt[0].clm.I_TAIRN][dm][pdyr]+237.3)))-vprdat[dyr].mon[dm];
    if(telmnt[0].climate[telmnt[0].clm.I_VPDN][dm][pdyr] < ZERO) { telmnt[0].climate[telmnt[0].clm.I_VPDN][dm][pdyr] = ZERO; }


    // AOT40 ozone index

    telmnt[0].climate[telmnt[0].clm.I_AOT40][dm][pdyr] = o3dat[dyr].mon[dm];

   //  Ndep
   //
    
    // 10-meter windspeed
    
    telmnt[0].climate[telmnt[0].clm.I_WS10][dm][pdyr] = ws10dat[dyr].mon[dm];

  }

  if(telmnt[0].lat >= 0.0) { telmnt[0].tem.veg.hemisphere = 1; }
  else { telmnt[0].tem.veg.hemisphere = 0; }

//**************************************************************


  // Calculate GIRR during first year of simulation
  //   (Note: use same values throughout simulation)

  if( 1 == telmnt[0].clm.sradflag )
  {
    lat = telmnt[0].lat;

    telmnt[0].clm.yrsumday = ZERO;

    for( dm = 0; dm < CYCLE; ++dm )
    {
      telmnt[0].climate[telmnt[0].clm.I_GIRR][dm][pdyr] = telmnt[0].clm.xgirr( lat,
                                                                               dm,
                                                                               telmnt[0].clm.yrsumday );
    }
  }
  else
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      telmnt[0].climate[telmnt[0].clm.I_GIRR][dm][pdyr] = girrdat[dyr].mon[dm] * Watts2cal;
    }
  }

  // Calculate NIRR, CLDINESS and PAR or retrieve from earlier calculations

  for( dm = 0; dm < CYCLE; ++dm )
  {
    if( 1 == telmnt[0].clm.cldflag )
    {
      telmnt[0].climate[telmnt[0].clm.I_CLDS][dm][pdyr] = cldsdat[dyr].mon[dm];

      telmnt[0].climate[telmnt[0].clm.I_NIRR][dm][pdyr] = telmnt[0].clm.xnirr( telmnt[0].climate[telmnt[0].clm.I_CLDS][dm][pdyr],
	                                                                 telmnt[0].climate[telmnt[0].clm.I_GIRR][dm][pdyr] );
    }
    else
    {
      telmnt[0].climate[telmnt[0].clm.I_NIRR][dm][pdyr] = nirrdat[dyr].mon[dm] * Watts2cal;

      telmnt[0].climate[telmnt[0].clm.I_CLDS][dm][pdyr] = telmnt[0].clm.mkclds( telmnt[0].climate[telmnt[0].clm.I_GIRR][dm][pdyr],
                                                                          telmnt[0].climate[telmnt[0].clm.I_NIRR][dm][pdyr] );
    }

    if( 0 == telmnt[0].clm.parflag )
    {

      telmnt[0].climate[telmnt[0].clm.I_PAR][dm][pdyr]  = telmnt[0].clm.xpar( telmnt[0].climate[telmnt[0].clm.I_CLDS][dm][pdyr],
                                                                        telmnt[0].climate[telmnt[0].clm.I_NIRR][dm][pdyr] );
    }
    else
    {
      telmnt[0].climate[telmnt[0].clm.I_PAR][dm][pdyr] = pardat[dyr].mon[dm] * Watts2cal;
    }
  }

  if(pdyr == 0)
  {
    for( dm = 0; dm < CYCLE; ++dm )
    {
      avgprec += telmnt[0].climate[telmnt[0].clm.I_PREC][dm][pdyr]/(double)CYCLE;
      avgtair += telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][pdyr]/(double)CYCLE;
      avgtaird += telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][pdyr]/(double)CYCLE;
    }
    telmnt[0].tem.veg.setRPREC( avgprec );
    telmnt[0].tem.veg.setRTAIR( avgtaird );

    // Optimum temperatures and the mean phenology of the first year, one pass over its steps
    for( dm = 0; dm < CYCLE; ++dm )
    {
      const double taird = telmnt[0].climate[telmnt[0].clm.I_TAIRD][dm][0];

      if( getTimestep45().isYEARSTART( dm ) )
      {
        telmnt[0].tem.veg.setTOPT( telmnt[0].tem.veg.getTOPTMIN( telmnt[0].tem.veg.cmnt ) );
        telmnt[0].tem.veg.setTOPTMIC( telmnt[0].tem.veg.getTOPTMIN( telmnt[0].tem.veg.cmnt ) );
      }

      if( taird > telmnt[0].tem.veg.getTOPT() ) { telmnt[0].tem.veg.setTOPT( taird ); }

      if( taird > telmnt[0].tem.veg.getTOPTMIC() ) { telmnt[0].tem.veg.setTOPTMIC( taird ); }

      telmnt[0].tem.veg.phenology( telmnt[0].tem.veg.cmnt,
                         telmnt[0].climate[telmnt[0].clm.I_TAIR][dm][0],
                         taird,
                         telmnt[0].climate[telmnt[0].clm.I_TAIRN][dm][0],
                         1.0,
                         1.0,
                         0.0);

      avgphi += telmnt[0].tem.veg.getPHI()/(double)CYCLE;
    }

    telmnt[0].tem.veg.setRPHI( avgphi );
    telmnt[0].tem.veg.setRTAIRPHI( avgtaird * avgphi );
  }

 // previous-month variables
  telmnt[0].tem.atms.setPREVTAIR( telmnt[0].climate[telmnt[0].clm.I_TAIR][CYCLE-1][0] );
  telmnt[0].tem.atms.setPREV2TAIR( telmnt[0].climate[telmnt[0].clm.I_TAIR][(CYCLE-2+CYCLE)%CYCLE][0] );
  telmnt[0].tem.soil.setPREVSPACK( ZERO );
  telmnt[0].tem.atms.setPREVCO2( telmnt[0].tem.atms.getCO2LEVEL() );


  telmnt[0].tem.initializeState();

  // Set year

  telmnt[0].year = telmnt[0].clm.startyr
                   - totsptime
                   - 1
                   + pdyr;

  // Copy TEMclm results to output variables

/*  if( 1 == telmnt[0].clm.predflag )
  {
    if ( (1 == spinoutfg
         && telmnt[0].year < telmnt[0].clm.startyr)
         || (2 == spinoutfg
         && telmnt[0].year >= (telmnt[0].clm.startyr-spinoutyrs))
         || (telmnt[0].year >= telmnt[0].clm.startyr) )
    {
      if( 1 == telmnt[0].clm.cldflag )
      {
        telmnt[0].carea = cldsdat[0].carea;
        telmnt[0].contnent = cldsdat[0].contnent;
      }
      else
      {
        telmnt[0].carea = nirrdat[0].carea;
        telmnt[0].contnent = nirrdat[0].contnent;
      }

      telmnt[0].atmswritepred( pdyr,
                               clmpredmap,
                               telmnt[0].natmspred );
    }
  } */ 

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************** */

void updateTLCLUCGridCell( const int& pdyr )
{

  int dyr;
  int tstyr;
  int gisend;
  int ichrt;


//  gridlulc.gotoFirstNode();   // start at first LULCListNode

  fatalerr = 0;

  if( 0 == pdyr )
  {
    // Get the total number of cohorts in the grid cell

    if( 1 == telmnt[0].lcluc.tlulcflag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        if (dyr <= telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1 ) { 


        gisend = mxcohrtdat[dyr].read( ifnumchrts );


        if( -1 == gisend )
        {
          cout << "Ran out of Number of Cohorts data";
          cout << endl << endl;
          flog1 << "Ran out of Number of Cohorts data";
          flog1 << endl << endl;

          exit( -1 );
        }

        // Check data for spatial coregistration errors

        fatalerr = telmnt[0].coregerr( flog1,
                                       "Climate",
                                       telmnt[0].lon,
                                       telmnt[0].lat,
                                      "MAXCOHORTS",
                                       mxcohrtdat[dyr].lon,
                                       mxcohrtdat[dyr].lat );

        if( fatalerr != 0 ) { exit( -1 ); }
      }
     else
     {
       mxcohrtdat[dyr].lon = mxcohrtdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1].lon;
       mxcohrtdat[dyr].lat = mxcohrtdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1].lat;
       mxcohrtdat[dyr].carea = mxcohrtdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1].carea;
       mxcohrtdat[dyr].year = telmnt[0].lcluc.lastyr + (dyr-(telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1));
       mxcohrtdat[dyr].total = mxcohrtdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1].total;
       mxcohrtdat[dyr].natchrts = mxcohrtdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1].natchrts;
       mxcohrtdat[dyr].contnent = mxcohrtdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1].contnent;
     }
      }
    }
    else // 0 == telmnt[0].lcluc.tlulcflag
    {
      gisend = mxcohrtdat[0].read( ifnumchrts );

      if( -1 == gisend )
      {
        cout << "Ran out of Number of Cohorts data";
        cout << endl << endl;
        flog1 << "Ran out of Number of Cohorts data";
        flog1 << endl << endl;

        exit( -1 );
      }

      // Check data for spatial coregistration errors

      fatalerr = telmnt[0].coregerr( flog1,
                                     "Climate",
                                     telmnt[0].lon,
                                     telmnt[0].lat,
                                     "MAXCOHORTS",
                                     mxcohrtdat[0].lon,
                                     mxcohrtdat[0].lat );

      if( fatalerr != 0 ) { exit( -1 ); }

      for( dyr = 1; dyr < (transtime); ++dyr )
      {
        mxcohrtdat[dyr].lon = mxcohrtdat[0].lon;
        mxcohrtdat[dyr].lat = mxcohrtdat[0].lat;
        mxcohrtdat[dyr].carea = mxcohrtdat[0].carea;
        mxcohrtdat[dyr].year = mxcohrtdat[0].year + dyr;
        mxcohrtdat[dyr].total = mxcohrtdat[0].total;
        mxcohrtdat[dyr].natchrts = mxcohrtdat[0].natchrts;
        mxcohrtdat[dyr].contnent = mxcohrtdat[0].contnent;
      }
    }
    // Get land use/land cover cohort data for  the grid cell

    if( 1 == telmnt[0].lcluc.tlulcflag )
    {
      for( dyr = 0; dyr < (transtime); ++dyr )
      {
        for( ichrt = 0; ichrt < mxcohrtdat[dyr].total; ++ichrt )
        {
         if (dyr <= telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1 ) {
          gisend = lulcdat[dyr][ichrt].read( iflulc );

          if( -1 == gisend )
          {
            flog1 << "Ran out of Land cover/land use data";
            flog1 << endl << endl;

            exit( -1 );
          }

          // Check data for spatial coregistration errors

          fatalerr = telmnt[0].coregerr( flog1,
                                         "Climate",
                                         telmnt[0].lon,
                                         telmnt[0].lat,
                                         "LULC",
                                         lulcdat[dyr][ichrt].lon,
                                         lulcdat[dyr][ichrt].lat );

          if( fatalerr != 0 ) { exit( -1 ); }
        }
      else
      {
       lulcdat[dyr][ichrt].lon = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].lon;
       lulcdat[dyr][ichrt].lat = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].lat;
       lulcdat[dyr][ichrt].varname = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].varname;
       lulcdat[dyr][ichrt].year = telmnt[0].lcluc.lastyr + (dyr-(telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1));
       lulcdat[dyr][ichrt].icohort = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].icohort;
       lulcdat[dyr][ichrt].isrccohort = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].isrccohort;
       lulcdat[dyr][ichrt].standage = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].standage;
       lulcdat[dyr][ichrt].chrtarea = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].chrtarea;
       lulcdat[dyr][ichrt].potveg = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].potveg;
       lulcdat[dyr][ichrt].currentveg = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].currentveg;
       lulcdat[dyr][ichrt].subtype = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].subtype;
       lulcdat[dyr][ichrt].agstate = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].agstate;
       lulcdat[dyr][ichrt].agprevstate = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].agprevstate;
       lulcdat[dyr][ichrt].tillflag = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].tillflag;
       lulcdat[dyr][ichrt].fertflag = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].fertflag;
       lulcdat[dyr][ichrt].irrgflag = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].irrgflag;
       lulcdat[dyr][ichrt].disturbflag = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].disturbflag;
       lulcdat[dyr][ichrt].disturbmonth = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].disturbmonth;
       lulcdat[dyr][ichrt].vconvert = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].vconvert;
       lulcdat[dyr][ichrt].prod10par = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].prod10par;
       lulcdat[dyr][ichrt].prod100par = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].prod100par;
       lulcdat[dyr][ichrt].sconvert = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].sconvert;
       lulcdat[dyr][ichrt].region = lulcdat[telmnt[0].lcluc.lastyr - telmnt[0].clm.startyr + 1][ichrt].region;


       }
       }
      }
    }
    else // 0 == telmnt[0].lcluc.tlulcflag
    {
      for( ichrt = 0; ichrt < mxcohrtdat[0].total; ++ichrt )
      {
        gisend = lulcdat[0][ichrt].read( iflulc );

        if( -1 == gisend )
        {
          flog1 << "Ran out of Land cover/land use data";
          flog1 << endl << endl;

          exit( -1 );
        }

        fatalerr = telmnt[0].coregerr( flog1,
                                       "Climate",
                                       telmnt[0].lon,
                                       telmnt[0].lat,
                                       "LULC",
                                       lulcdat[0][ichrt].lon,
                                       lulcdat[0][ichrt].lat );

        if( fatalerr != 0 ) { exit( -1 ); }

        for( dyr = 1; dyr < (transtime); ++dyr )
        {
          lulcdat[dyr][ichrt].year = lulcdat[0][ichrt].year + dyr;
          lulcdat[dyr][ichrt].isrccohort = lulcdat[0][ichrt].isrccohort;
          lulcdat[dyr][ichrt].standage = lulcdat[0][ichrt].standage;
          lulcdat[dyr][ichrt].chrtarea = lulcdat[0][ichrt].chrtarea;
          lulcdat[dyr][ichrt].potveg = lulcdat[0][ichrt].potveg;
          lulcdat[dyr][ichrt].currentveg = lulcdat[0][ichrt].currentveg;
          lulcdat[dyr][ichrt].subtype = lulcdat[0][ichrt].subtype;
          lulcdat[dyr][ichrt].agstate = lulcdat[0][ichrt].agstate;
          lulcdat[dyr][ichrt].agprevstate = lulcdat[0][ichrt].agprevstate;
          lulcdat[dyr][ichrt].tillflag = lulcdat[0][ichrt].tillflag;
          lulcdat[dyr][ichrt].fertflag = lulcdat[0][ichrt].fertflag;
          lulcdat[dyr][ichrt].irrgflag = lulcdat[0][ichrt].irrgflag;
          lulcdat[dyr][ichrt].disturbflag = lulcdat[0][ichrt].disturbflag;
          lulcdat[dyr][ichrt].disturbmonth = lulcdat[0][ichrt].disturbmonth;
          lulcdat[dyr][ichrt].vconvert = lulcdat[0][ichrt].vconvert;
          lulcdat[dyr][ichrt].prod10par = lulcdat[0][ichrt].prod10par;
          lulcdat[dyr][ichrt].prod100par = lulcdat[0][ichrt].prod100par;
          lulcdat[dyr][ichrt].sconvert = lulcdat[0][ichrt].sconvert;
          lulcdat[dyr][ichrt].region = lulcdat[0][ichrt].region;
        }
      }
    }
    // Fractions of the cell in natural cover and in broadleaf forest for each year (fire ignitions, as in GDSTEM-FIRE)

    if( 1 == vegfire )
    {
      for( dyr = 0; dyr < transtime; ++dyr )
      {
        double natural = 0.0, forest = 0.0;
        for( ichrt = 0; ichrt < mxcohrtdat[dyr].total; ++ichrt )
        {
          if( 9 == lulcdat[dyr][ichrt].currentveg || 11 == lulcdat[dyr][ichrt].currentveg || 25 == lulcdat[dyr][ichrt].currentveg )
            forest += lulcdat[dyr][ichrt].chrtarea;
          if( 0 == lulcdat[dyr][ichrt].agstate ) { natural += lulcdat[dyr][ichrt].chrtarea; }
        }
        telmnt[0].tem.ag.lwft[dyr] = natural / mxcohrtdat[dyr].carea;
        telmnt[0].tem.ag.forarea[dyr] = forest / mxcohrtdat[dyr].carea;
      }
    }
  } // end of 0 == pdyr

  if( 0 == pdyr ) { tstyr = 0; }
  else if( istateflag < 2 && pdyr < (totsptime+1) && telmnt[0].spinflag == 1 )
  {
    tstyr = 1;
  }
  else
  {
    if( istateflag < 2 )
    {
//  BSF fix for initiatlization subtract 1 since pdyr = 1 in transient
      tstyr = pdyr - totsptime -1;
    }
    else { tstyr = pdyr-1; }
  }

  // Check data for temporal coregistration errors in mxcohrtdat

  if( 1 == telmnt[0].lcluc.tlulcflag
      && pdyr >= (totsptime+1)
      && telmnt[0].year != mxcohrtdat[tstyr].year )
  {
    cout << " Year in CLM data does not match ";
    cout << " Year in MAXCOHORTS data" << endl;
    cout << " at Lon = " << telmnt[0].lon;
    cout << "  Lat = " << telmnt[0].lat << endl;
    cout << "  CLM year = " << telmnt[0].year;
    cout << "  MAXCOHORTS year = " << mxcohrtdat[tstyr].year;
    cout << endl << endl;

    flog1 << " Year in CLM data does not match ";
    flog1 << " Year in MAXCOHORTS data" << endl << endl;
    flog1 << " at Lon = " << telmnt[0].lon;
    flog1 << "  Lat = " << telmnt[0].lat << endl;
    flog1 << " CLM year = " << telmnt[0].year;
    flog1 << " MAXCOHORTS year = " << mxcohrtdat[tstyr].year;
    flog1 << endl << endl;

    exit( -1 );
  }


  // Pass mxcohortdat information to telmnt[0]

  telmnt[0].maxcohorts = mxcohrtdat[tstyr].total;
  telmnt[0].natcohorts = mxcohrtdat[tstyr].natchrts;
  telmnt[0].carea = mxcohrtdat[tstyr].carea;
  telmnt[0].contnent = mxcohrtdat[tstyr].contnent;

  if( 0 == tstyr )
  {
    telmnt[0].prvmxcohrts = mxcohrtdat[tstyr].total;
  }
  else
  {
    telmnt[0].prvmxcohrts = mxcohrtdat[tstyr-1].total;
  }
  for( ichrt = 0; ichrt < telmnt[0].maxcohorts; ++ichrt )
  {
    // Check data for temporal coregistration errors in lulcdat

    if( 1 == telmnt[0].lcluc.tlulcflag
        && pdyr >= (totsptime+1)
        && telmnt[0].year != lulcdat[tstyr][ichrt].year )
    {
      cout << " Year in CLM data does not match ";
      cout << " Year in LCLUC data" << endl;
      cout << " at Lon = " << telmnt[0].lon;
      cout << "  Lat = " << telmnt[0].lat << endl;
      cout << "  CLM year = " << telmnt[0].year;
      cout << "  LCLUC year = " << lulcdat[tstyr][ichrt].year;
      cout << " for cohort " << (ichrt+1);
      cout << endl << endl;

      flog1 << " Year in CLM data does not match ";
      flog1 << " Year in LCLUC data" << endl << endl;
      flog1 << " at Lon = " << telmnt[0].lon;
      flog1 << "  Lat = " << telmnt[0].lat << endl;
      flog1 << " CLM year = " << telmnt[0].year;
      flog1 << " LCLUC year = " << lulcdat[tstyr][ichrt].year;
      cout << " for cohort " << (ichrt+1);
      flog1 << endl << endl;

      exit( -1 );
    }


    // Pass lulcdat information to telmnt[0].cohort

    telmnt[0].cohort[ichrt].srcCohort = lulcdat[tstyr][ichrt].isrccohort;
    telmnt[0].cohort[ichrt].standage = lulcdat[tstyr][ichrt].standage;
    telmnt[0].cohort[ichrt].chrtarea = lulcdat[tstyr][ichrt].chrtarea;
    telmnt[0].cohort[ichrt].potveg = lulcdat[tstyr][ichrt].potveg;
    telmnt[0].cohort[ichrt].currentveg = lulcdat[tstyr][ichrt].currentveg;
    telmnt[0].cohort[ichrt].subtype = lulcdat[tstyr][ichrt].subtype;
    telmnt[0].cohort[ichrt].agstate = lulcdat[tstyr][ichrt].agstate;
    telmnt[0].cohort[ichrt].agprvstate = lulcdat[tstyr][ichrt].agprevstate;
    telmnt[0].cohort[ichrt].tillflag = lulcdat[tstyr][ichrt].tillflag;
    telmnt[0].cohort[ichrt].fertflag = lulcdat[tstyr][ichrt].fertflag;
    telmnt[0].cohort[ichrt].irrgflag = lulcdat[tstyr][ichrt].irrgflag;
    telmnt[0].cohort[ichrt].disturbflag = lulcdat[tstyr][ichrt].disturbflag;
    telmnt[0].cohort[ichrt].disturbmonth = lulcdat[tstyr][ichrt].disturbmonth;

    // If cohort is in agriculture the during first year of the
    //   historical study period, convert cohort to agriculture
    //   during the first year of the transient spinup.  Keep
    //   cohort in agriculture throughout spinup period

/*    if( 1 == pdyr
        && istateflag < 2
        && 1 == pdyr
        && 1 == telmnt[0].cohort[ichrt].agstate
        && 1 == telmnt[0].cohort[ichrt].agprvstate)
    {
      telmnt[0].cohort[ichrt].agprvstate = 0;
      telmnt[0].cohort[ichrt].disturbflag = 1;
      telmnt[0].cohort[ichrt].disturbmonth = 1;
    } */

   if( pdyr > 1
       && pdyr < (totsptime+2)
       && istateflag < 2
       && 1 == telmnt[0].cohort[ichrt].agstate
       && 1 == telmnt[0].cohort[ichrt].agprvstate )
   {
      telmnt[0].prvmxcohrts = mxcohrtdat[tstyr].total;
   }

    telmnt[0].cohort[ichrt].vconvert = lulcdat[tstyr][ichrt].vconvert;
    telmnt[0].cohort[ichrt].prod10par = lulcdat[tstyr][ichrt].prod10par;
    telmnt[0].cohort[ichrt].prod100par = lulcdat[tstyr][ichrt].prod100par;
    telmnt[0].cohort[ichrt].sconvert = lulcdat[tstyr][ichrt].sconvert;
    telmnt[0].region = lulcdat[tstyr][ichrt].region;

//    if( 0 == tstyr )
//    {
//      telmnt[0].cohort[ichrt].prvchrtarea = lulcdat[tstyr][ichrt].chrtarea;
//    }
//    else
//    {
//      telmnt[0].cohort[ichrt].prvchrtarea = lulcdat[tstyr-1][ichrt].chrtarea;
//    }

    telmnt[0].cohort[ichrt].cmnt = telmnt[0].lcluc.getCommunityType( lulcdat[tstyr][ichrt].subtype );


    telmnt[0].cohort[ichrt].agcmnt = telmnt[0].cohort[ichrt].cmnt;

//    if( pdyr > 0 && 1 == lulcdat[tstyr][ichrt].agstate )
   if( 1 == lulcdat[tstyr][ichrt].agstate || 2 == lulcdat[tstyr][ichrt].agstate )
    {
      telmnt[0].cohort[ichrt].agcmnt = telmnt[0].lcluc.getCommunityType( lulcdat[tstyr][ichrt].currentveg );
    }
//    cout << "lulc type = " << ichrt << " " << " " << telmnt[0].cohort[ichrt].cmnt << " " << telmnt[0].cohort[ichrt].agcmnt << " " << lulcdat[tstyr][ichrt].agstate << endl;
  }

};

/* *************************************************************
************************************************************* */


/* **************************************************************
************************************************************** */

void updateTTEMGridCell( const int& pdyr,
                         std::ostream& rflog1 )
{
  int dm;

  int ichrt;

  int tchrt;

/* *************************************************************
            INITIALIZE TEM STATE FOR NEW COHORTS
************************************************************* */

  if( telmnt[0].maxcohorts > telmnt[0].prvmxcohrts )
  {
    for ( ichrt = telmnt[0].prvmxcohrts;
          ichrt < telmnt[0].maxcohorts;
          ++ichrt )
    {
      tchrt = telmnt[0].cohort[ichrt].srcCohort - 1;

      // A cohort that appears in a year is initialised from the cohort it splits from, which must already exist.
      if( tchrt < 0 || tchrt >= telmnt[0].prvmxcohrts )
      {
        cerr << "Cohort " << ( ichrt + 1 ) << " of the cell at lon " << telmnt[0].lon << ", lat " << telmnt[0].lat
             << " is new in year " << ( telmnt[0].clm.startyr + pdyr ) << " but its source cohort (isrccohort) is "
             << telmnt[0].cohort[ichrt].srcCohort << ", and only cohorts 1 to " << telmnt[0].prvmxcohrts
             << " exist before it. Give isrccohort in the land cover file." << endl;
        flog1 << "new cohort " << ( ichrt + 1 ) << " has no valid isrccohort" << endl;
        exit( -1 );
      }

      telmnt[0].setCohortTEMState( telmnt[0].cohort[tchrt],
                                   telmnt[0].cohort[ichrt] );
    }
  }

/* *************************************************************
                      UPDATE TEM FOR GRID CELL
************************************************************* */


/* *************************************************************
                 BEGIN VEGETATION MOSAIC LOOP
************************************************************* */

  // the last transient year may be cut short (run_length)
  const int nrun = ( !equil && pdyr == RTIME - 1 ) ? transsteps : CYCLE;
  Ttem45::setCutStep( nrun < CYCLE ? nrun - 1 : -1 );

  // community type and soil texture of a cohort, then one step of it
  auto stepCohort = [&]( const int c, const int m )
  {
    telmnt[0].tem.veg.cmnt = telmnt[0].cohort[c].cmnt;

    telmnt[0].tem.soil.xtext( telmnt[0].tem.veg.cmnt,
                              telmnt[0].tem.soil.getPCTSILT(),
                              telmnt[0].tem.soil.getPCTCLAY() );

    telmnt[0].updateTEMmonth( equil, totsptime, pdyr, m, c, rflog1 );
  };

  // state and output of a cohort after its steps of the year
  auto closeCohort = [&]( const int c )
  {
    if( 2 == ostateflag && telmnt[0].tem.totyr == ostateyear )
    {
      { tem::state::Out sout; telmnt[0].writeCohortState( sout, c ); ofstate.put( telmnt[0].lon, telmnt[0].lat, c + 1, sout ); }
    }

    if ( (1 == spinoutfg && telmnt[0].tem.totyr < telmnt[0].tem.startyr)
          || (2 == spinoutfg
          && telmnt[0].tem.totyr >= (telmnt[0].tem.startyr-spinoutyrs))
          || (telmnt[0].tem.totyr >= telmnt[0].tem.startyr
          && telmnt[0].tem.totyr <= telmnt[0].tem.endyr
          && 0 == (telmnt[0].wrtyr%telmnt[0].tem.diffyr)) )
    {
      // Output TEM transient results for specified years to files

      telmnt[0].temwritepred( tempredmap,
                              pdyr,
                              c,
                              telmnt[0].ntempred,
                              spinoutfg,
                              1 );
    }
  };

  // each cohort takes the steps of the year in turn
  for( ichrt = 0; ichrt < telmnt[0].maxcohorts; ++ichrt )
  {
    for( dm = 0; dm < nrun; ++dm ) { stepCohort( ichrt, dm ); }

    closeCohort( ichrt );
  }

  Ttem45::setCutStep( -1 );  // the equilibrium of the next cell closes years normally
 
};


