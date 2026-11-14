/* *************************************************************
****************************************************************
TLCLUC_GDS.H - determines both potential and "actual" land
               cover, and identifies land uses within a grid
               cell

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Changed: Land cover is not read from text files here: the members cohorts and lulc and the functions
         getCohort and getNumberOfCohorts are removed. The driver reads the NetCDF land-cover stream.

Land-cover inputs are read directly by the model driver.

****************************************************************
************************************************************* */

#ifndef TLCLUC_GDS_H
#define TLCLUC_GDS_H

const int MXFRI = 2000;

// Tlcluc45 uses the MaxCohortdat45 class
#include "tmaxcohortdat_gds.h"

// Tcluc45 uses the Lulcdat45 class
#include "lulcdat_gds.h"

// Tlcluc45 inherits the Biome45 class
#include "tbiome_gds.h"

class Tlcluc45 : public Biome45
{

  public:

     Tlcluc45();

     int agcmnt;
     int currentveg;
     int endyr;

     int lastyr;

     int maxtype;

     int potveg;

     int startyr;

     // portion of carea in a particular cohort, km^2
     float subarea;

     int tlulcflag;

};

#endif
