#!/bin/bash

paths=(
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/australia/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/canada/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/china/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/france/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/japan/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/norway/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/united_kingdom/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/united_states_1/scripts/Xbakeoff.ipynb
/c/Users/jmkod/OneDrive/Desktop/Lehigh\ University/Research/Data/tem/processing/united_states_2/scripts/Xbakeoff.ipynb
)

 
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






