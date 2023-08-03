#!/bin/bash

set -euo pipefail

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
  echo "Done!"
}

compile_tem() {
  echo "Compiling tem..."
  make -f Makefile_biogeo.xtem xtem45_biogeo
  echo "Done!"
}

copy_executable() {
  echo "Copying tem executable to run directory..."
  cp xtem45_biogeo "../runs"
  cd "../"
  echo "Done!"
}



remove_temout_files() {
  
  echo "Removing all .TEMOUT files from directory..."
  rm -f "$runs"/*.TEMOUT
  echo "Done!"
}

remove_unnecessary_files() {
  echo "Removing unnecessary files..."
  rm -f "$runs"/FIRE.csv
  rm -f "$runs"/FIRE_VARS.csv
  rm -f "$runs"/MMDI.csv
  echo "Done!"
  
}

run_tem_executable() {
  echo "Running TEM executable..."
  ./xtem45_biogeo > junk &
  wait
  echo "Done!"
}


# Main script
clean_tem_core
compile_tem
copy_executable

cd "$runs"
remove_temout_files "$runs"
remove_unnecessary_files
run_tem_executable


# copy "tem_in.txt" and "*.xml" in runs to biogeography_module/module
cp "tem_in.txt"  "../tem_biogeography/module"
cp "*.xml" "../tem_biogeography/module"
cd "../"
echo "Done!"

cd "biogeography_module/module"
echo "Running biogeography model..."
python main.py
echo "Done!"

wait 

cd "../../"
cd "xtran/new_xtran"
echo "Running Xtran .."
python xtran.py

wait

echo "Done!"

echo "Complete!"


