/* *************************************************************
ELMNT_GDS.H - Contains functions to manage elements used in GIS
             (Note: Elements are usually grid cells)

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Changed: col and row renamed lon and lat.
20241109 Changed: the names have the suffix 50; nothing else.

************************************************************** */

#ifndef ELMNT_GDS_H
#define ELMNT_GDS_H

class Elmnt45
{

   public:

     Elmnt45( void );

/* **************************************************************
                 Public Functions
************************************************************** */

     int coregerr( std::ostream& rflog1,
                   const string& varname1,
                   const float& lon1,
                   const float& lat1,
                   const string& varname2,
                   const float& lon2,
                   const float& lat2 );

     void show( std::ostream& rflog1,
                const float& lon,
                const float& lat );

     void show( std::ostream& rflog1,
                const float& lon,
                const float& lat,
                const long& totyr,
                const double& tol,
                const double& errcnt );

/* **************************************************************
                 Public Variables
************************************************************** */

     // Column or longitude of element
     float lon;

     // Area of element
     float carea;

     // Continent location of element
     string contnent;

     // Count of elements in a region
     long count;

     // Mean Elevation of element
     double elev;

     int end;

     long grdcnt;

     long numskip;

     // Row or latitude of element
     float lat;

     int stopflag;

     int strtflag;

     int  totyr;
     
     int endflag;
     
     long numgrids;


   private:

/* **************************************************************
                 Private Variables
************************************************************** */

};

#endif

