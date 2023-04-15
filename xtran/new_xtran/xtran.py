# Created by modifying version of xtran.cpp by Jared M Kodero
# Check dependencies file for required libraries

import warnings
warnings.filterwarnings("ignore")  # setting ignore as a parameter
import warnings
import xml.etree.ElementTree as ET
import pandas as pd
import numpy as np
import os
import re
import sys
import datetime
from pathlib import Path





# Define data columns for TEM output files
var_cols = [
        "LON",
        "LAT",
        "VARIABLE",
        "ICOHORT",
        "STANDAGE",
        "POTVEG",
        "CURRENTVEG",
        "SUBTYPE",
        "CMNT",
        "PSIPLUSC",
        "QLCON",
        "CAREA",
        "SUBAREA",
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
        "REGION",
    ]

units_out_file = "UNITS.INFO"


#Define PFT names based on TEM vegetation codes
pft_description = {
        1: 'Ice',
        4: 'Boreal Forest',
        5: 'Forested Boreal Wetlands',
        6: 'Boreal Woodlands',
        8: 'Mixed Temperate Forests',
        9: 'Temperate Coniferous Forests',
        10: 'Temperate Deciduous Forests',
        11: 'Temperate Forested Wetlands',
        12: 'Tall Grasslands',
        13: 'Short Grasslands',
        14: 'Tropical Savannas',
        15: 'Arid Shrublands',
        16: 'Tropical Evergreen Forests',
        17: 'Tropical Forested Wetlands',
        18: 'Tropical Deciduous Forests',
        19: 'Xeromorphic Forests and Woodlands',
        20: 'Tropical Forested Floodplains',
        21: 'Deserts',
        25: 'Temperate Forested Floodplains',
        27: 'Wet Savannas',
        28: 'Salt Marsh',
        29: 'Mangroves',
        30: 'Tidal Freshwater Marshes',
        31: 'Temperate Savannas',
        32: 'Reserved',
        33: 'Temperate Broadleaved Evergreen Forests',
        34: 'Reserved2',
        35: 'Mediterranean Shrublands',
        36: 'Reserved3',
        37: 'Reserved4',
        38: 'Reserved5',
        39: 'Reserved6',
        40: 'Reserved7',
        41: 'Reserved8',
        42: 'Reserved9',
        43: 'Reserved10',
        44: 'Reserved11',
        45: 'Reserved12',
        46: 'Suburban',
        47: 'Rodale Pasture',
        48: 'Turflawn',
        49: 'Vegetable Farm',
        50: 'Crops',
        51: 'Pasture',
        52: 'Maize',
        53: 'Wheat',
        54: 'Rice',
        55: 'Soybean',
        56: 'Potato',
        "0000": 'All Plant Functional Types',
        '': ''}


# Define the function to process the TEM output files
def process_file(input_filename,filter_params):
      
    df = pd.read_csv(input_filename,names=var_cols)
    df["VARIABLE"] = df["VARIABLE"].str.strip()
    df["REGION"] = df["REGION"].str.strip()
    df["YEAR"] = df["YEAR"].astype(int)
    df["LAT"]=df["LAT"].round(1)
    df["LON"]=df["LON"].round(1)
    
    # Get the first part of the variable name
    variable_parts = df["VARIABLE"].unique()
    variable_first_part = variable_parts[0]
    
    # Define the stats output file name
    stats_out_file = variable_first_part + ".SUMMARY"
    
    
