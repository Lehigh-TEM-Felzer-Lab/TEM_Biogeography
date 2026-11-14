// telvdat_gds.cpp
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers
//          and writers (get, getdel, out, outdel) are removed.

#include "telvdat_gds.h"
#include "tem_netcdf_io.h"

int Elevdat45::read( tem::nc::Stream* infile )
{
  tem::nc::Record r;
  const int elvend = tem::nc::next( infile, r );
  if( 1 == elvend )
  {
    lon = r.lon; lat = r.lat; varname = r.varname; carea = r.area; elev = r.v[0];
    contnent = r.region;
  }
  return elvend;
}
