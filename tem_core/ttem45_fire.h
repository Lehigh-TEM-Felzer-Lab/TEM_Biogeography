
#ifndef TEMFIRE45_H
#define TEMFIRE45_H

bool isFireTrue(double vegc, double sm, double wiltpt, double awcapmm, double vpr, double vpdn, double vpdd,
                int vegtype, double col, double row, int pdm,
                // output
                double &rh, double &theta, double &THETA_E, double &FIRE_PROBABILITY_THRESHOLD, double &fireRandomness,
                double &severity, double &fRH, double &ftheta, double &Ni, double &fb, double &fm, double &fs,
                double &Nf, double &fireProbability, double &dwood, double &dleaf);

#endif
