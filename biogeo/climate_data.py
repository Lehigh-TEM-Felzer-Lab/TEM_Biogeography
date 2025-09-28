import calendar
import json

import numpy as np
import pandas as pd
from columns import climate_limits, tem_clim_cols
from paths import limits_path, prec_path, temp_path

temperature_data = pd.read_csv(temp_path, names=tem_clim_cols)
precipitation_data = pd.read_csv(prec_path, names=tem_clim_cols)


# Calculate days in month using numpy broadcasting
years = temperature_data["YEAR"].values
months = np.arange(1, 13)
days_in_month_arr = np.array(
    [calendar.monthrange(year, month)[1] for year in years for month in months]
).reshape(-1, 12)
temperature_data[
    [f"{calendar.month_name[month][:3].upper()}_DAYS" for month in range(1, 13)]
] = days_in_month_arr

# Calculate monthly GDD
temperature_monthly_data = temperature_data[
    ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
].values
days_in_month_arr = temperature_data[
    [f"{calendar.month_name[month][:3].upper()}_DAYS" for month in range(1, 13)]
].values
monthly_gdd = np.maximum(temperature_monthly_data - 5, 0) * days_in_month_arr
temperature_data[
    [f"{calendar.month_name[month][:3].upper()}_GDD" for month in range(1, 13)]
] = monthly_gdd

# Calculate total GDD for each year
temperature_data["TOTAL_GDD"] = np.sum(monthly_gdd, axis=1)

temperature_data = temperature_data[tem_clim_cols + ["TOTAL_GDD"]]


climate = pd.DataFrame()
climate["LON"] = temperature_data[
    "LON"
]  # The longitude coordinate of the location being modelled.
climate["LAT"] = temperature_data[
    "LAT"
]  # The latitude coordinate of the location being modelled.
climate["YEAR"] = temperature_data["YEAR"]  # Year
climate["T_MAX"] = temperature_data["MAX"]  # Mean temp of the hottest month
climate["T_MIN"] = temperature_data["MIN"]  # Mean temp of the coldest month
climate["GDD"] = temperature_data["TOTAL_GDD"]  # Total annual growing degree days
climate["TOTAL_PREC"] = precipitation_data["TOTAL"]  # Total annual precipitation

# load the JSON data
with open(limits_path, "r") as f:
    data = json.load(f)

# flatten the JSON data into a dataframe
climate_limits = pd.json_normalize(data)
# rename the columns using the dictionary
climate_limits = climate_limits.rename(columns=climate_limits)
