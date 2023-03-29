#!/bin/bash

paths=(
/processing/australia/scripts/biogeography_and_bakeoff_module.ipynb
/processing/canada/scripts/biogeography_and_bakeoff_module.ipynb
/processing/china/scripts/biogeography_and_bakeoff_module.ipynb
/processing/france/scripts/biogeography_and_bakeoff_module.ipynb
/processing/japan/scripts/biogeography_and_bakeoff_module.ipynb
/processing/norway/scripts/biogeography_and_bakeoff_module.ipynb
/processing/united_kingdom/scripts/biogeography_and_bakeoff_module.ipynb
/processing/united_states_1/scripts/biogeography_and_bakeoff_module.ipynb
/processing/united_states_2/scripts/biogeography_and_bakeoff_module.ipynb
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






