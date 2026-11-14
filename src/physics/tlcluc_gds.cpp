/* **************************************************************
*****************************************************************
TLCLUC_GDS.CPP - determines both potential and "actual" land
                 cover, and identifies land uses within a grid
                 cell

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Changed: Land cover is not read from text files here: getCohort and the file names are removed. The
         driver reads the NetCDF land-cover stream.

Land-cover inputs are read directly by the model driver.

*****************************************************************
************************************************************** */

#include <string>

using std::string;

#include "tlcluc_gds.h"

Tlcluc45::Tlcluc45() : Biome45()
{
  maxtype = -999;
  agcmnt = -999;
  cmnt = -999;
  lastyr = -999;
}
