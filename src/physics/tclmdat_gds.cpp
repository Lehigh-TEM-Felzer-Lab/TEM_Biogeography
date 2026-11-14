// tclmdat_gds.cpp
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers
//          and writers (get, getdel, out, outdel) are removed.
// 20241110 Changed: Yearly and per-step arrays are std::vector sized at run time (CYCLE, NYEARS, NCOHORTS);
//          MAXCYCLE, MAXRTIME and MAXCHRTS removed.

#include "tclmdat_gds.h"
#include "tem_netcdf_io.h"

int Clmdat45::read( tem::nc::Stream* ifile )
{
  tem::nc::Record r;
  const int clmend = tem::nc::next( ifile, r );
  if( 1 == clmend )
  {
    lon = r.lon; lat = r.lat; varname = r.varname; carea = (int) r.area; year = r.year;
    total = r.total; max = r.max; ave = r.ave; min = r.min;
    mon.assign( r.mon.begin(), r.mon.begin() + tem::nc::steps() );
    contnent = r.region;
  }
  return clmend;
}
