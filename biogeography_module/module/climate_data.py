import dependencies 
from paths import climate_path, climate_limits_path


# Import climate data
climate_columns = [
    "LON",          # The longitude coordinate of the location being modelled.
    "LAT",          # The latitude coordinate of the location being modelled.
    "YEAR",         # The year in which the climate data was collected.
    "T_MAX",        # The maximum temperature in degrees Celsius for the location and year being modelled.
    "T_MIN",        # The minimum temperature in degrees Celsius for the location and year being modelled.
    "GDD",          # The growing degree days for the location and year being modelled.
    "TOTAL_PREC"    # The total precipitation in millimeters for the location and year being modelled.
]

climate = dependencies.pd.read_csv(climate_path, names=climate_columns)

# Import climate limits data
climate_limits_columns = [
    "POTVEG",           # The potential natural vegetation type for the location being modelled.
    "SUBTYPE",          # A subtype of the vegetation being modelled.
    "MIN_GDD",          # The minimum growing degree days required for the vegetation to grow.
    "MAX_GDD",          # The maximum growing degree days allowed for the vegetation to grow.
    "Tc",               # Mean temperature of the coldest month.
    "Tw",               # Mean temperature of the warmest month.
    "MIN_PREC",         # The minimum monthly precipitation required for the vegetation to grow.
    "MIN_AET/PET",      # The minimum monthly actual / potential evapotranspiration required for the vegetation to grow.
]

climate_limits = dependencies.pd.read_csv(climate_limits_path, names=climate_limits_columns)
