import json
import os

import pandas as pd
from climate_data import climate, climate_limits
from columns import tem_out_cols
from moisture_stress_data import mean_summer_moisture_stress
from paths import (
    bakeoff_results_dir_path,
    early_century_persisting_pft_output_path,
    end_century_persisting_pft_output_path,
    mid_century_persisting_pft_output_path,
    npp_bakeoff_results_path,
    pft_description_path,
)
from tem_output_data import clmstartyr, dataframes, mxnumgrid, transtime, var_list


# Function to merge bakeoff variables
def merge_variables(df, climate_limits, climate, moisture_stress):
    # Merge the dataframe with climate_limits on potveg and subtype columns
    df = df.merge(climate_limits, on=["POTVEG", "SUBTYPE"], how="left")
    # Merge the dataframe with climate on col, row and year columns
    df = df.merge(climate, on=["LON", "LAT", "YEAR"], how="left")
    # Merge the dataframe with moisture_stress on col, row, potveg, subtype and year columns
    df = df.merge(
        moisture_stress, on=["LON", "LAT", "POTVEG", "SUBTYPE", "YEAR"], how="left"
    )

    return df


# Function to calculate possible PFTs  in each gridcell depending on climate
possible_cols = tem_out_cols + ["NPP", "POSSIBLE"]


def determine_possible_pft(df, bakeoff_variables):
    df["POSSIBLE"] = (
        (bakeoff_variables["GDD"] >= bakeoff_variables["MIN_GDD"])
        & (bakeoff_variables["TOTAL_PREC"] >= bakeoff_variables["MIN_PREC"])
        & (bakeoff_variables["T_MIN"] >= bakeoff_variables["Tc"])
        & (bakeoff_variables["T_MAX"] <= bakeoff_variables["Tw"])
        & (bakeoff_variables["AET/PET"] >= bakeoff_variables["MIN_AET/PET"])
    )

    return df[possible_cols]


def possible_pft_with_max_npp(group):
    # This code defines a function called "possible_pft_with_max_npp" that takes a group as an input.
    # The group is assumed to be a subset of a larger dataset that represents a grid cell with multiple plant functional types (PFTs) and their corresponding net primary productivity (NPP) values.
    # The function first checks if there are any PFTs in the group that are marked as "possible" (i.e., capable of growing in the current environmental conditions).
    # If there are, it selects the PFT with the highest NPP value among those that are marked as possible.
    # If there are no possible PFTs in the group, it selects the PFT with the highest NPP value among all PFTs in the group.
    # select max possible cohort in a group
    if group["POSSIBLE"].any():
        x = group[group["POSSIBLE"]]["NPP"].idxmax()

    # if no possible cohort, select max cohort in a group
    else:
        x = group["NPP"].idxmax()
    return group.loc[x]


# Function to clean dataframe
def clean_dataframe(df):

    df = df.drop(["POSSIBLE", "NPP"], axis=1)

    df.loc[df["ICOHORT"] > 0, "ICOHORT"] = 1
    df["SUBAREA"] = df["CAREA"]
    return df


# Function to export dataframe to csv
def export_to_csv(df, file_name):
    df.to_csv(file_name, index=False, header=False)


# Function to Count which PFT have the max Total NPP (30 Year Period) after applying  Bioclimatic Limits and the NPP bakeoff
def persisting_pft(df, path):
    # Count the number of times each PFT has the max Total NPP (30 Year Period) after applying Bioclimatic Limits and the NPP bakeoff
    count_max_npp = (
        df.groupby(["LON", "LAT", "POTVEG"]).size().reset_index(name="COUNT")
    )

    # Find the PFT with the max NPP in each grid cell
    idx = (
        count_max_npp.groupby(["LON", "LAT"])["COUNT"].transform("max")
        == count_max_npp["COUNT"]
    )

    persisting_pft = count_max_npp[idx]

    # Delete the unwanted 'count' column
    del persisting_pft["COUNT"]

    persisting_pft.to_csv(path, index=False, header=False)
    return persisting_pft


