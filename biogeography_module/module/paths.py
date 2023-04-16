import dependencies 

# Get the current working directory
THIS_FOLDER = dependencies.os.path.abspath(dependencies.os.path.dirname(__file__))

# Parse the XML file
xml_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath('./paths.xml'))
print('Reading paths from: ' + xml_path)
tree = dependencies.ET.parse(xml_path)
root = tree.getroot()

# Construct input file paths
input_files = root.find('input_files')
mean_monthly_moisture_stress_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('mean_monthly_moisture_stress').text))
mean_summer_moisture_stress_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('mean_summer_moisture_stress').text))
temperature_data_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('temperature_data').text))
precipitation_data_path = dependencies.os.path.join(THIS_FOLDER, dependencies.os.path.abspath(input_files.find('precipitation_data').text))
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



