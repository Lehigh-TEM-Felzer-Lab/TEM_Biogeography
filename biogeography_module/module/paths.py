import dependencies 

# Get the current working directory
THIS_FOLDER = dependencies.os.path.abspath(dependencies.os.path.dirname(__file__))

# Input file paths
mean_monthly_moisture_stress_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "input", "MMDI.csv")
mean_summer_moisture_stress_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "input", "MDI.csv")
climate_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "input", "temp-gdd-prec.csv")
climate_limits_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "input", "bio_limit.csv")

# Create directories if they don't exist
for path in [mean_monthly_moisture_stress_path, mean_summer_moisture_stress_path, climate_path, climate_limits_path]:
    dir_path = dependencies.os.path.dirname(path)
    if not dependencies.os.path.exists(dir_path):
        dependencies.os.makedirs(dir_path)

# Model bakeoff results file path
npp_bakeoff_results_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "output_bakeoff", "npp_bakeoff_result.csv")
bakeoff_results_dir_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "output_bakeoff")

# Create directories if they don't exist
for path in [npp_bakeoff_results_path, bakeoff_results_dir_path]:
    dir_path = dependencies.os.path.dirname(path)
    if not dependencies.os.path.exists(dir_path):
        dependencies.os.makedirs(dir_path)

# Model output file paths for different time periods
early_century_persisting_pft_output_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "output_bakeoff", "2015-2045_bakeoff.csv")
mid_century_persisting_pft_output_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "output_bakeoff", "2045-2070_bakeoff.csv")
end_century_persisting_pft_output_path = dependencies.os.path.join(THIS_FOLDER, "biogeo", "output_bakeoff", "2070-100_bakeoff.csv")

# Create directories if they don't exist
for path in [early_century_persisting_pft_output_path, mid_century_persisting_pft_output_path, end_century_persisting_pft_output_path]:
    dir_path = dependencies.os.path.dirname(path)
    if not dependencies.os.path.exists(dir_path):
        dependencies.os.makedirs(dir_path)


