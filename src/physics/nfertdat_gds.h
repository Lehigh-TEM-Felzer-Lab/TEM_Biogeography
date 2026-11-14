/* *************************************************************
NFERTDAT.H - object to read the annual nitrogen fertilizer rate
             of a grid cell (crop or pasture) from NetCDF

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Updated: Reads the annual nitrogen fertilizer rate of a cell (crop or pasture) from NetCDF.

************************************************************* */

#ifndef NFERTDAT_H
#define NFERTDAT_H

#include <cstdio>

namespace tem::nc { class Stream; }

class Nfertdat
{

  public:

     Nfertdat( void );

/* *************************************************************
		 Public Functions
************************************************************* */

     // read data structure; returns -1 at the end of the data

     int read( tem::nc::Stream* infile );

/* *************************************************************
		 Public Variables
************************************************************* */

     // longitude of grid cell (degrees)
     float lon;

     // latitude of grid cell (degrees)
     float lat;

     // year
     int year;

     // fertilizer rate
     float nfertval;
};

#endif
