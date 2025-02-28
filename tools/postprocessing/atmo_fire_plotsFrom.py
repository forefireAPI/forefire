#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jun 11 18:07:15 2024

@author: baggio_r
"""

import xarray as xr
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import numpy as np
import contextily as ctx
# from pyproj import Proj, transform
from pyproj import Transformer

import pandas as pd
from datetime import datetime
import os
from pylatex import Document, Section, Subsection, Itemize, Figure, NoEscape, Command
from pylatex import MiniPage,SubFigure

import glob
# Function to compute the normal vector
def compute_normal(gy, gx):
    norm = np.sqrt(gy**2 + gx**2)
    return gy / norm, -gx / norm

#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#                                   Plot n1
#------------------------------------------------------------------------------
    # 1) Surface wind field plotted with arrows
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Contours of turbolent energy above 1.5 m2 s-2 above 1.5 m2s-2 in the first ~1000 m above the ground
    # 5) Upward_air_velocity above 3 m s-1 in the first ~1000 m above the ground displayed with crosses in colorcode 
#------------------------------------------------------------------------------
def Plot_surfaceWind_TKE(timestep,boundarypoints,dsfile,ignitiondate,fireenddate,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",savename="TKE_Plot",dpi=200):
    """
    Routine to create .png plots displaying:
    
    # 1) Surface wind field plotted with arrows, colors and dimensions proportional to winfd magnitude
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Contours of turbolent energy above 1.5 m2 s-2 above 1.5 m2s-2 in the first ~1000 m above the ground
    # 5) Upward_air_velocity above 3 m s-1 in the first ~1000 m above the ground displayed with crosses in colorcode prop to w
        
    Parameters
    ----------
    timestep: timestep to plot
    boundarypoints : a list of points in  Web Mercator (epsg:3857) coordinates
        DESCRIPTION. The default are set up on the basis of the last simulation timestep where the plotted region is bigger.
    dsfile : Xarray DataSet created from the wildfire netCDF file using xr.load_dataset(filebmap,engine="netcdf4")   
    ignitiondate : date of ignition in format "2017-06-17T23:30:00"
    fireenddate :  last date of the simulation "2017-06-17T23:30:00"
    provider=contextily provider for basemap
    savefig : flag to save the plot
    savepath : path where plots are saved
    fontitle : fontsize of plot title
    
    Returns
    -------
    None.

    """
    #------------------------------------------------------------------------------
    # Recover the Dates
    #------------------------------------------------------------------------------
    FlagFireStarted=True
    datetime_str=dsfile["Time"][timestep].values
    print(f"datetime_str is {datetime_str}")
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    # datetime_str_start=dsfile["Time"][0].values
    # datetime_obj_start = pd.to_datetime(datetime_str_start)
    datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
    print(f"datetime_obj_start is {datetime_obj_start}")
    if datetime_obj_start > datetime_obj:
        print(f"Fire has not started yet! ignition:{datetime_obj_start} timestep:{datetime_obj}")
        FlagFireStarted=False
    day_start = datetime_obj_start.day
    # datetime_str_max=dsfile["Time"][-1].values
    # datetime_obj_max = pd.to_datetime(datetime_str_max)
    datetime_obj_max=fireenddate#atetime.strptime(fireenddate, "%Y-%m-%dT%H:%M:%S")
    # Extract the hour, minute, and second
    day_max = datetime_obj_max.day
    hour_max = datetime_obj_max.hour
    minute_max = datetime_obj_max.minute
    second_max = datetime_obj_max.second
   
    #------------------------------------------------------------------------------
    # Set up plot style and title
    #------------------------------------------------------------------------------
    # fontitle =fontitle
    
    fig = plt.figure(figsize=(15,15),constrained_layout=True)
    
    gs = fig.add_gridspec(3, 3, height_ratios=[1,1, 0.05], width_ratios=[1, 1, 0.05])
    
    ax = fig.add_subplot(gs[:2, :2])
    
    Namefire=dsfile["WildfireName"].values
    print(Namefire)
    plt.suptitle("Surface wind field, turbolence and fire fronts at ",fontsize=1.2*fontitle)
    plt.title(f"{datetime_str_form} UTC",fontsize=fontitle)
    
    #Set plot boundaries
    ax.axis([boundarypoints[0],boundarypoints[1],boundarypoints[2], boundarypoints[3]])
    ax.tick_params(axis='both', labelsize=0.8*fontitle)
    #------------------------------------------------------------------------------
    # Choose the map background(here with contextily)
    # ctx.add_basemap(ax,source=ctx.providers.CartoDB.Positron)
    # ctx.add_basemap(ax,source=ctx.providers.OpenStreetMap.Mapnik)
    ctx.add_basemap(ax,source=provider)
    # ctx.add_basemap(ax,source=ctx.providers.GeoportailFrance.plan)
    #------------------------------------------------------------------------------
    # Calculate the module of the surface wind:
    u = dsfile["Wind_U"][timestep,:,:].values
    v = dsfile["Wind_V"][timestep,:,:].values
    w = dsfile["Wind_W"][timestep,:,:].values
    windmodule = np.sqrt(np.power(u,2)+np.power(v,2)+np.power(w,2))
    #------------------------------------------------------------------------------
    # Recovering the TKE (only TKE>1.5 m2 s-2 is stored)
    tke2D=dsfile["TKE"][timestep].values
    # tke2D = np.max(tke, axis=0)
    masked_tke2D = np.ma.masked_where(tke2D == 0, tke2D)
    #------------------------------------------------------------------------------
    # Recovering the strong uplifts (positive upward_air_velocities >=3 m s-1 are stored)
    wup2D=dsfile["W_Upward"][timestep].values
    # wup2D = np.max(wup, axis=0)
    masked_wup = np.ma.masked_where(wup2D == 0, wup2D)
    #------------------------------------------------------------------------------
    # Coordinates necessary for plotting
    lon = dsfile["Wind_U"].coords["longitude"].values
    lat = dsfile["Wind_U"].coords["latitude"].values
    print(f"checking lat lon shape {len(lon)} {len(lat)}")
    lon_fine = dsfile["ArrivalTime"].coords["lon_bmap"].values
    lat_fine = dsfile["ArrivalTime"].coords["lat_bmap"].values

    # lon_fine[np.isnan(lon_fine)] = 0
    # lat_fine[np.isnan(lat_fine)] = 0
    
    lon_points=dsfile["FrontCoords"][timestep][:,1].values
    # print(lon_points)
    lat_points=dsfile["FrontCoords"][timestep][:,0].values
    lon_points=lon_points[~np.isnan(lon_points)]
    lat_points=lat_points[~np.isnan(lat_points)]
    # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
    # proj_wgs84 = Proj(init='epsg:4326')
    # proj_web_mercator = Proj(init='epsg:3857')
    transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)

    # Convert the coordinates to Web Mercator
    lon_web_mercator, lat_web_mercator = transformer.transform(lon, lat)# transform(proj_wgs84, proj_web_mercator, lon, lat)
    lon_web_mercator_fine, lat_web_mercator_fine = transformer.transform(lon_fine, lat_fine)#transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)
    lon_web_mercator_points, lat_web_mercator_points =transformer.transform(lon_points, lat_points)# transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)
    print(f"checking lon_web_mercator_fine {np.amin(lon_web_mercator_fine)} {np.amax(lon_web_mercator_fine)}  len: {len(lon_web_mercator_fine)}")
    print(f"checking N web_mercator_points {len(lon_web_mercator_points)} {len(lat_web_mercator_points)}")
    print(f"checking lon_web_mercator_points {np.amin(lon_web_mercator_points)} {np.amax(lon_web_mercator_points)}")
    #print(lon_web_mercator_points)


    if FlagFireStarted:
        #------------------------------------------------------------------------------
        #   Time of Arrival
        #------------------------------------------------------------------------------
        # Burning map and fire fronts
        #------------------------------------------------------------------------------
        normal_magnitudes=dsfile["FrontVelocity"][timestep].values
        # print(f"check normal_magnitudes: {normal_magnitudes} ")    
        normal_magnitudes = normal_magnitudes[~np.isnan(normal_magnitudes)]
        # normal_magnitudes = normal_magnitudes[np.isnan(normal_magnitudes)]=0
        print(f"check front len: MAGN:{len(normal_magnitudes)}  LAT:{len(lon_points)} LON:{len(lat_points)}")
        firefront = np.ma.masked_less(dsfile["ArrivalTime"][timestep].values, 0.0)
        print(f"check minmax firefront: {firefront.min()} {firefront.max()} ")    

        # # Use pcolormesh to display Time of Arrival intensity as a background
        
        # minvalue=np.amin(dsfile["Ignition"].values[~np.isnan(dsfile["Ignition"].values)])/3600        
        # min_colorscale=int(minvalue)
        # max_colorscale=int(24*(day_max-day_start)+hour_max)
        
        minvalue=datetime_obj_start.hour#np.amin(dsfile["Ignition"].values[~np.isnan(dsfile["Ignition"].values)])/3600    
        print(f"check minvalue: {minvalue} ")    
        min_colorscale=int(minvalue)
        max_colorscale=int(24*(day_max-day_start)+hour_max)
        
        hours=np.arange(min_colorscale,max_colorscale,2)
        print(f"check colorscale: {min_colorscale} {max_colorscale}")
        cmapt = plt.get_cmap('gist_heat', max_colorscale-min_colorscale)
        atime = ax.pcolormesh(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,cmap=cmapt, alpha=0.5)
        atimec = ax.contour(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,levels=hours, linewidth=2,cmap=cmapt)
        ###set the colorbar
        cax3 = fig.add_subplot(gs[0, 2])
        cbaatime=plt.colorbar(atime,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,1),fraction=0.015,aspect=30, pad=0.04)
        cbaatime=plt.colorbar(atimec,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,1),fraction=0.015,aspect=30, pad=0.04)
        cbaatime.set_label('Hours from ignition h',fontsize=0.9*fontitle)
        
        # Plot the boundary points of the fire front such that they are proportional to the propagation velocity
        sizes = normal_magnitudes * 700  # Adjust the multiplier for better visibility
        firef=ax.scatter(lon_web_mercator_points,lat_web_mercator_points, s=sizes, c=normal_magnitudes, label='Boundary Points',cmap="hot")
        # firef=ax.scatter(lon_web_mercator_points,lat_web_mercator_points, label='Boundary Points',cmap="hot")

        #set the colorbar for ROS
        cax4 = fig.add_subplot(gs[1, 2])
        cmapff=plt.colorbar(firef,cax=cax4,anchor=(0,0.5),fraction=0.015,aspect=30, pad=0.04)
        cmapff.set_label('ROS ms-1',fontsize=0.9*fontitle)
    
    
    # #------------------------------------------------------------------------------
    # #   Surface wind field 
    # #------------------------------------------------------------------------------
    # # # Adjust the length and width of the arrows based on intensity
    scale_factor = 0.005  # Adjust this factor to control the arrow size
    u_scaled = u * scale_factor
    v_scaled = v * scale_factor
    
    # # Define a slice to skip drawing some of the quiver arrows to reduce clutter
    skip = (slice(None, None, 2), slice(None, None, 2))
    windmodule_cmap=windmodule[skip]
    # # Use the quiver function to display wind field at the ground with their direction and intensity
    norm=plt.Normalize(np.min(windmodule_cmap), np.max(windmodule_cmap))
    cmapp = plt.cm.nipy_spectral_r  # Choose a colormap
    colors = cmapp(norm(windmodule_cmap))
    
    ax.quiver(lon_web_mercator[skip], lat_web_mercator[skip], u_scaled[skip], v_scaled[skip], color=colors.reshape(-1, 4), scale=1, width=0.001, headwidth=3)
    
    # Add the colorbar
    sm = plt.cm.ScalarMappable(cmap=cmapp, norm=norm)
    sm.set_array([])
    cbarwf = plt.colorbar(sm, ax=ax,fraction=0.025,aspect=40, pad=0.03,orientation='horizontal')
    cbarwf.set_label('Magnitude of surface wind (ms-1)',fontsize=0.9*fontitle)
    # cbar = plt.colorbar(sm, ax=ax,fraction=0.046, pad=0.04)
    # cbar.set_label('Magnitude of surface wind (ms-1)',fontsize=0.5*fontitle)
    
    # #------------------------------------------------------------------------------
    # #    TKE
    # #------------------------------------------------------------------------------
    cctke = ax.contourf(lon_web_mercator, lat_web_mercator,masked_tke2D,cmap="viridis",alpha=0.3)
    # Convert masked array to a regular array for contour detection
    masked_tke2Dfilled = masked_tke2D.filled(fill_value=0)
    # Find contours at a constant value of 0.5 (can be adjusted)
    tkecontour = ax.contour(lon_web_mercator, lat_web_mercator, masked_tke2Dfilled, levels=[0.5], linewidth=1,linestyles="dashed" ,cmap="viridis")
    # Add the colorbar
    cax2 = fig.add_subplot(gs[2, 1])
    cbartke = plt.colorbar(cctke,location="bottom", cax=cax2,fraction=0.015,aspect=30, pad=0.04)
    cbartke.set_label('Turbolent Energy m2s-2',fontsize=0.9*fontitle)
    
    # #------------------------------------------------------------------------------
    # #   Scatter points for intense vertical wind
    # #------------------------------------------------------------------------------
    # #Plot region where vertical winndsfile exceed threshold using scatter points:
    
    normw = plt.Normalize(wup2D.min(), wup2D.max())
    cmapw = plt.cm.cool 
    colorsw = cmapw(normw(wup2D))
    sc = ax.scatter(lon_web_mercator, lat_web_mercator,s=wup2D*50, c=wup2D, cmap='cool', marker='x')#
    
    cax1 = fig.add_subplot(gs[2, 0])
    cbarw = plt.colorbar(sc,location="bottom", cax=cax1,fraction=0.015,aspect=30, pad=0.04)
    cbarw.set_label('Vertical wind ms-1',fontsize=0.9*fontitle)
    # # Show the map
    if savefig:
        # plt.savefig(savepath+"plot_SurfaceWind_Turbolence_"+str(timestep)+"_"+str(Namefire)[:5]+"_"+str(datetime_str_form)[:-6].replace(" ", "_")+"_UTC.png", dpi=dpi)
        plt.savefig(savepath+savename, dpi=dpi)
    plt.show()
    
#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
#                                   Plot n2
#------------------------------------------------------------------------------
#wind field, smoke concentration at the ground and fire fronts
#------------------------------------------------------------------------------

def Plot_GroundSmokeConcentration(timestep,boundarypoints,dsfile,ignitiondate,fireenddate,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",savename="SmokePlot",dpi=200):
    """
    Routine to create .png plots displaying:
    
    # 1) Surface wind field direction plotted with arrows
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Levels of smoke concentration at the ground (kg kg-1)
        
    Parameters
    ----------
    timestep: timestep to plot
    boundarypoints : a list of points in  Web Mercator (epsg:3857) coordinates
        DESCRIPTION. The default are set up on the basis of the last simulation timestep where the plotted region is bigger.
    dsfile : Xarray DataSet created from the wildfire netCDF file using xr.load_dataset(filebmap,engine="netcdf4")      
    ignitiondate : date of ignition in format "2017-06-17T23:30:00"
    fireenddate :  last date of the simulation "2017-06-17T23:30:00"
    provider: contextily object for basemap
    savefig : flag to save the plot
    savepath : path where plots are saved
    fontitle : fontsize of plot title
    
    Returns
    -------
    None.
    """
    #------------------------------------------------------------------------------
    # Recover the Dates
    #------------------------------------------------------------------------------
    FlagFireStarted=True
    datetime_str=dsfile["Time"][timestep].values
    print(f"datetime_str is {datetime_str}")
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    # datetime_str_start=dsfile["Time"][0].values
    # datetime_obj_start = pd.to_datetime(datetime_str_start)
    datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
    print(f"datetime_obj_start is {datetime_obj_start}")
    if datetime_obj_start > datetime_obj:
        print(f"Fire has not started yet! ignition:{datetime_obj_start} timestep:{datetime_obj}")
        FlagFireStarted=False
    day_start = datetime_obj_start.day
    # datetime_str_max=dsfile["Time"][-1].values
    # datetime_obj_max = pd.to_datetime(datetime_str_max)
    # datetime_obj_max=datetime.strptime(fireenddate, "%Y-%m-%dT%H:%M:%S")
    datetime_obj_max=fireenddate#atetime.strptime(fireenddate, "%Y-%m-%dT%H:%M:%S")

    # Extract the hour, minute, and second
    day_max = datetime_obj_max.day
    hour_max = datetime_obj_max.hour
    minute_max = datetime_obj_max.minute
    second_max = datetime_obj_max.second
    #------------------------------------------------------------------------------
    # Set up plot style and title
    #------------------------------------------------------------------------------
    #fontitle = 22
    
    fig = plt.figure(figsize=(15,15),constrained_layout=True)
    
    gs = fig.add_gridspec(3, 3, height_ratios=[1,1, 0.05], width_ratios=[1, 1, 0.05])
    
    ax = fig.add_subplot(gs[:2, :2])
    
    Namefire=dsfile["WildfireName"].values
    plt.suptitle("Surface wind field and smoke concentration, fire fronts",fontsize=1.2*fontitle)
    plt.title(f"{datetime_str_form} UTC",fontsize=fontitle)
    
    #Set plot boundaries
    ax.axis([boundarypoints[0],boundarypoints[1],boundarypoints[2], boundarypoints[3]])
    ax.tick_params(axis='both', labelsize=0.8*fontitle)
    #------------------------------------------------------------------------------
    # Choose the map background(here with contextily)
    # ctx.add_basemap(ax,source=ctx.providers.CartoDB.Positron)
    # ctx.add_basemap(ax,source=ctx.providers.OpenStreetMap.Mapnik)
    ctx.add_basemap(ax,source=provider)
    # ctx.add_basemap(ax)
    # ctx.add_basemap(ax,source=ctx.providers.GeoportailFrance.plan)
    #------------------------------------------------------------------------------
    # Calculate the module of the surface wind:
    u = dsfile["Wind_U"][timestep,:,:].values
    v = dsfile["Wind_V"][timestep,:,:].values
    w = dsfile["Wind_W"][timestep,:,:].values
    windmodule = np.sqrt(np.power(u,2)+np.power(v,2)+np.power(w,2))
    #------------------------------------------------------------------------------
    #Recovering the Smoke at the ground
    smoke = dsfile["GroundSmoke"][timestep,:,:].values
    
    #------------------------------------------------------------------------------
    # Coordinates necessary for plotting
    lon = dsfile["Wind_U"].coords["longitude"].values
    lat = dsfile["Wind_U"].coords["latitude"].values

    lon_fine = dsfile["ArrivalTime"].coords["lon_bmap"].values
    lat_fine = dsfile["ArrivalTime"].coords["lat_bmap"].values

    lon_points=dsfile["FrontCoords"][timestep][:,1].values
    lat_points=dsfile["FrontCoords"][timestep][:,0].values
    lon_points=lon_points[~np.isnan(lon_points)]
    lat_points=lat_points[~np.isnan(lat_points)]
    # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
    transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)

    # Convert the coordinates to Web Mercator
    lon_web_mercator, lat_web_mercator = transformer.transform(lon, lat)# transform(proj_wgs84, proj_web_mercator, lon, lat)
    lon_web_mercator_fine, lat_web_mercator_fine = transformer.transform(lon_fine, lat_fine)#transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)
    lon_web_mercator_points, lat_web_mercator_points =transformer.transform(lon_points, lat_points)# transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)
   #------------------------------------------------------------------------------
    #   Surface wind field 
    #------------------------------------------------------------------------------
    # # Adjust the length and width of the arrows based on intensity
    scale_factor = 0.025  # Adjust this factor to control the arrow size
    u_plot = scale_factor*u/windmodule
    v_plot = scale_factor*v/windmodule
    
    # # Define a slice to skip drawing some of the quiver arrows to reduce clutter
    skip = (slice(None, None, 2), slice(None, None, 2))
    windmodule_cmap=windmodule[skip]
    # # Use the quiver function to display wind field at the ground with their direction and intensity
    norm=plt.Normalize(np.min(windmodule_cmap), np.max(windmodule_cmap))
    cmapp = plt.cm.nipy_spectral_r  # Choose a colormap
    colors = cmapp(norm(windmodule_cmap))
    
    ax.quiver(lon_web_mercator[skip], lat_web_mercator[skip], u_plot[skip], v_plot[skip], color="Black", scale=1, width=0.001, headwidth=3)
    
    #------------------------------------------------------------------------------
    #   Smoke at the ground
    #------------------------------------------------------------------------------
    ## avoid spurious negative values
    smokegroundp= np.where(smoke <= 0, 1e-30,smoke)
    #threshold is the air quality criterion for PM2.5 approx 12$10-9 kg/kg 24 h average
    aqgOMS=3*10**(-8)
    # Convert masked array to a regular array for contour detection
    # smokegroundpfilled = smokegroundp.filled(fill_value=1e-30)
    # custom_levels = [ 0.00000001*aqgOMS,0.0000001*aqgOMS, 0.000001*aqgOMS,0.00001*aqgOMS, 0.0001*aqgOMS,0.001*aqgOMS,0.01*aqgOMS,  0.1*aqgOMS,aqgOMS  ]
    # custom_levels = [  0.01*aqgOMS,0.1*aqgOMS,aqgOMS,10*aqgOMS, 100*aqgOMS,1000*aqgOMS, 10**4*aqgOMS ]
    
    custom_levels = [0.001*aqgOMS,0.01*aqgOMS,0.1*aqgOMS,aqgOMS,10*aqgOMS, 100*aqgOMS,1000*aqgOMS] #ppm

    # custom_levels = [ 0.0001*aqgOMS,0.001*aqgOMS, 0.01*aqgOMS,0.1*aqgOMS, aqgOMS,10*aqgOMS,100*aqgOMS,  1000*aqgOMS,10000*aqgOMS  ]
    cmapsmoke="copper_r"
    smokecontourf = ax.contourf(lon_web_mercator, lat_web_mercator, smokegroundp, levels=custom_levels[3:], norm=LogNorm(vmin=custom_levels[3], vmax=custom_levels[-1]), cmap=cmapsmoke,alpha=0.5)
    smokecontour = ax.contour(lon_web_mercator, lat_web_mercator, smokegroundp, levels=custom_levels[3:],linewidths=4 ,norm=LogNorm(vmin=custom_levels[3], vmax=custom_levels[-1]), cmap=cmapsmoke)
    
    class CustomScalarFormatter(ScalarFormatter):
        def __init__(self, precision=3):
            super().__init__(useOffset=False, useMathText=True)
            self.precision = precision
    
        def _set_format(self):
            self.format = f'%.{self.precision}e'
    
        def _set_order_of_magnitude(self):
            super()._set_order_of_magnitude()
            self.orderOfMagnitude = 0
    
        def format_data_short(self, value):
            return self.format % value
    
    
    cbarsmoke = plt.colorbar(smokecontourf,location="bottom", ax=ax,fraction=0.025,aspect=30, pad=0.04,extend="both")
    cbarsmoke.set_label('Levels of smoke concentration kg kg-1',fontsize=0.9*fontitle)
    cbarsmoke.set_ticks(custom_levels[3:])  # Set the same levels for colorbar ticks
    formatter = CustomScalarFormatter(precision=2)  # Set precision as desired
    cbarsmoke.ax.xaxis.set_major_formatter(formatter)
    
    #------------------------------------------------------------------------------
    #   Time of Arrival
    #------------------------------------------------------------------------------
    # Burning map and fire fronts
    #------------------------------------------------------------------------------
    if FlagFireStarted:
        firefront = np.ma.masked_less(dsfile["ArrivalTime"][timestep].values, 0.0)
        normal_magnitudes=dsfile["FrontVelocity"][timestep].values
        normal_magnitudes = normal_magnitudes[~np.isnan(normal_magnitudes)]

        # # Use pcolormesh to display Time of Arrival intensity as a background
        
        minvalue=datetime_obj_start.hour
        min_colorscale=int(minvalue)
        max_colorscale=int(24*(day_max-day_start)+hour_max)
        # print(f"check colorscale: {min_colorscale} {max_colorscale}")
        # cmapt = plt.get_cmap('gist_heat', max_colorscale-min_colorscale)
        # hours=np.arange(min_colorscale,max_colorscale,2)
        # # atime = ax.pcolormesh(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,cmap=cmapt)
        # atime = ax.contourf(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,levels=hours,vmin=min_colorscale, vmax=max_colorscale ,cmap=cmapt,alpha=0.1)
    
        # #set the colorbar
        # cax3 = fig.add_subplot(gs[0, 2])
        # cbaatime=plt.colorbar(atime,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,2),fraction=0.015,aspect=30, pad=0.04)
        # cbaatime.set_label('Hours from midnight h',fontsize=0.9*fontitle)
        
        # Plot the boundary and the boundary points of the fire front such that they are proportional to the propagation velocity
        ax.scatter(lon_web_mercator_points,lat_web_mercator_points,s=1,color="Black",linewidths=3)
        sizes = normal_magnitudes * 700  # Adjust the multiplier for better visibility
        firef=ax.scatter(lon_web_mercator_points,lat_web_mercator_points, s=sizes, c=normal_magnitudes, label='Boundary Points',cmap="hot")
        #set the colorbar for ROS
        cax4 = fig.add_subplot(gs[1, 2])
        cmapff=plt.colorbar(firef,cax=cax4,anchor=(0,0.5),fraction=0.015,aspect=30, pad=0.04)
        cmapff.set_label('ROS ms-1',fontsize=0.9*fontitle)
    
    # Show the map
    if savefig:
        # plt.savefig(savepath+"plot_SmokeGround_"+str(timestep)+"_"+str(Namefire)[:5]+"_"+str(datetime_str_form)[:-6].replace(" ", "_")+"_UTC.png", dpi=dpi)
        plt.savefig(savepath+savename, dpi=dpi)
    plt.show()

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#                                   Plot n3
#------------------------------------------------------------------------------
#     Plot the bottom of the plume
#------------------------------------------------------------------------------
#Altitude of smoke
def Plot_LargeScale_Plume(timestep,boundarypoints,dsfile,ignitiondate,fireenddate,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",savename="PlumePlot",dpi=200):
    """
    Routine to create 2 X .png plots displaying:
    
    # 1) Top of the FirePlume with altitude in m on the large domain (m)
    # 2) Bottom of the FirePlume with altitude in m on the large domain (m)
        
    Parameters
    ----------
    timestep: timestep to plot
    boundarypoints : a list of points in  Web Mercator (epsg:3857) coordinates
        DESCRIPTION. The default are set up on the basis of the last simulation timestep where the plotted region is bigger.
    dsfile : Xarray DataSet created from the wildfire netCDF file using xr.load_dataset(filebmap,engine="netcdf4")      
    ignitiondate : date of ignition in format "2017-06-17T23:30:00"
    fireenddate :  last date of the simulation "2017-06-17T23:30:00"
    provider: contextily object for basemap
    savefig : flag to save the plot
    savepath : path where plots are saved
    fontitle : fontsize of plot title
    
    Returns
    -------
    None.
    """
    
    #------------------------------------------------------------------------------
    # Recover the Dates
    #------------------------------------------------------------------------------
    FlagFireStarted=True
    datetime_str=dsfile["Time"][timestep].values
    print(f"datetime_str is {datetime_str}")
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    # datetime_str_start=dsfile["Time"][0].values
    # datetime_obj_start = pd.to_datetime(datetime_str_start)
    datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
    print(f"datetime_obj_start is {datetime_obj_start}")
    if datetime_obj_start > datetime_obj:
        print(f"Fire has not started yet! ignition:{datetime_obj_start} timestep:{datetime_obj}")
        FlagFireStarted=False
    
    if FlagFireStarted:
        day_start = datetime_obj_start.day
        # datetime_str_max=dsfile["Time"][-1].values
        # datetime_obj_max = pd.to_datetime(datetime_str_max)
        datetime_obj_max=fireenddate#atetime.strptime(fireenddate, "%Y-%m-%dT%H:%M:%S")
        # Extract the hour, minute, and second
        day_max = datetime_obj_max.day
        hour_max = datetime_obj_max.hour
        minute_max = datetime_obj_max.minute
        second_max = datetime_obj_max.second
        
        #------------------------------------------------------------------------------
        # Coordinates necessary for plotting
        #------------------------------------------------------------------------------
        lon = dsfile["Z_FirePlumeBottom"].coords["longitudeM1"].values
        lat = dsfile["Z_FirePlumeBottom"].coords["latitudeM1"].values
    
        lon_fine = dsfile["ArrivalTime"].coords["lon_bmap"].values
        lat_fine = dsfile["ArrivalTime"].coords["lat_bmap"].values
    
        lon_points=dsfile["FrontCoords"][timestep][:,0]
        lat_points=dsfile["FrontCoords"][timestep][:,1]
        # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
        # proj_wgs84 = Proj(init='epsg:4326')
        # proj_web_mercator = Proj(init='epsg:3857')
        transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)
    
    
        # Convert the coordinates to Web Mercator
    
        lon_Large_web_mercator, lat_Large_web_mercator = transformer.transform( lon, lat) #transform(proj_wgs84, proj_web_mercator, lon, lat)
        lon_web_mercator, lat_web_mercator = transformer.transform( lon_fine, lat_fine) #transform(proj_wgs84, proj_web_mercator, lon, lat)
        lon_web_mercator_points, lat_web_mercator_points =transformer.transform( lon_points, lat_points) # transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)
    
        #------------------------------------------------------------------------------
        # Recover the Dates
        #------------------------------------------------------------------------------
        datetime_str=dsfile["Time"][timestep].values
        print(f"datetime_str is {datetime_str}")
        datetime_obj = pd.to_datetime(datetime_str)
        datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")
    
        # datetime_str_start=dsfile["Time"][0].values
        # datetime_obj_start = pd.to_datetime(datetime_str_start)
        datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
        day_start = datetime_obj_start.day
        # datetime_str_max=dsfile["Time"][-1].values
        # datetime_obj_max = pd.to_datetime(datetime_str_max)
        datetime_obj_max=fireenddate#atetime.strptime(fireenddate, "%Y-%m-%dT%H:%M:%S")
        # Extract the hour, minute, and second
        day_max = datetime_obj_max.day
        hour_max = datetime_obj_max.hour
        minute_max = datetime_obj_max.minute
        second_max = datetime_obj_max.second
        
        ###############################################################################
        #------------------------------------------------------------------------------
        # PLOT1: Set up plot style and title
        #------------------------------------------------------------------------------
        ###############################################################################
        
        #fontitle =25
        
        # fig,ax= plt.subplots(2,1, figsize=(12,20),constrained_layout=True)
        fig = plt.figure(figsize=(15,15),constrained_layout=True)
        # gs = fig.add_gridspec(2, 2, height_ratios=[1,1], width_ratios=[1, 0.05])
        ax = fig.add_subplot(111)
        
        Namefire=dsfile["WildfireName"].values
        print(f"Namefire is {Namefire}")
        plt.suptitle("Top of large scale smoke plume",fontsize=1.2*fontitle)
        ax.set_title(f"{datetime_str_form} UTC",fontsize=fontitle)
        
        #Set plot boundaries
        
        #------------------------------------------------------------------------------
    
        #------------------------------------------------------------------------------
        #Recovering the Smoke at the ground
        smokeground = np.where(dsfile["Z_FirePlumeBottom"][timestep].values==0.1,dsfile["Z_FirePlumeBottom"][timestep].values,-1)
        #threshold is the air quality criterion for CO
        print(np.amax(smokeground),np.amin(smokeground))
        #------------------------------------------------------------------------------
        #                        Plot the top of the plume
        #------------------------------------------------------------------------------
        # ax = fig.add_subplot(gs[0, :1])
        #Set subplot boundaries
        ax.axis([np.amin(lon_Large_web_mercator),np.amax(lon_Large_web_mercator),np.min(lat_Large_web_mercator),np.amax(lat_Large_web_mercator)])
        # ctx.add_basemap(ax,source=ctx.providers.GeoportailFrance.plan)    
        # ctx.add_basemap(ax,source=ctx.providers.CartoDB.Positron)
        # ctx.add_basemap(ax,source=ctx.providers.OpenStreetMap.Mapnik)
        ctx.add_basemap(ax,source=provider)
        ax.tick_params(axis='both', labelsize=0.8*fontitle)

    
    
        z_plumetop_flat = dsfile["Z_FirePlumeTop"][timestep].values
    
        plumetop_flat_masked=np.ma.masked_equal(z_plumetop_flat ,np.min(z_plumetop_flat))
        vmin=0
        vmax=np.amax(np.amax(plumetop_flat_masked))
        # print(np.amax(plumetop_flat_masked.data))
        if len(np.unique(dsfile["Z_FirePlumeTop"]))>1:
            topcontourf = ax.contourf(lon_Large_web_mercator, lat_Large_web_mercator,plumetop_flat_masked,vmin=vmin,vmax=vmax,cmap="inferno",alpha=0.8)
            cbplumebot=plt.colorbar(topcontourf,ax=ax,fraction=0.025,aspect=30, pad=0.04)
            cbplumebot.set_label('Smoke altitude of upper fireplume m',fontsize=0.9*fontitle)
        if savefig:
            plt.savefig(savepath+savename+"TOP.png", dpi=dpi)
        plt.show()
        # smokecontour = ax[0].contour(lon_web_mercator, lat_web_mercator, smokegroundp, levels=custom_levels,linewidths=4 ,norm=LogNorm(vmin=custom_levels[0], vmax=custom_levels[-1]), cmap='Greys')
        ###############################################################################
        
        ###############################################################################
        #------------------------------------------------------------------------------
        # PLOT1: Set up plot style and title
        #------------------------------------------------------------------------------
        ###############################################################################
        
        fig1 = plt.figure(figsize=(15,15),constrained_layout=True)
        ax1 = fig1.add_subplot(111)
        plt.suptitle("Bottom of large scale smoke plume",fontsize=1.2*fontitle)
        ax1.set_title(f"{datetime_str_form} UTC",fontsize=fontitle)

        #------------------------------------------------------------------------------
        #                        Plot the bottom of the plume
        #------------------------------------------------------------------------------
        
        # ax2 = fig.add_subplot(gs[1:2, :1])
        #Set subplot boundaries
        ax1.axis([np.amin(lon_Large_web_mercator),np.amax(lon_Large_web_mercator),np.min(lat_Large_web_mercator),np.amax(lat_Large_web_mercator)])
        # ctx.add_basemap(ax1,source=ctx.providers.GeoportailFrance.plan)
        # ctx.add_basemap(ax1,source=ctx.providers.CartoDB.Positron)
        # ctx.add_basemap(ax1,source=ctx.providers.OpenStreetMap.Mapnik)
        ctx.add_basemap(ax1,source=provider)
        ax1.tick_params(axis='both', labelsize=0.8*fontitle)
        z_plumebottom_flat = dsfile["Z_FirePlumeBottom"][timestep].values
        # z_plumebottom_flat = z_plumebottom_flat[~np.isnan(z_plumebottom_flat)]
        # plumebottom_flat_masked=np.ma.masked_equal(z_plumebottom_flat ,np.min(z_plumebottom_flat))  
        plumebottom_flat_masked=np.ma.masked_equal(z_plumebottom_flat ,0)  
    
        # # print(np.amax(plumebottom_flat_masked.data))
        if len(np.unique(dsfile["Z_FirePlumeBottom"]))>1:
            bottomcontourf = ax1.contourf(lon_Large_web_mercator, lat_Large_web_mercator,plumebottom_flat_masked,vmin=vmin,mvax=vmax,cmap="inferno",alpha=0.8)
            cbplumebot=plt.colorbar(bottomcontourf,ax=ax1,fraction=0.025,aspect=30, pad=0.04)
            cbplumebot.set_label('Smoke altitude of lower fireplume  m',fontsize=0.9*fontitle)
        ax1.contour(lon_Large_web_mercator, lat_Large_web_mercator, smokeground,levels=[0,0.1],linewidths=2,linestyles=["-"],colors=["Red"])
        plotxrange=np.amax(lon_Large_web_mercator)- np.amin(lon_Large_web_mercator)
        plotyrange=np.amax(lat_Large_web_mercator)- np.amin(lat_Large_web_mercator)
        xtext=np.amin(lon_Large_web_mercator)+0.45*plotxrange
        ytext=np.amin(lat_Large_web_mercator)+0.92*plotyrange
        lbox=dict(boxstyle="square",
                       ec=(1., 0.5, 0.5),
                       fc=(1., 0.8, 0.8),
                       )
        ax1.text(x=xtext,y=ytext,s="_ Smoke at the ground",color="red",bbox=lbox,size=0.75*fontitle)
        # #------------------------------------------------------------------------------
        # #                   Plot the fire perimeter for reference
        #------------------------------------------------------------------------------
        # firef=ax[1].scatter(lon_web_mercator_points,lat_web_mercator_points,s=1,color="White",label="fire front")
        # ax.tight_layout()
        #Save the Plot:
        if savefig:
            plt.savefig(savepath+savename+"BOTTOM.png", dpi=dpi)
        plt.show()
    else:
        print(f'There is nothing to plot yet!')
    
#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#                                   Plot 1D plots
#------------------------------------------------------------------------------
    # 1) max ROS for timestep
    # 2) 

def Plot_1D_Plots(dsfile,fontitle=22,savefig=True,savepath="",savename="1DPlot",dpi=200):
    """
    

    Parameters
    ----------
    dsfile : Xarray DataSet created from the wildfire netCDF file using xr.load_dataset(filebmap,engine="netcdf4")      
    ignitiondate : date of ignition in format "2017-06-17T23:30:00"
    fireenddate :  last date of the simulation "2017-06-17T23:30:00"
    savefig : 
    savename : TYPE, optional
    dpi : TYPE, optional
    Returns
    -------
    None.

    """
    #------------------------------------------------------------------------------
    # Recover the Dates
    #------------------------------------------------------------------------------
    # ignition_dt_obj=
    # enddate_dt_object=
    plt.figure(figsize=(20,5))
    plt.plot(dsfile.BurnedArea.coords["extended_timesteps"].values,dsfile.BurnedArea.values/10000,"-o",color="Brown")
    plt.xlabel("UTC Time (h)",fontsize=0.8*fontitle)
    plt.ylabel("Burned area (ha)",fontsize=0.8*fontitle)
    plt.savefig(savepath+savename+"_1.png",dpi=dpi)
    plt.show()
    plt.figure(figsize=(20,5))
    plt.plot(dsfile.max_ROS.coords["extended_timesteps"].values,dsfile.max_ROS.values,"-o",color="Red")
    plt.xlabel("UTC Time (h)",fontsize=0.8*fontitle)
    plt.ylabel("Max Rate Of Spread (ms-1)",fontsize=0.8*fontitle)
    plt.savefig(savepath+savename+"_2.png",dpi=dpi)
    plt.show()
    plt.figure(figsize=(20,5))
    plt.plot(dsfile.max_PlumeHeight.coords["timestep"].values,dsfile.max_PlumeHeight.values,"-o",color="Grey")
    plt.xlabel("UTC Time (h)",fontsize=0.8*fontitle)
    plt.ylabel("Smoke plume altitude (m)",fontsize=0.8*fontitle)
    plt.savefig(savepath+savename+"_3.png",dpi=dpi)
    plt.show()
    
def plot_location(lat, lon, output_image="location.png",provider=ctx.providers.OpenStreetMap.Mapnik):
    """
    Plots a map with a marker at the given lat/lon using contextily and saves the figure.

    Parameters:
        lat (float): Latitude of the location
        lon (float): Longitude of the location
        output_image (str): Filename for saving the output image
    """

    # Convert lat/lon to Web Mercator (EPSG:3857) using pyproj
    transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)
    x, y = transformer.transform(lon, lat)

    # Create plot
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.scatter(x, y, color="red", marker="x", s=500, label="Ignition Point")  # Mark location with X
    ax.set_xlim(x - 5000, x + 5000)  # Set map zoom level
    ax.set_ylim(y - 5000, y + 5000)
    
    ctx.add_basemap(ax, crs="EPSG:3857", source=provider)

    ax.legend()
    #ax.set_title(f"Location: {lat}, {lon}")
    
    # Save the figure
    plt.savefig(output_image, dpi=300, bbox_inches="tight")
    plt.show()
    plt.close()

    print(f"Map saved as: {output_image}")
    return output_image

def create_pylatex_pdf(nc_file, dir1,dir2, output_pdf):
    """
    Extracts metadata from a NetCDF file, creates a LaTeX document, and compiles it into a PDF using pylatex.

    Parameters:
        nc_file (str): Path to the NetCDF file.
        dir (str): Directory containing the plot images.
        output_pdf (str): Output PDF filename.
    """

    # Open the NetCDF file
    ds = nc_file#xr.open_dataset(nc_file)

    # Extract the title variable (assuming the first variable is the title)
    title_var = "WildfireName" # Take the first variable as title
    title_value = str(ds[title_var].values) if title_var in ds else "Unknown Title"

    # Extract two variables for the bullet list
    variables = list(ds.data_vars.keys())
    bullet_var0 =  ds["WildfireName"].values
    bullet_var1 =  np.datetime_as_string(ds["IgnitionTime"].values, unit='s')#datetime.strptime(str(ds["IgnitionTime"].values), "%Y-%m-%dT%H:%M:%S")#variables[1] if len(variables) > 1 else "No Variable 1"
    bullet_var2 = ds["IgnitionCoords"].values[0]# if len(variables) > 2 else "No Variable 2"
    bullet_var3 = ds["IgnitionCoords"].values[1]
    
    # Retrieve all plot files from the directory
    plot_files = sorted(glob.glob(os.path.join(dir1, "*.png")))
    plot_files2 = sorted(glob.glob(os.path.join(dir2, "*.png")))
    plot_files2 = sorted(plot_files2, key=lambda x: int(os.path.basename(x).split('_')[0]))
    print(plot_files2)
    map_image=os.getcwd()+"/location.png"
    # Determine the number of pages needed for (2x2) subplots per page
    #num_time_steps = len(ds["Time"]) if "Time" in ds else 1  # Default to 1 if no time dimension
    filtered_timestamps = ds["Time"].values[ds["Time"].values > ds["IgnitionTime"].values]
    times=filtered_timestamps#len(ds["Time"].values)
    print(times)
    num_time_steps=len(times)
    print(num_time_steps)
    plots_per_page = 4  # 2x2 grid
    total_pages = int(np.ceil(num_time_steps))

    # Create a LaTeX document
    geometry_options = {
        #"paperwidth": "11in",  # Adjust paper width as needed
        #"paperheight": "8.5in",  # Adjust paper height as needed
        "margin": "0.5in"  # Adjust margins to make more space for figures
    }
    doc = Document(geometry_options=geometry_options)
    doc.packages.append(NoEscape(r'\usepackage{subcaption}'))  # Add subcaption package

    # Title and Metadata
    doc.preamble.append(Command('title',"Report Example" ))#f'{title_var}: {title_value}'))
    #doc.preamble.append(Command('author', 'Automated Report'))
    doc.preamble.append(Command('date', NoEscape(r'')))
    doc.append(NoEscape(r'\maketitle'))

    # Key Variables Section
    # with doc.create(Section("Time and location of Fire Ignition")):
    #     with doc.create(Itemize()) as itemize:
    #         itemize.add_item(f"Ignition Time: {bullet_var1}")
    #         itemize.add_item(f"Ignition Coordinates: lat={bullet_var2:.4f}, lon= {bullet_var3:.4f}")

    #with doc.create(Section("Time and location of Fire Ignition")):
    with doc.create(Figure(position='h!')) as fig:
        #fig.append(NoEscape(r'\centering'))

        # Create a minipage for the key variables (left side)
        with fig.create(MiniPage(width=NoEscape(r'0.5\textwidth'))) as left_side:
            #left_side.append(NoEscape(r'\textbf{Key Variables}'))
            with left_side.create(Itemize()) as itemize:
                itemize.add_item(f"Wildfire ID: {bullet_var0}")
                itemize.add_item(f"Ignition time: {bullet_var1}")
                itemize.add_item(f"Ignition point: lat={bullet_var2:.4f}, lon= {bullet_var3:.4f}")

        # Create a minipage for the map (right side)
        with fig.create(MiniPage(width=NoEscape(r'0.3\textwidth'))) as right_side:
            right_side.append(NoEscape(r'\raggedleft'))
            right_side.append(NoEscape(f'\includegraphics[width=\\textwidth]{{{map_image}}}'))
                #right_side.append(NoEscape(r'\caption{Location of the study area}'))    
                
    #####    Section for Single Plots
    with doc.create(Section("Fire activity")) as sec1:
        with doc.create(Figure(position="h!")) as fig1:
            fig1.add_image(plot_files[0],width=NoEscape(r'0.8\linewidth'), placement=NoEscape(r"\centering"))
            fig1.add_caption("Burned area (ha)")
        # with doc.create(Figure(position="h!")) as fig2:
            fig1.add_image(plot_files[1],width=NoEscape(r'0.8\linewidth'), placement=NoEscape(r"\centering"))
            fig1.add_caption("maximum value of the Rate Of Spread (ms-1)")
            # fig1.append(NoEscape(r"\newline"))
        # with doc.create(Figure(position="h!")) as fig3:
            fig1.add_image(plot_files[2],width=NoEscape(r'0.8\linewidth'), placement=NoEscape(r"\centering"))
            fig1.add_caption("Altitude of the smoke plume (ha)")        
            fig1.append(NoEscape(r"\newline"))
        # with doc.create(Figure(position='h!')) as fig:
            # fig.add_caption("Burned area (ha), max ROS (ms-1) and smoke plume altitude (m) evolution with time")
            # for i, plot in enumerate(plot_files, start=1):  # Show first 3 plots
            #     safe_plot_path = NoEscape(r'\detokenize{' + plot + '}') 
            #     print(f"i:{i}  plot:{plot}")
            #     fig.add_image(plot, width=NoEscape(r'0.8\linewidth'), placement=NoEscape(r"\centering"))
            #     #fig.append(r'//')
            #     fig.add_caption("")
            #     #fig.append(a*, width=NoEscape(0.8\textwidth), placement=NoEscape(\centering))  # Line break
    doc.append(NoEscape(r"\newpage"))
    ##### New Section for Multiple Pages of 2x2 Subplots
    with doc.create(Section("Wildfire Evolution")):
        subplot_labels = ["a", "b", "c", "d"]
        jj=0
        for page in range(total_pages):
            with doc.create(Figure(position='h!')) as fig:
                curtime=np.datetime_as_string( times[jj], unit='s')
                jj+=1
                fig.add_caption(f"Time: {curtime}")
                fig.append(NoEscape(r'\centering'))
        
                for i in range(plots_per_page):
                    plot_index = page * plots_per_page + i
                    if plot_index < len(plot_files2):
                        print(f"i:{i}  plot:{plot_files2[plot_index]}")
                        # Add subfigure correctly
                        with fig.create(SubFigure(width=NoEscape("0.45\\textwidth"))) as subfig:
                            subfig.add_image(plot_files2[plot_index], width=NoEscape(r"\textwidth"))
                            subfig.add_caption(f"Subplot {subplot_labels[i]}")
        
                        # Move to next row after 2 images
                        if i % 2 == 1:
                            fig.append(NoEscape(r'\\'))
    # Generate the PDF
    doc.generate_pdf(output_pdf.replace('.pdf', ''), clean_tex=False)

    print(f"PDF saved as: {output_pdf}")


