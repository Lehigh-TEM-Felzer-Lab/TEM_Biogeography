from pathlib import Path

import dependencies
from columns import tem_out_cols

txt_filepath = (
    Path.cwd() / "tem_in.txt"
)  # Path to the txt file containing the path to the XML file


with open(txt_filepath, "r") as f:
    tem_xml_path = f.read().strip()
    print(f"XML file used to run TEM found in txt file 'tem_in.txt': {tem_xml_path}")

if tem_xml_path:
    # Parse the XML
    try:
        tree = dependencies.ET.parse(tem_xml_path)
        root = tree.getroot()
    except FileNotFoundError:
        print(f"ERROR: xml file path not found in txt file: {tem_xml_path}")
        root = None

    if root:
        # Get temoutvars and temoutfiles from the XML
        temoutvars = root.find("temoutvars").text
        temoutfiles = root.find("temoutfiles").text
        itairfname = root.find("itairfname").text + root.find("itairend").text
        iprecfname = root.find("iprecfname").text + root.find("iprecend").text
        clmstartyr = root.find("clmstartyr").text
        mxnumgrid = root.find("mxnumgrid").text
        transtime = root.find("transtime").text

        # Split the comma-separated strings and store them in dictionaries
        var_list = temoutvars.split(",")
        file_list = temoutfiles.split(",")

        # Convert file paths to be compatible with the current OS
        file_list = [
            dependencies.os.path.abspath(
                dependencies.os.path.join(
                    dependencies.os.getcwd(),
                    file.replace("/", dependencies.os.sep).replace(
                        "\\", dependencies.os.sep
                    ),
                )
            )
            for file in file_list
        ]

        var_file_dict = dict(
            zip(var_list, file_list)
        )  # Dictionary of variable names and their corresponding file paths

        # Function to read CSV into DataFrame
        def read_csv_add_npp(var_name, file_name):
            df = dependencies.pd.read_csv(file_name, names=tem_out_cols)
            return df

        # Call the function for each variable and store the result in a dictionary
        dataframes = {}
        for var, file in var_file_dict.items():
            var_lower = var.lower()
            dataframes[var_lower] = read_csv_add_npp(var_lower, file)

        # Add NPP column to all dataframes
        npp_df = dataframes["npp"]
        for var, df in dataframes.items():
            df["NPP"] = npp_df["TOTAL"]
