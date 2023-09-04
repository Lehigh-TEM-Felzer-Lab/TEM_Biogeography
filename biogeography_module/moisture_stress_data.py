import dependencies 
from paths import mean_monthly_moisture_stress_path, mean_summer_moisture_stress_path
from columns import MEAN_MONTHLY_MOISTURE_STRESS_COLUMNS, MEAN_SUMMER_MOISTURE_STRESS_COLUMNS


# Import moisture stress data (Drought Index)


mean_monthly_moisture_stress = dependencies.pd.read_csv(mean_monthly_moisture_stress_path, names=MEAN_MONTHLY_MOISTURE_STRESS_COLUMNS)

# Select summer months (May - October)
mean_monthly_summer_moisture_stress = mean_monthly_moisture_stress.query("MONTH >= 5 & MONTH <= 10")

# Calculate mean annual values
mean_summer_moisture_stress = (mean_monthly_summer_moisture_stress.groupby(["LON", "LAT", "POTVEG", "SUBTYPE", "YEAR"])
                        .agg({"AET": "sum", "PET": "sum", "AET/PET": "mean", "PET-AET/PET": "sum", "THETA": "mean"})
                        .round(3))

# save mean annual values to csv then import it again to save memory space
mean_summer_moisture_stress.to_csv(mean_summer_moisture_stress_path, index=True, header=False)

# Import mean moisture annual moisture stress data (Drought Index)
mean_summer_moisture_stress = dependencies.pd.read_csv(mean_summer_moisture_stress_path, names=MEAN_SUMMER_MOISTURE_STRESS_COLUMNS)
