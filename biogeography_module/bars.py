# Importing libraries and setting up the environment
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.colors import ListedColormap, LinearSegmentedColormap
from statsmodels.genmod.families import Binomial
from matplotlib.ticker import MultipleLocator, FormatStrFormatter
from mpl_toolkits.basemap import Basemap
import matplotlib.ticker as plticker
import matplotlib.pyplot as plt
import scipy.stats as stats
import seaborn as sns
import pandas as pd
import numpy as np
import textwrap
import warnings

warnings.filterwarnings("ignore")  # setting ignore as a parameter


image_format = "pdf"


# Function to read in data
def process_historical_data(filepath):
    variable_data = pd.read_csv(
        filepath,
        names=[
            "POTVEG",
            "YEAR",
            "NGRID",
            "TOTCELLAREA",
            "TOTFORECOZONE",
            "MXPRED",
            "MNPRED",
            "MNBYAR",
            "SIMPMN",
            "STNDEV",
            "MNTOTYR",
        ],
    )

    variable_data["TOTFORECOZONE"] = variable_data["TOTFORECOZONE"]/1000
    variable_data["STNDEV"] = variable_data["STNDEV"]/1000
    return variable_data

historical_vegc_path = "./historical/ORIGINAL_LULC_VEGC.SUMMARY"
historical_npp_path = "./historical/ORIGINAL_LULC_NPP.SUMMARY"
historical_nep_path = "./historical/ORIGINAL_LULC_NEP.SUMMARY"
historical_soilorgc_path = "./historical/ORIGINAL_LULC_SOILORGC.SUMMARY"

historical_vegc_df = process_historical_data(historical_vegc_path)
historical_npp_df = process_historical_data(historical_npp_path)
historical_nep_df = process_historical_data(historical_nep_path)
historical_soilorgc_df = process_historical_data(historical_soilorgc_path)


def process_future_data(filepath):
    variable_data = pd.read_csv(
        filepath,
        names=[
            "VARIABLE",
            "POTVEG",
            "DESCRIPTION",
            "YEAR",
            "NGRID",
            "TOTFORECOZONE",
            "MNBYAR",
            "MXPRED",
            "MNPRED",
            "MNTOTYR",
            "STNDEV",
            "SIMPMN",
        ]
    )
    
   # Drop N/A
    variable_data = variable_data.dropna()
  
    variable_data["POTVEG"] = variable_data["POTVEG"].astype(int)
    variable_data["TOTFORECOZONE"] = variable_data["TOTFORECOZONE"]/1000
    variable_data["STNDEV"] = variable_data["STNDEV"]/1000
    

  

    return variable_data


future_vegc_path = "VEGC.SUMMARY"
future_npp_path = "NPP.SUMMARY"
future_nep_path = "NEP.SUMMARY"
future_soilorgc_path = "SOILORGC.SUMMARY"

future_vegc_df = process_future_data(future_vegc_path)
future_npp_df = process_future_data(future_npp_path)
future_nep_df = process_future_data(future_nep_path)
future_soilorgc_df = process_future_data(future_soilorgc_path)




