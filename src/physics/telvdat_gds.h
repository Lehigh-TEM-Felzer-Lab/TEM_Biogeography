/* *************************************************************
TELVDAT_GDS.H - object to read and write the structure of
                 elevation data from/to files used by the Water
                 Balance Model

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers and
         writers (get, getdel, out, outdel) are removed.

****************************************************************
************************************************************* */

#ifndef TELVDAT_GDS_H
#define TELVDAT_GDS_H

#include <string>

namespace tem::nc { class Stream; }

class Elevdat45
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

     // "ELEV"
     std::string varname;

     // area covered by grid cell (sq. km)
     float carea;

     // elevation of grid cell (m)
     double elev;

     // name of continent containing grid cell
     std::string contnent;
};

#endif

