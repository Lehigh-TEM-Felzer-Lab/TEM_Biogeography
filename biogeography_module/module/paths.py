import dependencies

# Define the base directory for data files
base_dir = "../data"

# File paths for various input and output files
moisture_stress_path = dependencies.os.path.join(base_dir, "MMDI.csv") # Path to the file containing moisture stress data for a location and period.
mean_moisture_stress_path = dependencies.os.path.join(base_dir, "MDI.csv") # Path to the file containing mean moisture stress data for a location and period.
climate_path = dependencies.os.path.join(base_dir, "temp-gdd-prec.csv") # Path to the file containing climate data (temperature, precipitation, and growing degree days) for a location and time period.
climate_limits_path = dependencies.os.path.join(base_dir, "bio_limit.csv") # Path to the file containing limits on the climate variables required for vegetation to grow.

# File paths for various output variables
tem_output_paths = {
    "npp_path": dependencies.os.path.join(base_dir, "NPP.csv"), # Path to the file containing net primary productivity data.
    "vegc_path": dependencies.os.path.join(base_dir, "VEGC.csv"), # Path to the file containing vegetation carbon data.
    "nep_path": dependencies.os.path.join(base_dir, "NEP.csv"), # Path to the file containing net ecdependencies.osystem productivity data.
    "availn_path": dependencies.os.path.join(base_dir, "AVAILN.csv"), # Path to the file containing available nitrogen data.
    "gpp_path": dependencies.os.path.join(base_dir, "GPP.csv"), # Path to the file containing grdependencies.oss primary productivity data.
    "h2oyield_path": dependencies.os.path.join(base_dir, "H2OYIELD.csv"), # Path to the file containing water yield data.
    "netnmin_path": dependencies.os.path.join(base_dir, "NETNMIN.csv"), # Path to the file containing net nitrogen mineralization data.
    "smois_path": dependencies.os.path.join(base_dir, "SMOIS.csv"), # Path to the file containing soil moisture data.
    "soilorgc_path": dependencies.os.path.join(base_dir, "SOILORGC.csv"), # Path to the file containing soil organic carbon data.
    "veginnpp_path": dependencies.os.path.join(base_dir, "VEGINNPP.csv"), # Path to the file containing vegetation input NPP data.
    "nce_path": dependencies.os.path.join(base_dir, "NCE.csv") # Path to the file containing net carbon exchange data.
}

# File paths for model bakeoff results
bakeoff_results_path = dependencies.os.path.join(base_dir, "output_bakeoff", "npp_bakeoff_result.csv") # Path to the file containing the results of a model bakeoff for NPP.

# File paths for model output for different time periods
early_century_persisting_pft_output_path = dependencies.os.path.join(base_dir, "output_bakeoff", "2015-2045_bakeoff.csv") # Path to the file containing output data for the model simulation for the period 2015-2045, assuming that the plant functional types (PFTs) persist throughout the simulation.
mid_century_persisting_pft_output_path = dependencies.os.path.join(base_dir, "output_bakeoff", "2045-2070_bakeoff.csv") # Path to the file containing output data for the model simulation for the period 2045-2070, assuming that the PFTs persist throughout the simulation.
end_century_persisting_pft_output_path = dependencies.os.path.join(base_dir, "output_bakeoff", "2070-2100_bakeoff.csv")
