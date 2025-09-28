import cartopy.crs as ccrs
import cartopy.feature as cfeature
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from matplotlib.colors import BoundaryNorm, ListedColormap
from matplotlib.patches import Patch

# Import Current Landcover Data
historical_model = pd.read_csv(
    "./biogeo/input/current_land_cover.csv",
    names=["LON", "LAT", "TYPE"],
)


future_model = pd.read_csv(
    "./biogeo/output_bakeoff/end.shift",
    names=["LON", "LAT", "TYPE"],
)


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
    return [
        Patch(color=color, label=label)
        for color, label in zip(cmap.colors, vegetation_list)
    ]


# Create a cartopy map with Miller projection
# Set Seaborn theme
sns.set_theme(style="ticks", font="sans-serif", rc={"lines.linewidth": 2.5})
fig, ax = plt.subplots(
    nrows=1,
    ncols=2,
    figsize=(12, 6),
    sharex=True,
    sharey=True,
    subplot_kw={"projection": ccrs.Miller()},
)
ax[0].set_extent([-125, -104.5, 31, 48.5])
ax[1].set_extent([-125, -104.5, 31, 48.5])

data_list = [historical_model, future_model]

for i, data in enumerate(data_list):
    lats = data["LAT"].values
    lons = data["LON"].values
    types = data["TYPE"].values

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
    mesh = ax[i].pcolormesh(
        lon,
        lat,
        grid_types,
        cmap=cmap,
        norm=norm,
        edgecolors="white",
        linewidth=0.5,
        transform=ccrs.PlateCarree(),
        shading="auto",
    )
    ax[i].add_feature(cfeature.COASTLINE)
    ax[i].add_feature(cfeature.BORDERS, linestyle="-", alpha=0.2)
    ax[i].add_feature(cfeature.STATES, linestyle="-", alpha=0.2)
    ax[i].set_title(
        (
            "a) Current dominant PFTs (1984 - 2014)"
            if i == 0
            else "b) Modeled change in dominant PFTs (2070 - 2100)"
        ),
        pad=10,
    )

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


plt.savefig("./biogeo/result.png", format="png", dpi=1200, bbox_inches="tight")
plt.show()
