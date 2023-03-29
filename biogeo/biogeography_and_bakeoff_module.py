#!/usr/bin/env python
# coding: utf-8

# APPLIED PER MODEL, IF USING MULTIPLE CLIMATE MODELS AS INPUT SEE , PROCESSING FOLDER, WITHIN EACH MODEL THERE IS DIR CALLED scripts, RUN, Xbakeoff.ipynb
# CHECK TO SEE IF YOU HAVE ALL THE INPUT DATA.

# ----------------------------------------------------------------------- Import libraries ----------------------------------------------------------------------- #
import numpy as np
import pandas as pd
import seaborn as sns
import datetime
import random
import array as arr
import matplotlib.pyplot as plt
import matplotlib.ticker as plticker
from mpl_toolkits.basemap import Basemap
from matplotlib.colors import ListedColormap, LinearSegmentedColormap
from matplotlib.patches import Patch
# ----------------------------------------------------------------------- Biogeoraphy Module ----------------------------------------------------------------------- #

# Import moisture stress data (Drought Index)
moisture_stress = pd.read_csv(
    "../data/MMDI.csv",
    names=[
        "LON",
        "LAT",
        "ICOHORT",
        "POTVEG",
        "SUBTYPE",
        "MONTH",
        "YEAR",
        "AET",
        "PET",
        "AET/PET",
        "PET-AET/PET",
        "THETA",
    ],
)


# Select summer months (May - October)
moisture_stress = moisture_stress.query("MONTH >= 5 & MONTH <= 10 ")

# Calculate mean annual values
mean_moisture_stress = (moisture_stress.groupby(["LON", "LAT", "POTVEG", "SUBTYPE", "YEAR"]) .agg(
        {
            "AET": "sum",
            "PET": "sum",
            "AET/PET": "mean",
            "PET-AET/PET": "sum",
            "THETA": "mean",
        }
    )
    .round(3)
)

# save mean annual values to csv then import it again to save memory space
mean_moisture_stress.to_csv("../data/MDI.csv", index=True, header=False)

# Import mean moisture annual moisture stress data (Drought Index)
moisture_stress = pd.read_csv("../data/MDI.csv",names=[
        "LON",
        "LAT",
        "POTVEG",
        "SUBTYPE",
        "YEAR",
        "AET",
        "PET",
        "AET/PET",
        "(PET-AET)/PET",
        "THETA",
    ],
)

# Import climate data
climate = pd.read_csv("../data/temp-gdd-prec.csv",names=["LON", "LAT", "YEAR", "T_MAX", "T_MIN", "GDD", "TOTAL_PREC"],
)

# Import climate limits data
climate_limits = pd.read_csv("../data/bio_limit.csv",names=[
        "POTVEG",
        "SUBTYPE",
        "MIN_GDD",
        "max_GDD",
        "Tc",
        "Tw",
        "MIN_PREC",
        "MIN_AET/PET",
    ],
)

