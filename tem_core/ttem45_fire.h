
#ifndef TEMFIRE45_H
#define TEMFIRE45_H

bool isFireTrue(double col, double row, int year, int month, int vegtype, double vegc, double sm, double wiltpt,
                double awcapmm, double ws, double vpr, double vpdn, double vpdd,double temperature,

                // output
                double &rh, double &theta, double &theta_e,
                double &fireProbability, double &fire_probability_threshold, double &fireRandomness,double &severity, 
                double &fb, double &fRH, double &ftheta, double &fm, double &fs,
                double &Ni, double &Nf, double &Ag, double &Ab,
                double &dwood, double &dleaf);


bool shouldHistoricalFireOccur(int vegtype, int lastFireYear, int currentYear);

#endif
