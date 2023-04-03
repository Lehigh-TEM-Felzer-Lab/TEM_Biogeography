import dependencies 
from paths import moisture_stress_path, mean_moisture_stress_path


# Import moisture stress data (Drought Index)
moisture_stress_columns = [
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

moisture_stress = dependencies.pd.read_csv(moisture_stress_path, names=moisture_stress_columns)

# Select summer months (May - October)
moisture_stress = moisture_stress.query("MONTH >= 5 & MONTH <= 10")

# Calculate mean annual values
mean_moisture_stress = (moisture_stress.groupby(["LON", "LAT", "POTVEG", "SUBTYPE", "YEAR"])
                        .agg({"AET": "sum", "PET": "sum", "AET/PET": "mean", "PET-AET/PET": "sum", "THETA": "mean"})
                        .round(3))

# save mean annual values to csv then import it again to save memory space
mean_moisture_stress.to_csv(mean_moisture_stress_path, index=True, header=False)

# Import mean moisture annual moisture stress data (Drought Index)
mean_moisture_stress_columns = [
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

mean_moisture_stress = dependencies.pd.read_csv(mean_moisture_stress_path, names=mean_moisture_stress_columns)
