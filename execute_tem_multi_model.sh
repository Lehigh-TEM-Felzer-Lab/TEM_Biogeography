#!/bin/bash

# Define the hosts
hosts=(
  monocacy
  monocacy2
  monocacy4
  monocacy6
  monocacy7
  monocacy8
)


set -e # Exit immediately if a command exits with a non-zero status.

# Navigate to tem_core directory
cd /tem_core

# Remove all .o files
echo "Removing all .o files..."
rm -f *.o

# Compile tem
echo "Compiling tem..."
make -f Makefile_biogeo.xtem xtem45_biogeo

# Copy tem executable to all directories 
echo "Copying tem executable to run directory..."

wait 60

#cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/historical/hist_west      #for historical compile tem with ttem45_biogeo_historical_fire.cpp
#cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/historical/hist_original_lc     #for historical compile tem with ttem45_biogeo_historical_fire.cpp
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/australia
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/canada
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/china
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/france
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/japan
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/norway
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/united_kingdom
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/usa_ccsm4_MAIN/west
cp xtem45_biogeo /TEM_Biogeography/runs/multi_model/usa_gfdl



# Remove all .csv files from the historical path
cd /TEM_Biogeography/runs/multi_model/historical/hist_west
ssh monocacy6 "rm -f *.csv"

# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo_oldfire executable on monocacy8
./xtem45_biogeo_oldfire > junk &

wait

# Remove all .csv files from the historical path2
cd /TEM_Biogeography/runs/multi_model/historical/hist_original_lc
ssh monocacy7 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo_oldfire executable on monocacy7
./xtem45_biogeo_oldfire > junk &

sleep 30


# Remove all .csv files from australia path
cd /TEM_Biogeography/runs/multi_model/australia
ssh monocacy2 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy2
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from canada path
cd /TEM_Biogeography/runs/multi_model/canada
ssh monocacy4 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy4
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from china path
cd /TEM_Biogeography/runs/multi_model/china
ssh monocacy6 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy6
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from france path
cd /TEM_Biogeography/runs/multi_model/france
ssh monocacy7 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy7
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from japan path
cd /TEM_Biogeography/runs/multi_model/japan
ssh monocacy8 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy8
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from norway path
cd /TEM_Biogeography/runs/multi_model/norway
ssh monocacy2 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy2
./xtem45_biogeo > junk &
sleep 30

# Remove all .csv files from united_kingdom path
cd /TEM_Biogeography/runs/multi_model/united_kingdom
ssh monocacy4 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy4
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from usa_ccsm4_MAIN/west path
cd /TEM_Biogeography/runs/multi_model/usa_ccsm4_MAIN/west
ssh monocacy6 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy6
./xtem45_biogeo > junk &

sleep 30

# Remove all .csv files from usa_gfdl path
cd /TEM_Biogeography/runs/multi_model/usa_gfdl
ssh monocacy7 "rm -f *.csv"
# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv

# Run the xtem45_biogeo executable on monocacy7
./xtem45_biogeo > junk &
sleep 30



# array of source directories
src_dirs=(
"/TEM_Biogeography/runs/multi_model/historical/hist_original_lc"
"/TEM_Biogeography/runs/multi_model/australia"
"/TEM_Biogeography/runs/multi_model/canada"
"/TEM_Biogeography/runs/multi_model/china"
"/TEM_Biogeography/runs/multi_model/france"
"/TEM_Biogeography/runs/multi_model/japan"
"/TEM_Biogeography/runs/multi_model/norway"
"/TEM_Biogeography/runs/multi_model/united_kingdom"
"/TEM_Biogeography/runs/multi_model/usa_ccsm4_MAIN/west"
"/TEM_Biogeography/runs/multi_model/usa_gfdl"
)