# columns names for all TEM  outputs (NPP, VEGC, NEP, AVAILN, GPP, H2OYIELD, NETNMIN, SMOIS, SOILORGC, VEGINNPP, NCE)
var_cols = [
    "LON",
    "LAT",
    "TMPVARNAME",
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

#  function to read CSV files and add NPP column
def read_csv_add_npp(filename):
    # Read the CSV file with the given filename and column names
    df = pd.read_csv(filename, names=var_cols)
    # Add a new column named "NPP" to the dataframe with values from the "TOTAL" column of the "npp" dataframe
    df["NPP"] = npp["TOTAL"]
    # Return the modified dataframe
    return df

# Read all CSV files and add NPP column
# Import NPP data and add NPP column from NPP TOTAL column, column will be used for bakeoff process
npp = read_csv_add_npp("../data/NPP.csv")
# Import VEGC data
vegc = read_csv_add_npp("../data/VEGC.csv")
# Import NEP data
nep = read_csv_add_npp("../data/NEP.csv")
# Import AVAILN data
availn = read_csv_add_npp("../data/AVAILN.csv")
# Import GPP data
gpp = read_csv_add_npp("../data/GPP.csv")
# Import H2OYIELD data
h2oyield = read_csv_add_npp("../data/H2OYIELD.csv")
# Import NETNMIN data
netnmin = read_csv_add_npp("../data/NETNMIN.csv")
# Import SMOIS data
smois = read_csv_add_npp("../data/SMOIS.csv")
# Import SOILORGC data
soilorgc = read_csv_add_npp("../data/SOILORGC.csv")
# Import VEGINNPP data
veginnpp = read_csv_add_npp("../data/VEGINNPP.csv")
# Import NCE data
nce = read_csv_add_npp("../data/NCE.csv")


def merge_bakeoff_variables(df, climate_limits, climate, moisture_stress):
    # Merge the dataframe with climate_limits on potveg and subtype columns
    df = df.merge(climate_limits, on=["POTVEG", "SUBTYPE"], how="left")
    # Merge the dataframe with climate on col, row and year columns
    df = df.merge(climate, on=["LON", "LAT", "YEAR"], how="left")
    # Merge the dataframe with moisture_stress on col, row, potveg, subtype and year columns
    df = df.merge(moisture_stress, on=["LON", "LAT", "POTVEG", "SUBTYPE", "YEAR"], how="left")
    
    return df


# Merge NPP Datasets using the merge_bakeoff_variables function
npp_bakeoff_variables = merge_bakeoff_variables( npp, climate_limits, climate, moisture_stress)

# Merge VEGC Datasets using the merge_bakeoff_variables function
vegc_bakeoff_variables = merge_bakeoff_variables( vegc, climate_limits, climate, moisture_stress)

# Merge NEP Datasets using the merge_bakeoff_variables function
nep_bakeoff_variables = merge_bakeoff_variables(nep, climate_limits, climate, moisture_stress)

# Merge AVAILN Datasets using the merge_bakeoff_variables function
availn_bakeoff_variables = merge_bakeoff_variables(availn, climate_limits, climate, moisture_stress)

# Merge GPP Datasets using the merge_bakeoff_variables function
gpp_bakeoff_variables = merge_bakeoff_variables(gpp, climate_limits, climate, moisture_stress)

# Merge H2OYIELD Datasets using the merge_bakeoff_variables function
h2oyield_bakeoff_variables = merge_bakeoff_variables(h2oyield, climate_limits, climate, moisture_stress)

# Merge NETNMIN Datasets using the merge_bakeoff_variables function
netnmin_bakeoff_variables = merge_bakeoff_variables(netnmin, climate_limits, climate, moisture_stress)

# Merge SMOIS Datasets using the merge_bakeoff_variables function
smois_bakeoff_variables = merge_bakeoff_variables(smois, climate_limits, climate, moisture_stress)

# Merge SOILORGC Datasets using the merge_bakeoff_variables function
smois_bakeoff_variables = merge_bakeoff_variables(smois, climate_limits, climate, moisture_stress)

# Merge SOILORGC Datasets using the merge_bakeoff_variables function
soilorgc_bakeoff_variables = merge_bakeoff_variables(soilorgc, climate_limits, climate, moisture_stress)

# Merge VEGINNPP Datasets using the merge_bakeoff_variables function
veginnpp_bakeoff_variables = merge_bakeoff_variables(veginnpp, climate_limits, climate, moisture_stress)

# Merge NCE Datasets using the merge_bakeoff_variables function
nce_bakeoff_variables = merge_bakeoff_variables(nce, climate_limits, climate, moisture_stress)



possible_cols = var_cols + ["NPP", "POSSIBLE"]


def calculate_possible(df, bakeoff_variables):
    df["POSSIBLE"] = (
        (bakeoff_variables["GDD"] >= bakeoff_variables["MIN_GDD"])
        & (bakeoff_variables["TOTAL_PREC"] >= bakeoff_variables["MIN_PREC"])
        & (bakeoff_variables["T_MIN"] >= bakeoff_variables["Tc"])
        & (bakeoff_variables["T_MAX"] <= bakeoff_variables["Tw"])
        & (bakeoff_variables["AET/PET"] >= bakeoff_variables["MIN_AET/PET"])
    )
    return df[possible_cols]


# Determine Possible PFTs depending on climate and moisture stress
npp_result_out = calculate_possible(npp, npp_bakeoff_variables)
vegc_result_out = calculate_possible(vegc, vegc_bakeoff_variables)
nep_result_out = calculate_possible(nep, nep_bakeoff_variables)
availn_result_out = calculate_possible(availn, availn_bakeoff_variables)
gpp_result_out = calculate_possible(gpp, gpp_bakeoff_variables)
h2oyield_result_out = calculate_possible(h2oyield, h2oyield_bakeoff_variables)
netnmin_result_out = calculate_possible(netnmin, netnmin_bakeoff_variables)
smois_result_out = calculate_possible(smois, smois_bakeoff_variables)
soilorgc_result_out = calculate_possible(soilorgc, soilorgc_bakeoff_variables)
veginnpp_result_out = calculate_possible(veginnpp, veginnpp_bakeoff_variables)
nce_result_out = calculate_possible(nce, nce_bakeoff_variables)


# Applying Bakeoff Logic
# Should take about 30 minutes to run, depending on the number of cores available

""" 
Function to apply bakeoff logic to each gridcell, and return the PFT which is possible in the gridcell based on climate and has  the maximum NPP.
If there are no possible PFTs in the gridcell, return the PFT with the maximum NPP.

"""
def possible_pft_with_max_npp(group):
    # select max possible cohort in a group
    if group["POSSIBLE"].any():
        x = group[group["POSSIBLE"]]["NPP"].idxmax()

    # if no possible cohort, select max cohort in a group
    else:
        x = group["NPP"].idxmax()
    return group.loc[x]


# apply function by first grouping by col, row & year for each dataset bassed on NPP

npp_bakeoff_result = npp_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
vegc_bakeoff_result = vegc_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
nep_bakeoff_result = nep_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
availn_bakeoff_result = availn_result_out.groupby( ["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
gpp_bakeoff_result = gpp_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
h2oyield_bakeoff_result = h2oyield_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
netnmin_bakeoff_result = netnmin_result_out.groupby( ["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
smois_bakeoff_result = smois_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
soilorgc_bakeoff_result = soilorgc_result_out.groupby( ["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
veginnpp_bakeoff_result = veginnpp_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
nce_bakeoff_result = nce_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)


# Clean Dataframe
def clean_dataframe(df):
    # delete unwanted  columns
    del df["POSSIBLE"]
    del df["NPP"]

    df.loc[df["ICOHORT"] > 0, "ICOHORT"] = 1
    df["SUBAREA"] = df["CAREA"]
    return df


npp_bakeoff_result = clean_dataframe(npp_bakeoff_result)
vegc_bakeoff_result = clean_dataframe(vegc_bakeoff_result)
nep_bakeoff_result = clean_dataframe(nep_bakeoff_result)
availn_bakeoff_result = clean_dataframe(availn_bakeoff_result)
gpp_bakeoff_result = clean_dataframe(gpp_bakeoff_result)
h2oyield_bakeoff_result = clean_dataframe(h2oyield_bakeoff_result)
netnmin_bakeoff_result = clean_dataframe(netnmin_bakeoff_result)
smois_bakeoff_result = clean_dataframe(smois_bakeoff_result)
soilorgc_bakeoff_result = clean_dataframe(soilorgc_bakeoff_result)
veginnpp_bakeoff_result = clean_dataframe(veginnpp_bakeoff_result)
nce_bakeoff_result = clean_dataframe(nce_bakeoff_result)

# export data to .csv
def export_to_csv(df, file_name):
    df.to_csv("../data/output_bakeoff/" + file_name, index=False, header=False) #save to local


# Apply the export_to_csv function to the three datasets
export_to_csv(npp_bakeoff_result, "npp_bakeoff_result.csv")
export_to_csv(vegc_bakeoff_result, "vegc_bakeoff_result.csv")
export_to_csv(nep_bakeoff_result, "nep_bakeoff_result.csv")
export_to_csv(availn_bakeoff_result, "availn_bakeoff_result.csv")
export_to_csv(gpp_bakeoff_result, "gpp_bakeoff_result.csv")
export_to_csv(h2oyield_bakeoff_result, "h2oyield_bakeoff_result.csv")
export_to_csv(netnmin_bakeoff_result, "netnmin_bakeoff_result.csv")
export_to_csv(smois_bakeoff_result, "smois_bakeoff_result.csv")
export_to_csv(soilorgc_bakeoff_result, "soilorgc_bakeoff_result.csv")
export_to_csv(veginnpp_bakeoff_result, "veginnpp_bakeoff_result.csv")
export_to_csv(nce_bakeoff_result, "nce_bakeoff_result.csv")



# Logic to determine the  most productive PFT (30 Year Period) applying the Bioclimatic Limits and the NPP bakeoff 

# Read in the NPP bakeoff result dataset
bakeoff_result = pd.read_csv("../data/output_bakeoff/npp_bakeoff_result.csv", names=var_cols)

#Split the dataset into three parts, early, mid and end century
early_century = bakeoff_result.query("YEAR <= 2045")
mid_century = bakeoff_result.query("YEAR >= 2045 & YEAR<= 2070")
end_century = bakeoff_result.query("YEAR >= 2070")

# Count which PFT have the max Total NPP (30 Year Period) after applying  Bioclimatic Limits and the NPP bakeoff

# Early Century
count_early_century = (
    pd.DataFrame(early_century.groupby(["LON", "LAT"])["POTVEG"].value_counts())
    .rename(columns={"POTVEG": "COUNT"})
    .reset_index()
)

# Mid Century
count_mid_century = (
    pd.DataFrame(mid_century.groupby(["LON", "LAT"])["POTVEG"].value_counts())
    .rename(columns={"POTVEG": "COUNT"})
    .reset_index()
)

# End Century
count_end_century = (
    pd.DataFrame(end_century.groupby(["LON", "LAT"])["POTVEG"].value_counts())
    .rename(columns={"POTVEG": "COUNT"})
    .reset_index()
)

# Merge the count datasets with the original datasets to get the PFT with max NPP in each gridcell
max_count_early_century = pd.merge(
    count_early_century.groupby(["LON", "LAT"]).agg({"COUNT": "max"}).reset_index(),
    count_early_century,
    on=["LON", "LAT", "COUNT"],
)
max_count_mid_century = pd.merge(
    count_mid_century.groupby(["LON", "LAT"]).agg({"COUNT": "max"}).reset_index(),
    count_mid_century,
    on=["LON", "LAT", "COUNT"],
)
max_count_end_century = pd.merge(
    count_end_century.groupby(["LON", "LAT"]).agg({"COUNT": "max"}).reset_index(),
    count_end_century,
    on=["LON", "LAT", "COUNT"],
)

# delete unwanted 'count' column
del max_count_early_century["COUNT"]
del max_count_mid_century["COUNT"]
del max_count_end_century["COUNT"]

max_count_early_century.to_csv( "../data/output_bakeoff/2015-2045_bakeoff.csv", index=False, header=False)
max_count_mid_century.to_csv("../data/output_bakeoff/2045-2070_bakeoff.csv", index=False, header=False)
max_count_end_century.to_csv("../data/output_bakeoff/2070-2100_bakeoff.csv", index=False, header=False)

