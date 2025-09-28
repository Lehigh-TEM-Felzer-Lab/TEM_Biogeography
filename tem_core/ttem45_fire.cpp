#include "ttem45_fire.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <tuple>
#include <unordered_map>

/* Reference 

// Lawrence, D. et al. CLM5.0 technical description. (2018).
// Li, F., Zeng, X. D. & Levis, S. A process-based fire parameterization of intermediate complexity in a dynamic global vegetation model. Biogeosciences 9, 2761–2780 (2012).

*/




// load fire data into a map
struct Data
{
    double area;
    double population_density;
    double flashes;
};

struct KeyHash
{
    size_t operator()(const std::tuple<double, double, double> &k) const
    {
        return std::hash<double>()(std::get<0>(k)) ^ std::hash<double>()(std::get<1>(k)) ^
               std::hash<double>()(std::get<2>(k));
    }
};

std::unordered_map<std::tuple<double, double, double>, Data, KeyHash> &load_fire_data()
{
    static bool data_loaded = false;
    static std::unordered_map<std::tuple<double, double, double>, Data, KeyHash> fire_data_map;

    if (data_loaded) // if data is already loaded, return the map
        return fire_data_map;

        

    FILE *fireData = fopen("./climate_data/global_0.5_deg_mean_mon_fire.data", "r");

    if (!fireData)
    {
        perror( "Error: Could not open fire data file at ./climate_data/global_0.5_deg_mean_mon_fire.data\n"
                "Expected format: lon,lat,month,area,population_density,flashes (CSV)\n"
                "Check that the file exists, is readable, and has six comma-separated values per line.\n");
        exit(1);
    }

    double file_lon, file_lat, file_month, file_area, population_density_value, file_flashes;
    while (fscanf(fireData, "%lf,%lf,%lf,%lf,%lf,%lf", &file_lon, &file_lat, &file_month, &file_area,
                  &population_density_value, &file_flashes) == 6)
    {
        std::tuple<double, double, double> key(file_lon, file_lat, file_month);
        Data value{file_area, population_density_value, file_flashes};
        fire_data_map[key] = value;
    }
    fclose(fireData);

    data_loaded = true; // mark data as loaded
    return fire_data_map;
}

// Fire model

bool isFireTrue(double col, double row, int year, int month, int vegtype, double vegc, double sm, double wiltpt,
                double awcapmm, double 
                ws, double vpr, double vpdn, double vpdd,double temperature,

                // output
                double &rh, double &theta, double &theta_e, double &fireProbability, double &fire_probability_threshold,
                double &fireRandomness, double &severity, double &fb, double &fRH, double &ftheta, double &fm,
                double &fs, double &Ni, double &Nf, double &Ag, double &Ab, double &dwood, double &dleaf)

