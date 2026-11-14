/* **************************************************************
FIRE45.CPP - fraction of a cohort burned by wildfire in a time
             step (GDSTEM-FIRE, ttem45_fire.cpp)

References:

Lawrence, D. et al. CLM5.0 technical description. (2018).
Li, F., Zeng, X. D. & Levis, S. A process-based fire parameterization of intermediate complexity in a
  dynamic global vegetation model. Biogeosciences 9, 2761-2780 (2012).

20241111 Changed: the fire is that of GDSTEM-FIRE. The factors of the human effects start at 1 (no effect) so that a
         cell with little population or a vegetation type without an entry does not use an undefined value.
************************************************************** */

#include <algorithm>
#include <cmath>

#include "fire.h"

using std::exp;
using std::cos;
using std::pow;
using std::min;
using std::max;

Tfire::Tfire()
  : enabled( 0 ),
    burned_area( 0.0 )
{
};

/* *************************************************************
************************************************************* */

double Tfire::ignitions( double ig, double pop, double lon, double igef, double lwft, double forarea ) const
{
  double lh = 0.0;
  double fs = 0.0;

  if( pop > -99999.0 )
  {
    lh = 0.0035 * 6.8 * pow( pop, 0.43 ) / 30.0 / 24.0;
    fs = 1.0 - (0.01 + 0.98 * exp( -0.025 * pop ));
  }

  if( ig > -99999.0 )
  {
    const double natural = ig / (5.16 + 2.16 * cos( M_PI / 180.0 * 3 * min( 60.0, fabs( lon ) ) ) * igef) * (1.0 - fs) * pow( lwft, 0.5 );

    if( forarea <= 0.6 ) { return lh + natural; }

    return natural;
  }

  return ig;
}

/* *************************************************************
************************************************************* */

double Tfire::fireburn( int vegtype, double vegc, double sm, double ws, double temperature,
                        double rootc, double resp_h, double root_rm, double rh,
                        double Ni, double gdp, double pop, double elev, double fsr, double fd, double lwft )
{
    double fuelc, afuel, arh, arh30, fire_m, cnfire_params_prh30, nfire, topoi, fb, fs;

    if (vegtype == 50 || vegtype == 51 || vegtype == 52 || vegtype == 53 || vegtype == 54)
    {
        burned_area = 0.0;
        return burned_area;
    }

    cnfire_params_prh30 = 0.6;

    // fire_m is the combustibility of fuel for fire occurrence

    // fuelc is the biomass of combined leaf, stem, litter, and woody debris pools
    fuelc = vegc - rootc + resp_h + root_rm;

    // fuel availability
    fb = max(0.0, min(1.0, (fuelc - 75.0) / 575.0));

    // afuel is the weight for arh and arh30
    afuel = min(1.0, max(0.0, (fuelc - 2500.0) / 2500.0));

    // arh and arh30 are the combustibility of fuel related to RH and RH30
    arh = 1.0 - max(0.0, min(1.0, (rh - 30.0) / 50.0));

    arh30 = 1.0 - max(cnfire_params_prh30, min(1.0, rh / 90.0));

    if ( (rh < 80.0) && (temperature > 0.0) )
    {
        fire_m = pow((afuel * arh30 + (1.0 - afuel) * arh), 1.5) * pow((1.0 - sm), 0.5);
    }
    else
    {
        fire_m = 0.0;
    }

    // fs: fire suppressed by humans

    const double epsilon1 = 0.99;
    const double epsilon2 = 0.98;

    if (pop > -99999)
    {
        fs = epsilon1 - epsilon2 * exp(-0.025 * pop);
    }
    else
    {
        fs = 0.0;
    }

    // reduce burnability at high elevations
    if (elev <= 2500.0)
    {
        topoi = 1.0;
    }
    else
    {
        topoi = 0.004;
    }

    // length-to-breadth ratio of the ellipse
    double LB = 1.0 + 10.0 * (1.0 - exp(-0.06 * ws));

    // combustibility of fuel for fire spread
    double spread_m = pow(fire_m, 0.5);

    // society effects over the vegetation types
    double lgdp = 1.0;
    double lgdp1 = 1.0;
    double lpop = 1.0;
    double fd_f;
    const double occur_hi_gdp_tree = 0.33;

    if (pop > 0.1)
    {
        switch (vegtype)
        {
            case 1: case 6: case 7: case 13: case 14: case 15:
            case 16: case 17: case 18: case 28: case 29:
            case 30: case 31: case 32: case 33:
                break;

            case 12: case 19: case 20: case 21: case 22: case 26: case 27:
                // lgdp, the effect of economic conditions on fire occurrence
                lgdp = (0.1 + 0.9 * exp(-1.0 * M_PI * pow(gdp / 8.0, 0.5)));

                // lgdp1, the economic impact factor on the average spread area of a fire
                lgdp1 = (0.2 + 0.8 * exp(-1.0 * M_PI * gdp / 7.0));

                // lpop, the demographic impact factor on the average spread area of a fire
                lpop = (0.2 + 0.8 * exp(-1.0 * M_PI * pow(pop / 450.0, 0.5)));
                break;

            case 2: case 3: case 4: case 5:
            case 8: case 9: case 10: case 11:
            case 23: case 24: case 25:
                if (gdp > 20.0)
                {
                    lgdp = occur_hi_gdp_tree;
                    lgdp1 = 0.62;
                }
                else if (gdp > 8.0)
                {
                    lgdp = 0.79;
                    lgdp1 = 0.83;
                }
                else
                {
                    lgdp = 1.0;
                    lgdp1 = 1.0;
                }
                lpop = (0.4 + 0.6 * exp(-1.0 * M_PI * (pop / 125.0)));
                break;
        }
    }

    if (gdp == -99999)
    {
        lgdp = 0.0;
    }

    // fd, the fire duration (h), in seconds
    fd_f = fd * 3600.0;

    // fire counts
    if (Ni > -99999)
    {
        nfire = Ni * fb * fire_m * lgdp * topoi;
    }
    else
    {
        nfire = 0.0;
    }

    fd_f = pow((lwft * lgdp * lpop), 0.5) * fd_f;

    burned_area = min(1.0, pow((0.05 * spread_m * fs * fd_f / 1000), 2.0) * nfire * M_PI * LB);

    return burned_area;
}
