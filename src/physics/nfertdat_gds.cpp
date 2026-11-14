// nfertdat_gds.cpp
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Updated: annual nitrogen fertilizer rate of a cell, read from NetCDF.

#include "nfertdat_gds.h"
#include "tem_netcdf_io.h"

Nfertdat::Nfertdat( void )
{

  nfertval = 0.0;
}

int Nfertdat::read( tem::nc::Stream* infile )
{

  tem::nc::Record r;

  const int nfertend = tem::nc::next( infile, r );

  if( 1 == nfertend )
  {
    lon = r.lon;
    lat = r.lat;
    year = r.year;
    nfertval = (float) r.v[0];
  }

  return nfertend;
}