{

    // Set the seed for the random number generator
    std::random_device rd; 
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    // Define constants for the model
    const double MISSING = -99999;
    const double UPPER_FUEL_THRESHOLD = 1050.0; // gCm^-2
    const double LOWER_FUEL_THRESHOLD = 155.0;  // gCm^-2
    const double UPPER_RH_THRESHOLD = 70.0;     // %
    const double LOWER_RH_THRESHOLD = 30.0;     // %
    const double LOWER_TEMPERATURE_THRESHOLD = 0.0; // C
    const double UPPER_TEMPERATURE_THRESHOLD = 21.0; // C
    const double EPSILON = 1e-6;                // a small number to avoid division by zero

    // Fire Variables
    bool fire = false;                              // fire occurrence
    rh = vpr / ((vpdn + vpdd) / 2.0 + vpr) * 100.0; // relative humidity
    theta = (sm - wiltpt) / awcapmm;                // soil wetness
    theta_e = 0.69;                                 // soil moisture threshold
    fire_probability_threshold = 0.5;               // minimum fire probability threshold
    severity = 0;                                   // fire severity

    // Fire Spread Variables
    double g0 = 0.05;
    double beta_low = 0.3;
    double beta_up = 0.7;
    double tau = 86400; // 1 day average fire duration

    // Compute C_beta using linear function
    double C_beta = beta_low + theta * (beta_up - beta_low);

    double LB = 1.0 + 10.0 * (1.0 - exp(-0.06 * ws));
    double HB = LB + (sqrt(pow(LB, 2) - 1)) / (LB - sqrt(pow(LB, 2) - 1));

    // Compute up
    double gW = (2 * LB / (1 + 1 / HB)) * g0;
    double umax; // Based on the given vegtype, set the umax value

       switch (vegtype)
    {
        case 4:
        case 8:
        case 10:
        case 19:
        case 33:
            umax = 0.11;
            break;

        case 9:
            umax = 0.15;
            break;

        case 13:
            umax = 0.2;
            break;

        case 15:
            umax = 0.17;
            break;

        default:
            umax = 0.11;
            break;
    }

    double up = umax * C_beta * rh * gW;
    Ab = ((M_PI * (pow(up, 2)) * (pow(tau, 2))) / (4 * LB)) * pow((1 + 1 / HB), 2) * 1e-6;

    double lambda = row; // latitude of the grid cell (initializing to row for now)
    double Il = 0.0;     // lightning flashes (per month)
    double Dp = 0.0;     // population density of the grid cell

    auto &fire_data_map = load_fire_data();
    auto it = fire_data_map.find(std::make_tuple(col, row, month));
    if (it == fire_data_map.end())
    {
        std::cerr << "Error: No data found for area, population density and lightning flashes! : " << col << " " << row
                  << " " << month << std::endl;
        exit(1);
    }
    else
    {
        Data &data = it->second;

        // Extract relevant variables from the data
        Il = data.flashes;            // lightning flashes (per month)
        Dp = data.population_density; // population density of the grid cell (Current)
        Ag = data.area;               // area of the grid cell


        if (Dp == MISSING || Il == MISSING)
        {
            fire = false;
            return fire;
        }


    
        // Adjust population density based on year
        if (year >= 1750 && year < 1850){
            Dp=Dp/4;
        }
        else if (year >= 1850 && year < 1950){
            Dp=Dp/2;
        }
        else if (year >= 1950 && year < 2015){
            Dp=Dp;
        }
        else{
            Dp = Dp * pow((1 + 0.0070), (year - 2015)); // Assuming USA population growth rate is abouth 0.70% per year

        }
        
     
        if (vegtype == 8)
        {
            Ag = Ag / 4; // Each PFT is 1/4 of the grid cell (adjust as needed to reflect actual PFT proportions)
            Il = Il / 4;
            Dp = Dp / 4;
        }
        else
        {
            Ag = Ag / 3; // Each PFT is 1/3 of the grid cell (adjust as needed to reflect actual PFT proportions)
            Il = Il / 3;
            Dp = Dp / 3;
        }
    }

    // Calculate In using Equation (4)
    int t_steps = 1; //  number of time steps in a month (change as needed)
    double psi = 1.0 / (5.16 + 2.16 * cos(3.0 * lambda));
    double In = psi * Il;

    // Calculate Ia (ignition due to anthropogenic sources)
    double alpha = 3.89e-3;
    double k_Dp = (Dp > 1e-9) ? (6.8 * pow(Dp, -0.6)) : 0.0; // avoid blowup at Dp=0
    double Ia = (alpha * Dp * k_Dp) / t_steps;

    // Calculate Ni (Ignition Factor)
    Ni = (In + Ia) * Ag;

    // Calculate fs (fire suppressed by humans)
    double epsilon1 = 0.99;
    double epsilon2 = 0.98;
    fs = epsilon1 - epsilon2 * exp(-0.025 * Dp);

    // Calculate temperature factor
    double temperatureFactor = (temperature - LOWER_TEMPERATURE_THRESHOLD) /
                            (UPPER_TEMPERATURE_THRESHOLD - LOWER_TEMPERATURE_THRESHOLD);
    if (temperatureFactor < 0.0)
        temperatureFactor = 0.0;
    else if (temperatureFactor > 1.0)
        temperatureFactor = 1.0;

    // Calculate fuel availability
    fb = (vegc - LOWER_FUEL_THRESHOLD) / (UPPER_FUEL_THRESHOLD - LOWER_FUEL_THRESHOLD);
    if (fb < 0.0)
        fb = 0.0;
    else if (fb > 1.0)
        fb = 1.0;

    // Calculate relative humidity factor
    fRH = 1.0;
    if (rh > UPPER_RH_THRESHOLD)
    {
        fRH = 0.0;
    }
    else if (rh > LOWER_RH_THRESHOLD)
    {
        fRH = (UPPER_RH_THRESHOLD - rh) / (UPPER_RH_THRESHOLD - LOWER_RH_THRESHOLD);
    }

    // Calculate soil wetness factor
    ftheta = exp(-M_PI * pow(theta / theta_e, 2.0));
    if (theta > theta_e)
    {
        ftheta = 0.0;
    }

    // Calculate fuel combustibility
    fm = fRH * ftheta;

    // Maximum fire count in a grid cell
    double Nf_max = Ni;

    // Calculate Nf (number of fires) safely
    Nf = (Ni * fb * fm * (1.0 - fs));

    // Adjust area burned based on fire count
    if (Nf_max > 0.0)
    {
        Ab *= (Nf / Nf_max);
        fireProbability = Nf / Nf_max;
    }
    else
    {
        Ab = 0.0;
        fireProbability = 0.0;
    }

    // Calculate fire severity
    severity = fb * fm;






double percentAreaBurned = (Ab / Ag) * 100;

// < 25% area burned Low 
// 25% - 75% area burned Medium
// > 75% area burned Stand Replacement

// Calculate a risk score
double riskScore = (severity * temperatureFactor * fireProbability);
double fireRandom = distribution(generator); // some randomness to fire occurrence

  if ((percentAreaBurned > 0) && (riskScore > fireRandom))
  {
        fire = true; // set to true if probability is high enough and temperature is high enough
  }
  
switch (vegtype)
{
    case 4:
        dwood = 0.175;
        dleaf = 0.725;
        break;
    case 8:
        dwood = 0.15;
        dleaf = 0.725;
        break;
    case 9:
        dwood = 0.20;
        dleaf = 0.75;
        break;
    case 10:
        dwood = 0.10;
        dleaf = 0.70;
        break;
    case 13:
        dwood = 0.0;
        dleaf = 0.80;
        break;
    case 15:
        dwood = 0.30;
        dleaf = 0.80;
        break;
    case 19:
        dwood = 0.15;
        dleaf = 0.725;
        break;
    case 33:
        dwood = 0.15;
        dleaf = 0.70;
        break;
   default:
        dwood = 0.15;
        dleaf = 0.70;
        return false;  // Return false for unexpected value
}


    dwood *= (Ab / Ag);
    dleaf *= (Ab / Ag);

    return fire; // Assuming fire is a boolean variable defined in fire.cpp
}



