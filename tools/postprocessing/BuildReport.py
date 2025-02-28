#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Feb 27 14:21:11 2025

@author: baggio_r2
"""
import xarray as xr
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
from scipy.interpolate import griddata
from datetime import datetime
from scipy import ndimage
import contextily as ctx
from pyproj import Transformer

#from skimage import measure
import os
import re
import sys 
from matplotlib.colors import LogNorm
home_dir = os.path.expanduser("~")

sys.path.append(home_dir+'/soft/firefront/tools/')
from preprocessing.ffToGeoJson import get_WSEN_LBRT_ZS_From_Pgd
from  atmo_fire_plotsFrom import Plot_surfaceWind_TKE,Plot_GroundSmokeConcentration,Plot_LargeScale_Plume,Plot_1D_Plots
from  atmo_fire_plotsFrom import create_pylatex_pdf,plot_location
from MNHCDF_to_FFNC_atmodataset import BuildDS4AllTimes, findSmokeScaleFactor, list_NetCDFfiles_in_directory,extract_number

#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------

casepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/newtestcases/005_runFIRE/"
mnfiles=""
savepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/newtestcases/"
smoke_scalefactor=findSmokeScaleFactor(casepath)
pathBMAP=casepath+"/ForeFire/Outputs/ForeFire.0.nc"
pathffParams=casepath+"/ForeFire/Params.ff"
list_nc=list_NetCDFfiles_in_directory(casepath+mnfiles)
formatted_numbers = [s[-6:-3] for s in list_nc]
# Sort the list of strings based on the extracted numbers
formatted_numbers = sorted(formatted_numbers, key=extract_number)
formatted_numbers =[int(s) for s in formatted_numbers]
timesteps=formatted_numbers[:-1] 
ignition="2017-06-17T13:30:00"
namefire="Pedrógão Grande"
# combined_ds=BuildDS4AllTimes(timesteps,casepath,mnfiles,pathBMAP,pathffParams,smoke_scalefactor,ignition,namefire,savepath)
combined_ds=xr.load_dataset(savepath+f"{namefire}_small.nc")
provider1=ctx.providers.CartoDB.Voyager
provider2=ctx.providers.CartoDB.Voyager
provider3=ctx.providers.CartoDB.Voyager
saveflag=True
savedirectory="/Users/baggio_r/soft/firefront/tools/postprocessing/plotsROB/"
savedirectory2="/Users/baggio_r/soft/firefront/tools/postprocessing/plotsROBTime/"
###############################################################################
endtime= pd.to_datetime(combined_ds["Time"][-1].values)#.strftime("%Y-%m-%d %H:%M:%S")
lon_fine = combined_ds["Wind_U"].coords["longitude"].values
lat_fine = combined_ds["Wind_U"].coords["latitude"].values
transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)

lon_min=np.amin(lon_fine)
lon_max=np.amax(lon_fine)
lat_min=np.amin(lat_fine)
lat_max=np.amax(lat_fine)
SW_boundary_point =  transformer.transform(lon_min, lat_min)
NW_boundary_point =  transformer.transform(lon_min, lat_max)
SE_boundary_point =  transformer.transform(lon_max, lat_min)
NE_boundary_point =  transformer.transform(lon_max, lat_max)

plotboundaries=[SW_boundary_point[0],SE_boundary_point[0],SW_boundary_point[1], NW_boundary_point[1]]
###############################################################################
Plot_1D_Plots(combined_ds,fontitle=25,savefig=True,savepath=savedirectory,savename="1DPlot",dpi=200)
i=0
font=30
for timestep in timesteps:
    if combined_ds.IgnitionTime.values>combined_ds.Time[timestep].values:
        print(f"Ignition:{combined_ds.IgnitionTime.values} TimeStep:{combined_ds.Time[timestep].values}")
        continue
    else:
        savename1=f"{i}_P1"
        savename2=f"{i}_P2"
        savename3=f"{i}_P3"
        Plot_surfaceWind_TKE(timestep=timestep,boundarypoints=plotboundaries,
                             dsfile=combined_ds,
                             ignitiondate=ignition,
                             fireenddate=endtime,
                             fontitle=font,
                             provider=provider1,
                             savefig=saveflag,
                             savepath=savedirectory2,
                             savename=savename1,
                             dpi=150)
        Plot_GroundSmokeConcentration(timestep=timestep,boundarypoints=plotboundaries,
                                      dsfile=combined_ds,
                                      ignitiondate=ignition,
                                      fireenddate=endtime,
                                      fontitle=font,
                                      provider=provider2,
                             savefig=saveflag,
                             savepath=savedirectory2,
                             savename=savename2,
                             dpi=150)
        Plot_LargeScale_Plume(timestep=timestep,boundarypoints=plotboundaries,
                              dsfile=combined_ds,
                            ignitiondate=ignition,
                            fireenddate=endtime,
                              fontitle=font,
                              provider=provider3,
                             savefig=saveflag,
                             savepath=savedirectory2,
                             savename=savename3,
                             dpi=150)
        i+=1

# # List only PNG files
# plot_files_time = [f for f in os.listdir(savedirectory2) if f.endswith(".png")]
# plot_files_time=[item for item in plot_files_time if not item.startswith(("0", "1"))]
# plot_files_time=sorted(plot_files_time)
# print(plot_files_time)
plot_location(combined_ds.IgnitionCoords.values[0],combined_ds.IgnitionCoords.values[1],provider=ctx.providers.OpenStreetMap.Mapnik)
create_pylatex_pdf(combined_ds, savedirectory,savedirectory2, savedirectory+"report.pdf")
# #::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# #------------------------------------------------------------------------------
# # Use the final burned area to set up the boundary of the plotting Domain
# #------------------------------------------------------------------------------

# firefront_max = np.ma.masked_less(combined_ds["ArrivalTime"][-1].values, 0.0)

# # Extract the boundary of the full burned region (to keep as reference for the plot)  
# # Find the contours of the masked region
# binary_mask_all = ~firefront_max.mask  # Convert masked array to binary mask
# labeled_array, num_features = ndimage.label(binary_mask_all)  # Label connected regions
# slices_all = ndimage.find_objects(labeled_array)  # Find the bounding box

# # lon_fine = combined_ds["ArrivalTime"].coords["lon_bmap"].values
# # lat_fine = combined_ds["ArrivalTime"].coords["lat_bmap"].values

# lon_fine = combined_ds["Wind_U"].coords["longitude"].values
# lat_fine = combined_ds["Wind_U"].coords["latitude"].values


# # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
# transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)

# # Convert the coordinates to Web Mercato
# # Convert the coordinates to Web Mercator
# # lon_web_mercator_fine, lat_web_mercator_fine = transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)

# #------------------------------------------------------------------------------
# #::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# factorlon=0.3 #sets up the distance of boundaries from the fire front
# factorlat=0.5 #sets up the distance of boundaries from the fire front
# # lon_min = lon_fine[slices_all[0]].min() - factorlon*abs( lon_fine[slices_all[0]].max() -lon_fine[slices_all[0]].min())
# # lon_max = lon_fine[slices_all[0]].max() + factorlon*abs( lon_fine[slices_all[0]].max() -lon_fine[slices_all[0]].min())
# # lat_min = lat_fine[slices_all[0]].min() - factorlat*abs( lat_fine[slices_all[0]].max() -lat_fine[slices_all[0]].min())
# # lat_max = lat_fine[slices_all[0]].max() + factorlat*abs( lat_fine[slices_all[0]].max() -lat_fine[slices_all[0]].min())
# lon_min=np.amin(lon_fine)
# lon_max=np.amax(lon_fine)
# lat_min=np.amin(lat_fine)
# lat_max=np.amax(lat_fine)
# SW_boundary_point =  transformer.transform(lon_min, lat_min)
# NW_boundary_point =  transformer.transform(lon_min, lat_max)
# SE_boundary_point =  transformer.transform(lon_max, lat_min)
# NE_boundary_point =  transformer.transform(lon_max, lat_max)

# plotboundaries=[SW_boundary_point[0],SE_boundary_point[0],SW_boundary_point[1], NW_boundary_point[1]]
# endtime="2017-06-17T23:30:00"
# for timestep in timesteps:
# # for timestep in timesteps[20:21]:
#     savename1="plot1"
#     savename2="plot2"
#     savename3="plot3"
#     Plot_surfaceWind_TKE(timestep=timestep,boundarypoints=plotboundaries,
#                          dsfile=combined_ds,
#                          ignitiondate=ignition,
#                          fireenddate=endtime,
#                          fontitle=22,
#                          provider=provider1,
#                          savefig=saveflag,
#                          savepath=savedirectory,
#                          savename=savename1,
#                          dpi=150)
#     Plot_GroundSmokeConcentration(timestep=timestep,boundarypoints=plotboundaries,
#                                   dsfile=combined_ds,
#                                   ignitiondate=ignition,
#                                   fireenddate=endtime,
#                                   fontitle=22,
#                                   provider=provider2,
#                          savefig=saveflag,
#                          savepath=savedirectory,
#                          savename=savename2,
#                          dpi=150)
#     Plot_LargeScale_Plume(timestep=timestep,boundarypoints=plotboundaries,
#                           dsfile=combined_ds,
#                         ignitiondate=ignition,
#                         fireenddate=endtime,
#                           fontitle=22,
#                           provider=provider3,
#                          savefig=saveflag,
#                          savepath=savedirectory,
#                          savename=savename3,
#                          dpi=150)


