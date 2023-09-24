# Importing libraries and setting up the environment
from matplotlib.colors import ListedColormap, LinearSegmentedColormap
import matplotlib.colors as mcolors
from scipy.interpolate import griddata
from statsmodels.genmod.families import Binomial
from mpl_toolkits.basemap import Basemap
import matplotlib.ticker as plticker
import matplotlib.pyplot as plt
import scipy.stats as stats
import seaborn as sns
import pandas as pd
import numpy as np
import warnings
from scipy.stats import ttest_rel
from scipy.interpolate import Rbf
import numpy as np

warnings.filterwarnings("ignore")  # setting ignore as a parameter



# Define the colors to use in the colormap
colors = ["darkgoldenrod", "white", "#228b22"]

# Define the position of each color in the colormap
positions = [0.0, 0.5, 1.0]

# Create the colormap object by interpolating between the colors
YlFG = LinearSegmentedColormap.from_list("", list(zip(positions, colors)))

# Import End of Century Land Cover data
end_of_century_land_cover = pd.read_csv(
    "../data/agg_models_landcover.csv", names=["LON", "LAT", "POTVEG"]
)
end_of_century_land_cover = end_of_century_land_cover.drop_duplicates()
end_of_century_land_cover = end_of_century_land_cover.query(
    "POTVEG != 13 and POTVEG !=15"
)

# Function to read in data
def process_data(filepath, year):
    variable_data = pd.read_csv(
        filepath,
        names=[
            "LON",
            "LAT",
            "TMPVARNAME",
            "ICOHORT",
            "STANDAGE",
            "POTVEG",
            "CURRENTVEG",
            "SUBTYPE",
            "CMNT",
            "PSIPLUSC",
            "QLCON",
            "CAREA",
            "SUBAREA",
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
        ],
    )

    variable_data = variable_data.query(f"YEAR >= {year}")
    variable_data = variable_data.merge(
        end_of_century_land_cover, on=["LAT", "LON", "POTVEG"]
    )

    variable_data = variable_data.drop(
        columns=[
            "TMPVARNAME",
            "POTVEG",
            "ICOHORT",
            "STANDAGE",
            "CURRENTVEG",
            "SUBTYPE",
            "CMNT",
            "PSIPLUSC",
            "QLCON",
            "CAREA",
            "SUBAREA",
            "TOTAL",
            "MAX",
            # "AVE",
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
        ]
    )

    variable_data = (
        variable_data.groupby(["LON", "LAT", "YEAR"]).agg({"AVE": "mean"}).reset_index()
    )
    return variable_data


future_vegc_path = "VEGC.TEMOUT"
future_npp_path = "NPP.TEMOUT"
future_nep_path = "NEP.TEMOUT"
future_soilorgc_path = "SOILORGC.TEMOUT"

historical_vegc_path = "./historical/VEGC.TEMOUT"
historical_npp_path = "./historical/NPP.TEMOUT"
historical_nep_path = "./historical/NEP.TEMOUT"
historical_soilorgc_path = "./historical/SOILORGC.TEMOUT"




vegc_df_future = process_data(future_vegc_path, 2069)
npp_df_future = process_data(future_npp_path, 2069)
nep_df_future = process_data(future_nep_path, 2069)
soilorgc_df_future = process_data(future_soilorgc_path, 2069)

vegc_df_historical = process_data(historical_vegc_path, 1984)
npp_df_historical = process_data(historical_npp_path, 1984)
nep_df_historical = process_data(historical_nep_path, 1984)
soilorgc_df_historical = process_data(historical_soilorgc_path, 1984)


# Function to merge historical and future models on LON, LAT, group by LON, LAT,
# and calculate the mean of the historical and future models in each grid, then calculate the % difference between the two time periods,
# perform a t-test on each grid to test if the change is significant, and if the p-value is less than 0.05, change pvalue to 1, otherwise leaave blank


def calculate_difference(historical_model, future_model):
    merged = pd.merge(
        historical_model,
        future_model,
        on=["LON", "LAT"],
        suffixes=["_HISTORICAL", "_FUTURE"],
    )

    gridded = (
        merged.groupby(["LON", "LAT"])
        .agg({"AVE_HISTORICAL": "mean", "AVE_FUTURE": "mean"})
        .reset_index()
    )

    # Calculate the difference between the historical and future models
    gridded["DIFFERENCE"] = gridded["AVE_FUTURE"] - gridded["AVE_HISTORICAL"]

    # Calculate the percent difference between the historical and future models
    gridded["PERCENT_DIFFERENCE"] = (
        (gridded["AVE_FUTURE"] - gridded["AVE_HISTORICAL"])
        / gridded["AVE_HISTORICAL"]
        * 100
    )

    gridded["t_statistic"], gridded["p_value"] = ttest_rel(
        gridded["AVE_HISTORICAL"], gridded["AVE_FUTURE"]
    )
    gridded["p_value"] = gridded["p_value"].apply(lambda x: 1 if x < 0.05 else 0)

    return gridded


