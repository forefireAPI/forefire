#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jun 11 18:07:15 2024

@author: baggio_r
"""

import xarray as xr
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from matplotlib.ticker import ScalarFormatter, LogFormatter
import numpy as np
import contextily as ctx
from pyproj import Proj, transform
import pandas as pd
from datetime import datetime
import os
from scipy import ndimage
from mpl_toolkits.axes_grid1 import make_axes_locatable


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
def Plot_surfaceWind_TKE(timestep,boundarypoints,dsfile,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",dpi=200):
    """
    Routine to create .png plots displaying:
    
    # 1) Surface wind field plotted with arrows, colors and dimensions proportional to winfd magnitude
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Contours of turbolent energy above 1.5 m2 s-2 above 1.5 m2s-2 in the first ~1000 m above the ground
    # 5) Upward_air_velocity above 3 m s-1 in the first ~1000 m above the ground displayed with crosses in colorcode prop to w
        
    A plot is created for every available timestep
    Parameters
    ----------
    boundarypoints : a list of points in  Web Mercator (epsg:3857) coordinates
        DESCRIPTION. The default are set up on the basis of the last simulation timestep where the plotted region is bigger.
    dsfile : Xarray DataSet created from the wildfire netCDF file using xr.load_dataset(filebmap,engine="netcdf4")   
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
    datetime_str=dsfile["Time"][timestep].values
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    datetime_str_start=dsfile["Time"][0].values
    datetime_obj_start = pd.to_datetime(datetime_str_start)
    day_start = datetime_obj_start.day
    datetime_str_max=dsfile["Time"][-1].values
    datetime_obj_max = pd.to_datetime(datetime_str_max)

    # Extract the hour, minute, and second
    day_max = datetime_obj_max.day
    hour_max = datetime_obj_max.hour
    minute_max = datetime_obj_max.minute
    second_max = datetime_obj_max.second
   
    #------------------------------------------------------------------------------
    # Set up plot style and title
    #------------------------------------------------------------------------------
    fontitle =fontitle
    
    fig = plt.figure(figsize=(15,15),constrained_layout=True)
    
    gs = fig.add_gridspec(3, 3, height_ratios=[1,1, 0.05], width_ratios=[1, 1, 0.05])
    
    ax = fig.add_subplot(gs[:2, :2])
    
    Namefire=dsfile["WildfireName"].values
    print(Namefire)
    plt.suptitle(Namefire,fontsize=1.2*fontitle)
    plt.title(f"Surface wind field, turbolence and fire fronts at {datetime_str_form} UTC",fontsize=fontitle)
    
    #Set plot boundaries
    ax.axis([plotboundaries[0],plotboundaries[1],plotboundaries[2], plotboundaries[3]])
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
    tke=dsfile["TKE"][timestep].values
    tke2D = np.max(tke, axis=0)
    masked_tke2D = np.ma.masked_where(tke2D == 0, tke2D)
    #------------------------------------------------------------------------------
    # Recovering the strong uplifts (positive upward_air_velocities >=3 m s-1 are stored)
    wup=dsfile["W_Upward"][timestep].values
    wup2D = np.max(wup, axis=0)
    masked_wup = np.ma.masked_where(wup2D == 0, wup2D)
    #------------------------------------------------------------------------------
    # Coordinates necessary for plotting
    lon = dsfile["Wind_U"].coords["longitude"].values
    lat = dsfile["Wind_U"].coords["latitude"].values

    lon_fine = dsfile["ArrivalTime"].coords["lon_bmap"].values
    lat_fine = dsfile["ArrivalTime"].coords["lat_bmap"].values

    lon_points=dsfile["FrontCoords"][timestep][:,0]
    lat_points=dsfile["FrontCoords"][timestep][:,1]
    # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
    proj_wgs84 = Proj(init='epsg:4326')
    proj_web_mercator = Proj(init='epsg:3857')

    # Convert the coordinates to Web Mercator
    lon_web_mercator, lat_web_mercator = transform(proj_wgs84, proj_web_mercator, lon, lat)
    lon_web_mercator_fine, lat_web_mercator_fine = transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)
    lon_web_mercator_points, lat_web_mercator_points = transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)
    #------------------------------------------------------------------------------
    #   Time of Arrival
    #------------------------------------------------------------------------------
    # Burning map and fire fronts
    #------------------------------------------------------------------------------
    firefront = np.ma.masked_less(dsfile["ArrivalTime"][timestep].values, 0.0)
    normal_magnitudes=dsfile["FrontVelocity"][timestep].values
    # # Use pcolormesh to display Time of Arrival intensity as a background
    
    minvalue=np.amin(dsfile["Ignition"].values[~np.isnan(dsfile["Ignition"].values)])/3600        
    min_colorscale=int(minvalue)
    max_colorscale=int(24*(day_max-day_start)+hour_max)
    hours=np.arange(min_colorscale,max_colorscale,2)
    print(f"check colorscale: {min_colorscale} {max_colorscale}")
    cmapt = plt.get_cmap('gist_heat', max_colorscale-min_colorscale)
    atime = ax.pcolormesh(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,cmap=cmapt, alpha=0.5)
    atimec = ax.contour(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,levels=hours, linewidth=2,cmap=cmapt)
    #set the colorbar
    cax3 = fig.add_subplot(gs[0, 2])
    cbaatime=plt.colorbar(atime,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,1),fraction=0.015,aspect=30, pad=0.04)
    cbaatime=plt.colorbar(atimec,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,1),fraction=0.015,aspect=30, pad=0.04)
    cbaatime.set_label('Hours from ignition h',fontsize=0.9*fontitle)
    
    # Plot the boundary points of the fire front such that they are proportional to the propagation velocity
    sizes = normal_magnitudes * 700  # Adjust the multiplier for better visibility
    firef=ax.scatter(lon_web_mercator_points,lat_web_mercator_points, s=sizes, c=normal_magnitudes, label='Boundary Points',cmap="hot")
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
        plt.savefig(savepath+"plot_SurfaceWind_Turbolence_"+str(timestep)+"_"+str(Namefire)[:5]+"_"+str(datetime_str_form)[:-6].replace(" ", "_")+"_UTC.png", dpi=dpi)
    plt.show()
    
