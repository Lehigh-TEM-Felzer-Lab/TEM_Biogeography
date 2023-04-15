import dependencies 
from paths import npp_bakeoff_results_path, early_century_persisting_pft_output_path, mid_century_persisting_pft_output_path, end_century_persisting_pft_output_path,bakeoff_results_dir_path
from climate_data import climate, climate_limits
from moisture_stress_data import mean_annual_moisture_stress
from tem_output_data import dataframes,var_cols,var_list
from functions import merge_variables, determine_possible_pft, possible_pft_with_max_npp, clean_dataframe, export_to_csv, persisting_pft

def main():
         # Set a default terminal width
    default_terminal_width = 100

    # Try to get the terminal width, and if it fails, use the default width
    try:
        terminal_width = dependencies.os.get_terminal_size().columns
    except OSError:
        terminal_width = default_terminal_width

    print()
    # Print a line of dashes that fills the terminal width
    print('\033[94m_\033[94m' * terminal_width)
    print()
    print("\033[94m****APPLYING BIOGEOGRAPHY MODULE TO TEM OUTPUT DATA****\033[94m")
    print('_' * terminal_width)
    print()
    print(f"\033[94mNumber of variables to be processed: {len(var_list)}\033[94m")
    for i, var in enumerate(var_list):
        print(f"\033[94m{i+1}. {var}\033[0m")
    print()
    
    print('_' * terminal_width)
    
    print()
    # Merge tem output data with climate data and moisture stress data
    print("\033[94m1: Preparing climate datasets and merging with TEM output files...\033[94m")
    print()
    
    # Call the merge_variables function for each variable and store the result in a dictionary
    bakeoff_variables = {}
    for var, df in dataframes.items():
        bakeoff_variables[var] = merge_variables(df, climate_limits, climate, mean_annual_moisture_stress)

    print("\033[94m2: Determing possible PFTs based on climate in each gridcell...\033[94m")
    print()

    # Determine Possible PFTs depending on climate and moisture stress
    result_out = {}
    for var, df in bakeoff_variables.items():
        result_out[var] = determine_possible_pft(df, bakeoff_variables[var])  # Pass the correct dataframe here

    print("\033[94m3: Applying bakeoff logic...\033[94m")
    print()
    
    # Applying Bakeoff Logic by first grouping by col, row & year for each dataset based on NPP
    bakeoff_result = {}
    for var, df in result_out.items():
        bakeoff_result[var] = df.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    
    print("\033[94m4: Cleaning dataframes...\033[94m")
    print()
    
    # Cleaning the dataframes by removing the columns that are not needed
    cleaned_bakeoff_result = {}
    for var, df in bakeoff_result.items():
        cleaned_bakeoff_result[var] = clean_dataframe(df)
    
    print("\033[94m5: Exporting dataframes to .csv files...\033[94m")
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

    print()
    print('_' * terminal_width)
    print()
    print("\033[92mDone!\033[0m")
    
    print()

# Run the main function only if the script is executed as the main module
if __name__ == "__main__":
    main()




