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
from matplotlib.colors import LogNorm
from matplotlib.ticker import ScalarFormatter, LogFormatter
from datetime import datetime
from scipy.interpolate import griddata
from datetime import datetime
from scipy import ndimage
import contextily as ctx
from pyproj import Transformer

#from skimage import measure
import os
import re
from datetime import datetime, timedelta
import sys 

from PIL import Image

import glob

import os
import pyproj

os.environ['PROJ_LIB'] = os.path.join(os.path.dirname(pyproj.__file__), 'proj_data')

#################################################################################
###     PRE-PROCESSING ROUTINES
#################################################################################
def get_ignition_datetime(file_path, dateTimestep):
    # Ensure dateTimestep is a datetime object
    if not isinstance(dateTimestep, datetime):
        dateTimestep = datetime.strptime(str(dateTimestep)[:19], "%Y-%m-%dT%H:%M:%S")

    # Read file and find the 'startFire' line
    with open(file_path, 'r') as f:
        for line in f:
            if line.startswith("startFire["):
                match = re.search(r't=(\d+)', line)
                if match:
                    seconds_since_midnight = int(match.group(1))
                    break
        else:
            raise ValueError("No 'startFire' line with time found in the file.")

    # Compute ignition datetime
    midnight = dateTimestep.replace(hour=0, minute=0, second=0, microsecond=0)
    ignition_datetime = midnight + timedelta(seconds=seconds_since_midnight)

    return ignition_datetime.strftime("%Y-%m-%dT%H:%M:%S")
# Function to print the file list inside a directory
# Function to extract the number from a string
def extract_number(s):
    match = re.search(r'\d+', s)  # Find the first sequence of digits
    return int(match.group()) if match else float('inf')
def list_NetCDFfiles_in_directory(directory):
    try:
        # List to store file names
        file_list = []
        # Define the regex pattern
        pattern = re.compile(r'^[a-zA-Z0-9]{3,5}\.2\.[a-zA-Z0-9]{3,5}.*\.nc')
        # Get all files in the directory
        with os.scandir(directory) as entries:
            for entry in entries:
                if entry.is_symlink():
                    continue
                
                if entry.is_file() and pattern.match(entry.name):
                    file_list.append(entry.name)
                # if entry.is_file():
                #     file_list.append(entry.name)
        
        return file_list
    except Exception as e:
        print(f"An error occurred: {e}")
        return []

# Function to compute the normal vector
def compute_normal(gy, gx):
    norm = np.sqrt(gy**2 + gx**2)
    return gy / norm, -gx / norm

#functipn to extract parameter from ff file
def find_first_integer_after_pattern(filename, pattern):
    with open(filename, 'r') as file:
        content = file.read()
    
    # Use regex to find the pattern and the first integer after it
    match = re.search(rf'{pattern}\D*(\d+)', content)
    
    if match:
        return int(match.group(1))
    else:
        return None
##-----------------------------------------------------------------------------

def findCSEGandCEXP(casepath, dirmesnhfilesMNH, nMod ):
    
    """
    Given the output directory of MNH for Model nMod, returns the latest time step, CSEG and CEXP
    CSEG and CEXP are the strings which compose a MesNH output file (CEXP.nMod.CSEG.000.nc)
    Parameters:
    -----------
    casepath=str, main directory with the MNH-Forefire simulation"
            example="/Users/user/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
    dirmesnhfilesMNH: str, path (relative to casepath) to the directory containing the nc files for the larger MNH model (MODEL1)
    nMod: int, number of the model we want to analyze
    Returns
    -------
    ltime(str),CEXP(str),CSEG(str)
    ltime is given as a string '001','002',...,'024'

    """
    file_names=list_NetCDFfiles_in_directory(casepath+dirmesnhfilesMNH)

    example_filename = file_names[0]
    match = re.search(r'^([a-zA-Z0-9]{3,5})\.2\.([a-zA-Z0-9]{3,5})\.', example_filename)
    if match:
        #extracts menh CEXP
        #CEXP: Experiment name (this is the name of the set of run, you have performed or you want to perform on the same physical subject) 
        #Please do not leave any blank character in this name!
        mnhcexp = match.group(1) 
        #extract mnh CSEG
        #CSEG: Name of segment (this is the name of the future run, you want to perform) Please do not leave any blank character in this name!
        mnhcseg = match.group(2)    
    formatted_numbers = [s[-6:-3] for s in file_names]
    # Sort the list of strings based on the extracted numbers
    formatted_numbers = sorted(formatted_numbers, key=extract_number)
    formatted_numbers=formatted_numbers[1:]
    return max(formatted_numbers),mnhcexp, mnhcseg
    
def findSmokeScaleFactor(paramsfile="ForeFire/Params.ff"):
    """
    paramsfile: path (relative to casepath) to the Forefire params file
    -------
    smokescalefator (float)

    """
    #Find the perimeter resolution:
    ffsettings=paramsfile    
    pattern_per = 'minimalPropagativeFrontDepth='
    perimeterres = find_first_integer_after_pattern(ffsettings, pattern_per)
    perimeterres = int(perimeterres)
    pattern_burning_pow="nominalHeatFlux="
    burning_pow=find_first_integer_after_pattern(ffsettings, pattern_burning_pow)
    burning_pow=int(burning_pow)
    pattern_burning_duration="burningDuration="
    burning_time=find_first_integer_after_pattern(ffsettings, pattern_burning_duration)
    burning_time=int(burning_time)
    ## PM2.5=24-hour mean: ≈204.49×10−9 kg/kg HAZARDOUS LEVEL
    ## rypical PM2.5 for a kg of biomass in case of Forest Fires PM2.5: Approximately 8-15 g/kg
    bratio_for_msquared=burning_pow*burning_time #Watt s m-2 => j m-2 #SVFF001 corresponding to a buened m2
    biomasskg=bratio_for_msquared/(18*10**6)#Bratio which corresponds to a kg of biomass burned (kg m-2)
    pm25kgform2=biomasskg*(0.015)  #(kg m-2)
    bratio_for_msquared=burning_time #(kg m-2) by costruction
    smoke_scalefactor=pm25kgform2/bratio_for_msquared
    return smoke_scalefactor


def Tstep2MNHnc(timestep,casepath,dirmesnhfiles,nMod):
    """
    Outputs the filenmae corresp to time step timestep of model nMod
    timestep: int, timestep (if none the last timestep is taken)
    casepath: str, main directory with the MNH-Forefire simulation
            example="/Users/user/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
    dirmesnhfiles: str, path to mnh output files directory
    nMod: int, model we want to analyse
    -------
    filenameMNH (str)

    """
    ##Open the correspondent MNH file""
    last_time,cexp,cseg=findCSEGandCEXP(casepath,dirmesnhfiles,nMod)
    if timestep==None:
        timestep=int(last_time)
    formatted_str = f"{timestep:03d}"
    filenameMNH=cexp+"."+str(nMod)+"."+cseg+"."+formatted_str+".nc"
    return filenameMNH
    
