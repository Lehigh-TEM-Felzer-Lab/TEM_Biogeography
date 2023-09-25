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
from matplotlib.ticker import MultipleLocator, FormatStrFormatter
from scipy.stats import f_oneway
from statsmodels.genmod.families import Binomial
from matplotlib.colors import ListedColormap
from scipy.stats import linregress
from scipy.stats import mannwhitneyu
from scipy.stats import kendalltau
import matplotlib.ticker as ticker
import cmath


palette = {
    "Arid Shrublands": "#cb7e0c",
    "Boreal Forests": "#0F52BA",
    "Mixed Temperate Forests": "#9acd32",
    "Short Grasslands": "#eebc1d",
    "Temperate Broadleaved Evergreen Forests": "#004953",
    "Temperate Coniferous Forests": "#1b9d77",
    "Temperate Deciduous Forests": "#7570b3",
    "Xeromorphic Forests and Woodlands": "#A4D4B4",
    "All Plant Functional Types": "gray",
}



# Function to read in data
def process_data(filepath):
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
    # filter for year > 2016
    variable_data = variable_data.query("YEAR > 2017")
    variable_data["POTVEG"] = variable_data["POTVEG"].astype(int)
  

    return variable_data

# Define paths to data
vegc_path = "VEGC.SUMMARY"
soilc_path = "SOILORGC.SUMMARY"
npp_path = "NPP.SUMMARY"
nep_path = "NEP.SUMMARY"
nce_path = "NCE.SUMMARY"

# Read in data

vegc_df = process_data(vegc_path)
soilc_df = process_data(soilc_path)
npp_df = process_data(npp_path)
nep_df = process_data(nep_path)
nce_df = process_data(nce_path)



# Now, you can calculate the cumulative sum
nce_df["CUMSUM"] = nce_df.groupby(["POTVEG", "YEAR"])["TOTFORECOZONE"].cumsum()
nce_df.loc [nce_df["POTVEG"] == 9, "TOTFORECOZONE"] += 100

# Plot Data
dataframes = [vegc_df, soilc_df, npp_df, nep_df, nce_df]
#Create a list with all the variable names
vars = [ "TOTALVEGC", "TOTALSOILORGC","TOTALNPP", "TOTALNEP", "TOTALNCE"]
titles = [ " a) Vegetation Carbon", " b) Soil Organic Carbon"," c) Net Primary Productivity", " d) Net Ecosystem Productivity", " e) Cumulative Net Carbon Exchange",]

#Create a list with all the y-axis labels
ylabels = [  "PgC","PgC","PgC/yr","PgC/yr", "PgC", ]


# Set the theme first
sns.set_theme(style="ticks", rc={"lines.linewidth": 2})

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

# Create the figure and subplots
fig, axs = plt.subplots(3, 2, figsize=(10, 10))

# Flatten the axs array
axs = axs.flatten()

# Loop through each variable and plot data
for i, var in enumerate(vars):
    # Extract the dataframe we need for var
    dataframe = dataframes[i]
    # Set the title for the subplot
    title = titles[i]
    # Set the y-label for the subplot
    ylabel = ylabels[i]
    
    dataframe["TOTFORECOZONE"] = ((dataframe["TOTFORECOZONE"].astype(float)) / 1000)
    # Filter dataframe for POTVEG!=99
    
    
    if var == "TOTALNCE":
        plot_df = dataframe.query("POTVEG != 99 and POTVEG != 9")
        plot_df_9 = dataframe.query("POTVEG == 9")
        plot_df_99 = dataframe.query("POTVEG == 99")

        # Plot data for var = "TOTALNCE" on the primary axis
        sns.lineplot(x="YEAR", y="CUMSUM", hue="DESCRIPTION", palette=palette, data=plot_df, ax=axs[i])

        # Set the y-axis label for the primary axis
        axs[i].set_ylabel("PgC  (All other PFTs)")
        axs[i].set_xlabel("Year")

        # Create a twin axis for POTVEG = 9
        ax2 = axs[i].twinx()

        # Plot data for POTVEG = 9 on the secondary axis
        sns.lineplot(x="YEAR", y="CUMSUM", hue="DESCRIPTION", palette=palette, data=plot_df_9, ax=ax2,legend=False)
        sns.lineplot(x="YEAR", y="CUMSUM", hue="DESCRIPTION", palette=palette, data=plot_df_99, ax=ax2,legend=False)
        
        ax2.tick_params(axis='both', which='both', top=False, direction='in', 
                  labelright=True, labelleft=False, 
                 labeltop=False, labelbottom= False, bottom=True, 
                 left=False, right=True,)

        # Set the y-axis label for the secondary axis
        ax2.set_ylabel("PgC " + " (Temperate Coniferous, Total)", fontsize=12)
          
    else:
        dataframe = dataframe.query("POTVEG != 99")
        # Plot data for var != "TOTALNCE" on the primary axis
        sns.lineplot(x="YEAR", y="TOTFORECOZONE", hue="DESCRIPTION", palette=palette, data=dataframe, ax=axs[i])

        # Set the y-axis label for the primary axis
        axs[i].set_ylabel(ylabel)
        axs[i].set_xlabel(r"Year")
    
    # Add title to subplot
    axs[i].set_title(title,fontweight='bold')
    
    # Remove legend 
    axs[i].get_legend().remove()

# Set plot parameters
plt.tight_layout(pad=1.5, w_pad=1.5, h_pad=1.5)

axs[-1].remove()
# Legend
axs[-1].axis('off') # Turn off last subplot
handles, labels = axs[0].get_legend_handles_labels()
fig.legend(handles, labels, loc='lower right', frameon=False, bbox_to_anchor=(1.01 , 0.05),fontsize=14)

# Save the plot
plt.savefig("./biogeo/trends.png", dpi=1200, format="png", bbox_inches="tight")
