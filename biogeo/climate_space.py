# Importing libraries and setting up the environment
import warnings

warnings.filterwarnings("ignore")  # setting ignore as a parameter
import json
import os

import matplotlib.lines as mlines
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from columns import FIRE_COLUMNS, msms_cols, tem_clim_cols, tem_out_cols
from sklearn.decomposition import PCA

if not os.path.exists("shift.results"):
    open("pca.results", "w").close()


def load_vegc(path):
    vegc = pd.read_csv(path, names=tem_out_cols)
    vegc.rename(
        columns={
            "TOTAL": "TOTAL_VEGC",
            "MAX": "MAX_VEGC",
            "AVE": "AVE_VEGC",
            "MIN": "MIN_VEGC",
        },
        inplace=True,
    )
    vegc = vegc[
        [
            "LON",
            "LAT",
            "POTVEG",
            "YEAR",
            "TOTAL_VEGC",
            "MAX_VEGC",
            "AVE_VEGC",
            "MIN_VEGC",
        ]
    ]
    return vegc


vegc_45 = load_vegc("../4.5/runs/VEGC.TEMOUT")
vegc_85 = load_vegc("../8.5/runs/VEGC.TEMOUT")
vegc_hist = load_vegc("../historical/runs/VEGC.TEMOUT")


def load_npp(path):
    npp = pd.read_csv(path, names=tem_out_cols)
    npp.rename(
        columns={
            "TOTAL": "TOTAL_NPP",
            "MAX": "MAX_NPP",
            "AVE": "AVE_NPP",
            "MIN": "MIN_NPP",
        },
        inplace=True,
    )
    npp = npp[
        ["LON", "LAT", "POTVEG", "YEAR", "TOTAL_NPP", "MAX_NPP", "AVE_NPP", "MIN_NPP"]
    ]
    return npp


npp_45 = load_npp("../4.5/runs/NPP.TEMOUT")
npp_85 = load_npp("../8.5/runs/NPP.TEMOUT")
npp_hist = load_npp("../historical/runs/NPP.TEMOUT")


def load_precipitation(path):
    precipitation = pd.read_csv(path, names=tem_clim_cols)
    precipitation = precipitation[["LON", "LAT", "YEAR", "TOTAL", "AVE", "MAX", "MIN"]]
    precipitation.rename(
        columns={
            "TOTAL": "PRECIP_TOTAL",
            "AVE": "PRECIP_AVE",
            "MAX": "PRECIP_MAX",
            "MIN": "PRECIP_MIN",
        },
        inplace=True,
    )
    return precipitation


precipitation_45 = load_precipitation("../4.5/runs/climate_data/pr_rcp45_2006_2099.csv")
precipitation_85 = load_precipitation("../8.5/runs/climate_data/pr_rcp85_2006_2099.csv")
precipitation_hist = load_precipitation(
    "../historical/runs/climate_data/west_pr_gridmetcor_1750_2014.csv"
)


def load_temperature(path):
    temperature = pd.read_csv(path, names=tem_clim_cols)
    temperature = temperature[["LON", "LAT", "YEAR", "TOTAL", "AVE", "MAX", "MIN"]]
    temperature.rename(
        columns={
            "TOTAL": "TEMP_TOTAL",
            "AVE": "TEMP_AVE",
            "MAX": "TEMP_MAX",
            "MIN": "TEMP_MIN",
        },
        inplace=True,
    )
    return temperature


temperature_45 = load_temperature("../4.5/runs/climate_data/tair_rcp45_2006_2099.csv")
temperature_85 = load_temperature("../8.5/runs/climate_data/tair_rcp85_2006_2099.csv")
temperature_hist = load_temperature(
    "../historical/runs/climate_data/west_tair_gridmetcor_1750_2014.csv"
)


def load_moisture_stress(path):
    moisture_stress = pd.read_csv(path, names=msms_cols)
    moisture_stress = moisture_stress[["LON", "LAT", "POTVEG", "YEAR", "AET/PET"]]
    return moisture_stress


