/* **************************************************************
*****************************************************************
FIRE45.H - object describes the fraction of a cohort burned by
           wildfire in a time step (GDSTEM-FIRE)

Fire follows Li, F., Zeng, X. D. and Levis, S. (2012) Biogeosciences
  9, 2761-2780 and Lawrence, D. et al. (2018) CLM5.0 technical
  description, as in GDSTEM-FIRE (ttem45_fire.h).

Version: XTEM version 4.5
20241111 Changed: the fire is that of GDSTEM-FIRE: class Tfire, fireburn() and the ignitions of its
         driver (ignitions()). Replaces the earlier fire module.

*****************************************************************
************************************************************** */

#ifndef FIRE45_H
#define FIRE45_H

class Tfire
{

  public:

     Tfire();

/* *************************************************************
		 Public Functions
************************************************************* */

     // Fraction (0 to 1) of the cohort burned in the step. vegtype is the current vegetation type (the crop and
     //   impervious types 50 to 54 do not burn), vegc the vegetation carbon (g C m-2), sm the soil wetness
     //   (y[I_VSM]), ws the wind speed (m s-1), temperature the air temperature (degrees C), rootc the root carbon,
     //   resp_h and root_rm the heterotrophic and root maintenance respiration (g C m-2), rh the relative humidity
     //   (%), Ni the ignitions (ignitions()), gdp the GDP per capita, pop the population density (km-2), elev the
     //   elevation (m), fsr and fd the fire spread rate and duration of the community (vegetation file) and lwft the
     //   fraction of the cell in natural cover. gdp, pop or Ni of -99999 mean that the value is missing.

     double fireburn( int vegtype, double vegc, double sm,
                      double ws, double temperature, double rootc, double resp_h, double root_rm,
                      double rh, double Ni,
                      double gdp, double pop, double elev, double fsr, double fd, double lwft );

     // Ignitions of the step, as in the driver of GDSTEM-FIRE: lightning ig (flashes km-2 of the step), population pop
     //   (km-2), the longitude of the cell, the ignition efficiency igef of the community (vegetation file), the
     //   fraction of the cell in natural cover lwft and in broadleaf forest forarea
     double ignitions( double ig, double pop, double lon, double igef, double lwft, double forarea ) const;

     // burned_area ****************************************************

     inline double getBURNED_AREA( void ) { return burned_area; }

     inline void setBURNED_AREA( const double& pburned_area ) { burned_area = pburned_area; }

     int enabled;   // veg_fire in tem_config.yaml

  private:

     double burned_area;

};

#endif
