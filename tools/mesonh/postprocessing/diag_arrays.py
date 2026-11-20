#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Nov  2 12:22:20 2023

@author: rbaggio
"""

import os
import numpy as np
import netCDF4 as nc
import xarray as xr

#%matplotlib notebook  


    

#Takes a .nc file and outputs a 2D array
    

def TKE(ncfile, level=2):
    datanc=xr.load_dataset(ncfile)
    tket=datanc["TKET"]
    levels=tket['level'].values
    print(f"tke at {levels[level]} m")
    tket_at_altitude=tket.sel(level=levels[level])
    tket_at_altitude_arr=tket_at_altitude.values
    # print(tket_at_altitude_arr)
    return tket_at_altitude_arr[0]

def surface_wind(ncfile, level=0):
    datanc=xr.load_dataset(ncfile)
    levels=datanc["UT"]['level'].values
    levels_w=datanc["WT"]['level_w'].values
    alt=levels[level]
    altw=levels_w[level]
    print(f"calculating Wind Speed module at {levels[level]} m")
    ut=datanc["UT"].sel(level=alt).values[0]
    vt=datanc["VT"].sel(level=alt).values[0]
    wt=datanc["WT"].sel(level_w=altw).values[0]
    modulewind=np.sqrt(ut*ut+vt*vt+wt*wt)
    return modulewind

def plume_height(ncfile, threshold):
    datanc=xr.load_dataset(ncfile)
    # print(datanc["SVT"])
    # for var_name in datanc.data_vars.keys():
    #     print(var_name)
    maxBR=np.amax(datanc["SVT002"])
    Bratio=datanc["SVT002"]/maxBR

    filtered_data = Bratio.where(Bratio > threshold)
    filtered_data_with_altitudes = Bratio['level']* filtered_data.notnull()
    max_altitudes = filtered_data_with_altitudes.max(dim='level')
    max_altitudes.plot()
    return max_altitudes.values[0]



