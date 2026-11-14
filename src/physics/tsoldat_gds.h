/* *************************************************************
TSOLDAT_GDS.H - object to read and write the structure of soil
                 texture data from/to files used by the
                 Terrestrial Ecosystem Model (TEM)

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers and
         writers (get, getdel, out, outdel) are removed.

************************************************************* */

#ifndef TSOLDAT_GDS_H
#define TSOLDAT_GDS_H

#include <string>

namespace tem::nc { class Stream; }

class Soildat45
{

  public:


/* *************************************************************
		    Public Functions
************************************************************* */

     int read( tem::nc::Stream* infile );

/* *************************************************************
		     Public Variables
************************************************************* */

     // longitude of grid cell (degrees)
     float lon;

     // latitude of grid cell (degrees)
     float lat;

     // "TEXTURE"
     std::string varname;

     // area covered by grid cell (sq. km)
     float carea;

     // percent sand of grid cell's soil texture
     double pctsand;

     // percent silt of grid cell's soil texture
     double pctsilt;

     // percent clay of grid cell's soil texture
     double pctclay;

     // wetland soil type designation (categorical data)
     int wsoil;

     // reference to data source
     std::string source;

     // name of continent containing grid cell
     std::string contnent;
};

#endif

