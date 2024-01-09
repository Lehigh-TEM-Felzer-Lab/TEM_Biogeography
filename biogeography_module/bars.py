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




original_lc = True

if original_lc:
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


    historical_vegc_path_or = "/users/jkodero/scratch/tem/historical/runs/VEGC.SUMMARY_ORG"
    historical_npp_path_or = "/users/jkodero/scratch/tem/historical/runs/NPP.SUMMARY_ORG"
    historical_nep_path_or = "/users/jkodero/scratch/tem/historical/runs/NEP.SUMMARY_ORG"
    historical_soilorgc_path_or = "/users/jkodero/scratch/tem/historical/runs/SOILORGC.SUMMARY_ORG"

    historical_vegc_df = process_historical_data(historical_vegc_path_or)
    historical_npp_df = process_historical_data(historical_npp_path_or)
    historical_nep_df = process_historical_data(historical_nep_path_or)
    historical_soilorgc_df = process_historical_data(historical_soilorgc_path_or)


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
    variable_data["POTVEG"] = variable_data["POTVEG"].astype(int)
    variable_data["TOTFORECOZONE"] = variable_data["TOTFORECOZONE"]/1000
    variable_data["STNDEV"] = variable_data["STNDEV"]/1000
    

  

    return variable_data


if original_lc == False:
    # Historical Path
    historical_vegc_path = "/users/jkodero/scratch/tem/historical/runs/VEGC.SUMMARY"
    historical_npp_path = "/users/jkodero/scratch/tem/historical/runs/NPP.SUMMARY"
    historical_nep_path = "/users/jkodero/scratch/tem/historical/runs/NEP.SUMMARY"
    historical_soilorgc_path = "/users/jkodero/scratch/tem/historical/runs/SOILORGC.SUMMARY"



    # Historical DataFrames
    historical_vegc_df = process_data(historical_vegc_path)
    historical_npp_df = process_data(historical_npp_path)
    historical_nep_df = process_data(historical_nep_path)
    historical_soilorgc_df = process_data(historical_soilorgc_path)

# RCP 4.5 Path
rcp45_vegc_path = "../4.5/runs/VEGC.SUMMARY"
rcp_45_npp_path = "../4.5/runs/NPP.SUMMARY"
rcp_45_nep_path = "../4.5/runs/NEP.SUMMARY"
rcp_45_soilorgc_path = "../4.5/runs/SOILORGC.SUMMARY"


# RCp 4.5 DataFrames
rcp_45_vegc_df = process_data(rcp45_vegc_path)
rcp_45_npp_df = process_data(rcp_45_npp_path)
rcp_45_nep_df = process_data(rcp_45_nep_path)
rcp_45_soilorgc_df = process_data(rcp_45_soilorgc_path)


# RCP 8.5 Path
rcp85_vegc_path = "../8.5/runs/VEGC.SUMMARY"
rcp_85_npp_path = "../8.5/runs/NPP.SUMMARY"
rcp_85_nep_path = "../8.5/runs/NEP.SUMMARY"
rcp_85_soilorgc_path = "../8.5/runs/SOILORGC.SUMMARY"

#rcp85_vegc_path = "../historical/runs/VEGC.SUMMARY_TS"
#rcp_85_npp_path = "../historical/runs/NPP.SUMMARY_TS"
#rcp_85_nep_path = "../historical/runs/NEP.SUMMARY_TS"
#rcp_85_soilorgc_path = "../historical/runs/SOILORGC.SUMMARY_TS"

# RCP 8.5 DataFrames
rcp_85_vegc_df = process_data(rcp85_vegc_path)
rcp_85_npp_df = process_data(rcp_85_npp_path)
rcp_85_nep_df = process_data(rcp_85_nep_path)
rcp_85_soilorgc_df = process_data(rcp_85_soilorgc_path)






# Define the unique POTVEG values that should be present

description = {
4: "Boreal Forests",
8: "Mixed Temperate Forests",
9: "Temperate Coniferous Forests",
10: "Temperate Deciduous Forests",
13: "Short Grasslands",
15: "Arid Shrublands",
19: "Xeromorphic Forests and Woodlands",
33: "Temperate Broadleaved Evergreen Forests",
}
    
unique_potveg = [4, 8, 9, 10, 13, 15, 19, 33]