# array of destination directories
dest_dirs=(
"/TEM_Biogeography/processing/historical/data"
"/TEM_Biogeography/processing/australia/data"
"/TEM_Biogeography/processing/canada/data"
"/TEM_Biogeography/processing/china/data"
"/TEM_Biogeography/processing/france/data"
"/TEM_Biogeography/processing/japan/data"
"/TEM_Biogeography/processing/norway/data"
"/TEM_Biogeography/processing/united_kingdom/data"
"/TEM_Biogeography/processing/united_states_1/data"
"/TEM_Biogeography/processing/united_states_2/data"
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

paths=(
/TEM_Biogeograph/processing/australia/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/canada/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/china/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/france/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/japan/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/norway/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/united_kingdom/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/united_states_1/scripts/biogeography_and_bakeoff_module.ipynb
/TEM_Biogeograph/processing/united_states_2/scripts/biogeography_and_bakeoff_module.ipynb
)

 # Path to jupyter-nbconvert.exe
jupyter_nbconvert="C:/ProgramData/Python/Python311/Scripts/jupyter-nbconvert.exe"

function convert_to_script {
  file_path=$1
  # cd into the given path
  cd "$(dirname "$file_path")"

  # convert jupyter notebook to python script
  "$jupyter_nbconvert" --to script "$(basename "$file_path")"

  # execute the python script
  python "${file_path%.*}.py"
}

function main {
  num_files=${#paths[@]}
  for ((i=0; i<num_files; i++))
  do
    if [ -f "${paths[$i]}" ]; then
      convert_to_script "${paths[$i]}"
      # show progress
      echo -ne "[$((i+1))/$num_files]\r"
      printf "\033[2K\r"
      echo -ne "[$((i+1))/$num_files]\033[32m DONE\033[0m\n"
    else
      echo "${paths[$i]} does not exist" >&2
    fi

    # delete the python script
    rm "${paths[$i]%.*}.py"
  done
  echo
}


main

#!/bin/bash

# Define the directories where the .csv files are located
directories=(
  /TEM_Biogeography/processing/historical/hist_original_lc/data/output_bakeoff
  /TEM_Biogeography/processing/australia/data/output_bakeoff
  /TEM_Biogeography/processing/canada/data/output_bakeoff
  /TEM_Biogeography/processing/china/data/output_bakeoff
  /TEM_Biogeography/processing/france/data/output_bakeoff
  /TEM_Biogeography/processing/japan/data/output_bakeoff
  /TEM_Biogeography/processing/norway/data/output_bakeoff
  /TEM_Biogeography/processing/united_kingdom/data/output_bakeoff
  /TEM_Biogeography/processing/usa_ccsm4_MAIN/west/data/output_bakeoff
  /TEM_Biogeography/processing/usa_gfdl/data/output_bakeoff
  )


# For Historical copy the original .csv files to the data/output_bakeoff directory  
cp /TEM_Biogeography/processing/historical/hist_original_lc/*.csv /TEM_Biogeography/processing/historical/hist_original_lc/data/output_bakeoff

# Change path to the data/output_bakeoff directory
cd /TEM_Biogeography/processing/historical/hist_original_lc/data/output_bakeoff/

# Remove files not needed
rm -f  FIRE.csv FIRE_VARS.csv MMDI.csv HISTORICAL_NIRR.csv 


# Define the name of the batch file
batch_file="xbatch"

# Loop through all the directories
for dir in "${directories[@]}"; do
  # Change the current directory to the directory with .csv files
  cd "$dir"
  
  # Loop through all the .csv files in the directory
  for file in *.csv; do
    # Change the first line of the batch file with the current file name
    sed -i "1s/.*/$file/" "$batch_file"

    # Check if the current directory is /TEM_Biogeography/processing/historical/hist_original_lc/data/output_bakeoff
    if [ "$dir" == "/TEM_Biogeography/processing/historical/hist_original_lc/data/output_bakeoff" ]; then
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
    
    # Run the data/output_bakeoff executable
    ./data/output_bakeoff45_cohortfix < "$batch_file"

    sleep 1
  done
done











