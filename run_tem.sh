#!/bin/bash

set -euo pipefail

# Define colors
#RED="\033[0;31m"
CYAN="\033[0;36m"
#BLUE="\033[0;34m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
NC="\033[0m" # No Color

# Function to display progress bar
progress_bar() {
    local width=50
    local percentage=$1
    local completed=$((percentage * width / 100))
    local remaining=$((width - completed))
    echo -e "${YELLOW}"
    printf " Progress : [%s%s] %.2f%%\r" "$(yes "#" | head -n "$completed" | tr -d '\n')" "$(yes "-" | head -n "$remaining" | tr -d '\n')" "$percentage"
    echo -e "${NC}"
}

# Header
echo " "
echo -e "${GREEN}*************************************************${NC}"
echo -e "${GREEN}***       Starting Biogeography Model        ***${NC}"
echo -e "${GREEN}*************************************************${NC}"

# Directories
tem_biogeography="."
tem_core="$tem_biogeography/tem_core"
runs="$tem_biogeography/runs"
echo " "
clean_tem_core() {
    cd "$tem_core"
    echo -e "${CYAN}*** Removing all .object files ***${NC}"
    rm -f *.o
    wait
    progress_bar 3
}

compile_tem() {
    echo " "
    echo -e "${CYAN}*** Compiling TEM ***${NC}"
    echo " "
    make -f Makefile_biogeo.xtem xtem45_biogeo
    wait
    progress_bar 5
}

copy_executable() {
    echo " "
    echo -e "${CYAN}*** Copying TEM executable to run directory ***${NC}"
    cp xtem45_biogeo "../runs"
    wait
    cd "../"
    progress_bar 6
}

remove_temout_files() {
    echo " "
    echo -e "${CYAN}*** Removing all .temout files from runs dir ***${NC}"
    rm -f "$runs"/*.temout
    wait
    progress_bar 7
}

prep_run_dir() {
    echo " "
    echo -e "${CYAN}*** Preparing run directory ***${NC}"
    rm -f fire.csv fire_vars.csv mmdi.csv *.log
    cp -f ../biogeography_module/* .
    cp -f ../xtran/* .
    wait
    progress_bar 8
}

run_tem_executable() {
    echo " "
    echo -e "${CYAN}*** Running TEM ***${NC}"
    echo " "
    chmod +x xtem45_biogeo
    ./xtem45_biogeo
    wait
    progress_bar 60
}

biogeography_and_post_processing() {
    echo " "
    echo -e "${CYAN}*** Running biogeography model ***${NC}"
    echo " "
    python main.py
    wait
    progress_bar 80
    
    echo " "
    echo -e "${CYAN}*** Running xtran ***${NC}"
    echo " "
    python xtran.py xbatch.xml
    progress_bar 90

    echo " "
    echo -e "${CYAN}*** Creating Vegetation Maps ***${NC}"
    python vegetation_maps.py
    wait
    progress_bar 95
    
   
}

# Function calls
clean_tem_core
compile_tem
copy_executable
cd "$runs"
remove_temout_files
prep_run_dir
run_tem_executable
biogeography_and_post_processing

# Completion message
echo -e "${YELLOW}***${NC}"
progress_bar 100
echo -e "${GREEN}*** Complete! ***${NC}"
echo -e "${YELLOW}*************************************************${NC}"