bool shouldHistoricalFireOccur(int vegtype, int lastRepFireYear, int year) {

    bool replacementFire = false;
    int yearsSinceLastRepFire = year - lastRepFireYear;

    int fireReturnInterval = 0; 
    double fireReturnIntervalDeviation = 0;

// Add and Update the fire return interval values as needed
switch(vegtype)
{
    case 2:  // alpine tundra (PNW alpine/subalpine grassland and meadow)
        fireReturnInterval = 350;
        fireReturnIntervalDeviation = 0.50;
        break;
    case 4:  // boreal forest
        fireReturnInterval = 265;
        fireReturnIntervalDeviation = 0.50;
        break;
    case 8: // Mixed Temperate Forest
        fireReturnInterval = 170; 
        fireReturnIntervalDeviation = 0.50;
        break;
    case 9:  // temperate coniferous
        fireReturnInterval = 150;
        fireReturnIntervalDeviation = 0.50;
        break;
    case 10:  // temperate deciduous
        fireReturnInterval = 180;
        fireReturnIntervalDeviation = 0.50;
        break;
    case 12: // Tall grassland
        fireReturnInterval = 8;
        fireReturnIntervalDeviation = 0.25;
        break;
    case 13:  // short grassland
        fireReturnInterval = 8;
        fireReturnIntervalDeviation = 0.25;
        break;
    case 15:  // arid shrublands
        fireReturnInterval = 50;
        fireReturnIntervalDeviation = 0.25;
        break;
    case 18://Tropical Deciduous Forests
        fireReturnInterval = 50;
        fireReturnIntervalDeviation = 0.25;
        break;
    case 19: // xeric forests
        fireReturnInterval = 65;
        fireReturnIntervalDeviation = 0.25;
        break;
    case 33: // temperate broadleaved evergreen
        fireReturnInterval = 140;
        fireReturnIntervalDeviation = 0.50;
        break;
    case 35: // Mediterranean shrublands (CA Chaparel or sage shrub)
        fireReturnInterval = 50;
        fireReturnIntervalDeviation = 0.25;
        break;
    default:
        fireReturnInterval = 50;
        fireReturnIntervalDeviation = 0.25;
        return false;  // Return false for unexpected values
}

        
    // Introduce randomness around the fireReturnInterval
    static std::random_device rd;
    static std::mt19937 gen(rd());
    int deviation = static_cast<int>(fireReturnInterval * fireReturnIntervalDeviation);
    std::uniform_int_distribution<> distrib(-deviation, deviation);
    int adjustedFRI = fireReturnInterval + distrib(gen);

    if (yearsSinceLastRepFire >= adjustedFRI) {
        replacementFire = true;
    }

    return replacementFire;
}
