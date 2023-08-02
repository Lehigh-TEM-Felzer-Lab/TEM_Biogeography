#!/bin/bash

set -euo pipefail

cd .
# Directories
tem_biogeography="."
tem_core="$tem_biogeography/tem_core"
runs="$tem_biogeography/runs"
biogeography_module="$tem_biogeography/biogeography_module/module"
data="$biogeography_module/biogeo"
output_bakeoff="$data/output_bakeoff"
process="$tem_biogeography/xtran/new_xtran"



# Functions
clean_tem_core() {
  cd "$tem_core"
  echo "Removing all .o files..."
  rm -f *.o
  echo "Done!"
}

compile_tem() {
  echo "Compiling tem..."
  make -f Makefile_biogeo.xtem xtem45_biogeo
  echo "Done!"
}

copy_executable() {
  echo "Copying tem executable to run directory..."
  cp xtem45_biogeo "$runs"
  echo "Done!"
}


remove_temout_files() {
  
  echo "Removing all .csv files from $dir directory..."
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
  cd "$runs"
  echo "Running the executable..."
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

cp "$runs"/tem_in.txt "$biogeography_module/module"
cp "$runs"/*.xml "$biogeography_module/module"
echo "Done!"

cd "$biogeography_module/module"
python main.py
echo "Done!"

wait 

cd "$process"
python xtran.py

wait

echo "Done!"

echo "Complete!"