moisture_stress_45 = load_moisture_stress("../4.5/runs/MDI.csv")
moisture_stress_85 = load_moisture_stress("../8.5/runs/MDI.csv")
moisture_stress_hist = load_moisture_stress("../historical/runs/MDI.csv")


def load_elevation(path):
    elevation = pd.read_csv(
        path, names=["LON", "LAT", "VAR", "CAREA", "ELEV", "REGION"]
    )
    elevation = elevation[["LON", "LAT", "ELEV"]]
    return elevation


elevation_45 = load_elevation("../4.5/runs/climate_data/west_crutbaselvlf.csv")
elevation_85 = load_elevation("../8.5/runs/climate_data/west_crutbaselvlf.csv")
elevation_hist = load_elevation("../historical/runs/climate_data/west_crutbaselvlf.csv")


def load_shift(path):
    shift = pd.read_csv(path, names=["LON", "LAT", "POTVEG"])
    return shift


shift_45 = load_shift("../4.5/runs/biogeo/output_bakeoff/end.shift")
shift_85 = load_shift("../8.5/runs/biogeo/output_bakeoff/end.shift")
shift_hist = load_shift("../historical/runs/biogeo/input/current_land_cover.csv")


def load_fire(path):
    fire = pd.read_csv(path, names=FIRE_COLUMNS)
    fire = fire[
        [
            "LON",
            "LAT",
            "POTVEG",
            "YEAR",
            "MONTH",
            "FIRE_PROBABILITY",
            "COUNT",
            "SEVERITY",
            "THETA",
            "STATUS",
            "AREA_BURNED",
        ]
    ]
    fire = fire.groupby(["LON", "LAT", "POTVEG"]).agg(
        {
            "FIRE_PROBABILITY": "mean",
            "SEVERITY": "mean",
            "COUNT": "sum",
            "THETA": "mean",
            "STATUS": "mean",
            "AREA_BURNED": "mean",
        }
    )

    return fire


fire_45 = load_fire("../4.5/runs/FIRE.csv")
fire_85 = load_fire("../8.5/runs/FIRE.csv")
fire_hist = load_fire("../historical/runs/FIRE.csv")


def load_climate_limits(path):
    with open(path, "r") as f:
        data = json.load(f)
    climate_limits = pd.json_normalize(data)
    climate_limits = climate_limits.rename(columns=climate_limits)
    climate_limits = climate_limits[["POTVEG", "Tc", "Tw", "MIN_PREC", "MIN_AET/PET"]]
    return climate_limits


climate_limits_45 = load_climate_limits(
    "../4.5/biogeography_module/pft_climate_limits.json"
)
climate_limits_85 = load_climate_limits(
    "../8.5/biogeography_module/pft_climate_limits.json"
)
climate_limits_hist = load_climate_limits(
    "../historical/biogeography_module/pft_climate_limits.json"
)


def combine_data(
    vegc,
    npp,
    precipitation,
    fire,
    temperature,
    moisture_stress,
    elevation,
    shift,
    climate_limits,
):

    # Start with the shift data, which ensures LAT and LON from it are prioritized.
    merged_data = shift.copy()

    # Merge in the remaining datasets
    merged_data = merged_data.merge(vegc, on=["LON", "LAT", "POTVEG"], how="left")
    merged_data = merged_data.merge(
        npp, on=["LON", "LAT", "YEAR", "POTVEG"], how="left"
    )
    merged_data = merged_data.merge(
        precipitation,
        on=[
            "LON",
            "LAT",
            "YEAR",
        ],
        how="left",
    )
    merged_data = merged_data.merge(
        temperature,
        on=[
            "LON",
            "LAT",
            "YEAR",
        ],
        how="left",
    )
    merged_data = merged_data.merge(
        moisture_stress, on=["LON", "LAT", "YEAR", "POTVEG"], how="left"
    )
    merged_data = merged_data.merge(elevation, on=["LON", "LAT"], how="left")

    merged_data = merged_data.merge(climate_limits, on=["POTVEG"], how="left")

    # Grouping and aggregating
    merged_data = merged_data.groupby(["LON", "LAT"], as_index=False).agg(
        {
            "TOTAL_NPP": "mean",
            "TOTAL_VEGC": "mean",
            "PRECIP_TOTAL": "mean",
            "PRECIP_AVE": "mean",
            "TEMP_TOTAL": "mean",
            "TEMP_AVE": "mean",
            "AET/PET": "mean",
            "ELEV": "mean",
        }
    )

    # Fill NaN values after merge

    merged_data = merged_data.merge(shift, on=["LON", "LAT"], how="left")
    merged_data = merged_data.merge(fire, on=["LON", "LAT", "POTVEG"], how="left")
    merged_data[
        ["FIRE_PROBABILITY", "COUNT", "SEVERITY", "THETA", "STATUS", "AREA_BURNED"]
    ] = merged_data[
        ["FIRE_PROBABILITY", "COUNT", "SEVERITY", "THETA", "STATUS", "AREA_BURNED"]
    ].fillna(
        0
    )
    # Removing any rows with NA values
    merged_data.dropna(inplace=True)

    return merged_data


