import dependencies 
from  columns import TEM_OUTPUT_COLUMNS



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
possible_cols = TEM_OUTPUT_COLUMNS + ["NPP", "POSSIBLE"]
def determine_possible_pft(df, bakeoff_variables):
    df["POSSIBLE"] = (
        (bakeoff_variables["GDD"] >= bakeoff_variables["MIN_GDD"])
        & (bakeoff_variables["TOTAL_PREC"] >= bakeoff_variables["MIN_PREC"])
        & (bakeoff_variables["T_MIN"] >= bakeoff_variables["Tc"])
        & (bakeoff_variables["T_MAX"] <= bakeoff_variables["Tw"])
        & (bakeoff_variables["AET/PET"] >= bakeoff_variables["MIN_AET/PET"])
    )
    
    return df[possible_cols]



#This code defines a function called "possible_pft_with_max_npp" that takes a group as an input.
#The group is assumed to be a subset of a larger dataset that represents a grid cell with multiple plant functional types (PFTs) and their corresponding net primary productivity (NPP) values.
#The function first checks if there are any PFTs in the group that are marked as "possible" (i.e., capable of growing in the current environmental conditions). 
#If there are, it selects the PFT with the highest NPP value among those that are marked as possible. 
#If there are no possible PFTs in the group, it selects the PFT with the highest NPP value among all PFTs in the group.

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
    df.to_csv(file_name, index=False, header=False)

    
    
 # Function to Count which PFT have the max Total NPP (30 Year Period) after applying  Bioclimatic Limits and the NPP bakeoff
def persisting_pft(df, path):
    # Count the number of times each PFT has the max Total NPP (30 Year Period) after applying Bioclimatic Limits and the NPP bakeoff
    count_max_npp = df.groupby(["LON", "LAT", "POTVEG"]).size().reset_index(name="COUNT")
  
    # Find the PFT with the max NPP in each grid cell
    idx = count_max_npp.groupby(["LON", "LAT"])["COUNT"].transform(max) == count_max_npp["COUNT"]
    persisting_pft = count_max_npp[idx]

 
    # Delete the unwanted 'count' column
    del persisting_pft["COUNT"]

    persisting_pft.to_csv(path, index=False, header=False)
    return persisting_pft

