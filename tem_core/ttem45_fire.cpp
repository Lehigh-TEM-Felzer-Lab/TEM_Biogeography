
#include "ttem45_fire.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime> 
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <algorithm>  


// Set the seed for the random number generator
static std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
static std::uniform_real_distribution<double> distribution(0.0, 1.0);



bool isFireTrue(double vegc, double sm, double wiltpt, double awcapmm, double vpr, double vpdn, double vpdd,
                  int vegtype, double col, double row, int pdm,double ws,
                  // output
                  double &rh, double &theta, double &theta_e, double &fire_probability_threshold,
                  double &fireRandomness, double &severity, double &fRH, double &ftheta, double &Ni, double &fb,
                  double &fm, double &fs, double &Nf, double &fireProbability, double &dwood, double &dleaf,double &Ag, double &Ab)
{
    // Define constants for the model
    const double UPPER_FUEL_THRESHOLD = 1050.0;    // gCm^-2
    const double LOWER_FUEL_THRESHOLD = 155.0;     // gCm^-2
    const double UPPER_RH_THRESHOLD = 70.0;        // %
    const double LOWER_RH_THRESHOLD = 30.0;        // %
    const double EPSILON = 1e-6;                   // a small number to avoid division by zero

    // Fire Variables
    bool fire = false; // fire occurrence
    rh = vpr / ((vpdn + vpdd) / 2.0 + vpr) * 100.0; // relative humidity
    theta = (sm - wiltpt) / awcapmm;                // soil wetness
    theta_e = 0.69;                                // soil moisture threshold 
    fire_probability_threshold = 0.5;              // minimum fire probability threshold
    severity = 0;                                    // fire severity


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
    
    if (vegtype == 4)
    {
        umax = 0.11;
    }
    else if (vegtype == 8)
    {
        umax = 0.11;
    }
    else if (vegtype == 9)
    {
        umax = 0.15;
    }
    else if (vegtype == 10)
    {
        umax = 0.11;
    }
    else if (vegtype == 13)
    {
        umax = 0.2;
    }
    else if (vegtype == 15)
    {
        umax = 0.17;
    }
    else if (vegtype == 19)
    {
        umax = 0.11;
    }
    else if (vegtype == 33)
    {
        umax = 0.11;
    }
    else
    {
        umax = 0.11;
    }

    double up = umax * C_beta * rh * gW;
    Ab =((M_PI* (pow(up,2)) *(pow(tau,2)) )/ (4 * LB)) * pow((1 + 1 / HB), 2) * 1e-6;


    double lambda = row; // latitude of the grid cell (initializing to row for now)
    double Il = 0.0;     // lightning flashes (per month)
    double Dp = 0.0;     // population density of the grid cell
  
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
            // Hash the values of the tuple into a single size_t value
            return std::hash<double>()(std::get<0>(k)) ^ std::hash<double>()(std::get<1>(k)) ^
                   std::hash<double>()(std::get<2>(k));
        }
    };

    std::unordered_map<std::tuple<double, double, double>, Data, KeyHash> data_map;
    std::string line;
    std::ifstream fireData("./climate_data/fire.data");

    if (!fireData)
    {
        std::cerr << "Error : File containing area, population density and lightning flashes not found!" << std::endl;
        exit(1);
    }

    double file_lon, file_lat, file_month, file_area, population_density_value, file_flashes;
    while (std::getline(fireData, line))
    {
        std::stringstream ss(line);
        char delim;
        ss >> file_lon >> delim >> file_lat >> delim >> file_month >> delim >> file_area >> delim >>
            population_density_value >> delim >> file_flashes;

        std::tuple<double, double, double> key(file_lon, file_lat, file_month);
        Data value{file_area, population_density_value, file_flashes};
        data_map[key] = value;
    }
    fireData.close();

    

    auto it = data_map.find(std::make_tuple(col, row, pdm + 1));
    if (it == data_map.end())
    {
        std::cerr << "Error: No data found for area, population density and lightning flashes! : " << col << " " << row << " " << pdm + 1 << std::endl;
        exit(1);
    }
    else
    {
        Data &data = it->second;

        // Extract relevant variables from the data
        Il = data.flashes;            // lightning flashes (per month) 
        Dp = data.population_density; // population density of the grid cell
        Ag = data.area;               // area of the grid cell

      if (vegtype == 8)
      {
          Ag = Ag / 4; // Each PFT is 1/4 of the grid cell
          Il = Il / 4;
           Dp = Dp / 4;
      }
      else
        {
            Ag = Ag / 3; // Each PFT is 1/3 of the grid cell
            Il = Il/3;
            Dp = Dp/3;
        }
     
        
        
    }

  
    // Calculate In using Equation (4)
    int n = 1; //  number of time steps in a month
    double psi = 1 / (5.16 + 2.16 * cos(3 * lambda));
    double In = psi * Il;

    // Calculate Ia (ignition due to anthropogenic sources)
    double alpha = 3.89e-3;
    double k_Dp = 6.8 * pow(Dp, -0.6);
    double Ia = (alpha * Dp * k_Dp) / n;

    // Calculate Ni (Ignition Factor)
     Ni = (In + Ia) * Ag;

    // Calculate fs (fire suppressed by humans)
    double epsilon1 = 0.99;
    double epsilon2 = 0.98;
    fs = epsilon1 - epsilon2 * exp(-0.025 * Dp);

 

    // Calculate fuel availability
  fb = (vegc - LOWER_FUEL_THRESHOLD) / (UPPER_FUEL_THRESHOLD - LOWER_FUEL_THRESHOLD);
    if (fb < 0.0)
    {
    fb = 0.0;
    }   else if (fb > 1.0)
    {
    fb = 1.0;
    }

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
    ftheta = exp(-M_PI * pow(theta / theta_e, 2));
    if (theta > theta_e)
    {
        ftheta = 0.0;
    }

    // Calculate fuel combustibility
    fm = fRH * ftheta;

    // Maximum fire count in a grid cell
    double Nf_max = Ni;

    // Calculate  Nf (number of fires)
     Nf = Ni * fb * fm * (1.0 - fs);
     

     // adjust area burned based on fire count
    Ab *= (Nf/Nf_max);


    // Calculate fire probability
    fireProbability = Nf / Nf_max;


    // Calculate fire severity
    severity =  fb*fm;



