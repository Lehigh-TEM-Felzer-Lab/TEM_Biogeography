import dependencies 
from paths import climate_limits_path, temperature_data_path, precipitation_data_path



climate_data_columns= [
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
    "REGION"
    ]
        
temperature_data = dependencies.pd.read_csv(temperature_data_path, names=climate_data_columns)
precipitation_data = dependencies.pd.read_csv(precipitation_data_path, names=climate_data_columns)




# Calculate days in month using numpy broadcasting
years = temperature_data["YEAR"].values
months = dependencies.np.arange(1, 13)
days_in_month_arr = dependencies.np.array([dependencies.calendar.monthrange(year, month)[1] for year in years for month in months]).reshape(-1, 12)
temperature_data[[f"{dependencies.calendar.month_name[month][:3].upper()}_DAYS" for month in range(1, 13)]] = days_in_month_arr

# Calculate monthly GDD
temperature_monthly_data = temperature_data[["JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]].values
days_in_month_arr = temperature_data[[f"{dependencies.calendar.month_name[month][:3].upper()}_DAYS" for month in range(1, 13)]].values
monthly_gdd = dependencies.np.maximum(temperature_monthly_data - 5, 0) * days_in_month_arr
temperature_data[[f"{dependencies.calendar.month_name[month][:3].upper()}_GDD" for month in range(1, 13)]] = monthly_gdd

# Calculate total GDD for each year
temperature_data["TOTAL_GDD"] = dependencies.np.sum(monthly_gdd, axis=1)

temperature_data = temperature_data[climate_data_columns + ["TOTAL_GDD"]]


climate = dependencies.pd.DataFrame()
climate["LON"] = temperature_data["LON"] # The longitude coordinate of the location being modelled.
climate["LAT"] = temperature_data["LAT"] # The latitude coordinate of the location being modelled.
climate["YEAR"] = temperature_data["YEAR"] # Year
climate["T_MAX"] = temperature_data["MAX"] # Mean temp of the hottest month
climate["T_MIN"] = temperature_data["MIN"] # Mean temp of the coldest month
climate["GDD"] = temperature_data["TOTAL_GDD"] # Total annual growing degree days
climate["TOTAL_PREC"] = precipitation_data["TOTAL"] # Total annual precipitation




# load the JSON data
with open(climate_limits_path, 'r') as f:
    data = dependencies.json.load(f)

# flatten the JSON data into a dataframe
climate_limits = dependencies.pd.json_normalize(data)

climate_limits_names = {
    "Description":"DESC",
    "Plant Functional Type": "POTVEG",
    "Subtype": "SUBTYPE",
    "Minimum Growing Degree Days": "MIN_GDD",
    "Maximum Growing Degree Days": "MAX_GDD",
    "Mean Temperature of Coldest Month": "Tc",
    "Mean Temperature of Warmest Month": "Tw",
    "Minimum Monthly Precipitation": "MIN_PREC",
    "Minimum Annual Actual Evapotranspiration/Potential Evapotranspiration": "MIN_AET/PET"
}


# rename the columns using the dictionary
climate_limits = climate_limits.rename(columns=climate_limits_names)