# Usage
future_data_45 = combine_data(
    vegc_45,
    npp_45,
    precipitation_45,
    fire_45,
    temperature_45,
    moisture_stress_45,
    elevation_45,
    shift_45,
    climate_limits_45,
)
future_data_85 = combine_data(
    vegc_85,
    npp_85,
    precipitation_85,
    fire_85,
    temperature_85,
    moisture_stress_85,
    elevation_85,
    shift_85,
    climate_limits_85,
)
historical_data = combine_data(
    vegc_hist,
    npp_hist,
    precipitation_hist,
    fire_hist,
    temperature_hist,
    moisture_stress_hist,
    elevation_hist,
    shift_hist,
    climate_limits_hist,
)


# historical_data = pd.read_csv("/users/jkodero/scratch/tem/archive/runs/4.5/historical/climate/historical_all_variables.csv",header=0)


def plot_elev_change(hist_data, fut_data_45, fut_data_85):

    historical = hist_data.copy()
    future_45 = fut_data_45.copy()
    future_85 = fut_data_85.copy()

    pft_labels = {
        4: "BF",
        8: "MTF",
        9: "TCF",
        10: "TDF",
        13: "SG",
        15: "AS",
        19: "XFW",
        33: "TBEF",
    }

    # Set Color Palette
    palette = {
        "AS": "#cb7e0c",
        "BF": "#0F52BA",
        "MTF": "#9ACD32",
        "SG": "#EEBC1D",
        "TBEF": "#004953",
        "TCF": "#1b9d77",
        "TDF": "#7570b3",
        "XFW": "#A4D4B4",
    }

    historical["POTVEG"] = historical["POTVEG"].map(pft_labels)
    future_45["POTVEG"] = future_45["POTVEG"].map(pft_labels)
    future_85["POTVEG"] = future_85["POTVEG"].map(pft_labels)

    # Group by POTVEG and calculate mean for each variable except LAT and LON

    historical = historical.groupby("POTVEG").mean().reset_index()
    future_45 = future_45.groupby("POTVEG").mean().reset_index()
    future_85 = future_85.groupby("POTVEG").mean().reset_index()

    historical = historical[["POTVEG", "ELEV", "TEMP_AVE"]]
    future_45 = future_45[["POTVEG", "ELEV", "TEMP_AVE"]]
    future_85 = future_85[["POTVEG", "ELEV", "TEMP_AVE"]]

    all_data = pd.merge(
        historical, future_45, on="POTVEG", how="outer", suffixes=("_HIST", "_45")
    )
    all_data = pd.merge(all_data, future_85, on="POTVEG", how="outer").rename(
        columns={"ELEV": "ELEV_85", "TEMP_AVE": "TEMP_AVE_85"}
    )

    def missing(df, POTVEG, HIST_ELEV, HIST_TEMP):
        df.loc[(df["POTVEG"] == POTVEG) & (df["ELEV_HIST"].isnull()), "ELEV_HIST"] = (
            HIST_ELEV
        )
        df.loc[
            (df["POTVEG"] == POTVEG) & (df["TEMP_AVE_HIST"].isnull()), "TEMP_AVE_HIST"
        ] = HIST_TEMP
        df.loc[(df["POTVEG"] == POTVEG) & (df["ELEV_45"].isnull()), "ELEV_45"] = df[
            "ELEV_HIST"
        ]
        df.loc[
            (df["POTVEG"] == POTVEG) & (df["TEMP_AVE_45"].isnull()), "TEMP_AVE_45"
        ] = (HIST_TEMP + 2)
        df.loc[(df["POTVEG"] == POTVEG) & (df["ELEV_85"].isnull()), "ELEV_85"] = df[
            "ELEV_HIST"
        ]
        df.loc[
            (df["POTVEG"] == POTVEG) & (df["TEMP_AVE_85"].isnull()), "TEMP_AVE_85"
        ] = (HIST_TEMP + 3)

        return df

    all_data = missing(all_data, "BF", 2500, 2)
    all_data = missing(all_data, "MTF", 300, 12)
    all_data = missing(all_data, "TCF", 1500, 7)
    all_data = missing(all_data, "TDF", 800, 10)
    all_data = missing(all_data, "SG", 1000, 7.5)
    all_data = missing(all_data, "AS", 1000, 17)
    all_data = missing(all_data, "XFW", 2000, 10)
    all_data = missing(all_data, "TBEF", 500, 14)

    sns.set_theme(style="ticks", font="sans-serif", rc={"lines.linewidth": 2.5})
    sns.set_context("paper")
    plt.rcParams["font.size"] = 14
    plt.rcParams["xtick.top"] = False
    plt.rcParams["xtick.direction"] = "in"
    plt.rcParams["xtick.minor.size"] = 5
    plt.rcParams["xtick.major.size"] = 10
    plt.rcParams["xtick.minor.visible"] = True
    plt.rcParams["ytick.right"] = False
    plt.rcParams["ytick.direction"] = "in"
    plt.rcParams["ytick.minor.size"] = 5
    plt.rcParams["ytick.major.size"] = 10
    plt.rcParams["ytick.minor.visible"] = True
    plt.rcParams["figure.autolayout"] = True

    plt.figure(figsize=(6, 6))
    # Plot with sns.lineplot
    a = sns.lineplot(
        data=all_data,
        x="TEMP_AVE_HIST",
        y="ELEV_HIST",
        hue="POTVEG",
        palette=palette,
        style="POTVEG",
        marker="^",
        markersize=10,
        linewidth=2.5,
        linestyle="--",
    )
    b = sns.lineplot(
        data=all_data,
        x="TEMP_AVE_45",
        y="ELEV_45",
        hue="POTVEG",
        palette=palette,
        style="POTVEG",
        marker="o",
        markersize=10,
        linewidth=2.5,
        linestyle="--",
    )
    c = sns.lineplot(
        data=all_data,
        x="TEMP_AVE_85",
        y="ELEV_85",
        hue="POTVEG",
        palette=palette,
        style="POTVEG",
        marker="s",
        markersize=10,
        linewidth=2.5,
        linestyle="--",
    )

    #     # Draw connecting lines
    # for potveg_category in all_data['POTVEG'].unique():
    #     # Filter data for each category
    #     data_hist = all_data[all_data['POTVEG'] == potveg_category]
    #     data_45 = all_data[all_data['POTVEG'] == potveg_category]

    #     # Assuming there's a one-to-one correspondence in the data for each category
    #     for i in range(len(data_hist)):
    #         plt.plot([data_hist.iloc[i]['TEMP_AVE_HIST'], data_45.iloc[i]['TEMP_AVE_45']],
    #                 [data_hist.iloc[i]['ELEV_HIST'], data_45.iloc[i]['ELEV_45']],
    #                 color=palette[potveg_category], marker='', linestyle='-')

    #     # Draw connecting lines
    # for potveg_category in all_data['POTVEG'].unique():
    #     # Filter data for each category
    #     data_hist = all_data[all_data['POTVEG'] == potveg_category]
    #     data_85 = all_data[all_data['POTVEG'] == potveg_category]

    #     # Assuming there's a one-to-one correspondence in the data for each category
    #     for i in range(len(data_hist)):
    #         plt.plot([data_hist.iloc[i]['TEMP_AVE_HIST'], data_85.iloc[i]['TEMP_AVE_85']],
    #                 [data_hist.iloc[i]['ELEV_HIST'], data_85.iloc[i]['ELEV_85']],
    #                 color=palette[potveg_category], marker='', linestyle='-')

    # Create legend entries for periods
    period_markers = {
        "^": "Historical\n(1984-2014)",
        "o": "RCP 4.5\n(2070-2100)",
        "s": "RCP 8.5\n(2070-2100)",
    }
    period_handles = [
        mlines.Line2D(
            [],
            [],
            color="black",
            marker=marker,
            linestyle="None",
            fillstyle="none",
            markersize=8,
            label=label,
        )
        for marker, label in period_markers.items()
    ]

    # Create legend entries for POTVEG categories
    pft_handles = [
        mlines.Line2D(
            [], [], color=color, marker="None", linestyle="-", linewidth=5, label=potveg
        )
        for potveg, color in palette.items()
    ]

    # Combine the handles
    all_handles = period_handles + pft_handles

    # Place the legend at the bottom center of the plot
    plt.legend(
        handles=all_handles,
        loc="upper center",
        bbox_to_anchor=(0.5, -0.10),
        ncol=4,
        frameon=False,
    )

    # Set axis labels and title
    plt.title("Mean Elevation vs. Mean Annual Temperature")
    plt.xlabel("Mean Annual Temperature (°C)")
    plt.ylabel("Mean Elevation (m)")

    # set ylim 0-3000
    plt.ylim(0, 3000)
    plt.xlim(0, 20)

    plt.tight_layout()
    plt.savefig(
        "elve_temp_space.png",
        format="png",
        dpi=1200,
        bbox_inches="tight",
        pad_inches=0.1,
    )

    # save as svg
    plt.savefig(
        "elve_temp_space.svg",
        format="svg",
        dpi=1200,
        bbox_inches="tight",
        pad_inches=0.1,
    )


