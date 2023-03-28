import numpy as np
import pandas as pd

x = pd.read_csv('T:/m8/kodero/runs/usa_ccsm4_MAIN/west/climate/west_windlf.csv', names=[
    'col', 'row', 'tmpvarname', 'area', 'year', 'total_pr', 'max_pr', 'ave_pr', 'min_pr', 'mon+0', 'mon+1', 'mon+2', 'mon+3', 'mon+4', 'mon+5', ' mon+6', 'mon+7', 'mon+8', 'mon+9', 'mon+10', 'mon+11', 'tmpregion'])

df1 = pd.read_csv('was.csv', names=[
                  'col', 'row', 'tmpvarname', 'area', 'year', 'total_pr', 'max_pr', 'ave_pr', 'min_pr', 'mon+0', 'mon+1', 'mon+2', 'mon+3', 'mon+4', 'mon+5', ' mon+6', 'mon+7', 'mon+8', 'mon+9', 'mon+10', 'mon+11', 'tmpregion'])

w = df1.groupby(['col', 'row', 'tmpvarname', 'area'], as_index=False).agg({'total_pr': 'mean', 'max_pr': 'mean', 'ave_pr': 'mean', 'min_pr': 'mean', 'mon+0': 'mean', 'mon+1': 'mean',
                                                                           'mon+2': 'mean', 'mon+3': 'mean', 'mon+4': 'mean', 'mon+5': 'mean', ' mon+6': 'mean', 'mon+7': 'mean', 'mon+8': 'mean', 'mon+9': 'mean', 'mon+10': 'mean', 'mon+11': 'mean'}).round(2)

w['area'] = x['area']
w['year'] = x['year']


wind = w[['col', 'row', 'tmpvarname', 'area', 'year', 'total_pr', 'max_pr', 'ave_pr', 'min_pr', 'mon+0', 'mon+1',
          'mon+2', 'mon+3', 'mon+4', 'mon+5', ' mon+6', 'mon+7', 'mon+8', 'mon+9', 'mon+10', 'mon+11']]

wind.to_csv('T:/m8/kodero/runs/australia/climate/was_CSIRO-Mk3-6-0_rcp85_2006_2099.csv',
            header=False, index=False)