# Function to calculate the IQR, Identify the rows with values outside the range and Drop the rows with outliers and return the dataframe
def remove_outliers(df):
    Q1 = df["PERCENT_DIFFERENCE"].quantile(0.25)
    Q3 = df["PERCENT_DIFFERENCE"].quantile(0.75)
    IQR = Q3 - Q1
    lower_cutoff = Q1 - 1.5 * IQR
    upper_cutoff = Q3 + 1.5 * IQR
    outliers = (df["PERCENT_DIFFERENCE"] < lower_cutoff) | (df["PERCENT_DIFFERENCE"] > upper_cutoff)
    mean = df["PERCENT_DIFFERENCE"].mean()
    df.loc[outliers, "PERCENT_DIFFERENCE"] = mean
    return df


vegc_difference = remove_outliers(calculate_difference(vegc_df_historical, vegc_df_future))
npp_difference = remove_outliers(calculate_difference(npp_df_historical, npp_df_future))
nep_difference = remove_outliers(calculate_difference(nep_df_historical, nep_df_future))
soilorgc_difference = remove_outliers(calculate_difference(soilorgc_df_historical, soilorgc_df_future))

# Drop the rows where the condition is True
# npp_difference.drop(npp_difference[npp_difference['PERCENT_DIFFERENCE'] > 100].index, inplace=True)



# Plot the data
data_list = [vegc_difference, soilorgc_difference,npp_difference, nep_difference, ]
subplot_titles =["a) Vegetation Carbon", " b) Soil Organic Carbon"," d) Net Primary Productivity", " c) Net Ecosystem Productivity" ]

# Create the subplot
fig, ax = plt.subplots(nrows=2, ncols=2, figsize=(10, 10), sharex=True, sharey=True)

# Iterate over the data to plot
for i, data in enumerate(data_list):
    lons =np.arange(-125, -104.5,0.5)
    lats = np.arange(31,49,0.5)
    
    for lon in lons:
        for lat in lats:
            if not ((data["LON"]==lon)&(data["LAT"]==lat)).any():
                
                data = pd.concat([data, pd.DataFrame({"LON": lon, "LAT": lat, "PERCENT_DIFFERENCE": [-99999], "p_value":[1]})], ignore_index=True)
                
    data =data.sort_values(by=["LON","LAT"])
    
    lon_new, lat_new = np.meshgrid(np.arange(-125, -104.5, 0.5), np.arange(31, 49, 0.5))
    z = griddata((data["LON"], data["LAT"]), data["PERCENT_DIFFERENCE"], (lon_new, lat_new), method="linear")
    
    # Select the axis to plot on
    subplot = ax.flat[i]

    sns.set_theme(style="ticks", font="sans-serif", rc={"lines.linewidth": 2.5})
    m = Basemap(
        projection="mill",
        llcrnrlat=31,
        llcrnrlon=-125,
        urcrnrlat=49,
        urcrnrlon=-104.5,
        resolution="l",
        ax=subplot  # Use the selected axis to plot
    )
    m.drawcoastlines()
    m.drawcountries()
    m.drawstates()
    x, y = m(lon_new, lat_new)

    # set the color of grids with PERCENT_DIFFERENCE == -99999 to white
    z_masked = np.ma.masked_where(z == -99999, z)

    pcm = m.pcolormesh(x, y, z_masked, cmap="viridis_r", edgecolor="white", linewidth=0.5)

    # mark grids with p_value ==1 with black bullet and PERCENT_DIFFERENCE != -99999
    """
    for lati in range(len(lats)-1):
        for loni in range(len(lons)-1):
            if (data[(data.LAT>=lats[lati])&(data.LAT<=lats[lati+1])&\
                    (data.LON>=lons[loni])&(data.LON<=lons[loni+1])].p_value.values[0]==1) & \
                    (data[(data.LAT>=lats[lati])&(data.LAT<=lats[lati+1])&\
                    (data.LON>=lons[loni])&(data.LON<=lons[loni+1])].PERCENT_DIFFERENCE.values[0]!=-99999):
                            plt.text(x[0][loni], y[lati][0], "•", fontsize=5, ha="center", va="center", color="black", transform=subplot.transdata)"""

    # Add colorbar for each plot
    cbar = plt.colorbar(pcm, orientation="vertical", shrink=0.5, pad=0.02)
    cbar.set_label(label="% change", labelpad=5)
    
    # set the intervals for parallels and meridians
    lat_interval = 5
    lon_interval = 5

    # draw parallels and meridians
    parallels = np.arange(25, 50, lat_interval)
    meridians = np.arange(-130, -104.5, lon_interval)
    m.drawparallels(
        parallels, labels=[True, False, False, False], fontsize=12, linewidth=0.001 ,dashes=[4,9900]
    )
    m.drawmeridians(
        meridians, labels=[False, False, False, True], fontsize=12, linewidth=0.001,dashes=[4,9900])

    # Set the title for the subplot
    subplot.set_title(f"{subplot_titles[i]}",fontweight='bold')

# Adjust the spacing between subplots
plt.subplots_adjust(hspace=0.05)

plt.savefig("./biogeo/difference.png", dpi=1200, format="png", bbox_inches="tight")



