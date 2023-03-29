#!/bin/bash

# array of source directories
src_dirs=(
"/runs/historical/hist_original_lc"
"/runs/australia"
"/runs/canada"
"/runs/china"
"/runs/france"
"/runs/japan"
"/runs/norway"
"/runs/united_kingdom"
"/runs/usa_ccsm4_MAIN/west"
"/runs/usa_gfdl"
)

# array of destination directories
dest_dirs=(
"/processing/historical/data"
"/processing/australia/data"
"/processing/canada/data"
"/processing/china/data"
"/processing/france/data"
"/processing/japan/data"
"/processing/norway/data"
"/processing/united_kingdom/data"
"/processing/united_states_1/data"
"/processing/united_states_2/data"
)
# loop through all source directories
for ((i=0;i<${#src_dirs[@]};i++))
do
  src_dir=${src_dirs[i]}
  dest_dir=${dest_dirs[i]}

  
  # copy .SUMMARY files from source to destination directories
  cp "$src_dir"/*.csv "$dest_dir"/

      # show progress
  echo -ne "[$((i+1))/${#dest_dirs[@]}]\r"
  printf "\033[2K\r"
  echo -ne "[$((i+1))/${#dest_dirs[@]}]\033[32m DONE\033[0m\n"
  done