import dependencies 
from tem_output_data import iprecfname, itairfname, tem_xml_path

# Get the current working directory
THIS_FOLDER = dependencies.os.path.abspath(dependencies.os.path.dirname(__file__))


# Parse the XML file
biogeo_xml_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath('./paths.xml'))

print()
try:
    tree = dependencies.ET.parse(biogeo_xml_path)
    root = tree.getroot()
    print('Reading paths from: ' + biogeo_xml_path)
except FileNotFoundError:
    print(f"\033[31mError: File not found: {biogeo_xml_path}\033[0m", file=dependencies.sys.stderr)
    exit(1)
    


# Construct input file paths
input_files = root.find('input_files')
mean_monthly_moisture_stress_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('mean_monthly_moisture_stress').text))
mean_summer_moisture_stress_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('mean_summer_moisture_stress').text))


# Get the paths to the climate data files
def find_file(file_name):
    return dependencies.os.path.abspath(file_name)

# Attempt to find the files with the initial names
temperature_data_path_from_tem_xml = find_file(dependencies.os.path.join(THIS_FOLDER, itairfname))
precipitation_data_path_from_tem_xml = find_file(dependencies.os.path.join(THIS_FOLDER, iprecfname))

if dependencies.os.path.exists(temperature_data_path_from_tem_xml) and dependencies.os.path.exists(precipitation_data_path_from_tem_xml):
    print(f"Using climate data path obtained from XML used for TEM input: {tem_xml_path}")
    
    temperature_data_path = temperature_data_path_from_tem_xml
    precipitation_data_path = precipitation_data_path_from_tem_xml
    
    print(temperature_data_path)
    print(precipitation_data_path)
    
else:
    # Use the alternative paths if the initial files are not found
    temperature_data_path_from_path_xml = find_file(dependencies.os.path.join(THIS_FOLDER, input_files.find('temperature_data').text))
    precipitation_data_path_from_path_xml = find_file(dependencies.os.path.join(THIS_FOLDER, input_files.find('precipitation_data').text))

    if dependencies.os.path.exists(temperature_data_path_from_path_xml) and dependencies.os.path.exists(precipitation_data_path_from_path_xml):
        print(f"Using climate data from paths defined in: {biogeo_xml_path}")
        
        temperature_data_path = temperature_data_path_from_path_xml
        precipitation_data_path = precipitation_data_path_from_path_xml
        
        print(temperature_data_path)
        print(precipitation_data_path)
    else:
        print("Error: temperature and precipitation data not found in specified paths, check paths.xml or TEM input XML file!")
        exit(1)



climate_limits_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('climate_limits').text))
pft_description_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('pft_description').text))

# Create directories if they don't exist
for path in [mean_monthly_moisture_stress_path, mean_summer_moisture_stress_path, climate_limits_path]:
    dir_path = dependencies.os.path.dirname(path)
    if not dependencies.os.path.exists(dir_path):
        dependencies.os.makedirs(dir_path)

# Construct output file paths
output_files = root.find('output_files')
npp_bakeoff_results_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(output_files.find('npp_bakeoff_results').text))
early_century_persisting_pft_output_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(output_files.find('early_century_persisting_pft_output').text))
mid_century_persisting_pft_output_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(output_files.find('mid_century_persisting_pft_output').text))
end_century_persisting_pft_output_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(output_files.find('end_century_persisting_pft_output').text))
bakeoff_results_dir_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(output_files.find('bakeoff_results_dir').text))

for path in [early_century_persisting_pft_output_path, mid_century_persisting_pft_output_path, end_century_persisting_pft_output_path, bakeoff_results_dir_path]:
    dir_path = dependencies.os.path.dirname(path)
    if not dependencies.os.path.exists(dir_path):
        dependencies.os.makedirs(dir_path)

print()
print()

