import os
import pandas as pd

# Define the directory containing the raw data files
raw_data_dir = "T:\\m8\\kodero\\raw_data\\MACA_DATA"

# Define the columns to be imported for climate data files
climate_data_columns = [
    "LON",
    "LAT",
    "VARNAME",
    "AREA",
    "YEAR",
    "TOTAL",
    "MAX",
    "AVE",
    "MIN",
    "JAN",
    "FEB",
    "MAR",
    "APR",
    "MAY",
    "JUN",
    "JUL",
    "AUG",
    "SEP",
    "OCT",
    "NOV",
    "DEC",
    "REGION"
]

# STEP 1: Import climate data files and set AREA column to corresponding values from extras file
for folder in os.listdir(raw_data_dir):
    folder_path = os.path.join(raw_data_dir, folder)
    if os.path.isdir(folder_path):
        for file in os.listdir(folder_path):
            
            if file.endswith(".climate"):
                # Import the raw data file as a DataFrame
                print(folder)
                print(file)
                file_path = os.path.join(folder_path, file)
                var = file.split(".")[0]
                df = pd.read_csv(file_path, names=climate_data_columns)
                # Import the extras file and merge with the climate data DataFrame
                extras_file_path = "T:\\m8\\kodero\\raw_data\\maca_to_tem\\extra_data.csv"
                df_extras = pd.read_csv(extras_file_path, names=["LON", "LAT", "AREA"])
                df["AREA"] = df_extras["AREA"]
                # Save the merged DataFrame to a CSV file
                
                print()
                print(f"Processing climate data file: {file} in {folder}")
                filename = f"{var}_{folder}_rcp85_2006_2099.csv"
               
                output_dir = os.path.join("T:\\m8\\kodero\\runs", folder, "climate")
                print(filename)
                print(output_dir)
                os.makedirs(output_dir, exist_ok=True)
                output_path = os.path.join(output_dir, filename)
                print(f"Saving climate data file: {filename} to {output_path}")
                df.to_csv(output_path, index=False, header=False)
            

