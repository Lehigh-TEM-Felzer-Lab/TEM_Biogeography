import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.colors import ListedColormap, BoundaryNorm
import cartopy.crs as ccrs
import cartopy.feature as cfeature
import seaborn as sns

# Import Data
current = pd.read_csv("./biogeo/input/current_land_cover.csv", names=["LON", "LAT", "TYPE"])
current_modelled = pd.read_csv("./historical/modeled_current_land_cover.csv", names=["LON", "LAT", "TYPE"])
future_model_rcp45 = pd.read_csv("./biogeo/output_bakeoff/end.shift", names=["LON", "LAT", "TYPE"])
future_model_rcp85 = pd.read_csv("/users/jkodero/scratch/tem/8.5/runs/biogeo/output_bakeoff/end.csv", names=["LON", "LAT", "TYPE"])

# Titles for each subplot
titles = [
    "a) Dominant PFTs (1984-2014)",
    "b) Modeled Dominant PFTs (1984-2014)",
    "c) RCP 4.5 Change in PFTs (2070-2100)",
    "d) RCP 8.5 Change in PFTs (2070-2100)"
]

# List of DataFrames
data_list = [current, current_modelled, future_model_rcp45, future_model_rcp85]

# Visualization
sns.set_theme(style="ticks", font="sans-serif", rc={"lines.linewidth": 2.5})
fig, axarr = plt.subplots(nrows=2, ncols=2, figsize=(13, 13), sharex=True, sharey=True, subplot_kw={'projection': ccrs.Miller()})
for ax in axarr.ravel():
    ax.set_extent([-125, -104.5, 31, 48.5])
    gl = ax.gridlines(crs=ccrs.PlateCarree(), draw_labels=True, linewidth=0, color='gray', alpha=0.5, linestyle='--')
    gl.top_labels = False
    gl.right_labels = False
    gl.xlocator = plt.MultipleLocator(5)
    gl.ylocator = plt.MultipleLocator(5)


for i, data in enumerate(data_list):
    ax = axarr[i//2, i%2]  # This will arrange the axes in a 2x2 grid
    lats = data['LAT'].values
    lons = data['LON'].values
    types = data['TYPE'].values


        # Create a mesh grid
    unique_lats = np.unique(lats)
    unique_lons = np.unique(lons)
    lon, lat = np.meshgrid(unique_lons, unique_lats)
    grid_types = np.empty(lon.shape, dtype=float)

    for m, ulat in enumerate(unique_lats):
        for n, ulon in enumerate(unique_lons):
            mask = (lats == ulat) & (lons == ulon)
            if mask.any():
                grid_types[m, n] = types[mask][0]
            else:
                grid_types[m, n] = np.nan

    # Use different color mappings for the two subplots
    if i == 0:
        colors_map = {
            4: "#0F52BA",
            8: "#9ACD32",
            9: "#1b9d77",
            10: "#7570b3",
            19: "#A4D4B4",
            33: "#004953",
        }
    else:
        colors_map = {
            4: "#0F52BA",
            8: "#9ACD32",
            9: "#1b9d77",
            10: "#7570b3",
            13: "#EEBC1D",
            15: "#cb7e0c",
            19: "#A4D4B4",
            33: "#004953",
        }
    
    cmap = ListedColormap(list(colors_map.values()))
    norm = BoundaryNorm(list(colors_map.keys()), cmap.N)

    # Use pcolormesh to plot the grid with white borders
    mesh = ax.pcolormesh(lon, lat, grid_types, cmap=cmap, norm=norm, edgecolors='white', linewidth=0.5, transform=ccrs.PlateCarree(), shading='auto')

    ax.add_feature(cfeature.COASTLINE)
    ax.add_feature(cfeature.BORDERS, linestyle='-', alpha=.2)
    ax.add_feature(cfeature.STATES, linestyle='-', alpha=.2)
    ax.set_title(titles[i], pad=10, fontweight='bold')  # Set title with bold font

# Future vegetation colormap and list
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

def create_patches(cmap, vegetation_list):
    return [Patch(color=color, label=label) for color, label in zip(cmap.colors, vegetation_list)]
# Create patches for the legend
patches = create_patches(future_vegetation_cmap, future_vegetation_list)

# Add the legend to the figure
fig.legend(
    handles=patches,
    bbox_to_anchor=(0.5, -0.1),
    loc="lower center",
    ncol=3,
    borderaxespad=0.5,
    frameon=False,
)
plt.subplots_adjust(wspace=0.1, hspace=0.3)  # Adjust these values as needed
plt.tight_layout()
plt.subplots_adjust(hspace=0.2)

plt.savefig("./biogeo/all.png", format="png", dpi=1200, bbox_inches="tight")
plt.show()

