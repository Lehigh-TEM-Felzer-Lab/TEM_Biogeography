#!/bin/bash

# array of source directories
src_dirs=(
"/t/m8/kodero/runs/historical/hist_original_lc"
"/t/m8/kodero/runs/australia"
"/t/m8/kodero/runs/canada"
"/t/m8/kodero/runs/china"
"/t/m8/kodero/runs/france"
"/t/m8/kodero/runs/japan"
"/t/m8/kodero/runs/norway"
"/t/m8/kodero/runs/united_kingdom"
"/t/m8/kodero/runs/usa_ccsm4_MAIN/west"
"/t/m8/kodero/runs/usa_gfdl"
)

# array of destination directories
dest_dirs=(
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/historical/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/australia/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/canada/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/china/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/france/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/japan/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/norway/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/united_kingdom/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/united_states_1/data"
"/c/Users/jmkod/OneDrive/Desktop/Lehigh University/Research/Data/tem/processing/united_states_2/data"
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