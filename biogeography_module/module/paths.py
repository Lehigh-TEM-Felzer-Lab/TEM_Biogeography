import dependencies 

THIS_FOLDER = dependencies.Path(__file__).parent.resolve()
dependencies.os.chdir(THIS_FOLDER)

# Construct the source directory path using os.path.join()
source_dir = dependencies.os.path.join("..", "..", "runs")
# Construct the target directory path using os.path.join()
target_dir = dependencies.os.path.join("..", "data")

# Get a list of all CSV files in the source directory
csv_files = [f for f in dependencies.os.listdir(source_dir) if f.endswith(".TEMOUT")]

# Copy each CSV file to the target directory
for f in csv_files:
    source_file = dependencies.os.path.join(source_dir, f)
    target_file = dependencies .os.path.join(target_dir, f)
    dependencies.shutil.copyfile(source_file, target_file)


# Define the file paths using pathlib, which works across different operating systems
# Input file paths
moisture_stress_path = THIS_FOLDER / ".." / "data" / "MMDI.csv"  # Path to moisture stress data
mean_moisture_stress_path = THIS_FOLDER / ".." / "data" / "MDI.csv"  # Path to mean moisture stress data
climate_path = THIS_FOLDER / ".." / "data" / "temp-gdd-prec.csv"  # Path to climate data (temperature, precipitation, growing degree days)
climate_limits_path = THIS_FOLDER / ".." / "data" / "bio_limit.csv"  # Path to limits on climate variables required for vegetation growth

#TEM output file paths, update these paths to point to the TEM output files you want to use
TEMOUT_files_path = {
    "npp_path": THIS_FOLDER / ".." / "data" / "NPP.TEMOUT",  # Path to net primary productivity data
    "vegc_path": THIS_FOLDER / ".." / "data" / "VEGC.TEMOUT",  # Path to vegetation carbon data
    "nep_path": THIS_FOLDER / ".." / "data" / "NEP.TEMOUT",  # Path to net ecosystem productivity data
    "availn_path": THIS_FOLDER / ".." / "data" / "AVAILN.TEMOUT",  # Path to available nitrogen data
    "gpp_path": THIS_FOLDER / ".." / "data" / "GPP.TEMOUT",  # Path to gross primary productivity data
    "h2oyield_path": THIS_FOLDER / ".." / "data" / "H2OYIELD.TEMOUT",  # Path to water yield data
    "netnmin_path": THIS_FOLDER / ".." / "data" / "NETNMIN.TEMOUT",  # Path to net nitrogen mineralization data
    "smois_path": THIS_FOLDER / ".." / "data" / "SMOIS.TEMOUT",  # Path to soil moisture data
    "soilorgc_path": THIS_FOLDER / ".." / "data" / "SOILORGC.TEMOUT",  # Path to soil organic carbon data
    "veginnpp_path": THIS_FOLDER / ".." / "data" / "VEGINNPP.TEMOUT",  # Path to vegetation input NPP data
    "nce_path": THIS_FOLDER / ".." / "data" / "NCE.TEMOUT"  # Path to net carbon exchange data
}
# Model bakeoff results file path
npp_bakeoff_results_path = THIS_FOLDER / ".." / "data" / "output_bakeoff" / "npp_bakeoff_result.csv" # path to model npp bakeoff results
bakeoff_results_dir_path = THIS_FOLDER / ".." / "data" / "output_bakeoff" # path to model bakeoff results directory

# Model output file paths for different time periods
early_century_persisting_pft_output_path = THIS_FOLDER / ".." / "data" / "output_bakeoff" / "2015-2045_bakeoff.csv"  # Path to output data for 2015-2045, assuming  PFTs persist
mid_century_persisting_pft_output_path = THIS_FOLDER / ".." / "data" / "output_bakeoff" / "2045-2070_bakeoff.csv"  # Path to output data for 2045-2070, assuming PFTs persist
end_century_persisting_pft_output_path = THIS_FOLDER / ".." / "data" / "output_bakeoff" / "2070-100_bakeoff.csv" # Path to output data for 2070-2100, assuming PFTs persist