def main():

    try:
        # Set a default terminal width
        default_terminal_width = 100

        # Try to get the terminal width, and if it fails, use the default width
        try:
            terminal_width = os.get_terminal_size().columns
        except OSError:
            terminal_width = default_terminal_width

        print("-" * terminal_width)
        print(
            "*" * 10
            + " APPLYING BIOGEOGRAPHY MODULE TO DETERMINE PFT DISTRIBUTION "
            + "*" * 10
        )
        print("-" * terminal_width)

        print(f"Running module starting from: {clmstartyr}")
        print(f"Running module for: {transtime} years")
        print(f"Number of grid cells: {mxnumgrid}")

        lon_unique = dataframes["npp"]["LON"].unique()
        lat_unique = dataframes["npp"]["LAT"].unique()

        if len(lon_unique) > 1 and len(lat_unique) > 1:
            lon_resolution = abs(lon_unique[1] - lon_unique[0])
            lat_resolution = abs(lat_unique[1] - lat_unique[0])
            print(f"Data resolution: {lon_resolution} x {lat_resolution} degrees")
        else:
            print("Unable to determine data resolution.")

        # Load PFT descriptions from the json file
        with open(pft_description_path, "r") as f:
            pft_descriptions = json.load(f)["pft_description"]

        # Initialize a dictionary to store the unique POTVEG values found
        pfts_found = {}

        # Loop through each dataframe and extract unique POTVEG values
        for var, df in dataframes.items():
            unique_pfts = df["POTVEG"].unique()
            for pft in unique_pfts:
                pfts_found[pft] = 1

        # Get the descriptions of the unique POTVEG values found
        pft_desc_found = {pft: pft_descriptions[str(pft)] for pft in pfts_found.keys()}

        print(
            "Biogeography of the following Plant Functional Types (PFTs) will be determined:"
        )
        for i, pft in enumerate(pft_desc_found):
            print(f"      {i+1}. {pft_desc_found[pft]}")

        print(f"Number of TEM output data to be processed: {len(var_list)}")

        # Merge tem output data with climate data and moisture stress data
        print("1. Preparing climate datasets and merging with TEM output files...")

        # Call the merge_variables function for each variable and store the result in a dictionary
        bakeoff_variables = {}
        for var, df in dataframes.items():
            bakeoff_variables[var] = merge_variables(
                df, climate_limits, climate, mean_summer_moisture_stress
            )

        print("2. Determining possible PFTs based on climate in each gridcell...")

        # Determine Possible PFTs depending on climate and moisture stress
        result_out = {}
        for var, df in bakeoff_variables.items():
            result_out[var] = determine_possible_pft(df, bakeoff_variables[var])

        print("3. Applying bakeoff logic to PFTs...")

        # Applying Bakeoff Logic by first grouping by col, row & year for each dataset based on NPP
        bakeoff_result = {}
        for var, df in result_out.items():
            bakeoff_result[var] = df.groupby(["LON", "LAT", "YEAR"], sort=False).apply(
                possible_pft_with_max_npp
            )

        print("4. Cleaning dataframes...")

        # Cleaning the dataframes by removing the columns that are not needed
        cleaned_bakeoff_result = {}
        for var, df in bakeoff_result.items():
            cleaned_bakeoff_result[var] = clean_dataframe(df)

        print("5. Exporting dataframes to .csv files...")

        # Apply the export_to_csv function to the dataframes
        for var, df in cleaned_bakeoff_result.items():
            export_to_csv(
                df,
                os.path.join(bakeoff_results_dir_path, f"{var}_bakeoff_result.csv"),
            )

        # Logic to determine the most productive PFT (30 Year Period) applying the Bioclimatic Limits and the NPP bakeoff
        # Read in the NPP bakeoff result dataset
        bakeoff_result = pd.read_csv(npp_bakeoff_results_path, names=tem_out_cols)

        # Split the dataset into three parts, early, mid and end century
        early_century = bakeoff_result.query("YEAR <= 2045")
        mid_century = bakeoff_result.query("YEAR >= 2045 & YEAR<= 2070")
        end_century = bakeoff_result.query("YEAR >= 2070")

        persisting_pft(early_century, early_century_persisting_pft_output_path)
        persisting_pft(mid_century, mid_century_persisting_pft_output_path)
        persisting_pft(end_century, end_century_persisting_pft_output_path)

        print("-" * terminal_width)
        print("*" * 10 + " BIOGEOGRAPHY MODULE COMPLETED SUCCESSFULLY " + "*" * 10)
        print("-" * terminal_width)

    except KeyboardInterrupt:
        print(
            "*" * 10
            + " Keyboard interrupt received. Stopping Biogeography Module! "
            + "*" * 10
        )


# Run the main function only if the script is executed as the main module
if __name__ == "__main__":
    main()
