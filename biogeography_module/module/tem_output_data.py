import dependencies 
from paths import temout_files_path


# columns names for all TEM  outputs (NPP, VEGC, NEP, AVAILN, GPP, H2OYIELD, NETNMIN, SMOIS, SOILORGC, VEGINNPP, NCE)
var_cols = [
    "LON",          # The longitude coordinate of the location being modelled.
    "LAT",          # The latitude coordinate of the location being modelled.
    "TMPVARNAME",   # The name of the output variable..
    "ICOHORT",      # An index indicating the cohort number of each PFT in a grdcell.
    "STANDAGE",     # The age of the vegetation stand in years.
    "POTVEG",       # The potential natural vegetation type for the location being modelled.
    "CURRENTVEG",   # The current vegetation type for the location being modelled.
    "SUBTYPE",      # A subtype of the PFT being modeled.
    "CMNT",         
    "PSIPLUSC",     
    "QLCON",        
    "CAREA",        # The total area of the grid cell
    "SUBAREA",      # The sub-area of each PFT in the grid cell
    "YEAR",         # The year in which the data was collected.
    "TOTAL",        # The total value of the variable being modelled
    "MAX",          # The maximum value of the variable being modelled
    "AVE",          # The average value of the variable being modelled
    "MIN",          # The minimum value of the variable being modelled
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC", # The values of the variable being measured for each month of the year.
    "REGION"        # The region or area where the data was collected.
]

#  function to read CSV files and add NPP column
def read_csv_add_npp(filename):
    # Read the CSV file with the given filename and column names
    df = dependencies.pd.read_csv(filename, names=var_cols)
    return df


npp = read_csv_add_npp(temout_files_path["npp_path"])
vegc = read_csv_add_npp(temout_files_path["vegc_path"])
nep = read_csv_add_npp(temout_files_path["nep_path"])
availn = read_csv_add_npp(temout_files_path["availn_path"])
gpp = read_csv_add_npp(temout_files_path["gpp_path"])
h2oyield = read_csv_add_npp(temout_files_path["h2oyield_path"])
netnmin = read_csv_add_npp(temout_files_path["netnmin_path"])
smois = read_csv_add_npp(temout_files_path["smois_path"])
soilorgc = read_csv_add_npp(temout_files_path["soilorgc_path"])
veginnpp = read_csv_add_npp(temout_files_path["veginnpp_path"])
nce = read_csv_add_npp(temout_files_path["nce_path"])


#  function to add NPP column to all dataframes
def add_npp_column(df, npp):
    df["NPP"] = npp["TOTAL"]

        
add_npp_column(npp, npp)
add_npp_column(vegc, npp)
add_npp_column(nep, npp)
add_npp_column(availn, npp)
add_npp_column(gpp, npp)
add_npp_column(h2oyield, npp)
add_npp_column(netnmin, npp)
add_npp_column(smois, npp)
add_npp_column(soilorgc, npp)
add_npp_column(veginnpp, npp)
add_npp_column(nce, npp)

