#!/bin/bash

set -euo pipefail

cd .
# Directories
tem_core="/tem_core"
tem_biogeography="."
runs="$tem_biogeography/runs"
biogeography_module="$tem_biogeography/biogeography_module/module"
data="$biogeography_module/biogeo"
output_bakeoff="$data/output_bakeoff"
output_summary="$runs/output_summary"



# Functions
clean_tem_core() {
  cd "$tem_core"
  echo "Removing all .o files..."
  rm -f *.o
}

compile_tem() {
  echo "Compiling tem..."
  make -f Makefile_biogeo.xtem xtem45_biogeo
}

copy_executable() {
  echo "Copying tem executable to run directory..."
  cp xtem45_biogeo "$runs"
}

remove_temout_files() {
  local dir="$1"
  echo "Removing all .csv files from $dir directory..."
  rm -f "$dir"/*.TEMOUT
}

remove_unnecessary_files() {
  echo "Removing unnecessary files..."
  rm -f FIRE.csv FIRE_VARS.csv MMDI.csv
}

run_tem_executable() {
  echo "Running the executable..."
  ./xtem45_biogeo > junk &
  wait
}


# Main script
clean_tem_core
compile_tem
copy_executable

cd "$runs"
remove_temout_files "$runs"
remove_unnecessary_files
run_tem_executable


cd "$biogeography_module/module"
python main.py

wait 
# Define the directories where the .csv files are located
directories=(
  /TEM_Biogeography/runs/biogeo/output_bakeoff
)

# Define the name of the batch file
batch_file="xbatch"

# Loop through all the directories
for dir in "${directories[@]}"; do
  # Change the current directory to the directory with .temout files
  cd "$dir"

  # Loop through all the .csv files in the directory
  for file in *.csv; do
    # Change the first line of the batch file with the current file name
    sed -i "1s/.*/$file/" "$batch_file"

    # Check if the current directory is /m8/kodero/runs/historical/hist_original_lc/xtran
    if [ "$dir" == "/m8/kodero/runs/historical/hist_original_lc/xtran" ]; then
      # Take the first part of the file name
      first_part=$(echo "$file" | awk -F'.' '{print $1}')

      # Replace the 28th line of the batch file with the first part of the file name appended to the original name
      modified_line="ORIGINAL_LULC_$first_part.SUMMARY"
      sed -i "28s/.*/$modified_line/" "$batch_file"
    else
      # Change the 28th line of the batch file with the upper case first part of the file name and appended _BAKEOFF.SUMMARY
      first_part=$(echo "$file" | awk -F'_' '{print $1}')
      upper_case=$(echo "$first_part" | tr '[:lower:]' '[:upper:]')
      modified_line="$upper_case"_BAKEOFF.SUMMARY
      sed -i "28s/.*/$modified_line/" "$batch_file"
    fi

    # Run the xtran executable
    echo "Running the xtran executable..."
    ./xtran45_cohortfix < "$batch_file"

    sleep 1
  done
done

# Array of source directories
src_dirs=(
  /TEM_Biogeography/runs/biogeo/output_bakeoff
)

# Array of destination directories
dest_dirs=(
   /TEM_Biogeography/runs/biogeo/output_summary
)

# Loop through all source directories
for ((i=0;i<${#src_dirs[@]};i++)); do
  src_dir=${src_dirs[i]}
  dest_dir=${dest_dirs[i]}

  # Remove existing .SUMMARY files in destination directories
  echo "Removing existing .SUMMARY files in destination directories..."
  rm "$dest_dir"/*.SUMMARY 2> /dev/null

  # Copy .SUMMARY files from source to destination directories
  echo "Copying .SUMMARY files from source to destination directories..."
  cp "$src_dir"/*.SUMMARY "$dest_dir"/

  # Show progress
  echo "[$((i+1))/${#dest_dirs[@]}] DONE"
done

model="/TEM_Biogeography/runs/biogeo/output_summary"

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

# Process .SUMMARY files
echo "Processing .SUMMARY files..."
process_summary_files "$model"

# Display completion message
echo "################################################### 100%"
echo "________________Finished processing .SUMMARY Files_________________________"
echo "_________________________Execution Done______________________________________"

