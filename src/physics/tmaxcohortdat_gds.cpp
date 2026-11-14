// tmaxcohortdat_gds.cpp
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers
//          and writers (get, getdel, out, outdel) are removed.

#include "tmaxcohortdat_gds.h"
#include "tem_netcdf_io.h"

int MaxCohortdat45::read( tem::nc::Stream* infile )
{
  tem::nc::Record r;
  const int chrtend = tem::nc::next( infile, r );
  if( 1 == chrtend )
  {
    lon = r.lon; lat = r.lat; varname = r.varname; carea = r.area; year = r.year;
    total = (int) r.v[0]; natchrts = (int) ( r.v.size() > 1 ? r.v[1] : r.v[0] );
    contnent = r.region;
  }
  return chrtend;
}
