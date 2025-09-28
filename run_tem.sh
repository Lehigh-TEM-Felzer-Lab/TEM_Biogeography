#!/bin/bash

set -euo pipefail

# Function to display progress bar
progress_bar() {
    local width=50
    local percentage=$1
    local completed=$((percentage * width / 100))
    local remaining=$((width - completed))
    printf " Progress : [%s%s] %.2f%%\r" "$(yes "#" | head -n "$completed" | tr -d '\n')" "$(yes "-" | head -n "$remaining" | tr -d '\n')" "$percentage"
}

# Header
echo " "
echo "*************************************************"
echo "***       Starting Biogeography Model        ***"
echo "*************************************************"

# Directories
tem_biogeography="."
tem_core="$tem_biogeography/tem_core"
runs="$tem_biogeography/runs"
echo " "
clean_tem_core() {
    cd "$tem_core"
    echo "*** Removing all .object files ***"
    rm -f *.o
    wait
    progress_bar 3
}

compile_tem() {
    echo " "
    echo "*** Compiling TEM ***"
    echo " "
    make -f Makefile_biogeo.xtem xtem45_biogeo
    wait
    progress_bar 5
}

copy_executable() {
    echo " "
    echo "*** Copying TEM executable to run directory ***"
    cp xtem45_biogeo "../runs"
    wait
    cd "../"
    progress_bar 6
}

remove_temout_files() {
    echo " "
    echo "*** Removing all .temout files from runs dir ***"
    rm -f "$runs"/*.TEMOUT
    wait
    progress_bar 7
}

prep_run_dir() {
    echo " "
    echo "*** Preparing run directory ***"
    rm -f FIRE.csv FIRE_VARS.csv MMDI.csv *.log
    rm -f *.SUMMARY
    rm -f *.TEMOUT
    cp -f ../biogeography_module/* .
    cp -f ../xtran/* .
    wait
    progress_bar 8
}

run_tem_executable() {
    echo " "
    echo "*** Running TEM ***"
    echo " "
    chmod +x xtem45_biogeo
    ./xtem45_biogeo 
    wait
    progress_bar 60
}

biogeography_and_post_processing() {
    echo " "
    echo "*** Running biogeography model ***"
    echo " "
    pwd
    python main.py
    wait
    progress_bar 80
    
    echo " "
    echo "*** Running xtran ***"
    echo " "
    
    python xtran.py xbatch.xml
    progress_bar 90

    echo " "
    echo "*** Creating Vegetation Maps ***"
    python vegetation_maps.py
    wait
    python trends.py
    progress_bar 95
}

clean_dir(){
    echo "*** Post Run Script ***"
    echo " "
    rm -f *.py
    rm -f *.md
    rm -f  xbatch.xml
    rm -f paths.xml
    rm -f *.json
}

# Function calls
clean_tem_core
pwd
compile_tem
pwd
copy_executable
cd "$runs"
pwd
remove_temout_files
pwd
prep_run_dir
pwd
run_tem_executable
pwd
biogeography_and_post_processing
pwd
clean_dir
pwd

# Completion message
echo "*************************************************"
progress_bar 100
echo "*** Complete! ***"
echo "*************************************************"
