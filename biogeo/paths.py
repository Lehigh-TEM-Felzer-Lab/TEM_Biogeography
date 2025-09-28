import sys
import xml.etree.ElementTree as ET
from pathlib import Path

from tem_output_data import iprecfname, itairfname, tem_xml_path

# Get the current working directory
script_dir = Path(__file__).resolve().parent

# Parse the XML file
biogeo_xml_path = script_dir / "paths.xml"

print()
try:
    tree = ET.parse(biogeo_xml_path)
    root = tree.getroot()
    print(f"Reading paths from: {biogeo_xml_path}")
except FileNotFoundError:
    print(f"Error: File not found: {biogeo_xml_path}", file=sys.stderr)


# Construct input file paths
input_files = root.find("input_files")
mean_monthly_moisture_stress_path = (
    script_dir / input_files.find("mean_monthly_moisture_stress").text
).resolve()
mean_summer_moisture_stress_path = (
    script_dir / input_files.find("mean_summer_moisture_stress").text
).resolve()


# Get the paths to the climate data files
def find_file(file_name):
    return Path(file_name).resolve()


# Attempt to find the files with the initial names
temperature_data_path_from_tem_xml = find_file(script_dir / itairfname)
precipitation_data_path_from_tem_xml = find_file(script_dir / iprecfname)

if (
    temperature_data_path_from_tem_xml.exists()
    and precipitation_data_path_from_tem_xml.exists()
):
    print(
        f"Using climate data path obtained from XML used for TEM input: {tem_xml_path}"
    )

    temp_path = temperature_data_path_from_tem_xml
    prec_path = precipitation_data_path_from_tem_xml

    print(f"{temp_path}")
    print(f"{prec_path}")

else:
    # Use the alternative paths if the initial files are not found
    temperature_data_path_from_path_xml = find_file(
        script_dir / input_files.find("temperature_data").text
    )
    precipitation_data_path_from_path_xml = find_file(
        script_dir / input_files.find("precipitation_data").text
    )

    if (
        temperature_data_path_from_path_xml.exists()
        and precipitation_data_path_from_path_xml.exists()
    ):
        print(f"Using climate data from paths defined in: {biogeo_xml_path}")

        temp_path = temperature_data_path_from_path_xml
        prec_path = precipitation_data_path_from_path_xml

        print(f"{temp_path}")
        print(f"{prec_path}")
    else:
        raise FileNotFoundError(
            f"Error: Climate data files not found. Checked paths from {tem_xml_path} and {biogeo_xml_path}"
        )


limits_path = (script_dir / input_files.find("climate_limits").text).resolve()
pft_description_path = (script_dir / input_files.find("pft_description").text).resolve()

# Create directories if they don't exist
for path in [
    mean_monthly_moisture_stress_path,
    mean_summer_moisture_stress_path,
    limits_path,
]:
    path.parent.mkdir(parents=True, exist_ok=True)

# Construct output file paths
output_files = root.find("output_files")
npp_bakeoff_results_path = (
    script_dir / output_files.find("npp_bakeoff_results").text
).resolve()
early_century_persisting_pft_output_path = (
    script_dir / output_files.find("early_century_persisting_pft_output").text
).resolve()
mid_century_persisting_pft_output_path = (
    script_dir / output_files.find("mid_century_persisting_pft_output").text
).resolve()
end_century_persisting_pft_output_path = (
    script_dir / output_files.find("end_century_persisting_pft_output").text
).resolve()
bakeoff_results_dir_path = (
    script_dir / output_files.find("bakeoff_results_dir").text
).resolve()

for path in [
    early_century_persisting_pft_output_path,
    mid_century_persisting_pft_output_path,
    end_century_persisting_pft_output_path,
    bakeoff_results_dir_path,
]:
    path.parent.mkdir(parents=True, exist_ok=True)
