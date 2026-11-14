/* *************************************************************
LULCDAT_GDS.H - object to read and write the structure of land
                  use / land cover cohort data from/to files

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers and
         writers (get, getdel, out, outdel) are removed.

************************************************************* */

#ifndef LULCDAT_GDS_H
#define LULCDAT_GDS_H

#include <string>

namespace tem::nc { class Stream; }

class Lulcdat45
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

     // flag whether or not grid cell was cultivated the previous
     //   year

     int agprevstate;

     // Flag to indicate land-use of cohort. Different values
     //   represent different land uses:
     //   0 = no land use (natural land cover)
     //   1 = row-crop agriculture
     int agstate;

     // area of a cohort within a grid cell
     float chrtarea;

     // longitude of grid cell (degrees)
     float lon;

     // Index of current vegetation type
     int currentveg;

     // Flag to indicate that a disturbance has occurred
     //   in a particular year.  Different values represent
     //   different types of disturbances:
     //   0 = no disturbance
     //   1 = conversion to agriculture
     //   2 = timber harvest
     //   3 = fire
     int disturbflag;

     // Month in which disturbance occurred
     // (e.g. 1 = January, ...., 12 = December)
     int disturbmonth;

     // Flag to indicate whether cropland in grid cell
     //   experiences optimum fertilizer application
     int fertflag;

     // Cohort index
     int icohort;

     // Flag to indicate whether cropland in grid cell
     //   experiences optimum irrigation
     int irrgflag;

     // Index of cohort that was source of current cohort
     int isrccohort;

     // percent of grid cell covered with agriculture
     double pctag;

     // Index of potential vegetation biome type
     int potveg;

     // Proportion of vegetation carbon extracted for
     // 10-year wood products
     double prod10par;

     // Proportion of vegetation carbon extracted for
     // 100-year wood products
     double prod100par;

     // name of region containing cohort
     std::string region;

     // latitude of grid cell (degrees)
     float lat;

     // Proportion of soil organic carbon lost
     // during a disturbance
     double sconvert;

     // Stand age (years)
     int standage;

     // Index for vegetation subtype
     int subtype;

     // Flag to indicate whether cropland in grid cell
     //   is tilled ( = 1 ) or not ( = 0 )
     int tillflag;

     // varname = "AGRICUL" for this data set
     std::string varname;

     // Proportion of vegetation carbon lost
     // during a disturbance
     double vconvert;

     // Year data represents
     int year;
};

#endif

