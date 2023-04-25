import os
import re
import pandas as pd


base_path = r"T:\m8\kodero\runs"
model_names = ['bcc_csm1_1', 'bcc_csm1_1_m', 'bnu_esm', 'canesm2', 'ccsm4', 'cnrm_cm5', 'csiro_mk3_6_0', 
            'gfdl_esm2g', 'gfdl_esm2m', 'hadgem2_cc365', 'hadgem2_es365', 'inmcm4', 'ipsl_cm5a_lr',
            'ipsl_cm5a_mr', 'ipsl_cm5b_lr', 'miroc5', 'miroc_esm', 'miroc_esm_chem', 'mri_cgcm3',
            'noresm1_m']

climate ="climate"

for model_name in model_names:
    was_path = os.path.join(base_path, model_name, climate)
    print(model_name)
    for file in os.listdir(was_path):
        if file.startswith("was_"):
            # read the file into a pandas DataFrame
            df = pd.read_csv(os.path.join(was_path, file),
                             names=[
                                "LON",
                                "LAT",
                                "VARNAME",
                                "AREA",
                                "YEAR",
                                "TOTAL",
                                "MAX",
                                "AVE",
                                "MIN",
                                "JAN",
                                "FEB",
                                "MAR",
                                "APR",
                                "MAY",
                                "JUN",
                                "JUL",
                                "AUG",
                                "SEP",
                                "OCT",
                                "NOV",
                                "DEC",
                                "REGION",
                            ])
            
            print(was_path, file)
            # group by LON and LAT, take the mean of all columns, set YEAR to -999, and round monthly values to 1 decimal place
            df = df.groupby(['LON', 'LAT'], as_index=False).agg({"AREA": "mean", "TOTAL": "mean", "MAX": "mean", "AVE": "mean", "MIN": "mean", "JAN": "mean", "FEB": "mean", "MAR": "mean", "APR": "mean", "MAY": "mean", "JUN": "mean", "JUL": "mean", "AUG": "mean", "SEP": "mean", "OCT": "mean", "NOV": "mean", "DEC": "mean"})
            df['YEAR'] = -999
            df["VARNAME"] = " WIND "
            df['REGION']= " WEST "
   
            
            
          # Round columns to specified data types
            # Set individual columns to specified data types
            df["LON"] = df["LON"].astype(float).round(4)
            df["LAT"] = df["LAT"].astype(float).round(4)
            df["AREA"] = df["AREA"].astype(int)
            df["YEAR"] = df["YEAR"].astype(int)
            df["VARNAME"] = df["VARNAME"].astype(str)
            df["REGION"] = df["REGION"].astype(str)
            df["TOTAL"] = df["TOTAL"].astype(float).round(1)
            df["MAX"] = df["MAX"].astype(float).round(1)
            df["AVE"] = df["AVE"].astype(float).round(1)
            df["MIN"] = df["MIN"].astype(float).round(1)
            df["JAN"] = df["JAN"].astype(float).round(1)
            df["FEB"] = df["FEB"].astype(float).round(1)
            df["MAR"] = df["MAR"].astype(float).round(1)
            df["APR"] = df["APR"].astype(float).round(1)
            df["MAY"] = df["MAY"].astype(float).round(1)
            df["JUN"] = df["JUN"].astype(float).round(1)
            df["JUL"] = df["JUL"].astype(float).round(1)
            df["AUG"] = df["AUG"].astype(float).round(1)
            df["SEP"] = df["SEP"].astype(float).round(1)
            df["OCT"] = df["OCT"].astype(float).round(1)
            df["NOV"] = df["NOV"].astype(float).round(1)
            df["DEC"] = df["DEC"].astype(float).round(1)

            # Rearrange columns in the desired order
            df = df[["LON", "LAT", "VARNAME", "AREA", "YEAR", "TOTAL", "MAX", "AVE", "MIN",
                    "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC", "REGION"]]
            print()
            
            print(df.head(5))
            print()
            print()
            # save the DataFrame with the original file name
            df.to_csv(os.path.join(was_path, file), header=False, index=False)
