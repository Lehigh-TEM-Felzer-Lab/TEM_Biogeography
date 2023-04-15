# The Terrestrial Ecosystem Model (TEM) + Biogeography

The Terrestrial Ecosystem Model (TEM) is a process-based ecosystem model that describes carbon, nitrogen and water dynamics of plants and soils for terrestrial ecosystems of the globe. 
The TEM uses spatially referenced information on climate, elevation, soils and vegetation as well as soil- and vegetation-specific parameters to make estimates of important carbon, nitrogen and water fluxes and pool sizes of terrestrial ecosystems. 
The TEM normally operates on a monthly time step and at a 0.5 degrees latitude/longitude spatial resolution, but the model has been applied at finer spatial resolutions (down to 1 hectare).


This Model has to be compiled using a C++ compiler.


Version built based on TEM-Hydro, Biogeography in based on BIOME4 Model

# Getting Started
## Directory Structure and Requirements for Running the Code

This program requires the following directory structure and files:

```
root_directory/
├── tem_in.txt
├── biogeo/
│   ├── input/
│   │   ├── MMDI.csv
│   │   ├── MDI.csv
│   │   ├── temp-gdd-prec.csv
│   │   └── bio_limit.csv
│   ├── output_bakeoff/

```

tem_in.txt is a text file containing the path to an XML file used for the TEM model. The user should run the program in the directory where TEM model was run, specifically in the TEM_Biogeography/runs directory that contains tem_in.txt.

The biogeo directory contains the input and output files for the biogeography module of the TEM model, and main.py is the main program file for this module.

The input directory within biogeo contains the following input files:

MMDI.csv: mean monthly moisture stress
MDI.csv: mean annual moisture stress
temp-gdd-prec.csv: climate data
bio_limit.csv: climate limits
The output_bakeoff directory



main.py is the main program file that imports and executes the necessary modules and functions to run the TEM biogeography model.

To run the program, the user must have Python 3 installed and all the necessary dependencies installed. They should navigate to the TEM_Biogeography/runs directory and run the main.py, or compiled excutable file. The program will then read the path to the XML file from tem_in.txt and use it to parse the necessary input and output files for the biogeography module of the TEM model. The output files will be generated in the output_bakeoff directory within the biogeo directory.

   





# Model framework

![tem_biogeog](https://user-images.githubusercontent.com/47959376/228410012-0da8310f-9e86-4e41-ad2f-d1de4f6ebb91.svg)


