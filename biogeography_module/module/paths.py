import dependencies 

# File paths for various input and output files
moisture_stress_path = dependencies.os.path.join("..", "data", "MMDI.csv") # Path to the file containing moisture stress data for a location and period.
mean_moisture_stress_path = dependencies.os.path.join("..", "data", "MDI.csv") # Path to the file containing mean moisture stress data for a location and period.
climate_path = dependencies.os.path.join("..", "data", "temp-gdd-prec.csv") # Path to the file containing climate data (temperature, precipitation, and growing degree days) for a location and time period.
climate_limits_path = dependencies.os.path.join("..", "data", "bio_limit.csv") # Path to the file containing limits on the climate variables required for vegetation to grow.

# File paths for various output variables
tem_output_paths = {
    "npp_path": dependencies.os.path.join("..", "data", "NPP.csv"), # Path to the file containing net primary productivity data.
    "vegc_path": dependencies.os.path.join("..", "data", "VEGC.csv"), # Path to the file containing vegetation carbon data.
    "nep_path": dependencies.os.path.join("..", "data", "NEP.csv"), # Path to the file containing net ecosystem productivity data.
    "availn_path": dependencies.os.path.join("..", "data", "AVAILN.csv"), # Path to the file containing available nitrogen data.
    "gpp_path": dependencies.os.path.join("..", "data", "GPP.csv"), # Path to the file containing gross primary productivity data.
    "h2oyield_path": dependencies.os.path.join("..", "data", "H2OYIELD.csv"), # Path to the file containing water yield data.
    "netnmin_path": dependencies.os.path.join("..", "data", "NETNMIN.csv"), # Path to the file containing net nitrogen mineralization data.
    "smois_path": dependencies.os.path.join("..", "data", "SMOIS.csv"), # Path to the file containing soil moisture data.
    "soilorgc_path": dependencies.os.path.join("..", "data", "SOILORGC.csv"), # Path to the file containing soil organic carbon data.
    "veginnpp_path": dependencies.os.path.join("..", "data", "VEGINNPP.csv"), # Path to the file containing vegetation input NPP data.
    "nce_path": dependencies.os.path.join("..", "data", "NCE.csv") # Path to the file containing net carbon exchange data.
}

# File paths for model bakeoff results
bakeoff_results_path = dependencies.os.path.join("..", "data", "output_bakeoff", "npp_bakeoff_result.csv") # Path to the file containing the results of a model bakeoff for NPP.

# File paths for model output for different time periods
early_century_persisting_pft_output_path = dependencies.os.path.join("..", "data", "output_bakeoff", "2015-2045_bakeoff.csv") # Path to the file containing output data for the model simulation for the period 2015-2045, assuming that the plant functional types (PFTs) persist throughout the simulation.
mid_century_persisting_pft_output_path = dependencies.os.path.join("..", "data", "output_bakeoff", "2045-2070_bakeoff.csv") # Path to the file containing output data for the model simulation for the period 2045-2070, assuming that the PFTs persist throughout the simulation.
end_century_persisting_pft_output_path = dependencies.os.path.join("..", "data", "output_bakeoff", "2070-100_bakeoff.csv") # Path to the file containing output data for the model simulation for the period 2070-2100, assuming that the PFTs persist throughout the simulation.