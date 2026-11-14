/* *************************************************************
ELMNT45_GDS.CPP - Contains functions to manage elements used in GIS

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: the names have the suffix 50; nothing else.

************************************************************** */

//#define BORLAND_CPP

#include<iostream>

  using std::cout;
  using std::ios;
  using std::endl;

#include<iomanip>

  using std::setprecision;

#include<fstream>

  using std::ofstream;

#include<string>

  using std::string;

#ifdef BORLAND_CPP

  #include<time>

  using std::time_t;

#else
  #include<ctime>

  using std::time_t;

#endif

#include "elmnt_gds.h"
#include "mpi_layer.h"
#include "tem_log.h"
#include <cstdio>


Elmnt45::Elmnt45( void )
{

  count = 0;
  numskip  = 0;
  numgrids = 64000;
  grdcnt  = 64000;
  totyr    = -99;
  endflag  = 1;
  stopflag = 0;

  lon = -999.9;
  lat = -999.9;
  carea = -99;
  elev  = -999.9;

  return;
};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

// Progress of the cells. The first call, before any cell has run, only starts the count. Later calls write a line to
// the shared log (tem_log.h) for the first grdcnt cells of the rank and then for every 100th cell.
void Elmnt45::show( std::ostream& rflog1,
                    const float& lon,
                    const float& lat )
{
  (void) rflog1;
  show( rflog1, lon, lat, 0, 0.0, 0.0 );
}

void Elmnt45::show( std::ostream& rflog1,
                    const float& lon,
                    const float& lat,
                    const long& totyr,
                    const double& tol,
                    const double& errcnt )
{
  (void) rflog1;

  const bool started = ( lon > -999990.0f ); // the first call, before any cell, has no position yet

  if( started && ( 1 == count || ( grdcnt != -99 && count <= grdcnt ) || 0 == count % 100 ) )
  {
    const long cellno = ( count - 1 ) * tem::mpi::size() + tem::mpi::rank() + 1 + numskip;
    char d[80];
    std::snprintf( d, sizeof d, "last year %ld, integrator tolerance %.4g", totyr, tol );
    tem::log::cell( cellno, lon, lat, d );
  }

  if( started && errcnt > 0.0 ) { tem::log::warn( "labile carbon error in the cell at lon " + std::to_string( lon ) + ", lat " + std::to_string( lat ) ); }

  if( 0 == endflag && count == numgrids ) { stopflag = 1; }

  ++count;
}

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

int Elmnt45::coregerr( std::ostream& rflog1,
                       const string& varname1,
                       const float& lon1,
                       const float& lat1,
                       const string& varname2,
                       const float& lon2,
                       const float& lat2 )
{

  int fatalerr = 0;

  if( lon1 != lon2 || lat1 != lat2 )
  {
    fatalerr = 1;

    cout << "ERROR:  " << varname1 << " data and ";
    cout << varname2 << "data are not coregistered." << endl;
    cout << "lon = " << lon1 << " and lat = " << lat1;
    cout << " in " << varname1 << " data" << endl;
    cout << "lon = " << lon2 << " and lat = " << lat2;
    cout << " in " << varname2 << " data" << endl;


    rflog1 << "ERROR:  " << varname1 << " data and ";
    rflog1 << varname2 << "data are not coregistered." << endl;
    rflog1 << "lon = " << lon1 << " and lat = " << lat1;
    rflog1 << " in " << varname1 << " data" << endl;
    rflog1 << "lon = " << lon2 << " and lat = " << lat2;
    rflog1 << " in " << varname2 << " data" << endl;
  }

  return fatalerr;

};
