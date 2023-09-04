#!/bin/bash

set -euo pipefail

# Function to display progress bar
function progress_bar() {
    local width=50
    local percentage=$1
    local completed=$((percentage * width / 100))
    local remaining=$((width - completed))
    echo " "

    printf " Progress : [%s%s] %.2f%%\r" "$(yes "#" | head -n "$completed" | tr -d '\n')" "$(yes "-" | head -n "$remaining" | tr -d '\n')" "$percentage"
    echo " "
}

echo " 0 1 2 3 4 5 6 7 8 9 "
echo " "
echo "<<< Starting Biogeography Model >>>"
echo " "
cd .

# Directories
tem_biogeography="."
tem_core="$tem_biogeography/tem_core"
runs="$tem_biogeography/runs"

# Clean TEM core
clean_tem_core() {
    cd "$tem_core"
    echo "Removing all .o files"
    rm -f *.o
    wait
   progress_bar 3
    
}

clean_tem_core



# Compile TEM
compile_tem() {
    echo " "
    echo "Compiling TEM"
    echo " "

    make -f Makefile_biogeo.xtem xtem45_biogeo
    wait
   
    progress_bar 5
}

compile_tem

# Copy executable
copy_executable() {
    echo " "
    echo "Copying tem executable to run directory."
    cp xtem45_biogeo "../runs"
    wait
    cd "../"
   
    progress_bar 6
}

copy_executable

# Remove TEMOUT files
remove_temout_files() {
    echo " "
    echo "Removing all .TEMOUT files from runs dir..."
    rm -f "$runs"/*.TEMOUT
    wait
   
    progress_bar 7
}

cd "$runs"
remove_temout_files "$runs"


# Remove unnecessary files
prep_run_dir() {
    echo " "
    echo "Preparing run directory."
    rm -f FIRE.csv
    rm -f FIRE_VARS.csv
    rm -f MMDI.csv
    rm -f *.log
    cp -f ../biogeography_module/* .
    cp -f ../xtran/* .
    echo " "

    progress_bar 8
}

prep_run_dir

# Run TEM executable
run_tem_executable() {
     echo " "
    echo "Running TEM "
    echo " "
    ./xtem45_biogeo
    wait
   
    progress_bar  60
}

run_tem_executable
wait


# Biogeography and post-processing
biogeography_and_post_processing() {
    pwd
    echo " "
    echo "Running biogeography model..."
    python main.py 
    wait
   
    progress_bar 80
     echo " "
    echo "Creating Vegetation Maps..."
    python vegetation_maps.py 
    wait
   
    progress_bar 90
     echo " "
    echo "Running xtran..."
    python xtran.py  xbatch.xml
    wait
   
    progress_bar  95
}

biogeography_and_post_processing
 echo " "
progress_bar  100
echo "Complete!"