def calculate_difference(future_df, historical_df):

    rows_to_concat = [
        {"POTVEG": 13, "YEAR": 2014, "NGRID": 0, "TOTCELLAREA": 0, "TOTFORECOZONE": 0.00000001,
            "MXPRED": 0, "MNPRED": 0, "MNBYAR": 0, "SIMPMN": 0, "STNDEV": 0.00000001, "MNTOTYR": 0},
        {"POTVEG": 15, "YEAR": 2014, "NGRID": 0, "TOTCELLAREA": 0, "TOTFORECOZONE": 0.00000001,
            "MXPRED": 0, "MNPRED": 0, "MNBYAR": 0, "SIMPMN": 0, "STNDEV": 0.00000001, "MNTOTYR": 0},
    ]
    historical_df = pd.concat([historical_df, pd.DataFrame(rows_to_concat)])

    # Filter DataFrame based on the conditions for each period
    df_period2 = future_df[(future_df['YEAR'] >= 2069)]
    df_period1 = historical_df[historical_df['YEAR'] >= 1984]

    # Group by POTVEG and calculate sum, standard deviation, and standard error
    period1 = df_period1.groupby('POTVEG').agg(
        TOTFORECOZONE_sum1=('TOTFORECOZONE', np.sum),
        STNDEV1=('STNDEV', np.sum),
        count1=('POTVEG', 'count')
    )

    period2 = df_period2.groupby('POTVEG').agg(
        TOTFORECOZONE_sum2=('TOTFORECOZONE', np.sum),
        STNDEV2=('STNDEV', np.sum),
        count2=('POTVEG', 'count')
    )

    # Merge the aggregated DataFrames
    merged_df = pd.merge(period1, period2, on='POTVEG')

    # Calculate the difference and propagate the errors
    merged_df['DIFFERENCE'] = merged_df['TOTFORECOZONE_sum2'] - \
        merged_df['TOTFORECOZONE_sum1']
    merged_df['STNDEV_DIFF'] = (merged_df['STNDEV2'] + merged_df['STNDEV1'])/2

    # STDERR = sqrt[(SD1^2/n1) + (SD2^2/n2)]
    merged_df['STDERR'] = np.sqrt(
        (merged_df['STNDEV1']**2/merged_df['count1']) + (merged_df['STNDEV2']**2/merged_df['count2']))

    pft_labels = {
        4: "BF",
        8: "MTF",
        9: "TCF",
        10: "TDF",
        13: "SG",
        15: "AS",
        19: "XFW",
        33: "TBEF",
        99: "Total",

    }

    # Reset index to make POTVEG a column
    merged_df.reset_index(inplace=True)

    merged_df["PFT"] = merged_df["POTVEG"].map(pft_labels)

    merged_df = merged_df[["PFT", "DIFFERENCE", "STDERR"]]

    return merged_df


vegc_diff_df = calculate_difference(future_vegc_df, historical_vegc_df)
npp_diff_df = calculate_difference(future_npp_df, historical_npp_df)
nep_diff_df = calculate_difference(future_nep_df, historical_nep_df)
soilorgc_diff_df = calculate_difference(
    future_soilorgc_df, historical_soilorgc_df)



# Define the colors for each vegetation type
vegetation_type_colors = {
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

# Create a list with all the data frames
data = [vegc_diff_df, soilorgc_diff_df, npp_diff_df, nep_diff_df]

# Set the titles and units for each plot
titles = [" a) Vegetation Carbon",  " b) Soil Organic Carbon",
          " c) Net Primary Productivity",  " d) Net Ecosystem Productivity"]
ylabel = ["PgC", "PgC", "PgC/30yrs", "PgC/30yrs"]
# Set the theme first
sns.set_theme(style="ticks", rc={"lines.linewidth": 1})

# Set rcParams
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

fig, axs = plt.subplots(nrows=2, ncols=2, figsize=(10, 8))

for i, ax in enumerate(axs.flatten()):
    if i < len(data):
        # Plot the data using a barplot
        sns.barplot(ax=ax,
                    x="PFT",
                    y="DIFFERENCE",
                    palette=vegetation_type_colors,
                    edgecolor="black",
                    width=0.5,
                    linewidth=1,
                    data=data[i])
        ax.set_xlabel("")
        # Add error bars
        for j, row in data[i].iterrows():
            ax.errorbar(x=j,
                        y=row['DIFFERENCE'],
                        yerr=row['STDERR']*0.14,
                        color='black',
                        capsize=3)
        # Set the title and y-label
        ax.set_title(titles[i], fontweight='bold')
        ax.set_ylabel(ylabel[i])

    else:
        # Hide the unused subplots
        ax.axis('off')


plt.tight_layout()
# Save the plot
plt.savefig("./biogeo/bars.png", dpi=1200, format="png", bbox_inches="tight")