def MODEL2_to_DataArray(smoke_scalefactor,filenameM2,first_day_date,namefire=None):
    """
    Opens a MNH output file from model2 and stores the variables needed for plotting in an xarray
    2D fields:
        Wind_U["lat", "lon"]
        Wind_V["lat", "lon"]
        Wind_W["lat", "lon"]
        TKE["lat", "lon"]
        W_Upward["lat", "lon"]
        GroundSmoke["lat", "lon"]
    scalars:
        Time["datetime_str"]
        WildfireName
    Parameters
    ----------
    smoke_scalefactor: real, factor used to scale the smoke tracer
    filenameM2: str, path to MNH file
    first_day_date str, date of ignition in format  DDMMYYYY
    namefire : name of wildfire case
    Returns
    -------
    a DataArray with the varaibles to be plotted

    """

    ds=xr.load_dataset(filenameM2,engine="netcdf4")

    datetime_str=ds["time"].values[0]
    ######################################
    # Assuming u v and w are 3D arrays on a staggered grid
    # Example data shapes, adjust these to your actual grid sizes
    
    
    # -----------------------------------------------------------------------------
    #                               WIND FIELD AT THE SURFACE m s-1
    # -----------------------------------------------------------------------------
    # Interpolate u v and w components to the vertices
    #------------------------------------------------------------------------------
    ny, nx = 152, 152
    u = ds["UT"][0,:,:,:]  # shape (ny, nx+1)
    v = ds["VT"][0,:,:,:] # shape (ny+1, nx)
    w = ds["WT"][0,:,:,:]
    
    
    # Define the original latitude and longitude coordinates
    lat_center = ds["WT"].coords["latitude"].values
    lon_center = ds["WT"].coords["longitude"].values
    
    # Calculate new lat and lon coordinates for vertices
    lat_vertex = ds["UT"].coords["latitude_u"].values
    lon_vertex = ds["VT"].coords["longitude_v"].values
    
    z_vertex = ds["UT"].coords["level"].values
    level = np.searchsorted(z_vertex, 1000) - 1
    #lat_vertexx=lat_vertex-0.5*(lat_vertex[10,:]-lat_vertex[9,:])
    
    u_vertex=np.zeros((u.shape))# 
    u_vertex[:,:,0] =  u[:,:, 1] 
    u_vertex[:,:,  0:-1] = (u[:,:, :-1] + np.roll(u[:,:,:],-1,axis=1)[:,:,:-1])/ 2
    u_vertex[:,:,-1]= (u[:,:, -2] + u[:,:, -1]) / 2
    
    v_vertex=np.zeros((v.shape))
    v_vertex[:,0,:]=v[:,0,:]
    v_vertex[:,:-1, :] = (v[:,:-1, :] +np.roll(v[:,:,:],-1,axis=2)[:,:-1,:])/ 2
    v_vertex[:,-1,:]= (v[:,-2,:] +v[:,-1,:]) / 2
    
    w_vertex=np.zeros((w.shape))# 
    w_vertex[:,:,0] =  w[:,:, 1] 
    w_vertex[:,:,  0:-1] = (w[:,:, :-1] + np.roll(w[:,:,:],-1,axis=1)[:,:,:-1])/ 2
    w_vertex[:,:,-1]= (w[:,:, -2] + w[:,:, -1]) / 2
    
    w_vertex[:,0,:]=w[:,0,:]
    w_vertex[:,:-1, :] = (w[:,:-1, :] +np.roll(w[:,:,:],-1,axis=2)[:,:-1,:])/ 2
    w_vertex[:,-1,:]= (w[:,-2,:] +w[:,-1,:]) / 2
    
    u_vertex_surf=u_vertex[0,:,:]
    v_vertex_surf=v_vertex[0,:,:]
    w_vertex_surf=w_vertex[0,:,:]
    z_vertex=z_vertex[:-1]

    # Create DataArrays for u v and w
    # -----------------------------------------------------------------------------
    #                               Smoke Tracer (adimensional)
    # -----------------------------------------------------------------------------
    # 
    #------------------------------------------------------------------------------
    smoke_unscaled = ds["SVFF001"][0,:,:,:]
    smoke=smoke_unscaled*smoke_scalefactor #relation btw 1kg BRatio and PM2.5
    smoke=smoke/0.5
    smoke_vertex=np.zeros((smoke.shape))# 
    smoke_vertex[:,:,0] =  smoke[:,:, 1] 
    smoke_vertex[:,:,  0:-1] = (smoke[:,:, :-1] + np.roll(smoke[:,:,:],-1,axis=1)[:,:,:-1])/ 2
    smoke_vertex[:,:,-1]= (smoke[:,:, -2] + smoke[:,:, -1]) / 2
    
    smoke_vertex[:,0,:]=smoke[:,0,:]
    smoke_vertex[:,:-1, :] = (smoke[:,:-1, :] +np.roll(smoke[:,:,:],-1,axis=-2)[:,:-1,:])/ 2
    smoke_vertex[:,-1,:]= (smoke[:,-2,:] +smoke[:,-1,:]) / 2
    
    #keep only positive smoke 
    smoke_vertex=np.where(smoke_vertex>=0,smoke_vertex,0.0)
    smoke_vertex=smoke_vertex[:-1,:,:]
    # smoke_vertex=smoke_vertex*(.15/600)
    smoke_vertex_ground=smoke_vertex[0,:,:]

    # -----------------------------------------------------------------------------
    #                               VERTICAL w UPLIFTS >=thr_w  m s-1
    # -----------------------------------------------------------------------------
    # Select the vertical wind > 3 m s-1
    #------------------------------------------------------------------------------
    ### keep w_up below approx 1000m
    w_up=w_vertex[:level,:,:]
    # ###Threshold to consider where fire is active 
    aqgOMS=3*10**(-8)
    thr=aqgOMS#smoke_scalefactor*10**(-6)
    # w_up=np.where(smoke_vertex[:level,:,:] >= thr, w_up, 0)
    w_up_th=np.where(w_up>=4,w_up,0)
    # Compute the max along the vertical (first) axis
    w_up_2D = np.max(w_up_th, axis=0)

    # -----------------------------------------------------------------------------
    #                               TKE m2 s-2 (or j/kg)
    # -----------------------------------------------------------------------------
    # Take TKE above 3 m2s-2 in the first ~1000 m above the ground
    #------------------------------------------------------------------------------
    # Constants
    ### keep Tke below approx 1000m
    tke = ds["TKET"][0,:level,:,:] 
    tke_vertex=np.zeros((tke.shape))# 
    tke_vertex[:,:,0] =  tke[:,:, 1] 
    tke_vertex[:,:,  0:-1] = (tke[:,:, :-1] + np.roll(tke[:,:,:],-1,axis=1)[:,:,:-1])/ 2
    tke_vertex[:,:,-1]= (tke[:,:, -2] + tke[:,:, -1]) / 2
    
    tke_vertex[:,0,:]=tke[:,0,:]
    tke_vertex[:,:-1, :] = (tke[:,:-1, :] +np.roll(tke[:,:,:],-1,axis=2)[:,:-1,:])/ 2
    tke_vertex[:,-1,:]= (tke[:,-2,:] +tke[:,-1,:]) / 2
    #keep only when there is influence from the fire
    # tke_vertex=np.where(smoke_vertex[:level,:,:]>=thr,tke_vertex,0)
    #keep only tke > 1.5 m2 s-2  ( 3 m2 s-2  is indicative of highly turbulent boundary layer Heilman Bian 2010)
    thrTKE=3#1.5
    tke_vertex_th=np.where(tke_vertex>=thrTKE,tke_vertex,0)
    tke_vertex_2D = np.max(tke_vertex_th, axis=0)

    ds.close()
    # Create DataArrays
    u_da = xr.DataArray(u_vertex_surf,
                        dims=["lat", "lon"],
                        coords={
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="Wind_U")
    v_da = xr.DataArray(v_vertex_surf,
                        dims=["lat", "lon"],
                        coords={
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="Wind_V")
    w_da = xr.DataArray(w_vertex_surf,                    
                        dims=["lat", "lon"],
                        coords={
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="Wind_W")    
 
    # store upward_air_velocity above 3 m s-1 
    wup_da = xr.DataArray(w_up_2D,                    
                        dims=[ "lat", "lon"],
                        coords={
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="W_Upward")
    
    # storeTKE above 1.5 m s-1
    tke_da = xr.DataArray(tke_vertex_2D,                    
                        dims=[ "lat", "lon"],
                        coords={
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="TKE")
    
    # store smoke density at the ground   
    smoke_da = xr.DataArray(smoke_vertex_ground,                    
                            dims=["lat", "lon"],
                            coords={
                                "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                                "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                                },
                            name="GroundSmoke")
    #----------------------------  Fire Generalities   ----------------------------
    if namefire==None:
        namefire = filenameM2.split('/')[-1].split('.')[0]+"___"+first_day_date
    name_da =xr.DataArray(namefire, name="WildfireName")        
    ###############################################################################
    #------------------------------------------------------------------------------
    # Create the reduced Dataset
    #------------------------------------------------------------------------------
    ###############################################################################
    ### MODEL 2
    dsnew = xr.Dataset({
        "WildfireName": name_da,
        "Time": datetime_str,
        "Wind_U": u_da,
        "Wind_V": v_da,
        "Wind_W": w_da,
        "W_Upward": wup_da,
        "TKE": tke_da,
        "GroundSmoke": smoke_da})
    #------------------------------------------------------------------------------
    # Add global attributes
    #------------------------------------------------------------------------------
    # MODEL 2 attributes
    
    dsnew.attrs["description"] = f"{filenameM2.split('/')[-1].split('.')[0]} wildfire"
    dsnew["Wind_U"].attrs["units"] = "m s-1"
    dsnew["Wind_U"].attrs["standard_name"] = "x_wind"
    dsnew["Wind_U"].attrs["comment"] = "x_wind at the surface"
    
    dsnew["Wind_V"].attrs["units"] = "m s-1"
    dsnew["Wind_U"].attrs["standard_name"] = "y_wind"
    dsnew["Wind_U"].attrs["comment"] = "y_wind at the surface"
    
    dsnew["Wind_W"].attrs["units"] = "m s-1"
    dsnew["Wind_W"].attrs["standard_name"] = "upward_air_velocity"
    dsnew["Wind_W"].attrs["comment"] = "upward_air_velocity at the surface"
    
    dsnew["TKE"].attrs["units"] = "m2 s-2"
    dsnew["TKE"].attrs["standard_name"] = "specific_turbulent_kinetic_energy_of_air"
    dsnew["TKE"].attrs["comment"] = "max TKE above 1.5 m2s-2 in the first ~1000 m above the ground"
    
    dsnew["W_Upward"].attrs["units"] = "m s-1"
    # dsnew["W_Upward"].attrs["standard_name"] = "specific_turbulent_kinetic_energy_of_air"
    dsnew["W_Upward"].attrs["comment"] = "max upward_air_velocity above 3 m s-1 in the first ~1000 m above the ground"
    
    dsnew["GroundSmoke"].attrs["units"] = "adimensional kg kg-1"
    dsnew["GroundSmoke"].attrs["comment"] = "Smoke density at the ground"
        
    return dsnew

def MODEL1_to_DataArray(smoke_scalefactor,filenameM1,first_day_date,namefire=None):
    """
    Opens a MNH output file for model 1 and stores the variables needed for plotting in an xarray 
    2D fields:
        Z_FirePlumeBottom["latM1", "lonM1"]
        Z_FirePlumeTop["latM1", "lonM1"]
    scalars:
        Time["datetime_str"]
        WildfireName
    Parameters
    ----------
    smoke_scalefactor: real, factor used to scale the smoke tracer
    filenameM2: str, path to MNH file
    first_day_date str, date of ignition in format  DDMMYYYY
    namefire : name of wildfire case
    Returns
    -------
    a DataArray with the varaibles to be plotted

    """
    ###############################################################################
    #
    #                                   MODEL 1
    # -----------------------------------------------------------------------------
    #
    #                   Low, Large Scale Resolution Variables:
    # -----------------------------------------------------------------------------
    # The following variables are stored from model 1:
    #
    ###############################################################################
    dsM1=xr.load_dataset(filenameM1,engine="netcdf4")
    datetime_str=dsM1["time"].values[0]
    
    smoke_unsc = dsM1["SVFF001"][0,:,:,:]
    
    smoke=smoke_unsc*smoke_scalefactor #relation btw 1kg BRatio and PM2.5
    lat_center_M1 = dsM1["SVFF001"].coords["latitude"].values
    lon_center_M1 = dsM1["SVFF001"].coords["longitude"].values
    z_vertex = dsM1["SVFF001"].coords["level"].values
    ## ----------------------------------------------------------------------------
    ## Surface Wind (Magnitude and Direction) 
    ## ----------------------------------------------------------------------------
    u = dsM1["UT"][0,:,:,:]  # shape (ny, nx+1)
    v = dsM1["VT"][0,:,:,:] # shape (ny+1, nx)
    w = dsM1["WT"][0,:,:,:]
    
    
    # Define the original latitude and longitude coordinates
    lat_center = dsM1["WT"].coords["latitude"].values
    lon_center = dsM1["WT"].coords["longitude"].values
    
    # Calculate new lat and lon coordinates for vertices
    lat_vertex = dsM1["UT"].coords["latitude_u"].values
    lon_vertex = dsM1["VT"].coords["longitude_v"].values
    
    z_vertex = dsM1["UT"].coords["level"].values
    level = np.searchsorted(z_vertex, 1000) - 1
    #lat_vertexx=lat_vertex-0.5*(lat_vertex[10,:]-lat_vertex[9,:])
    
    u_vertex=np.zeros((u.shape))# 
    u_vertex[:,:,0] =  u[:,:, 1] 
    u_vertex[:,:,  0:-1] = (u[:,:, :-1] + np.roll(u[:,:,:],-1,axis=1)[:,:,:-1])/ 2
    u_vertex[:,:,-1]= (u[:,:, -2] + u[:,:, -1]) / 2

    v_vertex=np.zeros((v.shape))
    v_vertex[:,0,:]=v[:,0,:]
    v_vertex[:,:-1, :] = (v[:,:-1, :] +np.roll(v[:,:,:],-1,axis=2)[:,:-1,:])/ 2
    v_vertex[:,-1,:]= (v[:,-2,:] +v[:,-1,:]) / 2
    
    w_vertex=np.zeros((w.shape))# 
    w_vertex[:,:,0] =  w[:,:, 1] 
    w_vertex[:,:,  0:-1] = (w[:,:, :-1] + np.roll(w[:,:,:],-1,axis=1)[:,:,:-1])/ 2
    w_vertex[:,:,-1]= (w[:,:, -2] + w[:,:, -1]) / 2
    
    w_vertex[:,0,:]=w[:,0,:]
    w_vertex[:,:-1, :] = (w[:,:-1, :] +np.roll(w[:,:,:],-1,axis=2)[:,:-1,:])/ 2
    w_vertex[:,-1,:]= (w[:,-2,:] +w[:,-1,:]) / 2
    
    u_vertex_surf=u_vertex[0,:,:]
    v_vertex_surf=v_vertex[0,:,:]
    w_vertex_surf=w_vertex[0,:,:]
    # -----------------------------------------------------------------------------
    #                          Plume Bottom
    # -----------------------------------------------------------------------------
    # 
    #------------------------------------------------------------------------------


    # smoke=smoke*(.15/600)
    aqgOMS=3*10**(-8)
    thr=aqgOMS
    #create a 3D altitude array for later use
    altitude_1D_array = z_vertex+np.absolute(np.min(z_vertex))
    altitude_3D_array = np.tile(altitude_1D_array[:, np.newaxis, np.newaxis], (1, smoke.shape[1], smoke.shape[2]))    
    z_plumebottom=np.where(smoke>=thr,altitude_3D_array,np.max(altitude_3D_array))
    z_plumebottom_flat = np.min(z_plumebottom, axis=0) 
    z_plumebottom_flat = np.where(z_plumebottom_flat==np.max(altitude_3D_array),0.0,z_plumebottom_flat )

    #add smoke above thr at the ground
    smokeground=smoke[0,:,:]
    smokeground_thr=np.where(smokeground>=thr,np.min(altitude_3D_array)+0.1,0.0)

    z_plumebottom_flat=z_plumebottom_flat+smokeground_thr

    # -----------------------------------------------------------------------------
    #                          Plume Top
    # -----------------------------------------------------------------------------

    
    #   Store altitude corresponding to plume top
    z_plumetop=np.where(smoke>=thr,altitude_3D_array,np.min(altitude_3D_array))
    z_plumetop_flat = np.max(z_plumetop, axis=0)    
    #
 
    #------------------------------------------------------------------------------
    # Altitude
    #------------------------------------------------------------------------------
    ZSM1 = dsM1["ZS"].data
    # -----------------------------------------------------------------------------
    #----------------------------  Fire Generalities   ----------------------------
    if namefire==None:
        namefire = filenameM1.split('/')[-1].split('.')[0]+"___"+first_day_date
    name_da =xr.DataArray(namefire, name="WildfireName")  
    #------------------------------------------------------------------------------
    # altitude_plume_top_WT_list.append(z_plumetop_flat)
    #   Store altitude corresponding to plume bottom
    zplumebottom_da = xr.DataArray(z_plumebottom_flat,                    
                        dims=["latM1", "lonM1"],
                        coords={
                            "latitudeM1": (["latM1", "lonM1"], lat_center_M1),  # Ensure correct dims here
                            "longitudeM1": (["latM1", "lonM1"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="Z_FirePlumeBottom")
    
    #   Store altitude corresponding to plume top
    zplumetop_da = xr.DataArray(z_plumetop_flat,                    
                        dims=["latM1", "lonM1"],
                        coords={
                            "latitudeM1": (["latM1", "lonM1"], lat_center_M1),  # Ensure correct dims here
                            "longitudeM1": (["latM1", "lonM1"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="Z_FirePlumeTop")
    # --- Compute magnitude ---
    wind_speed = np.sqrt(u_vertex_surf**2 + v_vertex_surf**2)
    
    # --- Compute direction (meteorological convention: degrees from north) ---
    # arctan2 returns radians counterclockwise from east, so adjust:
    wind_dir_rad = np.arctan2(u_vertex_surf, v_vertex_surf)  # arctan(U, V)
    wind_dir_deg = (np.degrees(wind_dir_rad) + 360) % 360  # Convert to degrees from north
    
    # --- Create xarray DataArrays ---

    
    wind_speed_da = xr.DataArray(
        wind_speed,
        dims=["latM1", "lonM1"],
        coords={
            "latitudeM1": (["latM1", "lonM1"], lat_center_M1),  # Ensure correct dims here
            "longitudeM1": (["latM1", "lonM1"], lon_center_M1)  # Ensure correct dims here
            },
        #dims=["latitude", "longitude"],
        name="WindSpeedM1",
        attrs={"units": "m/s", "description": "Wind speed magnitude at surface"}
    )
    
    wind_dir_da = xr.DataArray(
        wind_dir_deg,
        dims=["latM1", "lonM1"],
        coords={
            "latitudeM1": (["latM1", "lonM1"], lat_center_M1),  # Ensure correct dims here
            "longitudeM1": (["latM1", "lonM1"], lon_center_M1)  # Ensure correct dims here
            },
        #dims=["latitude", "longitude"],
        name="WindDirectionM1",
        attrs={"units": "degrees", "description": "Wind direction (from north, meteorological convention)"}
    )
    altM1_da = xr.DataArray(ZSM1,                    
                         dims=["latM1", "lonM1"],
                        coords={
                            "latitudeM1": (["latM1", "lonM1"], lat_center_M1),  # Ensure correct dims here
                            "longitudeM1": (["latM1", "lonM1"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="ZSM1")
    #------------------------------------------------------------------------------
    dsnewM1 = xr.Dataset({
            "WildfireName": name_da,
            "Time": datetime_str,
            "Z_FirePlumeBottom": zplumebottom_da,
            "Z_FirePlumeTop": zplumetop_da,
            "WindSpeedM1":wind_speed_da,
            "WindDirectionM1":wind_dir_da,
            "ZSM1":altM1_da
    })
    #------------------------------------------------------------------------------
    # MODEL 1 attributes
    
    dsnewM1["Z_FirePlumeBottom"].attrs["units"] = "m"
    dsnewM1["Z_FirePlumeBottom"].attrs["comment"] = "altitude of the lower boundary of the fireplume in meters"
    
    dsnewM1["Z_FirePlumeTop"].attrs["units"] = "m"
    dsnewM1["Z_FirePlumeTop"].attrs["comment"] = "altitude of the upper boundary of the fireplume in meters"
    
    dsnewM1["WindSpeedM1"].attrs["units"] = "ms-1"
    dsnewM1["WindSpeedM1"].attrs["comment"] = "magnitude of the surface wind speed"
    
    dsnewM1["WindDirectionM1"].attrs["units"] = "degrees from north"
    dsnewM1["WindDirectionM1"].attrs["comment"] = "direction of the surface wind speed "  
    
    dsnewM1["latitudeM1"].attrs["units"] = "degrees_north"
    dsnewM1["latitudeM1"].attrs["comment"] = "latitudes of the 800 resolution, large scale domain"
    dsnewM1["longitudeM1"].attrs["units"] = "degrees_east"
    dsnewM1["longitudeM1"].attrs["comment"] = "longitudes of the 800 resolution, large scale domain"
    
    dsnewM1["ZSM1"].attrs["standard_name"] = "surface_altitude"
    dsnewM1["ZSM1"].attrs["units"] = "m"
    dsnewM1["ZSM1"].attrs["comment"] = "surface altitude"
    
    dsM1.close()
    return dsnewM1

def BMAP_to_DataArray(datetime_str,dateFirstDay,smoke_scalefactor,filenameM2,fileBmap,ffsettings,namefire=None):
    """
    Opens a ForeFire TimeofArrival output file and stores the variables needed for plotting in an xarray 
    2D fields:
        ArrivalTime[lat_bmap,lon_bmap]
        Ignition[lat_bmap,lon_bmap]
    1D fields:
        FrontVelocity[coord_points]
        FrontCoords[coord_points]
    scalars:
        WildfireName
        Time
    Parameters
    ----------
    datetime_str: date string from MNH timestep (in the ISO 8601 extended format >> 2017-06-17T17:00:00.000000000)
    dateFirstDay: daystring of ignition as a timestamp 
    smoke_scalefactor: 
    fileBmap:
    ffsettings: path to ForeFire params file
    namefire : name of wildfire case
    savepath : str, path for output file 
    nameout : str, name for output .nc fle
    Returns
    -------
    a DataArray with the varaibles to be plotted
    """

    # -----------------------------------------------------------------------------
    #                          Fire Fronts, Time of arrival 
    # -----------------------------------------------------------------------------
    #  They have finer resolution, following the bmap
    #------------------------------------------------------------------------------
    first_day=dateFirstDay.day
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")
    # Extract the hour, minute, and second
    day = datetime_obj.day
    hour =  datetime_obj.hour
    minute = datetime_obj.minute
    second = datetime_obj.second
    # Convert to seconds since midnight
    seconds_since_midnight = 24*3600*(day-first_day)+hour * 3600 + minute * 60 + second
    ##-----------------------------------------------------------------------------
    #Find the perimeter resolution:
    #ffsettings=casepath+paramsfile    
    pattern_per = 'minimalPropagativeFrontDepth='
    perimeterres = find_first_integer_after_pattern(ffsettings, pattern_per)
    perimeterres = int(perimeterres)
    pattern_burning_pow="nominalHeatFlux="
    burning_pow=find_first_integer_after_pattern(ffsettings, pattern_burning_pow)
    burning_pow=int(burning_pow)
    pattern_burning_duration="burningDuration="
    burning_time=find_first_integer_after_pattern(ffsettings, pattern_burning_duration)
    burning_time=int(burning_time)
    ##-----------------------------------------------------------------------------
    dsbmap=xr.load_dataset(fileBmap,engine="netcdf4")

    ta=dsbmap["arrival_time_of_front"].values
    masked_ta = np.where(ta>seconds_since_midnight,np.min(ta),ta)
    # timearr_WT_list.append(masked_ta)
    # Define the new high-resolution grid dimensions
    new_shape = ta.shape
    ###Takes the lat lon coords from M2 mnh file
    ds=xr.load_dataset(filenameM2,engine="netcdf4")
    lat_vertex = ds["UT"].coords["latitude_u"].values
    lon_vertex = ds["VT"].coords["longitude_v"].values
    lat_high_res = np.linspace(lat_vertex.min(), lat_vertex.max(), new_shape[0])
    lon_high_res = np.linspace(lon_vertex.min(), lon_vertex.max(), new_shape[1])

    # Create a meshgrid for the high-resolution grid
    lon_high_res_grid, lat_high_res_grid = np.meshgrid(lon_high_res, lat_high_res)
    # Flatten the low resolution coordinates for interpolation
    points = np.column_stack((lat_vertex.ravel(), lon_vertex.ravel()))
    values_lat = lat_vertex.ravel()
    values_lon = lon_vertex.ravel()
    
    # Interpolate the latitude and longitude to the high-resolution grid
    lat_high_res_interp = griddata(points, values_lat, (lat_high_res_grid, lon_high_res_grid), method='linear')
    lon_high_res_interp = griddata(points, values_lon, (lat_high_res_grid, lon_high_res_grid), method='linear')
    # Fill NaNs with nearest neighbor interpolation
    nan_mask = np.isnan(lat_high_res_interp)

    lat_high_res_interp[nan_mask] = griddata(points, values_lat, (lat_high_res_grid, lon_high_res_grid),
                                             method='nearest')[nan_mask]
    
    nan_mask = np.isnan(lon_high_res_interp)
    
    lon_high_res_interp[nan_mask] = griddata(points, values_lon, (lat_high_res_grid, lon_high_res_grid),
                                             method='nearest')[nan_mask]
    # -----------------------------------------------------------------------------
    #                          Fire Front Velocity, m/s
    # -----------------------------------------------------------------------------
    #  They have finer resolution, following the bmap
    #------------------------------------------------------------------------------
    
    # Calculate the gradient of the entire array
    max_speed_filter=1.0 # Filter for max velocity
    # ta_all = np.where(ta<=0.0, -9999,ta)
    ta_all=ta 
    #
    #######################################################################
    
    gradient_y, gradient_x = np.gradient(ta_all,perimeterres,perimeterres)
    sqrdiv=np.sqrt(np.power(gradient_y,2)+np.power(gradient_x,2))
    sqrdiv=np.where(ta==np.min(ta),0.0,sqrdiv)
    ##This is useful to eliminate the "final" boundary which is very high because of the discontinuity in ta
    sqrdiv=np.where(sqrdiv>=100*burning_time,0.0,sqrdiv)
    #10 is the perimeter resolution!
    # fire_velocity=np.divide(perimeterres,sqrdiv)
    # fire_velocity=np.divide(1,sqrdiv)
    with np.errstate(divide='ignore', invalid='ignore'):
        fire_velocity = np.divide(1, sqrdiv)
    
    fire_velocity[np.isinf(fire_velocity)] = 0.0
    
    fire_velocity=np.where(fire_velocity==np.nan,0.0,fire_velocity)
    fire_velocity=np.where(fire_velocity==np.inf,0.0,fire_velocity)
    
    fire_velocity=np.where(fire_velocity>=max_speed_filter,0.0,fire_velocity)
            
    

    fire_velocity[fire_velocity > max_speed_filter] = max_speed_filter

    #fire_velocity=np.ma.masked_not_equal(fire_velocity,1)
    #find the  corrisponding hour to properly trim the bmap
    firefront =np.ma.masked_greater(ta, seconds_since_midnight)

    firefront =np.ma.masked_less_equal(firefront, 0.0)
    # Find the contours of the masked region
    binary_mask = ~firefront.mask  # Convert masked array to binary mask
    labeled_array, num_features = ndimage.label(binary_mask)  # Label connected regions
    slices = ndimage.find_objects(labeled_array)  # Find the bounding box

    normal_vectors = []
    normal_magnitudes = []
    lat_boundary = []
    lon_boundary = []
    
    ignition = np.ma.masked_greater(ta_all, np.unique(ta_all)[2])
    ignition = np.ma.masked_less_equal(ignition, 0.0)
    # if the fire has already started
    try:
        if slices:
            largest_region = slices[0]
        else:

            binary_mask = ~ignition.mask
            labeled_array, num_features = ndimage.label(binary_mask)
            slices = ndimage.find_objects(labeled_array)
            if slices:
                largest_region = slices[0]
            else:
                raise ValueError("No valid region found.")

        boundary_mask = np.zeros_like(binary_mask)
        boundary_mask[largest_region] = binary_mask[largest_region]
        boundary = ndimage.binary_dilation(boundary_mask) ^ boundary_mask
        boundary_coords = np.argwhere(boundary)

        for coord in boundary_coords:
            y, x = coord
            normal_y, normal_x = compute_normal(gradient_y[y, x], gradient_x[y, x])
            normal_vectors.append((normal_y, normal_x))
            normal_magnitudes.append(fire_velocity[y, x])
            lat_boundary.append(lat_high_res_interp[y, x])
            lon_boundary.append(lon_high_res_interp[y, x])

    except Exception as e:
        print(f"[WARNING] Could not compute fire front boundary: {e}, fronts and time of arrival matrix for time {datetime_str} will be empty!")

    lat_boundary = np.array(lat_boundary)
    lon_boundary = np.array(lon_boundary)
    normal_vectors = np.array(normal_vectors)
    normal_magnitudes = np.array(normal_magnitudes)

    n_points = len(lat_boundary)
    lat_data_array = np.full((n_points), np.nan)
    lon_data_array = np.full((n_points), np.nan)
    coords_points_ROS = np.full((n_points, 2), np.nan)
    magnitude_array = np.full((n_points), np.nan)

    if n_points > 0:
        lat_data_array[:] = lat_boundary
        lon_data_array[:] = lon_boundary
        coords_points_ROS[:, 0] = lat_boundary
        coords_points_ROS[:, 1] = lon_boundary
        magnitude_array[:] = normal_magnitudes
    
    # Create the DataArray for the points of the fire front
    ta_da = xr.DataArray(
        masked_ta,  # Replace with your high-resolution data array
        dims=['y', 'x'],
        coords={'lat_bmap': (['y', 'x'], lat_high_res_interp), 'lon_bmap': (['y', 'x'], lon_high_res_interp)},
        name="ArrivalTime"
    )
    
    coords_points_da = xr.DataArray(
        coords_points_ROS,
        dims=['points', 'coord_points'],
        coords={'points': range(n_points), 'coord_points': ['lat_points', 'lon_points']}
    )
    
    # Create the DataArray for magnitudes
    magnitudes_da = xr.DataArray(
        magnitude_array,
        dims=[ 'points'],
        coords={'points': range(n_points)}
    )
    # store velocities of the fire front
    front_velocity_da = xr.DataArray(
        normal_magnitudes,  
        dims=['n_points_coords'],
        coords={'lat_point': (['n_points_coords'],lat_boundary),'lon_point': (['n_points_coords'],lon_boundary)},
        name="FrontVelocity"
    )
    ignition_da = xr.DataArray(  ignition,  # Replace with your high-resolution data array
        dims=['y', 'x'],
        coords={'lat_bmap': (['y', 'x'], lat_high_res_interp), 'lon_bmap': (['y', 'x'], lon_high_res_interp)},
        name="Ignition")
    dsnew = xr.Dataset({
        "ArrivalTime": ta_da,
        "FrontVelocity": magnitudes_da,
        "FrontCoords": coords_points_da,
        "Ignition":ignition_da})
    dsnew["ArrivalTime"].attrs["units"] = "seconds from midnight"
    dsnew["ArrivalTime"].attrs["comment"] = "time of arrival of the fire fronts, measured in seconds from midnight of the date of ignition"
    
    dsnew["FrontVelocity"].attrs["units"] = "m s-1"
    dsnew["FrontVelocity"].attrs["comment"] = "Velocity of each point of the firefront "
    dsnew["lat_bmap"].attrs["units"] = "degrees_north"
    dsnew["lat_bmap"].attrs["comment"] = "latitudes of the high resolution (10 m) fire propagation model"
    dsnew["lon_bmap"].attrs["units"] = "degrees_east"
    dsnew["lon_bmap"].attrs["comment"] = "longitudes of the high resolution (10 m) fire propagation model"
    dsnew["coord_points"].attrs["units"] = "degrees east, degrees_north"
    dsnew["coord_points"].attrs["comment"] = "longitudes and latitudes of fire front (10 m resolution) points"
    return dsnew

def BuildDSfromAllSources(pathMNH1,pathMNH2,pathBMAP,pathffParams,smoke_scalefactor,ignitiondate,namefire):
    """
    

    Parameters
    ----------
    pathMNH1 : path to directory with MNH M1 .nc output files
    pathMNH2 : path to directory with MNH M2 .nc output files
    pathBMAP : path to ForeFire BMap .nc output file
    pathffParams : path to ForeFire Params.ff file
    smoke_scalefactor : float
    ignitiondate: str, Day at which fire start in timestamp
    Returns
    -------
    A merged dataset for one timestep

    """
    dsM2=MODEL2_to_DataArray(smoke_scalefactor,pathMNH2,ignitiondate,namefire)
    dsM1=MODEL1_to_DataArray(smoke_scalefactor,pathMNH1,ignitiondate,namefire)
    dateTimestep=dsM2["Time"].values
    print(dateTimestep)
    dateFirstDay=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
    # firstDay=dateFirstDay.day
    dsBMAp=BMAP_to_DataArray(dateTimestep,dateFirstDay,smoke_scalefactor,
                      pathMNH2,pathBMAP,
                      pathffParams,namefire=None)
    merged_ds = xr.merge([dsM2, dsM1, dsBMAp])
    return merged_ds

def ExtractBAandROSeveryNmin(N,ignitiondate,enddate,fileBmap,ffsettings):
    time_range = pd.date_range(start=ignitiondate, end=enddate, freq=f'{N}min')
    first_day=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S").day
    # Loop through the generated timestamps
    maxRos=[]
    BurnedA=[]
    timestepsN=[]
    for timestamp in time_range:
        day = timestamp.day
        hour =  timestamp.hour
        minute = timestamp.minute
        second = timestamp.second
        # Convert to seconds since midnight
        seconds_since_midnight = 24*3600*(day-first_day)+hour * 3600 + minute * 60 + second
        # print(f'firstday: {first_day},  day : {day}  and secs : {seconds_since_midnight}')
        pattern_per = 'minimalPropagativeFrontDepth='
        perimeterres = find_first_integer_after_pattern(ffsettings, pattern_per)
        perimeterres = int(perimeterres)
        pattern_burning_pow="nominalHeatFlux="
        burning_pow=find_first_integer_after_pattern(ffsettings, pattern_burning_pow)
        burning_pow=int(burning_pow)
        pattern_burning_duration="burningDuration="
        burning_time=find_first_integer_after_pattern(ffsettings, pattern_burning_duration)
        burning_time=int(burning_time)
        ##-----------------------------------------------------------------------------
        dsbmap=xr.load_dataset(fileBmap,engine="netcdf4")
        
        ta=dsbmap["arrival_time_of_front"].values
        masked_ta = np.where(ta>seconds_since_midnight,np.min(ta),ta)
        # Determine the minimum value (mask reference)
        mask_value = np.min(ta)
        tasort=np.sort(np.unique(ta))
        # Count valid (unmasked) pixels
        valid_pixel_count = np.sum(masked_ta != mask_value)
        
        # Compute total area
        total_area = valid_pixel_count * perimeterres**2

        # Calculate the gradient of the entire array
        max_speed_filter=1.0 # Filter for max velocity
        # ta_all = np.where(ta<=0.0, -9999,ta)
        ta_all=ta 
        #
        #######################################################################
        
        gradient_y, gradient_x = np.gradient(ta_all,perimeterres,perimeterres)
        sqrdiv=np.sqrt(np.power(gradient_y,2)+np.power(gradient_x,2))
        sqrdiv=np.where(ta==np.min(ta),0.0,sqrdiv)
        ##This is useful to eliminate the "final" boundary which is very high because of the discontinuity in ta
        sqrdiv=np.where(sqrdiv>=100*burning_time,0.0,sqrdiv)
        #10 is the perimeter resolution!
        # fire_velocity=np.divide(perimeterres,sqrdiv)
        # fire_velocity=np.divide(1,sqrdiv)
        with np.errstate(divide='ignore', invalid='ignore'):
            fire_velocity = np.divide(1, sqrdiv)
        
        fire_velocity[np.isinf(fire_velocity)] = 0.0
        
        fire_velocity=np.where(fire_velocity==np.nan,0.0,fire_velocity)
        fire_velocity=np.where(fire_velocity==np.inf,0.0,fire_velocity)
        fire_velocity=np.where(fire_velocity>=max_speed_filter,0.0,fire_velocity)

        fire_velocity[fire_velocity > max_speed_filter] = max_speed_filter

        #find the  corrisponding hour to properly trim the bmap
        firefront =np.ma.masked_greater(ta, seconds_since_midnight)

        firefront =np.ma.masked_less_equal(firefront, 0.0)
        
        # Find the contours of the masked region
        binary_mask = ~firefront.mask  # Convert masked array to binary mask
        labeled_array, num_features = ndimage.label(binary_mask)  # Label connected regions
        slices = ndimage.find_objects(labeled_array)  # Find the bounding box

        normal_vectors = []
        normal_magnitudes = []
        
        ignition =np.ma.masked_greater(ta_all, np.unique(ta_all)[2])
        ignition =np.ma.masked_less_equal(ignition, 0.0)
        
        # if the fire has already started
        try:
            if slices:
                largest_region = slices[0]
            else:
                binary_mask = ~ignition.mask
                labeled_array, num_features = ndimage.label(binary_mask)
                slices = ndimage.find_objects(labeled_array)
                if slices:
                    largest_region = slices[0]
                else:
                    raise ValueError("No valid region found.")

            boundary_mask = np.zeros_like(binary_mask)
            boundary_mask[largest_region] = binary_mask[largest_region]
            boundary = ndimage.binary_dilation(boundary_mask) ^ boundary_mask
            boundary_coords = np.argwhere(boundary)

            for coord in boundary_coords:
                y, x = coord
                normal_y, normal_x = compute_normal(gradient_y[y, x], gradient_x[y, x])
                normal_magnitudes.append(fire_velocity[y, x])

            if len(normal_magnitudes) >= 10:
                top_10_magnitudes = sorted(normal_magnitudes, reverse=True)[:10]
                mean_top_10 = np.mean(top_10_magnitudes)
            elif len(normal_magnitudes) > 0:
                mean_top_10 = np.mean(normal_magnitudes)
            else:
                mean_top_10 = 0.0

        except Exception as e:
            print(f"[WARNING] timestep: {timestamp} Could not compute fire front boundary: {e}, ROS will be 0!")
            mean_top_10 = 0.0
            total_area = 0.0

        BurnedA.append(total_area)
        maxRos.append(mean_top_10)
        timestepsN.append(timestamp)

    return BurnedA, maxRos, timestepsN
    
    
def BuildDS4AllTimes(timesteps,casepath,mnfiles,pathBMAP,pathffParams,smoke_scalefactor,ignitiondate,namefire):
    """

    Parameters
    ----------
    timesteps : a list of int, corresponding to timestesps
    casepath : path to directory withMesoNH/ForeFire simulations
    mnfiles : path to directory with MNH M2 and M1 .nc output files
    pathBMAP : path to ForeFire BMap .nc output file
    pathffParams : path to ForeFire Params.ff file
    smoke_scalefactor : float
    ignitiondate: str, Day at which fire start in format timestamp
    Returns
    -------
    A merged dataset

    """
    datasets = []
    expanded_datasets = []

    for timestep in timesteps:
        mnhFilenameM2=Tstep2MNHnc(timestep+1,casepath,mnfiles,2)
        mnhFilenameM1=Tstep2MNHnc(timestep+1,casepath,mnfiles,1)
        print(f"timestep:{timestep} MNH M1 filename: {mnhFilenameM1},   MNH M2 filename: {mnhFilenameM2}")
        #smoke_scalefactor=findSmokeScaleFactor(pathffParams)
        pathMNH1=os.path.join(casepath, mnfiles, mnhFilenameM1)
        pathMNH2=os.path.join(casepath, mnfiles, mnhFilenameM2)
        ds=BuildDSfromAllSources(pathMNH1,pathMNH2,pathBMAP,pathffParams,
                              smoke_scalefactor,
                              ignitiondate,
                              namefire)
        datasets.append(ds)

    max_points = max(ds.FrontVelocity.sizes['points'] for ds in datasets)
    for i, ds in enumerate(datasets):
        # First, assign a standard coordinate for the points dimension if not already present
        ds = ds.assign_coords(points=np.arange(ds.FrontVelocity.sizes['points']))
        # Reindex the points dimension to the common range [0, max_points)
        ds = ds.reindex(points=np.arange(max_points), fill_value=np.nan)
        
        # If FrontCoords also uses a "points" dimension, apply the same patch:
        ds = ds.assign_coords(points=np.arange(ds.FrontCoords.sizes['points']))
        ds = ds.reindex(points=np.arange(max_points), fill_value=np.nan)
        
        datasets[i] = ds
        
    expanded_datasets = []
    listnoExp = ['WildfireName','ZSM1']
    enddate=datasets[-1]["Time"].values#datetime.strptime(datasets[-1]["Time"].values, "%Y-%m-%dT%H:%M:%S")
    BArea,maxRos,Time_Nmin_timesteps=ExtractBAandROSeveryNmin(6,ignitiondate,enddate,pathBMAP,pathffParams)
    maxPlumeHeight=[]
    for ds in datasets:
        time_val = ds.Time.values  # extract the scalar time value
        new_data_vars = {}
        # maxRos.append(np.nanmax(ds['FrontVelocity'].values))
        maxPlumeHeight.append(np.nanmax(ds['Z_FirePlumeTop'].values))
        # Loop over all data variables
        for var in ds.data_vars:
            # if var in listnoExp:
            #     # Leave these variables unchanged
            #     new_data_vars[var] = ds[var]
            if var not in listnoExp:
                # Expand these variables along a new "timestep" dimension
                new_data_vars[var] = ds[var].expand_dims(dim="timestep")
        
        # Create a new Dataset with the new data variables and preserve existing coordinates
        new_ds = xr.Dataset(data_vars=new_data_vars, coords=ds.coords)
        # Assign the new timestep coordinate to the dataset
        new_ds = new_ds.assign_coords(timestep=[time_val])
        
        expanded_datasets.append(new_ds)
    
    combined_ds = xr.concat(expanded_datasets, dim="timestep")
    combined_ds['WildfireName']=datasets[0]['WildfireName']
    combined_ds['Ignition']=datasets[0]['Ignition']
    # combined_ds['IgnitionTime']=np.datetime64(datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S").isoformat())
    combined_ds['IgnitionTime'] = np.datetime64(ignitiondate, 'ns')

    ignitionlat=np.mean(combined_ds.Ignition.coords["lat_bmap"].values[~np.isnan(combined_ds.Ignition.values)])
    ignitionlon=np.mean(combined_ds.Ignition.coords["lon_bmap"].values[~np.isnan(combined_ds.Ignition.values)])
    combined_ds['IgnitionCoords']=[ignitionlat,ignitionlon]
    combined_ds["IgnitionCoords"].attrs["units"] = "degrees_north, degrees_east"
    
    combined_ds['ZSM1']=datasets[0]['ZSM1']
    
    max_ROS_da = xr.DataArray(
        maxRos,  
        dims=['extended_timesteps'],
        coords={
            "extended_timesteps":Time_Nmin_timesteps},
        name="max_ROS"
    )
    BurnedArea_da = xr.DataArray(
        BArea,  
        dims=['extended_timesteps'],
        coords={
            "extended_timesteps":Time_Nmin_timesteps},
        name="BurnedArea"
    )
    max_PlumeH_da = xr.DataArray(
        maxPlumeHeight,  
        dims=['timestep'],
        coords={
            "timestep":combined_ds.Wind_U.coords["timestep"].values},
        name="max_PlumeHeight"
    )
    combined_ds['max_ROS'] = max_ROS_da
    combined_ds['BurnedArea'] = BurnedArea_da
    combined_ds['max_PlumeHeight'] = max_PlumeH_da
    return combined_ds



###############################################################################
# PLOTTING ROUTINES
###############################################################################
def compute_normal(gy, gx):
    norm = np.sqrt(gy**2 + gx**2)
    return gy / norm, -gx / norm

def compute_edges_from_centers(centers):
    edges = 0.5 * (centers[1:] + centers[:-1])
    first_edge = centers[0] - (edges[0] - centers[0])
    last_edge = centers[-1] + (centers[-1] - edges[-1])
    return np.concatenate([[first_edge], edges, [last_edge]])
#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#                                   Plot n1
#------------------------------------------------------------------------------
    # 1) Surface wind field plotted with arrows
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Contours of turbulent energy above 1.5 m2 s-2 above 1.5 m2s-2 in the first ~1000 m above the ground
    # 5) Upward_air_velocity above 3 m s-1 in the first ~1000 m above the ground displayed with crosses in colorcode 
#------------------------------------------------------------------------------
def Plot_surfaceWind_TKE(timestep,boundarypoints,dsfile,ignitiondate,fireenddate,fontitle=22,provider=ctx.providers.CartoDB.Voyager,savefig=True,savepath="",savename="TKE_Plot",dpi=200):
    """
    Routine to create .png plots displaying:
    
    # 1) Surface wind field plotted with arrows, colors and dimensions proportional to winfd magnitude
    # 2) Time evolution of the fire plotted with shadows of red
    # 3) Active points in the fire front (point color proportional to ROS)
    # 4) Contours of turbulent energy above 1.5 m2 s-2 above 1.5 m2s-2 in the first ~1000 m above the ground
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
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    # datetime_str_start=dsfile["Time"][0].values
    # datetime_obj_start = pd.to_datetime(datetime_str_start)
    datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
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
    plt.suptitle("Surface wind field, turbulence and fire front",fontsize=1.2*fontitle)
    plt.title(f"{datetime_str_form} UTC",fontsize=fontitle)
    
    #Set plot boundaries
    ax.axis([boundarypoints[0],boundarypoints[1],boundarypoints[2], boundarypoints[3]])
    ax.set_aspect('equal') 
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
    lon_fine = dsfile["ArrivalTime"].coords["lon_bmap"].values
    lat_fine = dsfile["ArrivalTime"].coords["lat_bmap"].values
# Flatten the 2D arrays for transformation
    lon_fine_flat = lon_fine.flatten()
    lat_fine_flat = lat_fine.flatten()
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
        firefront = np.ma.masked_less(dsfile["ArrivalTime"][timestep].values, 0.0)

        # # Use pcolormesh to display Time of Arrival intensity as a background
        
        # minvalue=np.amin(dsfile["Ignition"].values[~np.isnan(dsfile["Ignition"].values)])/3600        
        # min_colorscale=int(minvalue)
        # max_colorscale=int(24*(day_max-day_start)+hour_max)
        
        minvalue=datetime_obj_start.hour#np.amin(dsfile["Ignition"].values[~np.isnan(dsfile["Ignition"].values)])/3600    
        min_colorscale=int(minvalue)
        max_colorscale=int(24*(day_max-day_start)+hour_max)
        
        # hours=np.arange(min_colorscale,max_colorscale,2)
        hours=np.arange(0,max_colorscale-min_colorscale,1)

        cmapt = plt.get_cmap('gist_heat', max_colorscale-min_colorscale)
        
        lon_edges = compute_edges_from_centers(lon_web_mercator_fine)
        lat_edges = compute_edges_from_centers(lat_web_mercator_fine)

        atime = ax.pcolormesh(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,cmap=cmapt, alpha=0.5)
        atimec = ax.contour(lon_web_mercator_fine, lat_web_mercator_fine,firefront/3600 ,vmin=min_colorscale, vmax=max_colorscale ,levels=hours, linewidths=2,cmap=cmapt)
    
        ###set the colorbar
        cax3 = fig.add_subplot(gs[0, 2]) 
        # cbaatime=plt.colorbar(atime,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,1),fraction=0.015,aspect=30, pad=0.04)
        # cbaatime=plt.colorbar(atimec,cax=cax3, ticks=np.arange(min_colorscale,max_colorscale,1),fraction=0.015,aspect=30, pad=0.04)
        cbaatime=plt.colorbar(atime,cax=cax3, ticks=hours,fraction=0.015,aspect=30, pad=0.04)
        cbaatime=plt.colorbar(atimec,cax=cax3, ticks=hours,fraction=0.015,aspect=30, pad=0.04)
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
    scale_factor = 0.01  # Adjust this factor to control the arrow size
    u_scaled = u * scale_factor
    v_scaled = v * scale_factor
    
    # # Define a slice to skip drawing some of the quiver arrows to reduce clutter
    skip = (slice(None, None, 2), slice(None, None, 2))
    windmodule_cmap=windmodule[skip]
    # # Use the quiver function to display wind field at the ground with their direction and intensity
    norm=plt.Normalize(np.min(windmodule_cmap), np.max(windmodule_cmap))
    cmapp = plt.cm.jet#plt.cm.nipy_spectral_r  # Choose a colormap
    colors = cmapp(norm(windmodule_cmap))
    
    ax.quiver(lon_web_mercator[skip], lat_web_mercator[skip], u_scaled[skip], v_scaled[skip], color=colors.reshape(-1, 4), scale=1, width=0.003, headwidth=5)
    ax.quiver(lon_web_mercator[skip], lat_web_mercator[skip], u_scaled[skip], v_scaled[skip], color="Black", scale=1, width=0.001, headwidth=0,alpha=0.8)

    # Add the colorbar
    sm = plt.cm.ScalarMappable(cmap=cmapp, norm=norm)
    sm.set_array([])
    cbarwf = plt.colorbar(sm, ax=ax,fraction=0.025,aspect=40, pad=0.03,orientation='horizontal')
    cbarwf.set_label('Magnitude of surface wind (ms-1)',fontsize=0.9*fontitle)
    
    # #------------------------------------------------------------------------------
    # #    TKE
    # #------------------------------------------------------------------------------
    cctke = ax.contourf(lon_web_mercator, lat_web_mercator,masked_tke2D,cmap="viridis",alpha=0.3)
    # Convert masked array to a regular array for contour detection
    masked_tke2Dfilled = masked_tke2D.filled(fill_value=0)
    # Find contours at a constant value of 0.5 (can be adjusted)
    tkecontour = ax.contour(lon_web_mercator, lat_web_mercator, masked_tke2Dfilled,levels=[0.5], linewidths=1,linestyles="dashed" ,cmap="viridis")
    # Add the colorbar
    cax2 = fig.add_subplot(gs[2, 1])
    cbartke = plt.colorbar(cctke,location="bottom", cax=cax2,fraction=0.015,aspect=30, pad=0.04)
    cbartke.set_label('Turbulent Energy m2s-2',fontsize=0.9*fontitle)
    
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
        plt.savefig(savepath+savename+".png", dpi=dpi)
        jpg_filename = savepath+savename+".jpg"
        image = Image.open(savepath+savename+".png")
        image = image.convert("RGB")  # Ensure no transparency
        image.save(jpg_filename, "JPEG", quality=20, optimize=True)
    plt.close()
    
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
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")


    datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
    if datetime_obj_start > datetime_obj:
        print(f"Fire has not started yet! ignition:{datetime_obj_start} timestep:{datetime_obj}")
        FlagFireStarted=False
    day_start = datetime_obj_start.day

    datetime_obj_max=fireenddate

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
    plt.suptitle("Surface wind field and smoke concentration, fire front",fontsize=1.2*fontitle)
    plt.title(f"{datetime_str_form} UTC",fontsize=fontitle)
    
    #Set plot boundaries
    ax.axis([boundarypoints[0],boundarypoints[1],boundarypoints[2], boundarypoints[3]])
    ax.set_aspect('equal') 
    ax.tick_params(axis='both', labelsize=0.8*fontitle)
    #------------------------------------------------------------------------------
    # Choose the map background(here with contextily)
    ctx.add_basemap(ax,source=provider)

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
    scale_factor = 0.05# 0.025  # Adjust this factor to control the arrow size
    with np.errstate(invalid='ignore', divide='ignore'):
        u_plot = scale_factor * np.divide(u, windmodule, out=np.zeros_like(u), where=windmodule != 0)
        v_plot = scale_factor * np.divide(v, windmodule, out=np.zeros_like(v), where=windmodule != 0)

    
    # # Define a slice to skip drawing some of the quiver arrows to reduce clutter
    skip = (slice(None, None, 2), slice(None, None, 2))
    windmodule_cmap=windmodule[skip]
    # # Use the quiver function to display wind field at the ground with their direction and intensity
    norm=plt.Normalize(np.min(windmodule_cmap), np.max(windmodule_cmap))
    cmapp = plt.cm.nipy_spectral_r  # Choose a colormap
    colors = cmapp(norm(windmodule_cmap))
    
    ax.quiver(lon_web_mercator[skip], lat_web_mercator[skip], u_plot[skip], v_plot[skip], color="Black", scale=1, width=0.003, headwidth=5)
    
    #------------------------------------------------------------------------------
    #   Smoke at the ground
    #------------------------------------------------------------------------------
    ## avoid spurious negative values
    smokegroundp= np.where(smoke <= 0, 1e-30,smoke)
    #threshold is the air quality criterion for PM2.5 approx 12$10-9 kg/kg 24 h average
    aqgOMS=3*10**(-8)

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
        cmapt = plt.get_cmap('gist_heat', max_colorscale-min_colorscale)
        hours=np.arange(0,max_colorscale-min_colorscale,1)
        
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
        plt.savefig(savepath+savename+".png", dpi=dpi)
        jpg_filename = savepath+savename+".jpg"
        image = Image.open(savepath+savename+".png")
        image = image.convert("RGB")  # Ensure no transparency
        image.save(jpg_filename, "JPEG", quality=20, optimize=True)
    plt.close()


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
    datetime_obj = pd.to_datetime(datetime_str)
    datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")

    datetime_obj_start=datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S")
    if datetime_obj_start > datetime_obj:
        print(f"Fire has not started yet! ignition:{datetime_obj_start} timestep:{datetime_obj}")
        FlagFireStarted=False
    
    if FlagFireStarted:
        day_start = datetime_obj_start.day

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

        transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)
    
    
        # Convert the coordinates to Web Mercator
    
        lon_Large_web_mercator, lat_Large_web_mercator = transformer.transform( lon, lat) #transform(proj_wgs84, proj_web_mercator, lon, lat)
        lon_web_mercator, lat_web_mercator = transformer.transform( lon_fine, lat_fine) #transform(proj_wgs84, proj_web_mercator, lon, lat)
        lon_web_mercator_points, lat_web_mercator_points =transformer.transform( lon_points, lat_points) # transform(proj_wgs84, proj_web_mercator, lon_points, lat_points)
    
        #------------------------------------------------------------------------------
        # Recover the Dates
        #------------------------------------------------------------------------------
        datetime_str=dsfile["Time"][timestep].values
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
        
        fig = plt.figure(figsize=(15,15),constrained_layout=True)
        ax = fig.add_subplot(111)
        
        Namefire=dsfile["WildfireName"].values
        plt.suptitle("Top of large scale smoke plume",fontsize=1.2*fontitle)
        ax.set_title(f"{datetime_str_form} UTC",fontsize=fontitle)

        #------------------------------------------------------------------------------
         #Recovering the Smoke at the ground
        #------------------------------------------------------------------------------
        
        smokeground = np.where(dsfile["Z_FirePlumeBottom"][timestep].values==0.1,dsfile["Z_FirePlumeBottom"][timestep].values,-1)
        #threshold is the air quality criterion for CO
        #------------------------------------------------------------------------------
        #                        Plot the top of the plume
        #------------------------------------------------------------------------------
        #Set subplot boundaries
        ax.axis([np.amin(lon_Large_web_mercator),np.amax(lon_Large_web_mercator),np.min(lat_Large_web_mercator),np.amax(lat_Large_web_mercator)])
        ctx.add_basemap(ax,source=provider)
        ax.tick_params(axis='both', labelsize=0.8*fontitle)

    
    
        z_plumetop_flat = dsfile["Z_FirePlumeTop"][timestep].values
    
        plumetop_flat_masked=np.ma.masked_equal(z_plumetop_flat ,np.min(z_plumetop_flat))
        vmin=0       
        vmax=np.amax(np.amax(plumetop_flat_masked))
        if len(np.unique(dsfile["Z_FirePlumeTop"]))>1:
            topcontourf = ax.contourf(lon_Large_web_mercator, lat_Large_web_mercator,plumetop_flat_masked,vmin=vmin,vmax=vmax,cmap="inferno",alpha=0.8)
            cbplumebot=plt.colorbar(topcontourf,ax=ax,fraction=0.025,aspect=30, pad=0.04)
            cbplumebot.set_label('Smoke altitude of upper fireplume m',fontsize=0.9*fontitle)
        if savefig:
            plt.savefig(savepath+savename+"TOP.png", dpi=dpi)
            jpg_filename = savepath+savename+"TOP.jpg"
            image = Image.open(savepath+savename+"TOP.png")
            image = image.convert("RGB")  # Ensure no transparency
            image.save(jpg_filename, "JPEG", quality=20, optimize=True)
        plt.close()
        ###############################################################################
        
        ###############################################################################
        #------------------------------------------------------------------------------
        # PLOT2: Set up plot style and title
        #------------------------------------------------------------------------------
        ###############################################################################
        
        fig1 = plt.figure(figsize=(15,15),constrained_layout=True)
        ax1 = fig1.add_subplot(111)
        plt.suptitle("Bottom of large scale smoke plume",fontsize=1.2*fontitle)
        ax1.set_title(f"{datetime_str_form} UTC",fontsize=fontitle)

        #------------------------------------------------------------------------------
        #                        Plot the bottom of the plume
        #------------------------------------------------------------------------------
        
        #Set subplot boundaries
        ax1.axis([np.amin(lon_Large_web_mercator),np.amax(lon_Large_web_mercator),np.min(lat_Large_web_mercator),np.amax(lat_Large_web_mercator)])
        ctx.add_basemap(ax1,source=provider)
        ax1.tick_params(axis='both', labelsize=0.8*fontitle)
        z_plumebottom_flat = dsfile["Z_FirePlumeBottom"][timestep].values
        plumebottom_flat_masked=np.ma.masked_equal(z_plumebottom_flat ,0)  
    
        if len(np.unique(dsfile["Z_FirePlumeBottom"]))>1:
            bottomcontourf = ax1.contourf(lon_Large_web_mercator, lat_Large_web_mercator,plumebottom_flat_masked,vmin=vmin,vmax=vmax,cmap="inferno",alpha=0.8)
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
        if savefig:
            plt.savefig(savepath+savename+"BOTTOM.png", dpi=dpi)
            jpg_filename = savepath+savename+"BOTTOM.jpg"
            image = Image.open(savepath+savename+"BOTTOM.png")
            image = image.convert("RGB")  # Ensure no transparency
            image.save(jpg_filename, "JPEG", quality=20, optimize=True)
        plt.close()
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
    plt.figure(figsize=(20,5))
    plt.plot(dsfile.BurnedArea.coords["extended_timesteps"].values,dsfile.BurnedArea.values/10000,"-o",color="Brown")
    plt.xlabel("UTC Time (h)",fontsize=0.8*fontitle)
    plt.ylabel("Burned area (ha)",fontsize=0.8*fontitle)
    plt.savefig(savepath+savename+"_1.png",dpi=dpi)
    jpg_filename = savepath+savename+"_1.jpg"
    image = Image.open(savepath+savename+"_1.png")
    image = image.convert("RGB")  # Ensure no transparency
    image.save(jpg_filename, "JPEG", quality=20, optimize=True)
    plt.close()
    plt.figure(figsize=(20,5))
    plt.plot(dsfile.max_ROS.coords["extended_timesteps"].values,dsfile.max_ROS.values,"-o",color="Red")
    plt.xlabel("UTC Time (h)",fontsize=0.8*fontitle)
    plt.ylabel("Max Rate Of Spread (ms-1)",fontsize=0.8*fontitle)
    plt.savefig(savepath+savename+"_2.png",dpi=dpi)
    jpg_filename = savepath+savename+"_2.jpg"
    image = Image.open(savepath+savename+"_2.png")
    image = image.convert("RGB")  # Ensure no transparency
    image.save(jpg_filename, "JPEG", quality=20, optimize=True)
    plt.close()
    plt.figure(figsize=(20,5))
    plt.plot(dsfile.max_PlumeHeight.coords["timestep"].values,dsfile.max_PlumeHeight.values,"-o",color="Grey")
    plt.xlabel("UTC Time (h)",fontsize=0.8*fontitle)
    plt.ylabel("Smoke plume altitude (m)",fontsize=0.8*fontitle)
    plt.savefig(savepath+savename+"_3.png",dpi=dpi)
    jpg_filename = savepath+savename+"_3.jpg"
    image = Image.open(savepath+savename+"_3.png")
    image = image.convert("RGB")  # Ensure no transparency
    image.save(jpg_filename, "JPEG", quality=20, optimize=True)
    plt.close()
    
def plot_location(lat, lon, boundarypoints,output_image="location",provider=ctx.providers.OpenStreetMap.Mapnik):
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
    ax.axis([boundarypoints[0],boundarypoints[1],boundarypoints[2], boundarypoints[3]])
    ax.scatter(x, y, color="red", marker="x", s=200, label="Ignition Point")  # Mark location with X

    ctx.add_basemap(ax, crs="EPSG:3857", source=provider)

    ax.legend(fontsize=22)
    
    # Save the figure
    plt.savefig(output_image, dpi=120, bbox_inches="tight")
    plt.close()
    
    # Step 2: Convert PNG to JPG with compression using Pillow
    jpg_filename = output_image+".jpg"
    image = Image.open(output_image+".png")
    image = image.convert("RGB")  # Ensure no transparency
    image.save(jpg_filename, "JPEG", quality=20, optimize=True)

    plt.close()

    return output_image
###############################################################################
# import cartopy.feature as cfeature  # if you want to add vector boundaries
def plot_meteorology(dsfile,boundarypoints, output_image1="windst0",output_image2="windst1",provider=ctx.providers.OpenStreetMap.Mapnik,fontitle=22,dpi=120):
    """
    Plots a map with a marker at the given lat/lon using contextily and saves the figure.

    Parameters:
        boundarypoints
        output_image (str): Filename for saving the output image
    """
    #------------------------------------------------------------------------------
    ## PLOTn1 
    #------------------------------------------------------------------------------
    timestep=0

    windmodule = dsfile["WindSpeedM1"][timestep,:,:].values
    winddirection = dsfile["WindDirectionM1"][timestep,:,:].values
    ZS=dsfile["ZSM1"][:,:].values
    # Create plot
    fig, ax = plt.subplots(figsize=(8, 6))
    # Calculate the module of the surface wind:
    
    # windmodule =windmodule.values
    
    # Convert direction + magnitude into u, v components (east/north)
    # First convert degrees from north (meteorological) to radians from east (math)
    theta = np.radians(90 - winddirection)  # convert to radians from east (standard for matplotlib quiver)
    u = windmodule * np.cos(theta)
    v = windmodule * np.sin(theta)

    # Coordinates necessary for plotting
    lon = dsfile["WindSpeedM1"].coords["longitudeM1"].values
    lat = dsfile["WindSpeedM1"].coords["latitudeM1"].values

    # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
    transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)

    # Convert the coordinates to Web Mercator
    lon_web_mercator, lat_web_mercator = transformer.transform(lon, lat)# transform(proj_wgs84, proj_web_mercator, lon, lat)

   #------------------------------------------------------------------------------
    #   Surface wind field 
    #------------------------------------------------------------------------------
    ax.axis([boundarypoints[0],boundarypoints[1],boundarypoints[2], boundarypoints[3]])
    c = ax.contourf(lon_web_mercator, lat_web_mercator, windmodule, levels=20, cmap=plt.cm.jet , alpha=0.3)
    # --- Add a colorbar based on the contourf ---
    cbar = plt.colorbar(c, ax=ax, orientation='vertical', pad=0.02)
    cbar.set_label('Wind Speed [ms-1]')  # Update units as needed
    # # Adjust the length and width of the arrows based on intensity
    scale_factor = 0.025  # Adjust this factor to control the arrow size
    with np.errstate(invalid='ignore', divide='ignore'):
        u_plot = scale_factor * np.divide(u, windmodule, out=np.zeros_like(u), where=windmodule != 0)
        v_plot = scale_factor * np.divide(v, windmodule, out=np.zeros_like(v), where=windmodule != 0)

    # --- Add wind arrows ---
    # # Define a slice to skip drawing some of the quiver arrows to reduce clutter
    skip = (slice(None, None, 5), slice(None, None, 5))
    windmodule_cmap=windmodule[skip]
    # # Use the quiver function to display wind field at the ground with their direction and intensity    
    ax.quiver(lon_web_mercator[skip], lat_web_mercator[skip], u_plot[skip], v_plot[skip], color="Black", scale=1, width=0.001, headwidth=3)
    ctx.add_basemap(ax, crs="EPSG:3857", source=provider)
    ###########################################################################
    #------------------------------------------------------------------------------
    #   Orography
    #------------------------------------------------------------------------------
    # Define contour levels (adjust as needed)
    levels = np.linspace( np.nanmin(ZS), np.nanmax(ZS), 5)
    # # Plot orography as grey contours with a default linewidth
    if np.nanmin(ZS) != np.nanmax(ZS):
        orog_contours = ax.contour(lon_web_mercator, lat_web_mercator, ZS, levels=levels, colors='Grey', linewidths=0.5)
    
        if hasattr(orog_contours, "collections") and orog_contours.collections:
            for lev, coll in zip(orog_contours.levels, orog_contours.collections):
                if np.isclose(lev, 0.0):
                    coll.set_linewidth(1.0)
        else:
            print("No contour lines were generated — valid levels but no isolines matched.")
    else:
        print("ZS is flat — no variation to contour.")

    # ax.imshow(ZS, extent=(lon_web_mercator.min(), lon_web_mercator.max(), 
    #                   lat_web_mercator.min(), lat_web_mercator.max()),
    #       origin='lower', cmap='Greys', alpha=0.8)    
    ###########################################################################
    # ax.legend()    
    # Save the figure
    plt.savefig(output_image1, dpi=dpi, bbox_inches="tight")
    jpg_filename = output_image1+".jpg"
    image = Image.open(output_image1+".png")
    image = image.convert("RGB")  # Ensure no transparency
    image.save(jpg_filename, "JPEG", quality=20, optimize=True)
    plt.close()
    print(f"Meteorology 1 saved as: {output_image1}")
    
    #------------------------------------------------------------------------------
    ## PLOTn2
    #------------------------------------------------------------------------------
    timesteps=dsfile["WindSpeedM1"].shape[0]
    avg_magnitudes = []
    avg_directions = []
            
    for timestep in np.arange(0,timesteps):# len(timesteps):
        windmodule = dsfile["WindSpeedM1"][timestep,:,:].values
        winddirection = dsfile["WindDirectionM1"][timestep,:,:].values
        ny, nx = windmodule.shape[-2], windmodule.shape[-1]
        center_y = ny // 2
        center_x = nx // 2
        
        # Select 2x2 central block
        windmodule_central = windmodule[center_y-1:center_y+1, center_x-1:center_x+1]
        winddirection_central = winddirection[center_y-1:center_y+1, center_x-1:center_x+1]

        # Compute magnitude and direction

        # Average over the 2x2 block
        avg_mag = windmodule_central.mean().item()
        avg_dir = winddirection_central.mean().item()

        avg_magnitudes.append(avg_mag)
        avg_directions.append(avg_dir)
        
    fig, ax1 = plt.subplots(figsize=(20, 5))    
    time=dsfile["Time"].values
    # Plot avg_magnitudes on the left y-axis
    ax1.plot(time, avg_magnitudes, '-o', color='red', label='Avg Magnitudes')
    ax1.set_xlabel('Time',fontsize=0.8*fontitle)
    ax1.set_ylabel('Forecasted Wind Magnitude [ms-1]',fontsize=0.7*fontitle)
    # ax1.tick_params(axis='y', labelcolor='red')
    
    # Create a second y-axis for avg_directions
    ax2 = ax1.twinx()
    ax2.plot(time, avg_directions, '--*', color='blue', label='Avg Directions')
    ax2.set_ylabel('Forecasted Wind Direction [rads]' ,fontsize=0.8*fontitle)
    # ax2.tick_params(axis='y',fontsize=0.8*fontitle)
    
    # Add legends
    ax1.legend(loc='upper left',fontsize=0.8*fontitle)
    ax2.legend(loc='upper right',fontsize=0.8*fontitle)
    plt.savefig(output_image2,dpi=dpi)
    jpg_filename2 = output_image2+".jpg"
    image = Image.open(output_image2+".png")
    image = image.convert("RGB")  # Ensure no transparency
    image.save(jpg_filename2, "JPEG", quality=20, optimize=True)
    plt.xticks(rotation=45)
    plt.close()
    print(f"Meteorology 1 saved as: {output_image2}")

    

###############################################################################

#------------------------------------------------------------------------------
#::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#------------------------------------------------------------------------------
print(f"**********************************************************************")
print(f"Postprocessing of MesoNH and ForeFire outputs for report.tex ")
print(f"**********************************************************************")
filenameM1="RUN12.2.FIRE.001.nc"
dsM1=xr.load_dataset(filenameM1,engine="netcdf4")
datetime_str=dsM1["time"].values[0]
print(f"date of first day: {datetime_str}")
ignition=get_ignition_datetime("ForeFire/Init.ff",datetime_str )
print(f"date of ignition: {ignition}")


print(f"datatetime:{datetime_str}   ignition:{ignition}")
if np.datetime64(ignition) < np.datetime64(datetime_str):
    raise ValueError(f"[ERROR] Ignition time is earlier than than initialization of MNH!!. PLOTTING STOPPED to avoid inconsistencies")
current_dir = os.getcwd()
# casepath=os.path.abspath(os.path.join(current_dir, "..", "005_runFIRE"))
mnfiles=""
# print(f"checking casepath {casepath}")
savepath=current_dir
pathBMAP=os.path.join(current_dir, "ForeFire", "Outputs","ForeFire.0.nc")#casepath+"/ForeFire/Outputs/ForeFire.0.nc"
pathffParams=os.path.join(current_dir, "ForeFire", "Params.ff")#casepath+"/ForeFire/Params.ff"

smoke_scalefactor=findSmokeScaleFactor(paramsfile=pathffParams)

list_nc=list_NetCDFfiles_in_directory(current_dir)
formatted_numbers = [s[-6:-3] for s in list_nc]
# # Sort the list of strings based on the extracted numbers
formatted_numbers = sorted(formatted_numbers, key=extract_number)
formatted_numbers =[int(s) for s in formatted_numbers]
timesteps=formatted_numbers[:-1] 
print(f"timesteps found :{timesteps}")

namefire=os.path.basename(current_dir)


combined_ds=BuildDS4AllTimes(timesteps,current_dir,mnfiles,pathBMAP,pathffParams,smoke_scalefactor,ignition,namefire)
namesave=os.path.join(current_dir, f"{namefire}.nc")
combined_ds.to_netcdf(f"{namefire}.nc", compute=True)


# combined_ds=xr.load_dataset(savepath+f"/{namefire}.nc")

# print(combined_ds.data_vars)

provider1=ctx.providers.CartoDB.Voyager
provider2=ctx.providers.CartoDB.Voyager
provider3=ctx.providers.CartoDB.Voyager
saveflag=True
savedirectory=current_dir + "/report/"#"/plots1D/"
savedirectory2=current_dir + "/report/"
# ###############################################################################
endtime= pd.to_datetime(combined_ds["Time"][-1].values)#.strftime("%Y-%m-%d %H:%M:%S")
lon_fine = combined_ds["Wind_U"].coords["longitude"].values
lat_fine = combined_ds["Wind_U"].coords["latitude"].values
transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)
###############################################################################
firefront_max = np.ma.masked_less(combined_ds["ArrivalTime"][-1].values, 0.0)

binary_mask_all = ~firefront_max.mask  # Convert masked array to binary mask
labeled_array, num_features = ndimage.label(binary_mask_all)  # Label connected regions
slices_all = ndimage.find_objects(labeled_array)  # Find the bounding box

lon_fine = combined_ds["ArrivalTime"].coords["lon_bmap"].values
lat_fine = combined_ds["ArrivalTime"].coords["lat_bmap"].values


lonM1 = combined_ds["WindDirectionM1"].coords["longitudeM1"].values
latM1 = combined_ds["WindDirectionM1"].coords["latitudeM1"].values
# lon_fine = combined_ds["Wind_U"].coords["longitude"].values
# lat_fine = combined_ds["Wind_U"].coords["latitude"].values


# # Define the projection: WGS84 (lat/lon) and Web Mercator (EPSG:3857)
# transformer = Transformer.from_crs("EPSG:4326", "EPSG:3857", always_xy=True)

# # Convert the coordinates to Web Mercato
# # Convert the coordinates to Web Mercator
# # lon_web_mercator_fine, lat_web_mercator_fine = transform(proj_wgs84, proj_web_mercator, lon_fine, lat_fine)

# #------------------------------------------------------------------------------
# #::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
factorlon=0.2 #sets up the distance of boundaries from the fire front
factorlat=0.2 #sets up the distance of boundaries from the fire front
lon_min = lon_fine[slices_all[0]].min() - factorlon*abs( lon_fine[slices_all[0]].max() -lon_fine[slices_all[0]].min())
lon_max = lon_fine[slices_all[0]].max() + factorlon*abs( lon_fine[slices_all[0]].max() -lon_fine[slices_all[0]].min())
lat_min = lat_fine[slices_all[0]].min() - factorlat*abs( lat_fine[slices_all[0]].max() -lat_fine[slices_all[0]].min())
lat_max = lat_fine[slices_all[0]].max() + factorlat*abs( lat_fine[slices_all[0]].max() -lat_fine[slices_all[0]].min())
###############################################################################
# lon_min=np.amin(lon_fine)
# lon_max=np.amax(lon_fine)
# lat_min=np.amin(lat_fine)
# lat_max=np.amax(lat_fine)
SW_boundary_point =  transformer.transform(lon_min, lat_min)
NW_boundary_point =  transformer.transform(lon_min, lat_max)
SE_boundary_point =  transformer.transform(lon_max, lat_min)
NE_boundary_point =  transformer.transform(lon_max, lat_max)

plotboundaries=[SW_boundary_point[0],SE_boundary_point[0],SW_boundary_point[1], NW_boundary_point[1]]

SW_boundary_pointM1 =  transformer.transform(np.amin(lonM1), np.amin(latM1))
NW_boundary_pointM1 =  transformer.transform(np.amin(lonM1), np.amax(latM1))
SE_boundary_pointM1 =  transformer.transform(np.amax(lonM1), np.amin(latM1))
NE_boundary_pointM1 =  transformer.transform(np.amax(lonM1), np.amax(latM1))
plotboundariesM1=[SW_boundary_pointM1[0],SE_boundary_pointM1[0],SW_boundary_pointM1[1], NW_boundary_pointM1[1]]

# ###############################################################################
print(f"**********************************************************************")
print(f"Creation of the png and jpg plots to be used in report.tex ")
print(f"**********************************************************************")
i=0
font=30
dpi=80

print("---------------------------------------------------------------")
print("Starting the creation of 1D plots for ROS, Burned Area and Smoke Altiitude.")
print("---------------------------------------------------------------")
Plot_1D_Plots(combined_ds,fontitle=28,savefig=True,savepath=savedirectory,savename="1DPlot",dpi=200)
print("1D summary plots for Burned area ROS, smoke plume altitude are ready")
print("---------------------------------------------------------------")
print("Starting the creation of Summary plots for local Metereology.")
print("---------------------------------------------------------------")
plot_meteorology(combined_ds,plotboundariesM1, output_image1="report/windst0",output_image2="report/windst1",provider=provider1,fontitle=28)
print("2D summary plots of metereological conditions before ignition are ready")
print("---------------------------------------------------------------")
print("Starting the creation of 2D plots for turbulence, ground smoke, and plume top and bottom heights.")
print("---------------------------------------------------------------")
for timestep in timesteps:
    if combined_ds.IgnitionTime.values>combined_ds.Time[timestep].values:
        print(f"Ignition:{combined_ds.IgnitionTime.values} TimeStep:{combined_ds.Time[timestep].values}")
        continue
    else:
        savename1=f"{i}_P1"
        savename2=f"{i}_P2"
        savename3=f"{i}_P3"
        print("---------------------------------------------------------------")
        print(f"plotting  time:  {combined_ds.Time[timestep].values}")
        

        Plot_surfaceWind_TKE(timestep=timestep,boundarypoints=plotboundaries,
                             dsfile=combined_ds,
                             ignitiondate=ignition,
                             fireenddate=endtime,
                             fontitle=font,
                             provider=provider1,
                             savefig=saveflag,
                             savepath=savedirectory2,
                             savename=savename1,
                             dpi=dpi)

        Plot_GroundSmokeConcentration(timestep=timestep,boundarypoints=plotboundaries,
                                      dsfile=combined_ds,
                                      ignitiondate=ignition,
                                      fireenddate=endtime,
                                      fontitle=font,
                                      provider=provider2,
                             savefig=saveflag,
                             savepath=savedirectory2,
                             savename=savename2,
                             dpi=dpi)

        Plot_LargeScale_Plume(timestep=timestep,boundarypoints=plotboundaries,
                              dsfile=combined_ds,
                            ignitiondate=ignition,
                            fireenddate=endtime,
                              fontitle=font,
                              provider=provider3,
                             savefig=saveflag,
                             savepath=savedirectory2,
                             savename=savename3,
                             dpi=dpi)
        i+=1
print("---------------------------------------------------------------")
print("2D  are plots are ready")
plot_location(combined_ds.IgnitionCoords.values[0],combined_ds.IgnitionCoords.values[1],plotboundariesM1,"report/location",provider=ctx.providers.OpenStreetMap.Mapnik)
print("location plot is  ready")