# Define the function to get units
    def get_units(variable, column):
        value = df["VARIABLE"][0]
        if value == "CH4FLUX" or value == "CH4EMISS" or value == "CH4CNSMP":
            if column == "MNBYAR":
                return "mgC/m^2"
            elif column == "TOTFORECOZONE":
                return "(gX10^12)"
        elif value == "VSTRUCTN" or value == "SOILORGN" or value == "VEGN":
            if column == "MNBYAR":
                return "gN/m^2"
            elif column == "TOTFORECOZONE":
                return "Tg"
        elif value in [
            "VSTOREN",
            "AVAILN",
            "NETNMIN",
            "NLOST",
            "NINPUT",
            "VEGNUP",
            "LTRN",
            "MICRONUP",
            "VNMOBIL",
            "VNRESORB",
            "VEGSUP",
            "VEGLUP",
            "N2OFLUX",
        ]:
            if column == "MNBYAR":
                return "mgN/m^2"
            elif column == "TOTFORECOZONE":
                return "(gX10^9)"
        else:
            if column == "MNBYAR":
                return "gC/m^2"
            elif column == "TOTFORECOZONE":
                return "(gX10^12)"
        if column == "TOTCELLAREA":
            return "m^2"
        else:
            return ""

    # Define the columns to get units for
    columns = [
        "TOTCELLAREA",
        "TOTFORECOZONE",
        "MNBYAR"
    ]

    # Create an empty DataFrame to store the results
    df_units = pd.DataFrame(columns=["VARIABLE", "TOTCELLAREA", "TOTFORECOZONE", "MNBYAR", "DATE", "TIME"])

    # get the current date and time
    now = datetime.datetime.now()
    date = now.strftime("%d %B %Y")
    time = now.strftime("%H:%M")

    # get unique variables in the dataframe
    variables = df["VARIABLE"].unique()

    # loop through each variable and column
    for variable in variables:
        row = {"VARIABLE": variable, "DATE": date, "TIME": time}
        for column in columns:
            # get the units for each variable and column
            units = get_units(variable, column)

            # store the units in the row
            row[column] = units

        # append the row to the DataFrame
        df_units = pd.concat([df_units, pd.DataFrame(row, index=[0])], ignore_index=True)

    # save the units to a CSV file
    df_units.to_csv(units_out_file, index=False, header=False, mode='a')

    
    # Define the function to filter the dataframe
    def filter_dataframe(df, filter_params):
            if filter_params is not None:
                filter_criteria = []
                if filter_params.get("lat_min") is not None:
                    try:
                        lat_min = float(filter_params["lat_min"])
                        filter_criteria.append(f"LAT >= {lat_min}")
                    except ValueError:
                        print("\033[91m" +"Invalid value for lat_min. Must be a float." + "\033[0m")
                if filter_params.get("lat_max") is not None:
                    try:
                        lat_max = float(filter_params["lat_max"])
                        filter_criteria.append(f"LAT <= {lat_max}")
                    except ValueError:
                        print("\033[91m" +"Invalid value for lat_max. Must be a float." + "\033[0m")
                if filter_params.get("lon_min") is not None:
                    try:
                        lon_min = float(filter_params["lon_min"])
                        filter_criteria.append(f"LON >= {lon_min}")
                    except ValueError:
                        print("\033[91m" +"Invalid value for lon_min. Must be a float." + "\033[0m")
                if filter_params.get("lon_max") is not None:
                    try:
                        lon_max = float(filter_params["lon_max"])
                        filter_criteria.append(f"LON <= {lon_max}")
                    except ValueError:
                        print("\033[91m" +"Invalid value for lon_max. Must be a float." + "\033[0m")
                if filter_params.get("region") is not None:
                    region = filter_params.get("region")
                    filter_criteria.append(f"REGION == '{region}'")
                if filter_params.get("start_year") is not None:
                    try:
                        start_year = int(filter_params["start_year"])
                        filter_criteria.append(f"YEAR >= {start_year}")
                    except ValueError:
                        print("\033[91m" +"Invalid value for start_year. Must be an integer." + "\033[0m")
                if filter_params.get("end_year") is not None:
                    try:
                        end_year = int(filter_params["end_year"])
                        filter_criteria.append(f"YEAR <= {end_year}")
                    except ValueError:
                        print("\033[91m" +"Invalid value for end_year. Must be an integer." + "\033[0m")

                if filter_criteria:
                    query_string = " and ".join(filter_criteria)
                    try:
                        df = df.query(query_string)
                    except ValueError:
                        print("\033[91m" +"Invalid filter criteria. Please check your parameters and try again." + "\033[0m")

            return df

    # Filter the dataframe based on the filter parameters
    df = filter_dataframe(df, filter_params)

        
    
  
    # Function to calculate summary statistics for each POTVEG and REGION
    def summary_stats(df, group_col):
        summary = df.groupby([group_col, "YEAR", "VARIABLE"]).agg({
            "CAREA": ["count", "sum"],
            "TOTAL": ["max", "min", "mean", "std"],
            "AVE": "mean"
        }).reset_index()
        summary.columns = [
            group_col,
            "YEAR",
            "VARIABLE",
            "NGRID",
            "TOTCELLAREA",
            "MXPRED",
            "MNPRED",
            "MNBYAR",
            "STNDEV",
            "MNTOTYR"
        ]
        summary["TOTCELLAREA"] 
        summary["TOTFORECOZONE"] = (summary["TOTCELLAREA"] * summary["MNBYAR"])/1000000
        summary["SIMPMN"] = summary["MNBYAR"]
        
        return summary

    # Calculate summary statistics for POTVEG 
    summary_stats_potveg = summary_stats(df, "POTVEG")
    summary_stats_potveg  = summary_stats_potveg.round(2)
    summary_stats_potveg["POTVEG"]= summary_stats_potveg["POTVEG"].round().astype(int)
    summary_stats_potveg["YEAR"] = summary_stats_potveg["YEAR"].astype(int).astype(str)


    # Calculate summary statistics for REGION
    summary_stats_region = summary_stats(df, "REGION")
    summary_stats_region= summary_stats_region.round(2)
    summary_stats_region["POTVEG"] = "0000"
    summary_stats_region["YEAR"] = summary_stats_region["YEAR"].astype(int).astype(str)
    

    # Concatenate the two summary statistics dataframes
    summary_stats_final = pd.concat([summary_stats_potveg, pd.DataFrame(np.nan, index=[0], columns=summary_stats_potveg.columns), summary_stats_region])
    summary_stats_final = summary_stats_final.round(2)

    # Drop the REGION column
    summary_stats_final=summary_stats_final.drop("REGION",axis=1)
  
    
    # Add a description column
    summary_stats_final["DESCRIPTION"] = summary_stats_final["POTVEG"].map(pft_description)
    
    # Reorder the columns
    summary_stats_final= summary_stats_final.reindex(columns=['VARIABLE', 'POTVEG','DESCRIPTION', 'YEAR', 'NGRID', 'TOTFORECOZONE', 'MNBYAR', 'MXPRED', 'MNPRED', 'MNTOTYR', 'STNDEV', 'SIMPMN'])

    # Save summary statistics to a CSV file
    summary_stats_final.to_csv(stats_out_file, index=False)

