/* *************************************************************
TMAXCOHORTDAT_GDS.H - object to read and write the structure of
                       maximum number of cohorts grid cell data
                       from/to files used by the
                       Terrestrial Ecosystem Model

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: The record is read from a NetCDF stream (read) instead of a text file; the text readers and
         writers (get, getdel, out, outdel) are removed.

****************************************************************
************************************************************* */

#ifndef TMAXCOHORTDAT_GDS_H
#define TMAXCOHORTDAT_GDS_H

#include <string>

namespace tem::nc { class Stream; }

class MaxCohortdat45
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

     // area covered by grid cell (sq. km)
     float carea;

     // longitude of grid cell (degrees)
     float lon;

     // name of continent containing grid cell
     std::string contnent;

     // number of cohorts representing the
     //   original (or potential) vegetation
     //   in a grid cell
     int natchrts;

     // latitude of grid cell (degrees)
     float lat;

     // total number of cohorts in a grid cell
     int total;

     // "MAXNCHRT"
     std::string varname;

     // date represented by data
     int year;
};

#endif

