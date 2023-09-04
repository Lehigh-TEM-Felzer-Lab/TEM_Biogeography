# Importing libraries and setting up the environment
import warnings

warnings.filterwarnings("ignore")  # setting ignore as a parameter
import numpy as np
import pandas as pd
import seaborn as sns
import scipy.stats as stats
import matplotlib.pyplot as plt
import matplotlib.ticker as plticker
from mpl_toolkits.basemap import Basemap
from statsmodels.genmod.families import Binomial
from matplotlib.colors import ListedColormap, LinearSegmentedColormap
from matplotlib.patches import Patch
from scipy.interpolate import Rbf
from scipy.interpolate import griddata


# Historical Land Cover Color Map
current_vegetation_cmap = ListedColormap(
     ["#0F52BA", "#9ACD32", "#1b9d77", "#7570b3", "#A4D4B4", "#004953"]
)


# Set Future Land Cover Color Map
future_vegetation_cmap = ListedColormap(
    [
        "#0F52BA",
        "#9ACD32",
        "#1b9d77",
        "#7570b3",
        "#EEBC1D",
        "#cb7e0c",
        "#A4D4B4",
        "#004953",
    ]
)

# Create a list of labels for the different colors in the colormap
current_vegetation_list = [
    "Boreal Forests",
    "Mixed Temperate Forests",
    "Temperate Coniferous Forests",
    "Temperate Deciduous Forests",
    "Xeromorphic Forests and Woodlands",
    "Temperate Broadleaved Evergreen Forests",
]
future_vegetation_list = [
    "Boreal Forests",
    "Mixed Temperate Forests",
    "Temperate Coniferous Forests",
    "Temperate Deciduous Forests",
    "Short Grasslands",
    "Arid Shrublands",
    "Xeromorphic Forests and Woodlands",
    "Temperate Broadleaved Evergreen Forests",
]

# In[35]:


# Import Current Landcover Data
historical_model = pd.read_csv("./biogeo/input/current_land_cover.csv",
    names=["LON", "LAT", "VEGETATION_TYPE"],
)


# Create a dictionary to map the PFT identifiers to the new ones
mapping_historical = {10: 20, 4: 5, 8: 10, 9: 15, 19: 25, 33: 30}
historical_model["VEGETATION_TYPE"] = historical_model["VEGETATION_TYPE"].map(
    mapping_historical
)

# In[36]:


# Import  Future Models Data
future_model = pd.read_csv("./biogeo/output_bakeoff/end.shift",
    names=["LON", "LAT", "VEGETATION_TYPE"],
)


# Create a dictionary to map the PFT identifiers to the new ones
mapping_future = {15: 30, 10: 20, 4: 5, 8: 10, 9: 15, 13: 25, 19: 35, 33: 40}
future_model["VEGETATION_TYPE"] = future_model["VEGETATION_TYPE"].map(mapping_future)

# In[38]:




# Create a list of data to plot in each subplot
data_list = [historical_model, future_model]

fig, ax = plt.subplots(nrows=1, ncols=2, figsize=(12, 6), sharex=True, sharey=True)

for i, data in enumerate(data_list):
    # add missing grids with -99999 value
    lons = np.arange(-125, -104.5, 0.5)
    lats = np.arange(31, 49, 0.5)
    for lon in lons:
        for lat in lats:
            if not ((data["LON"] == lon) & (data["LAT"] == lat)).any():
              
                data= pd.concat([data, pd.DataFrame({"LON": lon, "LAT": lat, "VEGETATION_TYPE": [-99999]})], ignore_index=True)

    # sort the data by longitude and latitude
    data = data.sort_values(by=["LON", "LAT"])

    # interpolate data to 0.25 by 0.25
    lon_new, lat_new = np.meshgrid(np.arange(-125, -104.5, 0.5), np.arange(31, 49, 0.5))
    z = griddata((data["LON"], data["LAT"]), data["VEGETATION_TYPE"], (lon_new, lat_new), method="linear")

    # plot the data using pcolormesh
    ax[i].set_title("a) Current dominant PFTs (1984 - 2014)" if i == 0 else "b) Modeled change in dominant PFTs (2070 - 2100)", pad=10)
    
    # Get the appropriate colormap for the subplot
    if i == 0:
        cmap = current_vegetation_cmap
    else:
        cmap = future_vegetation_cmap
        
        
    sns.set_theme(style="ticks", font="sans-serif", rc={"lines.linewidth": 2.5})
    m = Basemap(
        projection="mill",
        llcrnrlat=31,
        llcrnrlon=-125,
        urcrnrlat=49,
        urcrnrlon=-104.5,
        resolution="l",
        ax=ax[i]
    )
    m.drawcoastlines()
    m.drawcountries()
    m.drawstates()
    x, y = m(lon_new, lat_new)

    # set the color of grids with VEGETATION_TYPE == -99999 to white
    z_masked = np.ma.masked_where(z == -99999, z)
    
    
    

    m.pcolormesh(x, y, z_masked, cmap=cmap,edgecolor="white", linewidth=0.5)

    # set the intervals for parallels and meridians
    lat_interval = 5
    lon_interval = 5

    # draw parallels and meridians
    parallels = np.arange(25, 50, lat_interval)
    meridians = np.arange(-130, -104.5, lon_interval)
    m.drawparallels(
        parallels, labels=[True, False, False, False], fontsize=12, linewidth=0.001
    )
    m.drawmeridians(
        meridians, labels=[False, False, False, True], fontsize=12, linewidth=0.001
    )

    # Create a list of patches for the legend
    patches = [
        Patch(color=color, label=label) for color, label in zip(future_vegetation_cmap.colors, future_vegetation_list)
    ]

 

 # Create a list of patches for the legend
    patches = [
        Patch(color=color, label=label) for color, label in zip(future_vegetation_cmap.colors, future_vegetation_list)
    ]

# Add the legend to the figure
fig.legend(
    handles=patches,
    bbox_to_anchor=(0.5, -0.1),
    loc="lower center",
    ncol=3,
    borderaxespad=0.5,
    frameon=False,
)

plt.savefig("image.png", dpi=1200, bbox_inches="tight")
plt.show()