plot_elev_change(historical_data, future_data_45, future_data_85)


def plot_corr(hist_data, fut_data_45, fut_data_85):

    historical = hist_data.copy()
    future_45 = fut_data_45.copy()
    future_85 = fut_data_85.copy()

    palette = {
        "AS": "#cb7e0c",
        "BF": "#0F52BA",
        "MTF": "#9ACD32",
        "SG": "#EEBC1D",
        "TBEF": "#004953",
        "TCF": "#1b9d77",
        "TDF": "#7570b3",
        "XFW": "#A4D4B4",
        "Total": "grey",
    }

    pft_labels_2 = {
        4: "BF",
        8: "MTF",
        9: "TCF",
        10: "TDF",
        13: "SG",
        15: "AS",
        19: "XFW",
        33: "TBEF",
    }

    historical["POTVEG"] = historical["POTVEG"].map(pft_labels_2)
    future_45["POTVEG"] = future_45["POTVEG"].map(pft_labels_2)
    future_85["POTVEG"] = future_85["POTVEG"].map(pft_labels_2)

    # Subset the columns of interest
    historical_subset = historical[
        ["ELEV", "TEMP_AVE", "PRECIP_TOTAL", "AET/PET", "TOTAL_NPP"]
    ]

    future_subset_45 = future_45[
        [
            "TEMP_AVE",
            "PRECIP_TOTAL",
            "AET/PET",
            "THETA",
            "AREA_BURNED",
            "TOTAL_NPP",
            "TOTAL_VEGC",
        ]
    ]
    future_subset_45.rename(
        columns={
            "TEMP_AVE": "ave_tair",
            "PRECIP_TOTAL": "total_precip",
            "AET/PET": "aet/pet",
            "TOTAL_NPP": "npp",
            "AREA_BURNED": "area_burned",
            "THETA": "theta",
            "TOTAL_VEGC": "vegc",
        },
        inplace=True,
    )
    future_subset_85 = future_85[
        [
            "TEMP_AVE",
            "PRECIP_TOTAL",
            "AET/PET",
            "THETA",
            "AREA_BURNED",
            "TOTAL_NPP",
            "TOTAL_VEGC",
        ]
    ]
    future_subset_85.rename(
        columns={
            "TEMP_AVE": "ave_tair",
            "PRECIP_TOTAL": "total_precip",
            "AET/PET": "aet/pet",
            "TOTAL_NPP": "total_npp",
            "AREA_BURNED": "area_burned",
            "THETA": "theta",
            "TOTAL_VEGC": "vegc",
        },
        inplace=True,
    )

    # Calculate the correlation coefficients
    # historical_corr = historical_subset.corr(method='pearson')
    future_corr_45 = future_subset_45.corr(method="pearson")
    future_corr_85 = future_subset_85.corr(method="pearson")

    # Print the correlation matrices
    with open("pca.results", "a") as file:
        file.write("Future correlation matrix:\n")
        file.write(str(future_corr_45))
        file.write("\n")
        file.write("Future correlation matrix:\n")
        file.write(str(future_corr_85))
        file.write("\n")
    print("Future correlation matrix:\n", future_corr_45)
    print("Future correlation matrix:\n", future_corr_85)

    # Or, calculate Spearman rank correlation coefficients
    # historical_corr = historical_subset.corr(method='spearman')
    # future_corr = future_subset.corr(method='spearman')

    # Plot the correlation matrices using seaborn heatmap
    # sns.heatmap(historical_corr, cmap='coolwarm', annot=True, vmin=-1, vmax=1)

    sns.set_context("paper")
    plt.subplots_adjust(wspace=0.5)
    plt.tight_layout(pad=0.5)
    plt.rcParams["font.size"] = 12
    plt.rcParams["xtick.top"] = False
    plt.rcParams["xtick.direction"] = "in"
    plt.rcParams["xtick.minor.size"] = 5
    plt.rcParams["xtick.major.size"] = 10
    plt.rcParams["xtick.minor.visible"] = True
    plt.rcParams["ytick.right"] = False
    plt.rcParams["ytick.direction"] = "in"
    plt.rcParams["ytick.minor.size"] = 5
    plt.rcParams["ytick.major.size"] = 10
    plt.rcParams["ytick.minor.visible"] = True
    plt.rcParams["figure.autolayout"] = True

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(7, 7))
    sns.heatmap(
        future_corr_45,
        cmap="viridis",
        annot=True,
        vmin=-1,
        vmax=1,
        annot_kws={"size": 10},
        ax=ax1,
    )
    ax1.set_title("a) RCP 4.5")

    sns.heatmap(
        future_corr_85,
        cmap="viridis",
        annot=True,
        vmin=-1,
        vmax=1,
        annot_kws={"size": 10},
        ax=ax2,
    )
    ax2.set_title("b) RCP 8.5")
    # remove cbar
    ax1.collections[0].colorbar.remove()
    ax2.collections[0].colorbar.remove()

    plt.tight_layout()

    plt.rcParams["font.size"] = 10
    plt.savefig(
        "corr.png",
        format="png",
        dpi=1200,
        bbox_inches="tight",
        pad_inches=0.1,
    )

    # save as svg
    plt.savefig(
        "corr.svg",
        format="svg",
        dpi=1200,
        bbox_inches="tight",
        pad_inches=0.1,
    )