#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
#                                   Plot n2
#------------------------------------------------------------------------------
#wind field, smoke concentration at the ground and fire fronts
#------------------------------------------------------------------------------

def Plot_GroundSmokeConcentration(timestep,boundarypoints,dsfile,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",dpi=200):
    """
    Routine to create .png plots displaying:
    
    # 1) Surface wind field direction plotted with arrows
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Levels of smoke concentration at the ground (kg kg-1)
        
    A plot is created for every available timestep
    Parameters
    ----------
    boundarypoints : a list of points in  Web Mercator (epsg:3857) coordinates
        DESCRIPTION. The default are set up on the basis of the last simulation timestep where the plotted region is bigger.
    dsfile : Xarray DataSet created from the wildfire netCDF file using xr.load_dataset(filebmap,engine="netcdf4")      
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
    datetime_str=dsfile["Time"][timestep].values
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    datetime_str_start=dsfile["Time"][0].values
    datetime_obj_start = pd.to_datetime(datetime_str_start)
    day_start = datetime_obj_start.day
    datetime_str_max=dsfile["Time"][-1].values
    datetime_obj_max = pd.to_datetime(datetime_str_max)

    # Extract the hour, minute, and second
    day_max = datetime_obj_max.day
    hour_max = datetime_obj_max.hour
    minute_max = datetime_obj_max.minute
    second_max = datetime_obj_max.second
    #------------------------------------------------------------------------------
    # Set up plot style and title
    #------------------------------------------------------------------------------
    fontitle = 22
    
    fig = plt.figure(figsize=(15,15),constrained_layout=True)
    
    gs = fig.add_gridspec(3, 3, height_ratios=[1,1, 0.05], width_ratios=[1, 1, 0.05])
    
    ax = fig.add_subplot(gs[:2, :2])
    
    Namefire=dsfile["WildfireName"].values
    plt.suptitle(Namefire,fontsize=1.2*fontitle)
    plt.title(f"Surface wind field and smoke concentration, fire fronts at {datetime_str_form} UTC",fontsize=fontitle)
    
    #Set plot boundaries
    ax.axis([SW_boundary_point[0],SE_boundary_point[0],SW_boundary_point[1], NW_boundary_point[1]])
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

    lon_points=dsfile["FrontCoords"][timestep][:,0]
    lat_points=dsfile["FrontCoords"][timestep][:,1]
    # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
    proj_wgs84 = Proj(init='epsg:4326')
    proj_web_mercator = Proj(init='epsg:3857')

    # Convert the coordinates to Web Mercator
    lon_web_mercator, lat_web_mercator = transform(proj_wgs84, proj_web_mercator, lon, lat)
    lon_web_mercator_fine, lat_web_mercator_fine = transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)
    lon_web_mercator_points, lat_web_mercator_points = transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)
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
    firefront = np.ma.masked_less(dsfile["ArrivalTime"][timestep].values, 0.0)
    normal_magnitudes=dsfile["FrontVelocity"][timestep].values
    # # Use pcolormesh to display Time of Arrival intensity as a background
    
    minvalue=np.amin(dsfile["Ignition"].values[~np.isnan(dsfile["Ignition"].values)])/3600        
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
        plt.savefig(savepath+"plot_SmokeGround_"+str(timestep)+"_"+str(Namefire)[:5]+"_"+str(datetime_str_form)[:-6].replace(" ", "_")+"_UTC.png", dpi=dpi)
    plt.show()

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#                                   Plot n3
#------------------------------------------------------------------------------
#     Plot the bottom of the plume
#------------------------------------------------------------------------------
#Altitude of smoke
def Plot_LargeScale_Plume(timestep,boundarypoints,dsfile,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",dpi=200):
    #------------------------------------------------------------------------------
    # Coordinates necessary for plotting
    #------------------------------------------------------------------------------
    lon = dsfile["Z_FirePlumeBottom"].coords["large_scale_longitude"].values
    lat = dsfile["Z_FirePlumeBottom"].coords["large_scale_latitude"].values

    lon_fine = dsfile["ArrivalTime"].coords["lon_bmap"].values
    lat_fine = dsfile["ArrivalTime"].coords["lat_bmap"].values

    lon_points=dsfile["FrontCoords"][timestep][:,0]
    lat_points=dsfile["FrontCoords"][timestep][:,1]
    # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
    proj_wgs84 = Proj(init='epsg:4326')
    proj_web_mercator = Proj(init='epsg:3857')

    # Convert the coordinates to Web Mercator

    lon_Large_web_mercator, lat_Large_web_mercator = transform(proj_wgs84, proj_web_mercator, lon, lat)
    lon_web_mercator, lat_web_mercator = transform(proj_wgs84, proj_web_mercator, lon, lat)
    lon_web_mercator_points, lat_web_mercator_points = transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)

    #------------------------------------------------------------------------------
    # Recover the Dates
    #------------------------------------------------------------------------------
    datetime_str=dsfile["Time"][timestep].values
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    datetime_str_start=dsfile["Time"][0].values
    datetime_obj_start = pd.to_datetime(datetime_str_start)
    day_start = datetime_obj_start.day
    datetime_str_max=dsfile["Time"][-1].values
    datetime_obj_max = pd.to_datetime(datetime_str_max)

    # Extract the hour, minute, and second
    day_max = datetime_obj_max.day
    hour_max = datetime_obj_max.hour
    minute_max = datetime_obj_max.minute
    second_max = datetime_obj_max.second
    #------------------------------------------------------------------------------
    # Set up plot style and title
    #------------------------------------------------------------------------------
    
    fontitle =25
    
    fig,ax= plt.subplots(2,1, figsize=(12,20),constrained_layout=True)
    # gs = fig.add_gridspec(2, 2, height_ratios=[1,1], width_ratios=[1, 0.05])

    
    Namefire=dsfile["WildfireName"].values
    plt.suptitle(Namefire,horizontalalignment='left',fontsize=1.2*fontitle)
    ax[0].set_title(f"Large scale smoke plume at {datetime_str_form} UTC",fontsize=fontitle)
    
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
    ax[0].axis([np.amin(lon_Large_web_mercator),np.amax(lon_Large_web_mercator),np.min(lat_Large_web_mercator),np.amax(lat_Large_web_mercator)])
    # ctx.add_basemap(ax[0],source=ctx.providers.GeoportailFrance.plan)    
    # ctx.add_basemap(ax[0],source=ctx.providers.CartoDB.Positron)
    # ctx.add_basemap(ax[0],source=ctx.providers.OpenStreetMap.Mapnik)
    ctx.add_basemap(ax[0],source=provider)


    z_plumetop_flat = dsfile["Z_FirePlumeTop"][timestep].values

    plumetop_flat_masked=np.ma.masked_equal(z_plumetop_flat ,np.min(z_plumetop_flat))
    
    vmin=0
    vmax=np.amax(np.amax(plumetop_flat_masked))
    # print(np.amax(plumetop_flat_masked.data))
    if len(np.unique(ds["Z_FirePlumeTop"]))>1:
        topcontourf = ax[0].contourf(lon_Large_web_mercator, lat_Large_web_mercator,plumetop_flat_masked,vmin=vmin,vmax=vmax,cmap="inferno",alpha=0.8)
        cbplumebot=plt.colorbar(topcontourf,ax=ax[0],fraction=0.025,aspect=30, pad=0.04)
        cbplumebot.set_label('Smoke altitude of upper fireplume m',fontsize=0.8*fontitle)
    # smokecontour = ax[0].contour(lon_web_mercator, lat_web_mercator, smokegroundp, levels=custom_levels,linewidths=4 ,norm=LogNorm(vmin=custom_levels[0], vmax=custom_levels[-1]), cmap='Greys')

    #------------------------------------------------------------------------------
    #                        Plot the bottom of the plume
    #------------------------------------------------------------------------------
    # ax2 = fig.add_subplot(gs[1:2, :1])
    #Set subplot boundaries
    ax[1].axis([np.amin(lon_Large_web_mercator),np.amax(lon_Large_web_mercator),np.min(lat_Large_web_mercator),np.amax(lat_Large_web_mercator)])
    # ctx.add_basemap(ax[1],source=ctx.providers.GeoportailFrance.plan)
    # ctx.add_basemap(ax[1],source=ctx.providers.CartoDB.Positron)
    # ctx.add_basemap(ax[1],source=ctx.providers.OpenStreetMap.Mapnik)
    ctx.add_basemap(ax[1],source=provider)

    z_plumebottom_flat = dsfile["Z_FirePlumeBottom"][timestep].values
    # z_plumebottom_flat = z_plumebottom_flat[~np.isnan(z_plumebottom_flat)]
    # plumebottom_flat_masked=np.ma.masked_equal(z_plumebottom_flat ,np.min(z_plumebottom_flat))  
    plumebottom_flat_masked=np.ma.masked_equal(z_plumebottom_flat ,0)  

    print(timestep,len(np.unique(ds["Z_FirePlumeBottom"])))
    # # print(np.amax(plumebottom_flat_masked.data))
    if len(np.unique(ds["Z_FirePlumeBottom"]))>1:
        bottomcontourf = ax[1].contourf(lon_Large_web_mercator, lat_Large_web_mercator,plumebottom_flat_masked,vmin=vmin,mvax=vmax,cmap="inferno",alpha=0.8)
        cbplumebot=plt.colorbar(bottomcontourf,ax=ax[1],fraction=0.025,aspect=30, pad=0.04)
        cbplumebot.set_label('Smoke altitude of lower fireplume  m',fontsize=0.8*fontitle)
    ax[1].contour(lon_Large_web_mercator, lat_Large_web_mercator, smokeground,levels=[0,0.1],linewidths=2,linestyles=["-"],colors=["Red"])
    plotxrange=np.amax(lon_Large_web_mercator)- np.amin(lon_Large_web_mercator)
    plotyrange=np.amax(lat_Large_web_mercator)- np.amin(lat_Large_web_mercator)
    xtext=np.amin(lon_Large_web_mercator)+0.45*plotxrange
    ytext=np.amin(lat_Large_web_mercator)+0.92*plotyrange
    lbox=dict(boxstyle="square",
                   ec=(1., 0.5, 0.5),
                   fc=(1., 0.8, 0.8),
                   )
    ax[1].text(x=xtext,y=ytext,s="_ Smoke at the ground",color="red",bbox=lbox,size=0.75*fontitle)
    # #------------------------------------------------------------------------------
    # #                   Plot the fire perimeter for reference
    #------------------------------------------------------------------------------
    # firef=ax[1].scatter(lon_web_mercator_points,lat_web_mercator_points,s=1,color="White",label="fire front")
    # ax.tight_layout()
    #Save the Plot:
    if savefig:
        plt.savefig(savepath+"plot_largeScalePlume_"+str(timestep)+"_"+str(Namefire)[:5]+"_"+str(datetime_str_form)[:-6].replace(" ", "_")+"_UTC.png", dpi=dpi)
    # Show the map
    plt.show()
    
    
    
    
    
#------------------------------------------------------------------------------
# Read the netcdf file
#------------------------------------------------------------------------------
#filepath = "/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Aquitaine/LaTeste/"
filepath = "/scratch/baggio_r/fcouto/KDATABASE/Valleseco_20190817/2NEST/RESULTS/" 
# filepath = "/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Canaries/" 

# filepath = "/scratch/baggio_r/fcouto/KDATABASE/TesteDeBuch_20220712/2NEST/RESULTS/" 
# filepath = "/scratch/baggio_r/fcouto/KDATABASE/Valleseco_20190817/2NEST/RESULTS/" 
# filepath = "/scratch/baggio_r/fcouto/KDATABASE/SaoJoao_20200710/2NEST/RESULTS/"

# filepath ="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Canaries/"

savedirectory=filepath+"plots/"
# namef="TESTE_hour_010"
# namef="TESTE.nc"
# namef="SAOJA.nc"


#namef="TESTEv2.nc"
namef="VALLE.nc"

filenc = filepath+namef
ds=xr.load_dataset(filenc,engine="netcdf4")
print(ds.data_vars)
saveflag=True
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
# Use the final burned area to set up the boundary of the plotting Domain
#------------------------------------------------------------------------------

firefront_max = np.ma.masked_less(ds["ArrivalTime"][-1].values, 0.0)

# Extract the boundary of the full burned region (to keep as reference for the plot)  
# Find the contours of the masked region
binary_mask_all = ~firefront_max.mask  # Convert masked array to binary mask
labeled_array, num_features = ndimage.label(binary_mask_all)  # Label connected regions
slices_all = ndimage.find_objects(labeled_array)  # Find the bounding box

lon_fine = ds["ArrivalTime"].coords["lon_bmap"].values
lat_fine = ds["ArrivalTime"].coords["lat_bmap"].values

# Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
proj_wgs84 = Proj(init='epsg:4326')
proj_web_mercator = Proj(init='epsg:3857')

# Convert the coordinates to Web Mercator
lon_web_mercator_fine, lat_web_mercator_fine = transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)

#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
factorlon=0.3 #sets up the distance of boundaries from the fire front
factorlat=0.5 #sets up the distance of boundaries from the fire front
lon_min = lon_fine[slices_all[0]].min() - factorlon*abs( lon_fine[slices_all[0]].max() -lon_fine[slices_all[0]].min())
lon_max = lon_fine[slices_all[0]].max() + factorlon*abs( lon_fine[slices_all[0]].max() -lon_fine[slices_all[0]].min())
lat_min = lat_fine[slices_all[0]].min() - factorlat*abs( lat_fine[slices_all[0]].max() -lat_fine[slices_all[0]].min())
lat_max = lat_fine[slices_all[0]].max() + factorlat*abs( lat_fine[slices_all[0]].max() -lat_fine[slices_all[0]].min())

SW_boundary_point =  transform(proj_wgs84, proj_web_mercator,lon_min, lat_min)
NW_boundary_point =  transform(proj_wgs84, proj_web_mercator,lon_min, lat_max)
SE_boundary_point =  transform(proj_wgs84, proj_web_mercator,lon_max, lat_min)
NE_boundary_point =  transform(proj_wgs84, proj_web_mercator,lon_max, lat_max)

plotboundaries=[SW_boundary_point[0],SE_boundary_point[0],SW_boundary_point[1], NW_boundary_point[1]]
#------------------------------------------------------------------------------



timesteps=ds.Time.coords['timestep'].values
#------------------------------------------------------------------------------
provider1=ctx.providers.CartoDB.Voyager
provider2=ctx.providers.CartoDB.Voyager
provider3=ctx.providers.CartoDB.Voyager

# provider1=ctx.providers.GeoportailFrance.plan
# provider2=ctx.providers.GeoportailFrance.plan
# provider3=ctx.providers.GeoportailFrance.plan
#------------------------------------------------------------------------------
for timestep in timesteps:
# for timestep in timesteps[20:21]:
    Plot_surfaceWind_TKE(timestep=timestep,boundarypoints=plotboundaries,dsfile=ds,fontitle=22,provider=provider1,savefig=saveflag,savepath=savedirectory,dpi=150)
    Plot_GroundSmokeConcentration(timestep=timestep,boundarypoints=plotboundaries,dsfile=ds,fontitle=22,provider=provider2,savefig=saveflag,savepath=savedirectory,dpi=150)
    Plot_LargeScale_Plume(timestep=timestep,boundarypoints=plotboundaries,dsfile=ds,fontitle=22,provider=provider3,savefig=saveflag,savepath=savedirectory,dpi=150)