# Get the list of files to process, and optinal filters from the input XML file  
def get_file_list(input_path):
    if input_path.endswith(".xml"):
        tree = ET.parse(input_path)
        root = tree.getroot()
        file_list = []
        for elem in root.findall("files/file"):
            file_path = elem.text
            if file_path.startswith("./") or file_path.startswith(".\\"):
                file_path = os.path.abspath(os.path.join(os.getcwd(), file_path))
            else:
                file_path = os.path.abspath(file_path)
            file_list.append(file_path)
        filter_params = None
        filter_params_elem = root.find("filter_params")
        if filter_params_elem is not None:
            filter_params = {}
            lat_min = filter_params_elem.findtext("lat_min")
            lat_max = filter_params_elem.findtext("lat_max")
            lon_min = filter_params_elem.findtext("lon_min")
            lon_max = filter_params_elem.findtext("lon_max")
            region = filter_params_elem.findtext("region")
            start_year = filter_params_elem.findtext("start_year")
            end_year = filter_params_elem.findtext("end_year")
            if lat_min.strip() or lat_max.strip() or lon_min.strip() or lon_max.strip() or region.strip() or start_year.strip() or end_year.strip():
                filter_params["lat_min"] = round(float(lat_min), 1) if lat_min.strip() else None
                filter_params["lat_max"] = round(float(lat_max), 1) if lat_max.strip() else None
                filter_params["lon_min"] = round(float(lon_min), 1) if lon_min.strip() else None
                filter_params["lon_max"] = round(float(lon_max), 1) if lon_max.strip() else None
                filter_params["region"] = region.strip() if region.strip() else None
                filter_params["start_year"] = int(start_year) if start_year.strip() else None
                filter_params["end_year"] = int(end_year) if end_year.strip() else None
                
        else:
            input_path = input_path.replace("./", "").replace(".\\", "")
            file_list = [os.path.abspath(input_path)]
            filter_params = None
    
    return file_list, filter_params
