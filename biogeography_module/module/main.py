import dependencies 
from paths import npp_bakeoff_results_path, early_century_persisting_pft_output_path, mid_century_persisting_pft_output_path, end_century_persisting_pft_output_path,bakeoff_results_dir_path,pft_description_path
from climate_data import climate, climate_limits
from moisture_stress_data import mean_summer_moisture_stress
from tem_output_data import dataframes,var_cols,var_list, clmstartyr, mxnumgrid, transtime
from functions import merge_variables, determine_possible_pft, possible_pft_with_max_npp, clean_dataframe, export_to_csv, persisting_pft

# ANSI color codes
BOLD = '\033[1m'
GREEN = '\033[32m'
BLUE = '\033[34m'
MAGENTA = '\033[35m'
CYAN = '\033[36m'
RED = '\033[31m'
YELLOW = '\033[33m'
RESET = '\033[0m'


def main():
    
    try:
            # Set a default terminal width
        default_terminal_width = 100

        # Try to get the terminal width, and if it fails, use the default width
        try:
            terminal_width = dependencies.os.get_terminal_size().columns
        except OSError:
            terminal_width = default_terminal_width

        print()
        print(CYAN + '_' * terminal_width + RESET)
        print()
        print(BOLD + GREEN + "APPLYING BIOGEOGRAPHY MODULE TO DETERMINE PFT DISTRIBUTION" + RESET)      
        print()
        
        print(BOLD + BLUE + "Running module staring from: " + RESET + CYAN+BOLD +f"{clmstartyr}" + RESET)
        print(BOLD + BLUE + "Running module for: " + RESET + CYAN+BOLD +f"{transtime} years" + RESET)
        print(BOLD + BLUE + "Number of grid cells: " + RESET + CYAN+BOLD +f"{mxnumgrid}" + RESET)
        

        lon_unique = dataframes['npp']['LON'].unique()
        lat_unique = dataframes['npp']['LAT'].unique()

        if len(lon_unique) > 1 and len(lat_unique) > 1:
            lon_resolution = abs(lon_unique[1] - lon_unique[0])
            lat_resolution = abs(lat_unique[1] - lat_unique[0])
            print(f"{BOLD + BLUE}Data resolution: {RESET}  {BOLD+ CYAN} {lon_resolution} x {lat_resolution} degrees {RESET}")
        else:
            print(BOLD+RED+"Unable to determine data resolution."+RESET)
            
        print()
     
        
        # Load PFT descriptions from the json file
        with open(pft_description_path, "r") as f:
            pft_descriptions = dependencies.json.load(f)["pft_description"]

        # Initialize a dictionary to store the unique POTVEG values found
        pfts_found = {}

        # Loop through each dataframe and extract unique POTVEG values
        for var, df in dataframes.items():
            unique_pfts = df["POTVEG"].unique()
            for pft in unique_pfts:
                pfts_found[pft] = 1

        # Get the descriptions of the unique POTVEG values found
        pft_desc_found = {}
        for pft in pfts_found.keys():
            pft_desc_found[pft] = pft_descriptions[str(pft)]

        # Print the descriptions of the unique POTVEG values found
        print(BOLD + BLUE +"Biogeography of the following Plant Functional Types (PFTs) will be determined:" + RESET)
        print()
        for i, pft in enumerate(pft_desc_found):
            print(MAGENTA +BOLD+f"      {i+1}. {pft_desc_found[pft]}"+ RESET)
            print()
        print(BOLD + BLUE + "Number of TEM output data to be processed: " + MAGENTA + f"{len(var_list)}" + RESET)
        print()

    
            
        
        # Merge tem output data with climate data and moisture stress data
        print(BOLD + YELLOW + "Preparing climate datasets and merging with TEM output files..." + RESET)
        print()
        
        # Call the merge_variables function for each variable and store the result in a dictionary
        bakeoff_variables = {}
        for var, df in dataframes.items():
            bakeoff_variables[var] = merge_variables(df, climate_limits, climate, mean_summer_moisture_stress)

        print(BOLD + YELLOW + "Determining possible PFTs based on climate in each gridcell..." + RESET)
        print()

        # Determine Possible PFTs depending on climate and moisture stress
        result_out = {}
        for var, df in bakeoff_variables.items():
            result_out[var] = determine_possible_pft(df, bakeoff_variables[var])  # Pass the correct dataframe here

        print(BOLD + YELLOW + "Applying bakeoff logic to PFTs..." + RESET)
        print()
        
        # Applying Bakeoff Logic by first grouping by col, row & year for each dataset based on NPP
        bakeoff_result = {}
        for var, df in result_out.items():
            bakeoff_result[var] = df.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
        
        print(BOLD + YELLOW + "Cleaning dataframes..." + RESET)
        print()
        
        # Cleaning the dataframes by removing the columns that are not needed
        cleaned_bakeoff_result = {}
        for var, df in bakeoff_result.items():
            cleaned_bakeoff_result[var] = clean_dataframe(df)
        
        print(BOLD + YELLOW + "Exporting dataframes to .csv files..." + RESET)
        print()
        
        # Apply the export_to_csv function to the dataframes
        for var, df in cleaned_bakeoff_result.items():
            export_to_csv(df, dependencies.os.path.join(bakeoff_results_dir_path, f"{var}_bakeoff_result.csv"))
        
        # Logic to determine the  most productive PFT (30 Year Period) applying the Bioclimatic Limits and the NPP bakeoff 
        # Read in the NPP bakeoff result dataset
        bakeoff_result = dependencies.pd.read_csv(npp_bakeoff_results_path, names=var_cols)

        #Split the dataset into three parts, early, mid and end century
        early_century = bakeoff_result.query("YEAR <= 2045")
        mid_century = bakeoff_result.query("YEAR >= 2045 & YEAR<= 2070")
        end_century = bakeoff_result.query("YEAR >= 2070")


        persisting_pft(early_century, early_century_persisting_pft_output_path)
        persisting_pft(mid_century, mid_century_persisting_pft_output_path)
        persisting_pft(end_century, end_century_persisting_pft_output_path)
        
        
    
        print(BOLD + GREEN + "BIOGEOGRAPHY MODULE COMPLETED SUCCESSFULLY" + RESET)
        print(CYAN + '_' * terminal_width + RESET)
        print()
   
    except KeyboardInterrupt:
        print(BOLD + RED +"Keyboard interrupt received. Stopping Biogeography Module !" + RESET)    

# Run the main function only if the script is executed as the main module
if __name__ == "__main__":
    main()




