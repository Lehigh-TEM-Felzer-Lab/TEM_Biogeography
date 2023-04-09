import xml.etree.ElementTree as ET
import pandas as pd
import numpy as np
import os
import re

def process_file(input_filename,filter_params):
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
    df = pd.read_csv(input_filename,names=var_cols)
    #df["VARIABLE"] = df["VARIABLE"].str.strip()
    df["REGION"] = df["REGION"].str.strip()
    def filter_dataframe(df, filter_params):
        if filter_params is not None:
            filter_criteria = {}
            if filter_params.get("lat_min") is not None:
                filter_criteria["LAT >= "] = float(filter_params["lat_min"])
            if filter_params.get("lat_max") is not None:
                filter_criteria["LAT <= "] = float(filter_params["lat_max"])
            if filter_params.get("lon_min") is not None:
                filter_criteria["LON >= "] = float(filter_params["lon_min"])
            if filter_params.get("lon_max") is not None:
                filter_criteria["LON <= "] = float(filter_params["lon_max"])
            if filter_params.get("region") is not None:
                filter_criteria["REGION == "] = f"'{filter_params['region']}'"

            if filter_criteria:
                query_string = " and ".join([k + str(v) for k, v in filter_criteria.items()])
                df = df.query(query_string)

        return df
    
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

    # Calculate summary statistics for POTVEG and REGION
    summary_stats_potveg = summary_stats(df, "POTVEG")
    summary_stats_potveg  = summary_stats_potveg.round(2)
    summary_stats_potveg["POTVEG"]= summary_stats_potveg["POTVEG"].round().astype(int)
    summary_stats_potveg["YEAR"] = summary_stats_potveg["YEAR"].astype(int).astype(str)



    summary_stats_region = summary_stats(df, "REGION")
    summary_stats_region= summary_stats_region.round(2)
    summary_stats_region["POTVEG"] = "ALL PFTs"
    summary_stats_region["YEAR"] = summary_stats_region["YEAR"].astype(int).astype(str)
    

    # Concatenate the two summary statistics dataframes
    summary_stats_final = pd.concat([summary_stats_potveg, pd.DataFrame(np.nan, index=[0], columns=summary_stats_potveg.columns), summary_stats_region])
    summary_stats_final = summary_stats_final.round(2)

    variable_parts = df["VARIABLE"].unique()[0].split()
    variable_first_part = variable_parts[0]
    stats_out_file = variable_first_part + ".SUMMARY"
    units_out_file = variable_first_part + ".UNITS"

    summary_stats_final=summary_stats_final.drop("REGION",axis=1)
    summary_stats_final= summary_stats_final.reindex(columns=['VARIABLE', 'POTVEG', 'YEAR', 'NGRID', 'TOTFORECOZONE', 'MNBYAR', 'MXPRED', 'MNPRED', 'MNTOTYR', 'STNDEV', 'SIMPMN'])



    # Save summary statistics to a CSV file
    summary_stats_final.to_csv(stats_out_file, index=False)

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

    # Open the output file and write the units for each variable and column
    with open(units_out_file, "w") as f:
        variables = df["VARIABLE"].unique()
        for variable in variables:
            for column in columns:
                units = get_units(variable, column)
                f.write(f"{variable}-{column}: {units}\n")




def get_file_list(input_path):
    if input_path.endswith(".xml"):
        tree = ET.parse(input_path)
        root = tree.getroot()
        file_list = [elem.text for elem in root.findall("files/file")]
        filter_params = None
        filter_params_elem = root.find("filter_params")
        if filter_params_elem is not None:
            filter_params = {}
            lat_min = filter_params_elem.findtext("lat_min")
            lat_max = filter_params_elem.findtext("lat_max")
            lon_min = filter_params_elem.findtext("lon_min")
            lon_max = filter_params_elem.findtext("lon_max")
            region = filter_params_elem.findtext("region")
            if lat_min.strip() or lat_max.strip() or lon_min.strip() or lon_max.strip() or region.strip():
                filter_params["lat_min"] = float(lat_min) if lat_min.strip() else None
                filter_params["lat_max"] = float(lat_max) if lat_max.strip() else None
                filter_params["lon_min"] = float(lon_min) if lon_min.strip() else None
                filter_params["lon_max"] = float(lon_max) if lon_max.strip() else None
                filter_params["region"] = region.strip() if region.strip() else None
        else:
            file_list = [input_path]
            filter_params = None
    else:
        file_list = [input_path]
        filter_params = None
    
    return file_list, filter_params

input_path = input("Please enter the file name, path or a .XML file containing file paths and filter infomation: ")
file_list, filter_params = get_file_list(input_path)

for file_path in file_list:
    process_file(file_path, filter_params)