# Define a function to add missing POTVEG entries with zero values
def add_missing_potveg_entries(df, years, potveg_values, year_reference):
    # Create an empty list to store rows to be appended
    rows_to_append = []

    for year in years:
        existing_potveg = df.loc[df['YEAR'] == year, 'POTVEG'].unique()
        missing_potveg = set(potveg_values) - set(existing_potveg)
        for potveg in missing_potveg:
            potveg_description = description.get(potveg, "Unknown Description") # Default to "Unknown Description" if not found
            # Create a dictionary for the missing row and append it to the list
            if original_lc == False:
                rows_to_append.append({
                    "VARIABLE": df['VARIABLE'].unique()[0],
                    "POTVEG": potveg,
                    "DESCRIPTION": potveg_description,
                    "YEAR": year_reference.astype(int),
                    "NGRID": 0,
                    "TOTFORECOZONE": 0.00000001,
                    "MNBYAR": 0.00000001,
                    "MXPRED": 0.00000001,
                    "MNPRED": 0.00000001,
                    "MNTOTYR": 0.00000001,
                    "STNDEV": 0.00000001,
                    "SIMPMN": 0.00000001
                })

            else:

                rows_to_append.append({
               "POTVEG": potveg,
                "YEAR": year_reference.astype(int),
                "NGRID": 0,
                "TOTCELLAREA": 0.00000001,
                "TOTFORECOZONE": 0.00000001,
                "MXPRED": 0.00000001,
                "MNPRED": 0.00000001,
                "MNBYAR": 0.00000001,
                "SIMPMN": 0.00000001,
                "STNDEV": 0.00000001,
                "MNTOTYR": 0.00000001,
            })


            

    # Convert the list of dictionaries to a DataFrame
    new_rows_df = pd.DataFrame(rows_to_append)
    
    # Concatenate the new rows to the original DataFrame
    df = pd.concat([df, new_rows_df], ignore_index=True)
    
    
    return df




def calculate_difference(rcp_df, historical_df):


    rcp_ = rcp_df[(rcp_df['YEAR'] >= 2069)]
    historical = historical_df[historical_df['YEAR'] >= 1984]
    rcp_years = rcp_['YEAR'].unique()
    historical_years = historical['YEAR'].unique()

    for year in rcp_years:
        # Apply the function to rcp_df and historical_df
        rcp_ = add_missing_potveg_entries(rcp_, rcp_years, unique_potveg, year)
    
    rcp_ = rcp_.sort_values(by=['POTVEG', 'YEAR'])



    for year in historical_years:
        historical= add_missing_potveg_entries(historical, historical_years, unique_potveg, year)
    
    historical=historical.sort_values(by=['POTVEG', 'YEAR'])
 

    


 # Group by POTVEG and calculate sum, standard deviation, and standard error
    historical_agg = historical.groupby('POTVEG').agg(
    TOTFORECOZONE_HIST=('TOTFORECOZONE', np.sum),
    STNDEV_HIST=('STNDEV', np.sum),
    count_HIST=('POTVEG', 'count')
    ).reset_index()

    # Group by POTVEG and calculate sum, standard deviation, and count for RCP data
    rcp_agg = rcp_.groupby('POTVEG').agg(
    TOTFORECOZONE_RCP=('TOTFORECOZONE', np.sum),
    STNDEV_RCP=('STNDEV', np.sum),
    count_RCP=('POTVEG', 'count')
    ).reset_index()

    # Merge the aggregated DataFrames
    merged_df = pd.merge(historical_agg, rcp_agg, on='POTVEG', suffixes=("_HIST", "_RCP"))

    # Calculate the difference in totals and the standard deviation of the difference
    merged_df['DIFFERENCE'] = merged_df['TOTFORECOZONE_RCP'] - merged_df['TOTFORECOZONE_HIST']
    merged_df['STNDEV_DIFF'] = np.sqrt(merged_df['STNDEV_HIST']**2 + merged_df['STNDEV_RCP']**2)

    # Calculate the standard error of the mean (SEM) for the difference
    merged_df['STDERR'] = np.sqrt((merged_df['STNDEV_HIST']**2 / merged_df['count_HIST']) + (merged_df['STNDEV_RCP']**2 / merged_df['count_RCP']))

    # Map POTVEG to PFT labels
    pft_labels = {
    4: "BF",
    8: "MTF",
    9: "TCF",
    10: "TDF",
    13: "SG",
    15: "AS",
    19: "XFW",
    33: "TBEF",
    99: "TOTAL"
    }

    merged_df["PFT"] = merged_df["POTVEG"].map(pft_labels)

    # Select relevant columns for output
    final_df = merged_df[["PFT", "DIFFERENCE", "STNDEV_DIFF", "STDERR"]]

    return final_df



# Apply the function for RCP 4.5

rcp_45_vegc = calculate_difference(rcp_45_vegc_df, historical_vegc_df)
rcp_45_npp = calculate_difference(rcp_45_npp_df, historical_npp_df)
rcp_45_nep = calculate_difference(rcp_45_nep_df, historical_nep_df)
rcp_45_soilorgc = calculate_difference(rcp_45_soilorgc_df, historical_soilorgc_df)


# Apply the function for RCP 8.5

rcp_85_vegc = calculate_difference(rcp_85_vegc_df, historical_vegc_df)
rcp_85_npp = calculate_difference(rcp_85_npp_df, historical_npp_df)
rcp_85_nep = calculate_difference(rcp_85_nep_df, historical_nep_df)
rcp_85_soilorgc = calculate_difference(rcp_85_soilorgc_df, historical_soilorgc_df)









# Create lists with data frames for both RCP_4.5 and RCP_8.5
rcp_45_data = [rcp_45_vegc, rcp_45_soilorgc, rcp_45_npp, rcp_45_nep]
rcp_85_data = [rcp_85_vegc, rcp_85_soilorgc, rcp_85_npp, rcp_85_nep]
data_labels = ['vegc', 'soilorgc', 'npp', 'nep']