// Calculate fire randomness

    double randomValue1 = distribution(generator);
    double randomValue2 = distribution(generator);
    double randomValue3 = distribution(generator);
    fireRandomness = (sin(randomValue1 * M_PI) + cos(randomValue2 * M_PI * 2.0) + randomValue3) / 3.0;

    // // Bias the fire probability based on factors.
    double environmentFactor = 1 - (fb * fm * (1.0 - fs)); // This is the probability of fire given fuel, moisture, and suppression

    if ((Ab > 0) && (fireRandomness < environmentFactor))
    {

        fire = true; // set to true if probability is high enough and random factor is less than probability
    }

    // if (fireProbability >= fire_probability_threshold && fireRandomness < fireProbability)
    // {
    //     fire = true; // set to true if probability is high enough and random factor is less than probability
    // }









    // Set dwood and dleaf based on vegtype dwood and dleaf are the combustion completeness of wood and leaf, taken from CLM 5.0
    if (vegtype == 4)
    {
        dwood = 0.175;
        dleaf = 0.725;
    }
    else if (vegtype == 8)
    {
        dwood = 0.15;
        dleaf = 0.725;
    }
    else if (vegtype == 9)
    {
        dwood = 0.20;
        dleaf = 0.75;
    }
    else if (vegtype == 10)
    {
        dwood = 0.10;
        dleaf = 0.70;
    }
    else if (vegtype == 13)
    {
        dwood = 0.00;
        dleaf = 0.85;
    }
    else if (vegtype == 15)
    {
        dwood = 0.30;
        dleaf = 0.80;
    }
    else if (vegtype == 19)
    {
        dwood = 0.15;
        dleaf = 0.725;
    }
    else if (vegtype == 33)
    {
        dwood = 0.15;
        dleaf = 0.70;
    }

    dwood *=(Ab/Ag);
    dleaf *=(Ab/Ag); 

    return fire;  // Assuming fire is a boolean variable defined in fire.cpp
}
