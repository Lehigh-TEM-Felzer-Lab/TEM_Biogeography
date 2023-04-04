import dependencies 
from tem_output_data import var_cols
from paths import bakeoff_results_dir_path



# Function to merge bakeoff variables
def merge_variables(df, climate_limits, climate, moisture_stress):
    # Merge the dataframe with climate_limits on potveg and subtype columns
    df = df.merge(climate_limits, on=["POTVEG", "SUBTYPE"], how="left")
    # Merge the dataframe with climate on col, row and year columns
    df = df.merge(climate, on=["LON", "LAT", "YEAR"], how="left")
    # Merge the dataframe with moisture_stress on col, row, potveg, subtype and year columns
    df = df.merge(moisture_stress, on=["LON", "LAT", "POTVEG", "SUBTYPE", "YEAR"], how="left")
    
    return df


# Function to calculate possible PFTs  in each gridcell depending on climate 
possible_cols = var_cols + ["NPP", "POSSIBLE"]
def determine_possible_pft(df, bakeoff_variables):
    df["POSSIBLE"] = (
        (bakeoff_variables["GDD"] >= bakeoff_variables["MIN_GDD"])
        & (bakeoff_variables["TOTAL_PREC"] >= bakeoff_variables["MIN_PREC"])
        & (bakeoff_variables["T_MIN"] >= bakeoff_variables["Tc"])
        & (bakeoff_variables["T_MAX"] <= bakeoff_variables["Tw"])
        & (bakeoff_variables["AET/PET"] >= bakeoff_variables["MIN_AET/PET"])
    )
    
    return df[possible_cols]



#Function to apply bakeoff logic to each gridcell, and return the PFT which is possible in the gridcell based on climate and has  the maximum NPP.
# If there are no possible PFTs in the gridcell, return the PFT with the maximum NPP, 
# Only calculate other outputs for the PFT with the maximum NPP
def possible_pft_with_max_npp(group):
    # select max possible cohort in a group
    if group["POSSIBLE"].any():
        x = group[group["POSSIBLE"]]["NPP"].idxmax()

    # if no possible cohort, select max cohort in a group
    else:
        x = group["NPP"].idxmax()
    return group.loc[x]




# Function to clean dataframe
def clean_dataframe(df):
    
    df = df.drop(["POSSIBLE","NPP"], axis=1)

    df.loc[df["ICOHORT"] > 0, "ICOHORT"] = 1
    df["SUBAREA"] = df["CAREA"]
    return df



# Function to export dataframe to csv
def export_to_csv(df, file_name):
    file_path = (bakeoff_results_dir_path, file_name)
    df.to_csv(file_path, index=False, header=False)

    
    
 
# Function to Count which PFT have the max Total NPP (30 Year Period) after applying  Bioclimatic Limits and the NPP bakeoff
def persisting_pft(df, path):
    # Count the number of times each PFT has the max Total NPP (30 Year Period) after applying  Bioclimatic Limits and the NPP bakeoff
    count_max_npp = (
        dependencies.pd.DataFrame(df.groupby(["LON", "LAT"])["POTVEG"].value_counts())
        .rename(columns={"POTVEG": "COUNT"})
        .reset_index()
    )
    # Merge the count datasets with the original datasets to get the PFT with max NPP in each gridcell
    persisting_pft = dependencies.pd.merge(
        count_max_npp.groupby(["LON", "LAT"]).agg({"COUNT": "max"}).reset_index(),
        count_max_npp,
        on=["LON", "LAT", "COUNT"],
    )
    # delete unwanted 'count' column
    del persisting_pft["COUNT"]
    
    persisting_pft.to_csv( path, index=False, header=False)
    return persisting_pft