// lulcdat_gds.cpp
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers
//          and writers (get, getdel, out, outdel) are removed.

#include "lulcdat_gds.h"
#include "tem_netcdf_io.h"

int Lulcdat45::read( tem::nc::Stream* infile )
{
  tem::nc::Record r;
  const int lulcend = tem::nc::next( infile, r );
  if( 1 == lulcend )
  {
    lon = r.lon; lat = r.lat; varname = r.varname; year = r.year;
    icohort = r.index; isrccohort = r.index;  // replaced below when the file gives the source cohort
    standage = (int) r.v[0]; chrtarea = (float) r.v[1]; potveg = (int) r.v[2];
    currentveg = (int) r.v[3]; subtype = (int) r.v[4]; agstate = (int) r.v[5];
    agprevstate = (int) r.v[6]; disturbflag = (int) r.v[7];
    if( r.v.size() > 8 && r.v[8] >= 1.0 ) { isrccohort = (int) r.v[8]; }  // 1-based index of the cohort a new cohort splits from
    tillflag = 0; fertflag = 0; irrgflag = 0; disturbmonth = 0;
    vconvert = 0.0; prod10par = 0.0; prod100par = 0.0; sconvert = 0.0;
    region = r.region;
  }
  return lulcend;
}
