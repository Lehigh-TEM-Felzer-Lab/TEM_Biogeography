/* *************************************************************
TCLMDAT_GDS.H - object to read and write the structure of the
                climate data from files used by the Terrestrial
                Ecosystem Model (TEM)

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: Monthly arrays are sized MAXCYCLE for the run step.
20241110 Changed: Yearly and per-step arrays are std::vector sized at run time (CYCLE, NYEARS, NCOHORTS);
         MAXCYCLE, MAXRTIME and MAXCHRTS removed.

************************************************************* */

#ifndef TCLMDAT_GDS_H
#define TCLMDAT_GDS_H

#include "temconsts_gds.hpp"

#include <string>

namespace tem::nc { class Stream; }

class Clmdat45
{

  public:


/* *************************************************************
		      Public Functions
************************************************************* */

     // read data structure.

     int read( tem::nc::Stream* ifile );

/* *************************************************************
		     Public Variables
************************************************************* */

     // longitude of grid cell (degrees)
     float lon;

     // latitude of grid cell (degrees)
     float lat;

     // climate variable name
     std::string varname;

     // area covered by grid cell (sq. km)
     float carea;

      // date (year) of data
     //long year;
     int year;

     // annual sum of monthly data for grid cell
     double total;

      // maximum monthly value for grid cell
     double max;

     // mean annual value for grid cell
     double ave;

     // minimum monthly value for grid cell
     double min;

     // monthly values for the grid cell
     std::vector<double> mon = std::vector<double>( CYCLE );

      // name of continent containing grid cell
     std::string contnent;
};

#endif
