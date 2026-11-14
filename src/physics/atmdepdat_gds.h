/* *************************************************************
ATMDEPDAT.H - record of the atmospheric nitrogen deposition of a grid
               cell, read from NetCDF

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: The record is read from a NetCDF stream (read) instead of a comma- separated text file
         (getdel).

************************************************************* */

#ifndef ATMDEPDAT_GDS_H
#define ATMDEPDAT_GDS_H

namespace tem::nc { class Stream; }

class Atmdepdat
{

  public:


/* *************************************************************
                      Public Functions
************************************************************* */

// read data structure.
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

     // ndep value
     float adepval;
};

#endif

