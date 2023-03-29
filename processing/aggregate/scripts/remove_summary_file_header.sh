#!/bin/bash

# array of source directories
src_dirs=(
    /runs/historical/hist_original_lc/xtran
    /runs/australia/xtran
    /runs/canada/xtran
    /runs/china/xtran
    /runs/france/xtran
    /runs/japan/xtran
    /runs/norway/xtran
    /runs/united_kingdom/xtran
    /runs/usa_ccsm4_MAIN/west/xtran
    /runs/usa_gfdl/xtran
)

# array of destination directories
dest_dirs=(
    "/processing/aggregate/data/historical"
    "/processing/australia/data/output_summary"
    "/processing/canada/data/output_summary"
    "/processing/china/data/output_summary"
    "/processing/france/data/output_summary"
    "/processing/japan/data/output_summary"
    "/processing/norway/data/output_summary"
    "/processing/united_kingdom/data/output_summary"
    "/processing/united_states_1/data/output_summary"
    "/processing/united_states_2/data/output_summary"
)

# loop through all source directories
for ((i=0;i<${#src_dirs[@]};i++))
do
  src_dir=${src_dirs[i]}
  dest_dir=${dest_dirs[i]}

  # remove existing .SUMMARY files in destination directories
  rm "$dest_dir"/*.SUMMARY 2> /dev/null

  # copy .SUMMARY files from source to destination directories
  cp "$src_dir"/*.SUMMARY "$dest_dir"/
    
   # show progress
  echo -ne "[$((i+1))/${#dest_dirs[@]}]\r"
  printf "\033[2K\r"
  echo -ne "[$((i+1))/${#dest_dirs[@]}]\033[32m DONE\033[0m\n"
  done


model_1="/processing/australia/data/output_summary/"
model_2="/processing/canada/data/output_summary/"
model_3="/processing/china/data/output_summary/"
model_4="/processing/france/data/output_summary/"
model_5="/processing/japan/data/output_summary/"
model_6="/processing/norway/data/output_summary/"
model_7="/processing/united_kingdom/data/output_summary/"
model_8="/processing/united_states_1/data/output_summary/"
model_9="/processing/united_states_2/data/output_summary/"
historical_model="/processing/aggregate/data/historical/"

process_summary_files() {
  local directory="$1"
  for filename in "$directory"*.SUMMARY; do
    if [ -f "$filename" ]; then
      sed '1,11d
        /Information on grids rejected for analysis/d
        /EZ, NGRID, TOTCELLAREA/d
        s/ *[(]10[*][*]6m[*][*]2[)]//
      ' "$filename" > "$filename.tmp"
      mv "$filename.tmp" "$filename"
    fi
  done
}

process_summary_files "$model_1"
process_summary_files "$model_2"
process_summary_files "$model_3"
process_summary_files "$model_4"
process_summary_files "$model_5"
process_summary_files "$model_6"
process_summary_files "$model_7"
process_summary_files "$model_8"
process_summary_files "$model_9"
process_summary_files "$historical_model"

echo "Finished processing .SUMMARY files"