plot_corr(historical_data, future_data_45, future_data_85)


# Concatenate the two data frames


def plot_pca(fut_data, name):

    future = fut_data.copy()

    palette = {
        "AS": "#cb7e0c",
        "BF": "#0F52BA",
        "MTF": "#9ACD32",
        "SG": "#EEBC1D",
        "TBEF": "#004953",
        "TCF": "#1b9d77",
        "TDF": "#7570b3",
        "XFW": "#A4D4B4",
        "Total": "grey",
    }

    pft_labels_2 = {
        "4": "BF",
        "8": "MTF",
        "9": "TCF",
        "10": "TDF",
        "13": "SG",
        "15": "AS",
        "19": "XFW",
        "33": "TBEF",
    }

    future["POTVEG"] = future["POTVEG"].astype(str)
    future["POTVEG"] = future["POTVEG"].map(pft_labels_2)

    future_pca = future[
        [
            "ELEV",
            "TEMP_AVE",
            "PRECIP_TOTAL",
            "AET/PET",
            "TOTAL_NPP",
            "POTVEG",
            "SEVERITY",
            "AREA_BURNED",
        ]
    ]

    # Extract the climatic variables from the combined data frame
    climatic_variables = future_pca[
        ["TEMP_AVE", "PRECIP_TOTAL", "AET/PET", "SEVERITY", "AREA_BURNED"]
    ]

    # Standardize the climatic variables
    climatic_variables_standardized = (
        climatic_variables - climatic_variables.mean()
    ) / climatic_variables.std()

    # Perform PCA on the standardized climatic variables
    pca = PCA()
    pca.fit(climatic_variables_standardized)

    # Get the explained variance ratio of each principal component
    explained_variance_ratio = pca.explained_variance_ratio_

    # Print the explained variance ratio of each principal component
    for i, ratio in enumerate(explained_variance_ratio):
        with open("pca.results", "a") as file:
            file.write(f"Principal component {i+1}: {ratio:.3f}\n")

        print(f"Principal component {i+1}: {ratio:.3f}")
    # Get the coefficients of the variables in the first principal component
    coefficients = pd.DataFrame(pca.components_, columns=climatic_variables.columns)

    # Get the absolute values of the coefficients
    absolute_coefficients = coefficients.abs()

    # Print the ranking of the variables by importance
    ranked_variables = absolute_coefficients.iloc[0].sort_values(ascending=False).index
    for i, variable in enumerate(ranked_variables):
        with open("pca.results", "a") as file:
            file.write(f"{i+1}. {variable}\n")
        print(f"{i+1}. {variable}")

    # Get the first two principal components
    pca_df = pd.DataFrame(
        pca.transform(climatic_variables_standardized)[:, :2], columns=["PC1", "PC2"]
    )

    # Add the vegetation type to the data frame
    pca_df["POTVEG"] = future["POTVEG"].values

    sns.set_theme(style="ticks", font="sans-serif", rc={"lines.linewidth": 2.5})
    plt.minorticks_on()
    plt.tick_params(
        direction="in",
        which="minor",
        length=5,
        bottom=True,
        top=False,
        left=True,
        right=False,
    )
    plt.tick_params(
        direction="in",
        which="major",
        length=10,
        bottom=True,
        top=False,
        left=True,
        right=False,
    )

    # Create the scatter plot
    plt.figure(figsize=(5, 6), dpi=1200)
    ax = sns.scatterplot(
        data=pca_df,
        x="PC1",
        y="PC2",
        hue="POTVEG",
        palette=palette,
        edgecolor="gray",
    )
    # Plot Horizontal Line at y=0 and Vertical Line at x=0
    ax.axhline(y=0, color="gray", linestyle="--", linewidth=0.3)
    ax.axvline(x=0, color="gray", linestyle="--", linewidth=0.3)
    ax.legend(loc="lower center", bbox_to_anchor=(0.5, -0.3), ncol=4, frameon=False)

    plt.title(f"RCP{name}")
    plt.savefig(
        f"pca{name}.png",
        format="png",
        dpi=1200,
        bbox_inches="tight",
        pad_inches=0.1,
    )

    # save as svg
    plt.savefig(
        f"pca{name}.svg",
        format="svg",
        dpi=1200,
        bbox_inches="tight",
        pad_inches=0.1,
    )

    plt.show()


plot_pca(future_data_45, "_4.5")
plot_pca(future_data_85, "_8.5")
