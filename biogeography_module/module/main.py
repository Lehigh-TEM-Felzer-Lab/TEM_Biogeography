import dependencies 
from paths import npp_bakeoff_results_path, early_century_persisting_pft_output_path, mid_century_persisting_pft_output_path, end_century_persisting_pft_output_path
from climate_data import climate, climate_limits
from moisture_stress_data import moisture_stress
from tem_output_data import npp, vegc, nep, availn, gpp, h2oyield, netnmin, smois,soilorgc,veginnpp,nce,var_cols
from functions import merge_variables, determine_possible_pft, possible_pft_with_max_npp, clean_dataframe, export_to_csv, persisting_pft

def main():
    
    print("\n")
    print("\033[92m****APPLYING BIOGEOGRAPHY MODULE TO TEM OUTPUT DATA****\033[0m")
    print("\n")
    # Merge tem output data with climate data and moisture stress data
    print("\033[92m1: Preparing climate datasets and merging with TEM output files...\033[0m")
    print("\n")
    npp_bakeoff_variables = merge_variables( npp, climate_limits, climate, moisture_stress)
    vegc_bakeoff_variables = merge_variables( vegc, climate_limits, climate, moisture_stress)
    nep_bakeoff_variables = merge_variables(nep, climate_limits, climate, moisture_stress)
    availn_bakeoff_variables = merge_variables(availn, climate_limits, climate, moisture_stress)
    gpp_bakeoff_variables = merge_variables(gpp, climate_limits, climate, moisture_stress)
    h2oyield_bakeoff_variables = merge_variables(h2oyield, climate_limits, climate, moisture_stress)
    netnmin_bakeoff_variables = merge_variables(netnmin, climate_limits, climate, moisture_stress)
    smois_bakeoff_variables = merge_variables(smois, climate_limits, climate, moisture_stress)
    smois_bakeoff_variables = merge_variables(smois, climate_limits, climate, moisture_stress)
    soilorgc_bakeoff_variables = merge_variables(soilorgc, climate_limits, climate, moisture_stress)
    veginnpp_bakeoff_variables = merge_variables(veginnpp, climate_limits, climate, moisture_stress)
    nce_bakeoff_variables = merge_variables(nce, climate_limits, climate, moisture_stress)


    print("\033[92m2: Determing possible PFTs based on climate in each gridcell...\033[0m")
    print("\n")
    # Determine Possible PFTs depending on climate and moisture stress
    npp_result_out = determine_possible_pft(npp, npp_bakeoff_variables)
    vegc_result_out = determine_possible_pft(vegc, vegc_bakeoff_variables)
    nep_result_out = determine_possible_pft(nep, nep_bakeoff_variables)
    availn_result_out = determine_possible_pft(availn, availn_bakeoff_variables)
    gpp_result_out = determine_possible_pft(gpp, gpp_bakeoff_variables)
    h2oyield_result_out = determine_possible_pft(h2oyield, h2oyield_bakeoff_variables)
    netnmin_result_out = determine_possible_pft(netnmin, netnmin_bakeoff_variables)
    smois_result_out = determine_possible_pft(smois, smois_bakeoff_variables)
    soilorgc_result_out = determine_possible_pft(soilorgc, soilorgc_bakeoff_variables)
    veginnpp_result_out = determine_possible_pft(veginnpp, veginnpp_bakeoff_variables)
    nce_result_out = determine_possible_pft(nce, nce_bakeoff_variables)


    print("\033[92m3: Applying bakeoff logic...\033[0m")
    print("\n")
    # Applying Bakeoff Logic by first grouping by col, row & year for each dataset bassed on NPP
    # Should take about 30 minutes to run, depending on the number of cores available
    npp_bakeoff_result = npp_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    vegc_bakeoff_result = vegc_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    nep_bakeoff_result = nep_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    availn_bakeoff_result = availn_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    gpp_bakeoff_result = gpp_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    h2oyield_bakeoff_result = h2oyield_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    netnmin_bakeoff_result = netnmin_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    smois_bakeoff_result = smois_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    soilorgc_bakeoff_result = soilorgc_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    veginnpp_bakeoff_result = veginnpp_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    nce_bakeoff_result = nce_result_out.groupby(["LON", "LAT", "YEAR"], sort=False).apply(possible_pft_with_max_npp)
    
    
   

    print("\033[92m4: Cleaning dataframes...\033[0m")
    print("\n")
    # Cleaning the dataframes by removing the columns that are not needed
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


    print("\033[92m5: Exporting dataframes to .csv files...\033[0m")
    print("\n")
    # Apply the export_to_csv function to the dataframes 

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
    bakeoff_result = dependencies.pd.read_csv(npp_bakeoff_results_path, names=var_cols)

    #Split the dataset into three parts, early, mid and end century
    early_century = bakeoff_result.query("YEAR <= 2045")
    mid_century = bakeoff_result.query("YEAR >= 2045 & YEAR<= 2070")
    end_century = bakeoff_result.query("YEAR >= 2070")


    persisting_pft(early_century, early_century_persisting_pft_output_path)
    persisting_pft(mid_century, mid_century_persisting_pft_output_path)
    persisting_pft(end_century, end_century_persisting_pft_output_path)

    print("\033[92mDone!\033[0m")
    print("\n")

# Run the main function only if the script is executed as the main module
if __name__ == "__main__":
    main()
