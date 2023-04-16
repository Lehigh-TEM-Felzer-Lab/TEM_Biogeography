import dependencies 

print()
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





txt_filepath = dependencies.os.path.join(dependencies.os.getcwd(), "tem_in.txt") # Path to the txt file containing the path to the XML file

try:
    with open(txt_filepath, 'r') as f:
        tem_xml_path = f.read().strip()
        print(f"XML file used to run TEM found in txt file 'tem_in.txt': {tem_xml_path}")
except FileNotFoundError:
    print("\033[91m" + f"ERROR: txt file not found: {txt_filepath}" + "\033[0m")
    tem_xml_path = None
except UnicodeDecodeError:
    print("\033[91m" + f"ERROR: txt file is blank: {txt_filepath}" + "\033[0m")
    tem_xml_path = None

if tem_xml_path:
    # Parse the XML
    try:
        tree = dependencies.ET.parse(tem_xml_path)
        root = tree.getroot()
    except FileNotFoundError:
        print("\033[91m" + f"ERROR: xml file path not found in txt file: {tem_xml_path}" + "\033[0m")
        root = None
    
    if root:
        # Get temoutvars and temoutfiles from the XML
        temoutvars = root.find('temoutvars').text
        temoutfiles = root.find('temoutfiles').text
        itairfname = root.find('itairfname').text
        itairend = root.find('itairend').text
        iprecfname = root.find('iprecfname').text
        iprecend = root.find('iprecend').text
        itairfname = itairfname + itairend
        iprecfname = iprecfname + iprecend
        
       

        # Split the comma-separated strings and store them in dictionaries
        var_list = temoutvars.split(',')
        file_list = temoutfiles.split(',')

        # Convert file paths to be compatible with the current OS
        file_list = [dependencies.os.path.abspath(dependencies.os.path.join(dependencies.os.getcwd(), file.replace('/', dependencies.os.sep).replace('\\', dependencies.os.sep))) for file in file_list]

        var_file_dict = dict(zip(var_list, file_list))

        # Modify the read_csv_add_npp function to handle different variable names
       
        def read_csv_add_npp(var_name, file_name):
            df = dependencies.pd.read_csv(file_name, names=var_cols)
            return df

        # Call the read_csv_add_npp function for each variable and store the result in a dictionary
        dataframes = {}
        for var, file in var_file_dict.items():
            var_lower = var.lower()
            dataframes[var_lower] = read_csv_add_npp(var_lower, file)

        # Add NPP column to all dataframes
        npp_df = dataframes["npp"]
        for var, df in dataframes.items():
            df["NPP"] = npp_df["TOTAL"]



