#!/bin/bash

set -euo pipefail

echo "<<<Starting Biogeography Model >>>"
cd .
# Directories
tem_biogeography="."
tem_core="$tem_biogeography/tem_core"
runs="$tem_biogeography/runs"




# Functions
clean_tem_core() {
  cd "$tem_core"
  echo "Removing all .o files..."
  rm -f *.o./
  wait  
  echo "Done!"
}

clean_tem_core


compile_tem() {
  echo "Compiling tem..."
  make -f Makefile_biogeo.xtem xtem45_biogeo
  wait
  echo "Done!"
}

compile_tem

copy_executable() {
  echo "Copying tem executable to run directory..."
  cp xtem45_biogeo "../runs"
  wait
  cd "../"
  echo "Done!"
}

copy_executable

remove_temout_files() {
  
  echo "Removing all .TEMOUT files from directory..."
  rm -f "$runs"/*.TEMOUT
  wait
  echo "Done!"
}

cd "$runs"
remove_temout_files "$runs"

remove_unnecessary_files() {
  echo "Removing unnecessary files..."
  rm -f "$runs"/FIRE.csv
  rm -f "$runs"/FIRE_VARS.csv
  rm -f "$runs"/MMDI.csv
  rm -f "$runs"/*.log
  echo "Done!"
  
}

remove_unnecessary_files

run_tem_executable() {
  echo "Running TEM executable..."
  ./xtem45_biogeo 
  wait
  echo "Done!"
}


run_tem_executable
wait
echo "TEM run complete"

biogeography_and_post_processing(){
	
  pwd
  

  echo "Running biogeography model..."
  python main.py
  wait 
  echo "Done!"
  echo "Creating Vegetation Maps.."
  python vegetation_maps.py 
  wait 	
  echo "Done!"
  echo "Running xtran .."
  python xtran.py <<< xbatch.xml
  wait
  echo "Done!"
  
}

biogeography_and_post_processing

echo "Complete!"

