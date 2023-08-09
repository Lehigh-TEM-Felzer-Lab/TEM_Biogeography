TEM_OUTPUT_COLUMNS = [
    "LON",  # The longitude coordinate of the location being modelled.
    "LAT",  # The latitude coordinate of the location being modelled.
    "TMPVARNAME",  # The name of the output variable..
    "ICOHORT",  # An index indicating the cohort number of each PFT in a grdcell.
    "STANDAGE",  # The age of the vegetation stand in years.
    "POTVEG",  # The potential natural vegetation type for the location being modelled.
    "CURRENTVEG",  # The current vegetation type for the location being modelled.
    "SUBTYPE",  # A subtype of the PFT being modeled.
    "CMNT",
    "PSIPLUSC",
    "QLCON",
    "CAREA",  # The total area of the grid cell
    "SUBAREA",  # The sub-area of each PFT in the grid cell
    "YEAR",  # The year in which the data was collected.
    "TOTAL",  # The total value of the variable being modelled
    "MAX",  # The maximum value of the variable being modelled
    "AVE",  # The average value of the variable being modelled
    "MIN",  # The minimum value of the variable being modelled
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
    "DEC",  # The values of the variable being measured for each month of the year.
    "REGION",  # The region or area where the data was collected.
]


TEM_CLIMATE_COLUMNS= [
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


MEAN_MONTHLY_MOISTURE_STRESS_COLUMNS = [
    "LON",              # The longitude coordinate of the location being modelled.
    "LAT",              # The latitude coordinate of the location being modelled.
    "ICOHORT",          # An index indicating the cohort of the vegetation being modelled.
    "POTVEG",           # The potential natural vegetation type for the location being modelled.
    "SUBTYPE",          # A subtype of the vegetation being modelled.
    "MONTH",            # Month.
    "YEAR",             # Year.
    "AET",              # The actual evapotranspiration for the location and time period being modelled.
    "PET",              # The potential evapotranspiration for the location and time period being modelled.
    "AET/PET",          # The ratio of actual to potential evapotranspiration for the location and time period being modelled.
    "PET-AET/PET",      # The difference between potential and actual evapotranspiration, normalized by potential evapotranspiration, for the location and time period being modelled.
    "THETA",            # The soil water potential for the location and time period being modelled.
]

MEAN_SUMMER_MOISTURE_STRESS_COLUMNS = [
    "LON", # The longitude coordinate of the location being modelled.
    "LAT", # The latitude coordinate of the location being modelled.
    "POTVEG", # The potential natural vegetation type for the location being modelled.
    "SUBTYPE", # A subtype of the vegetation being modelled.
    "YEAR", # Year.
    "AET", # The actual evapotranspiration for the location and time period being modelled. 
    "PET", # The potential evapotranspiration for the location and time period being modelled.
    "AET/PET", # The ratio of actual to potential evapotranspiration for the location and time period being modelled.
    "(PET-AET)/PET", # The difference between potential and actual evapotranspiration, normalized by potential evapotranspiration, for the location and time period being modelled.
    "THETA", # The soil water potential for the location and time period being modelled.
]

CLIMATE_LIMITS_COLUMNS = {
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