/* *************************************************************
****************************************************************
TBIOME_GDS.H - object describing general characteristics of
                 vegetation mosaic used in the Terrestrial
                 Ecosystem Model (TEM)

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Changed: Parameters are read with tem::yaml::Reader (tem_yaml.h) instead of ProcessXML, because the
         parameter files are YAML.
20241109 Fixed: Numbers are parsed strictly: a missing key or a value that is not a number stops the run, with
         the file and key named (atoi and atof read such values as 0).

****************************************************************
************************************************************* */

#ifndef TBIOME_GDS_H
#define TBIOME_GDS_H

#include "temconsts_gds.hpp"
#include "tem_yaml.h"


class Biome45
{

  public:

     Biome45( void );

 /* ************************************************************
		 Public Functions
************************************************************* */

     int   getCommunityType( const int& tveg );
     int    getVegMosaic( const int& tveg );
     double getVegSubarea( const int& tveg,
                           const int& dtype,
                           const float& carea );
      int   getVegSubtype( const int& tveg, const int& dtype );
     void   getvtype( const string& ecd );

/* *************************************************************
		 Public Variables
************************************************************* */

     // biome type or ecozone (categorical data)
     int temveg;

     // vegetation community type (categorical data)
     int cmnt;

     // number of community types in a vegetation mosaic
     int numtype[NUMVEG];

     // community types of a vegetation mosaic
     int subtype[NUMVEG][NUMMSAC];

     // percent coverage of a community type in a vegetation
     //   mosaic
     double pcttype[NUMVEG][NUMMSAC];

     //Description of a vegetation community type

     string cmnt_name;

     // Area covered by a vegetation community type

     float subarea;

};

#endif