# Main function
def main():
    
        # Set a default terminal width
    default_terminal_width = 100

    # Try to get the terminal width, and if it fails, use the default width
    try:
        terminal_width = os.get_terminal_size().columns
    except OSError:
        terminal_width = default_terminal_width

    # Print a line of dashes that fills the terminal width
    print('\033[94m_\033[0m' * terminal_width)
    print()
    
    
                        

    # open the file in write mode
    with open(units_out_file, "w") as f:
        # write the string to the file
        f.write("VARIABLE, TOTCELLAREA, TOTFORECOZONE, MNBYAR, DATE, TIME\n")

    error_occurred = False

    try:
        print("\033[94m Running xtran to calculate summary statistics for the input file(s) \033[0m")
        print('\033[94m_\033[0m' * terminal_width)
        print()
        input_path = input("Please enter the filename, path or a XML file containing file paths and filter info: ")
        input_path = os.path.join(os.getcwd(), input_path) # Normalize the path for cross-platform compatibility
        file_list, filter_params = get_file_list(input_path)
        print(f"\033[94mUsing {input_path} as input file.\033[0m\n")
        print(f"\033[94mNumber of files to be processed: {len(file_list)}\033[0m")
        print(f"\033[94mFilter parameters to be applied: \033[0m ")
        print(f"\033[94m\tMinimum latitude: {filter_params['lat_min']}\033[94m")
        print(f"\033[94m\tMaximum latitude: {filter_params['lat_max']}\033[94m")
        print(f"\033[94m\tMinimum longitude: {filter_params['lon_min']}\033[94m")
        print(f"\033[94m\tMaximum longitude: {filter_params['lon_max']}\033[94m")
        print(f"\033[94m\tRegion: {filter_params['region']}\033[94m")
        print(f"\033[94m\tStart year: {filter_params['start_year']}\033[94m")
        print(f"\033[94m\tEnd year: {filter_params['end_year']}\033[94m\n")
        
        print(f"\033[94mProcessing files...\033[0m")
  
        
    except FileNotFoundError:
        print("\033[91m" +f"Error: XML file '{input_path}' not found." + "\033[0m")
        error_occurred = True
    except ValueError as e:
        print(f"Error: {e}")
        error_occurred = True

    if not error_occurred:
        for i, file_path in enumerate(file_list):
            try:
                process_file(file_path, filter_params)
                print("Working on -> {}".format(file_path))
                print(f"\033[94mFile {i+1} of {len(file_list)} processed successfully.\033[0m\n")
            except FileNotFoundError:
                print("\033[91m" + f"Error: File '{file_path}' not found." + "\033[0m")
                error_occurred = True
                continue
                
    if not error_occurred:
        print('_' * terminal_width)
        print()
        print("\033[92mProgram executed successfully! Check 'UNITS.INFO' for variable units and '.SUMMARY' for stats. Thank you!\033[0m")
        print()
       


# Call the main function            
if __name__ == "__main__":
    main()



