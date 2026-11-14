// atmdepdat_gds.cpp
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Changed: The record is read from a NetCDF stream (read) instead of a comma-separated text file
//          (getdel).

#include "atmdepdat_gds.h"
#include "tem_netcdf_io.h"

int Atmdepdat::read( tem::nc::Stream* infile )
{
  tem::nc::Record r;
  const int atmdepend = tem::nc::next( infile, r );
  if( 1 == atmdepend )
  {
    lon = r.lon; lat = r.lat; year = r.year; adepval = (float) r.v[0];
  }
  return atmdepend;
}