concatinated = {}

# Modify the PFT column data and concatenate
for label, df_45, df_85 in zip(data_labels, rcp_45_data, rcp_85_data):
    df_85['PFT'] = df_85['PFT'] + '_RCP_8_5'
    df_45['PFT'] = df_45['PFT'] + '_RCP_4_5'
    concatenated_df = pd.concat([df_45, df_85], ignore_index=True)
    concatenated_df = concatenated_df.sort_values(by=['PFT'])
    concatinated[f'rcp_{label}'] = concatenated_df


data = [concatinated['rcp_vegc'], concatinated['rcp_soilorgc'], concatinated['rcp_npp'], concatinated['rcp_nep']]



# Define the colors for each vegetation type

vegetation_type_colors={
"AS_RCP_4_5": "#cb7e0c",
"AS_RCP_8_5": "#cb7e0c",
"BF_RCP_4_5": "#0F52BA",
"BF_RCP_8_5": "#0F52BA",
"MTF_RCP_4_5": "#9ACD32",
"MTF_RCP_8_5": "#9ACD32",
"SG_RCP_4_5": "#EEBC1D",
"SG_RCP_8_5": "#EEBC1D",
"TBEF_RCP_4_5": "#004953",
"TBEF_RCP_8_5": "#004953",
"TCF_RCP_4_5": "#1b9d77",
"TCF_RCP_8_5": "#1b9d77",
"TDF_RCP_4_5": "#7570b3",
"TDF_RCP_8_5": "#7570b3",
"TOTAL_RCP_4_5": "grey",
"TOTAL_RCP_8_5": "grey",
"XFW_RCP_4_5": "#A4D4B4",
"XFW_RCP_8_5": "#A4D4B4",
    }

    




order_list =[
    "BF_RCP_4_5", 
    "BF_RCP_8_5",
    "MTF_RCP_4_5",
    "MTF_RCP_8_5",
    "TCF_RCP_4_5",
    "TCF_RCP_8_5",
    "TDF_RCP_4_5",
    "TDF_RCP_8_5",
    "SG_RCP_4_5",
    "SG_RCP_8_5",
    "AS_RCP_4_5", 
    "AS_RCP_8_5",
    "XFW_RCP_4_5",
    "XFW_RCP_8_5",
    "TBEF_RCP_4_5",
    "TBEF_RCP_8_5",
    "TOTAL_RCP_4_5", 
    "TOTAL_RCP_8_5",



]

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
        data[i] = data[i].set_index('PFT').loc[order_list].reset_index()
        # Plot the data using a barplot
        barplot = sns.barplot(ax=ax,
                            x="PFT",
                            y="DIFFERENCE",
                            palette=vegetation_type_colors,
                            edgecolor="black",
                            width=0.5,
                            linewidth=1,
                            data=data[i])
        

        # Adding hatches to bars where PFT ends with RCP_8_5
        for bar, label in zip(barplot.patches, barplot.get_xticklabels()):
            if label.get_text().endswith('RCP_8_5'):
                bar.set_hatch('//')
                
        ax.set_xticks([])
        # Add error bars
        for pft_value, row in data[i].iterrows():
            position = order_list.index(row['PFT'])  # Fetching position based on order_list
            ax.errorbar(x=position,
                        y=row['DIFFERENCE'],
                        yerr=row['STDERR']*0.14,
                        color='black',
                        capsize=3)
        # Set the title and y-label
        ax.set_title(titles[i], fontweight='bold')
        ax.set_ylabel(ylabel[i])

        import matplotlib.patches as mpatches


        legend_colors = {
            "AS": "#cb7e0c",
            "BF": "#0F52BA",
            "MTF": "#9ACD32",
            "SG": "#EEBC1D",
            "TBEF": "#004953",
            "TCF": "#1b9d77",
            "TDF": "#7570b3",
            "XFW": "#A4D4B4",
            "TOTAL": "grey",
        }

                # Create custom legend with square patches
        legend_patches = []

        # Adding color squares based on vegetation_type_colors
        for vegetation, color in legend_colors.items():
            legend_patches.append(mpatches.Patch(color=color, label=vegetation))

        # Add an additional square for the RCP 8.5 hatch
        legend_patches.append(mpatches.Patch(facecolor='white', edgecolor='black', hatch='//', label='RCP 8.5'))
        legend_patches.append(mpatches.Patch(facecolor='white', edgecolor='black', hatch='-', label='RCP 4.5'))

        # Display the legend on the ax
        

    else:
        # Hide the unused subplots
        ax.axis('off')

# ... (rest of the plotting code)

fig.legend(handles=legend_patches, loc='lower center', bbox_to_anchor=(0.5, -0.15), frameon=False, ncol=5)


plt.tight_layout()
# Save the plot
plt.savefig("bars_combined.png", dpi=1200, format="png", bbox_inches="tight")
# save as svg
plt.savefig("bars_combined.svg", dpi=1200, format="svg", bbox_inches="tight")


