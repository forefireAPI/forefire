#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Jun 10 10:34:40 2024

@author: baggio_r
"""

import xarray as xr
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
from scipy.interpolate import griddata
from datetime import datetime
from scipy import ndimage
from skimage import measure
import os
import re
from matplotlib.colors import LogNorm

##-----------------------------------------------------------------------------
# Function to print the file list inside a directory
def list_NetCDFfiles_in_directory(directory):
    try:
        # List to store file names
        file_list = []
        # Define the regex pattern
        pattern = re.compile(r'^[a-zA-Z0-9]{3,5}\.2\.[a-zA-Z0-9]{3,5}.*\.nc')
        # Get all files in the directory
        with os.scandir(directory) as entries:
            for entry in entries:
                print(f"entry is {entry}")
                if entry.is_file() and pattern.match(entry.name):
                    file_list.append(entry.name)
                # if entry.is_file():
                #     file_list.append(entry.name)
        
        return file_list
    except Exception as e:
        print(f"An error occurred: {e}")
        return []
# Function to extract the number from a string
def extract_number(s):
    match = re.search(r'\d+', s)  # Find the first sequence of digits
    return int(match.group()) if match else float('inf')
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
from collections import defaultdict
# Define the pattern
# pattern = re.compile(r'^[a-zA-Z]{5}\.1\.[a-zA-Z]{3,5}.*\.nc')
# filepath = "/Users/baggio_r/Documents/mount/scratchorsu/fcouto/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
# filepath = "/Users/baggio_r/Documents/mount/scratchorsu/fcouto/KDATABASE/Valleseco_20190817/2NEST/005_runff/"

# dirmesnhfilesM1=""

def to_MNHfiles_to_FFNC(filepath, dirmesnhfilesM1="",dirmesnhfilesM2="",bmappath="ForeFire/Outputs/ForeFire.0.nc",paramsfile="ForeFire/Params.ff",savepath="",namefire=None,nameout=None):

    """
    filepath="main directory with the MNH-Forefire simulation" 
            example="/Users/tiziocaio/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
    dirmesnhfilesM1: path (relative to filepath) to the directory containing the nc files for the larger MNH model (MODEL1)
    dirmesnhfilesM2: path (relative to filepath) to the directory containing the nc files of the small scale MNH model (MODEL2)
    bmappath: path (relative to filepath) to the Forefire time of Arrival matrix
    paramsfile: path (relative to filepath) to the Forefire params file
    savepath: path to save the reduced .nc file (with name nameout.nc)
    namefire: Name of the wildife that will be noted on the output .nc file (if None, it is taken from the name of the experiment directory)
                --> namefire = filenameM2.split('/')[-1].split('.')[0]+"___"+first_day_obj.strftime('%m/%d/%Y')
    nameout: Name of the output nc file, if None gives:
                --> nameout = mnhcexp (the mesonh experiment name parameter)
    savepath: Output directory 
    """
    # List to store matching files
    matching_files = []
    # matching_files = list_NetCDFfiles_in_directory(filepath+dirmesnhfilesM1)
    matching_files = list_NetCDFfiles_in_directory(filepath+dirmesnhfilesM2)
    print(matching_files)
    
    # Create a dictionary to store sublists based on the second sequence of characters
    sublist_dict = defaultdict(list)
    
    # Populate the dictionary
    for filename in matching_files:
        print(filename)
        # Extract the second sequence of characters
        match = re.search(r'^[a-zA-Z0-9]{3,5}\.2\.([a-zA-Z0-9]{3,5})\.', filename)
        if match:
            key = match.group(1)
            sublist_dict[key].append(filename)
    
    # Convert the dictionary values to lists
    sublists = list(sublist_dict.values())
    
    #The name of files we are interested in are in the larger sublist 
    file_names = max(sublists, key=len)
    print(file_names)
    # Extract and print the two strings for one element of the largest sublist
    if file_names:
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
    #list of available M1 files (and then timestep)
    if nameout == None:
        nameout=mnhcexp
    # print(file_names)
    
    formatted_numbers = [s[-6:-3] for s in file_names]
    # Sort the list of strings based on the extracted numbers
    formatted_numbers = sorted(formatted_numbers, key=extract_number)
    print(formatted_numbers)
    formatted_numbers=formatted_numbers[1:]
    print(f"There are {len(formatted_numbers)} time steps available")
    ##-----------------------------------------------------------------------------
    #Find the perimeter resolution:
    ffsettings=filepath+paramsfile    
    pattern_per = 'minimalPropagativeFrontDepth='
    perimeterres = find_first_integer_after_pattern(ffsettings, pattern_per)
    perimeterres = int(perimeterres)
    print(f"perimeter resolution is {perimeterres}")
    pattern_burning_pow="nominalHeatFlux="
    burning_pow=find_first_integer_after_pattern(ffsettings, pattern_burning_pow)
    burning_pow=int(burning_pow)
    print(f"burning_pow  is {burning_pow}")
    pattern_burning_duration="burningDuration="
    burning_time=find_first_integer_after_pattern(ffsettings, pattern_burning_duration)
    burning_time=int(burning_time)
    print(f"burning_time is {burning_time}")
    ## PM2.5=24-hour mean: ≈204.49×10−9 kg/kg HAZARDOUS LEVEL
    ## rypical PM2.5 for a kg of biomass in case of Forest Fires PM2.5: Approximately 8-15 g/kg
    bratio_for_msquared=burning_pow*burning_time #Watt s m-2 => j m-2 #BRATIO corresponding to a buened m2
    biomasskg=bratio_for_msquared/(18*10**6)#Bratio which corresponds to a kg of biomass burned (kg m-2)
    pm25kgform2=biomasskg*(0.015)  #(kg m-2)
    bratio_for_msquared=burning_time #(kg m-2) by costruction
    smoke_scalefactor=pm25kgform2/bratio_for_msquared
    print(f"smoke_scalefactor is {smoke_scalefactor}")
    ##-----------------------------------------------------------------------------
    #Inizialize empty lists for late stacking
    timesteps_array=[]
    wind_u_WT_list = []
    wind_v_WT_list = []
    wind_w_WT_list = []
    w_up_TW_list = []
    tke_WT_list = []
    smoke_ground_WT_list = []
    timearr_WT_list = []
    lon_boundary_WT_list = []
    lat_boundary_WT_list = []
    ROS_fr_WT_list = []
    altitude_plume_bottom_WT_list = []
    altitude_plume_top_WT_list = []
    # A dictionary is needed for front velocities:
    lon_pointsROS = {}
    lat_pointsROS = {}
    magnitudesROS = {}
    ##-----------------------------------------------------------------------------
    # Satr looping on available time steps
    time_XA=[]
    flagfirststep=True
    for formatted_number in formatted_numbers:
    
        filenameM1 = filepath+dirmesnhfilesM1+mnhcexp+".1."+mnhcseg+"."+formatted_number+".nc"
        filenameM2 = filepath+dirmesnhfilesM2+mnhcexp+".2."+mnhcseg+"."+formatted_number+".nc"   
        
        time_step=int(formatted_number)
        timesteps_array.append(time_step)
        ###############################################################################
        #
        #                                   MODEL 2
        # -----------------------------------------------------------------------------
        #
        #                   High, Small Scale Resolution Variables:
        # -----------------------------------------------------------------------------
        # The following variables are stored from model 2:
        #
        ###############################################################################
        filebmap=filepath+bmappath
        dsbmap=xr.load_dataset(filebmap,engine="netcdf4")
        # print(dsbmap.data_vars)
        ds=xr.load_dataset(filenameM2,engine="netcdf4")
        # print(ds.data_vars)
        # print(ds["WT"])
        # print(ds["WT"].shape)
        # print(ds["UT"].coords["latitude_u"],ds["UT"].coords["latitude_u"].shape)
        # print(ds["WT"].coords["latitude"],ds["WT"].coords["latitude"].shape)
        
    
        datetime_str=ds["time"].values[0]
        time_XA.append(datetime_str)
        first_day_str=time_XA[0]
        first_day_obj = pd.to_datetime(first_day_str)
        first_day = first_day_obj.day
        print(f"datetime_str: {datetime_str}, n proc steps: {len(time_XA)}")
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
        #lat_vertexx=lat_vertex-0.5*(lat_vertex[10,:]-lat_vertex[9,:])
        
        u_vertex=np.zeros((u.shape))# 
        u_vertex[:,:,0] =  u[:,:, 1] 
        u_vertex[:,:,  0:-1] = (u[:,:, :-1] + np.roll(u[:,:,:],-1,axis=1)[:,:,:-1])/ 2
        u_vertex[:,:,-1]= (u[:,:, -2] + u[:,:, -1]) / 2
        
        # u_vertex[1:-1,:,:]=(u_vertex[1:-1,:, :] + np.roll(u_vertex[:,:,:],-1,axis=0)[1:-1,:,:])/ 2
        # u_vertex[-1,:,:]= (u_vertex[-2,:, :] + u_vertex[-1,:, :]) / 2
        
        v_vertex=np.zeros((v.shape))
        v_vertex[:,0,:]=v[:,0,:]
        v_vertex[:,:-1, :] = (v[:,:-1, :] +np.roll(v[:,:,:],-1,axis=2)[:,:-1,:])/ 2
        v_vertex[:,-1,:]= (v[:,-2,:] +v[:,-1,:]) / 2
        
        # print(v.shape,v_vertex.shape)
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
    
        # Create the DataArray with coordinates
        wind_u_WT_list.append(u_vertex_surf)
        wind_v_WT_list.append(v_vertex_surf)
        wind_w_WT_list.append(w_vertex_surf)
        
        # -----------------------------------------------------------------------------
        #                               VERTICAL w UPLIFTS >=thr_w  m s-1
        # -----------------------------------------------------------------------------
        # Select the vertical wind > 3 m s-1
        #------------------------------------------------------------------------------
        w_up=w_vertex[:16,:,:]
        w_up_th=np.where(w_up>=3,w_up,0)
        w_up_TW_list.append(w_up_th)
        # -----------------------------------------------------------------------------
        #                               TKE m2 s-2 (or j/kg)
        # -----------------------------------------------------------------------------
        # Take TKE above 1.5 m2s-2 in the first ~1000 m above the ground
        #------------------------------------------------------------------------------
        # Constants
        tke = ds["TKET"][0,:16,:,:] 
        tke_vertex=np.zeros((tke.shape))# 
        tke_vertex[:,:,0] =  tke[:,:, 1] 
        tke_vertex[:,:,  0:-1] = (tke[:,:, :-1] + np.roll(tke[:,:,:],-1,axis=1)[:,:,:-1])/ 2
        tke_vertex[:,:,-1]= (tke[:,:, -2] + tke[:,:, -1]) / 2
        
        tke_vertex[:,0,:]=tke[:,0,:]
        tke_vertex[:,:-1, :] = (tke[:,:-1, :] +np.roll(tke[:,:,:],-1,axis=2)[:,:-1,:])/ 2
        tke_vertex[:,-1,:]= (tke[:,-2,:] +tke[:,-1,:]) / 2
        #keep Tke below approx 1000m
        #keep only tke > 1.5 m2 s-2  ( 3 m2 s-2  is indicative of highly turbolent boundary layer Heilman Bian 2010)
        tke_vertex_th=np.where(tke_vertex>=1.5,tke_vertex,0)
        tke_WT_list.append(tke_vertex_th)
        # Create DataArray for TKE
    
        
        # -----------------------------------------------------------------------------
        #                               Smoke Tracer (adimensional)
        # -----------------------------------------------------------------------------
        # 
        #------------------------------------------------------------------------------
        smoke_unscaled = ds["SVT002"][0,:,:,:]
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
        smoke_ground_WT_list.append(smoke_vertex[0,:,:])
        # Create DataArray for smoke
    
        
        # -----------------------------------------------------------------------------
        #                          Fire Fronts, Time of arrival 
        # -----------------------------------------------------------------------------
        #  They have finer resolution, following the bmap
        #------------------------------------------------------------------------------
        
        datetime_obj = pd.to_datetime(datetime_str)
        datetime_str_form = datetime.strptime(str(datetime_str), "%Y-%m-%dT%H:%M:%S.%f000000")
        
        # Extract the hour, minute, and second
        day = datetime_obj.day
        hour =  datetime_obj.hour
        minute = datetime_obj.minute
        second = datetime_obj.second
        # Convert to seconds since midnight
        seconds_since_midnight = 24*3600*(day-first_day)+hour * 3600 + minute * 60 + second
        print(f'firstday: {first_day},  day : {day}  and secs : {seconds_since_midnight}')
        ta=dsbmap["arrival_time_of_front"].values
        masked_ta = np.where(ta>seconds_since_midnight,np.min(ta),ta)
        # plt.imshow(masked_ta)
        # plt.show()
        timearr_WT_list.append(masked_ta)
        # Define the new high-resolution grid dimensions
        new_shape = ta.shape
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
    
        # -----------------------------------------------------------------------------
        #                          Fire Front Velocity, m/s
        # -----------------------------------------------------------------------------
        #  They have finer resolution, following the bmap
        #------------------------------------------------------------------------------
        
        # Calculate the gradient of the entire array
        max_speed_filter=1.0 # Filter for max velocity
        # ta_all = np.where(ta<=0.0, -9999,ta)
        ta_all=ta 
        print("ta_all GRAD:  ",np.amax(ta_all),np.amin(ta_all))
        #
        #######################################################################
        # l,b,r,t = self.lbrt
        # norm_data = np.copy(self.atime )
         
        # #norm_data = np.where(self.atime < -9990, np.nan, self.atime)
        # norm_data[norm_data < 0] =  np.nan
        
        # BMapresolution = float( (r-l) / norm_data.shape[0] )
        # gradient_y, gradient_x = np.gradient(norm_data, 1)
        # dspeed = np.sqrt(gradient_x**2 + gradient_y**2)
        # print("computing ROS at resolution :",BMapresolution, dspeed.shape, norm_data.shape, " fitering averything over (in m/s):",max_speed_filter)
        # #######################################################################
        
        gradient_y, gradient_x = np.gradient(ta_all)
        sqrdiv=np.sqrt(np.power(gradient_y,2)+np.power(gradient_x,2))
        sqrdiv=np.where(ta==np.min(ta),0.0,sqrdiv)
        sqrdiv=np.where(sqrdiv>=100*burning_time,0.0,sqrdiv)

        #10 is the perimeter resolution!
        fire_velocity=np.divide(perimeterres,sqrdiv)
        fire_velocity=np.where(fire_velocity==np.nan,0.0,fire_velocity)
        fire_velocity=np.where(fire_velocity==np.inf,0.0,fire_velocity)
        fire_velocity=np.where(fire_velocity>=max_speed_filter,0.0,fire_velocity)

                
        

        fire_velocity[fire_velocity > max_speed_filter] = max_speed_filter
        #fire_velocity=np.ma.masked_not_equal(fire_velocity,1)
        #find the  corrisponding hour to properly trim the bmap
        firefront =np.ma.masked_greater(ta, seconds_since_midnight)
        firefront =np.ma.masked_less_equal(firefront, 0.0)
        plt.imshow(firefront)
        plt.show()
        # Find the contours of the masked region
        binary_mask = ~firefront.mask  # Convert masked array to binary mask
        labeled_array, num_features = ndimage.label(binary_mask)  # Label connected regions
        slices = ndimage.find_objects(labeled_array)  # Find the bounding box
        
        normal_vectors = []
        normal_magnitudes = []
        
        
        # if the fire has already started
        if slices: 
        # Extract the boundary of the largest region (if there are multiple regions)
            largest_region = slices[0]  # Assuming the first region is the largest
        ## if the fire has noot started yet take the ignition point
            if flagfirststep:
                ignition =np.ma.masked_greater(ta_all, np.unique(ta_all)[2])
                ignition =np.ma.masked_less_equal(ignition, 0.0)
        else:
            ignition =np.ma.masked_greater(ta_all, np.unique(ta_all)[2])
            ignition =np.ma.masked_less_equal(ignition, 0.0)
            binary_mask = ~ignition.mask  # Convert masked array to binary mask
            labeled_array, num_features = ndimage.label(binary_mask)  # Label connected regions
            slices = ndimage.find_objects(labeled_array)  
            largest_region = slices[0]  
            
        boundary_mask = np.zeros_like(binary_mask)
        boundary_mask[largest_region] = binary_mask[largest_region]
        # Step 1: Label the connected components in the mask
        ####################################################
        # labeled_mask, num_features = ndimage.label(boundary_mask)
        
        # # Step 2: Find the largest connected component (which is the external boundary)
        # sizes = ndimage.sum(boundary_mask, labeled_mask, range(1, num_features + 1))
        # largest_component = (sizes == sizes.max()).nonzero()[0] + 1
        
        # # Create a mask for the largest component
        # largest_component_mask = (labeled_mask == largest_component)
        
        # # Step 3: Dilate the mask and find the boundary
        # dilated_mask = ndimage.binary_dilation(largest_component_mask)
        # external_boundary = dilated_mask ^ largest_component_mask
########################################################
        boundary = ndimage.binary_dilation(boundary_mask) ^ boundary_mask  # Get the boundary
        # Find the coordinates of the boundary points
        boundary_coords = np.argwhere(boundary)
        # Create an array to store the normal gradients
        
        # Calculate the magnitude of each normal vector
    
        for coord in boundary_coords:
            y, x = coord
            normal_y, normal_x = compute_normal(gradient_y[y, x], gradient_x[y, x])
            normal_vectors.append((normal_y, normal_x))
            normal_magnitudes.append(fire_velocity[y,x])
            
        # Initialize lists to store latitude and longitude of boundary points
        lat_boundary = []
        lon_boundary = []
        
        # Map boundary indices to latitude and longitude
        for y, x in boundary_coords:
            lat_boundary.append(lat_high_res_interp[y, x])
            lon_boundary.append(lon_high_res_interp[y, x])
        
        lat_boundary = np.array(lat_boundary)
        lon_boundary = np.array(lon_boundary)
            
        normal_vectors = np.array(normal_vectors)
        normal_magnitudes = np.array(normal_magnitudes)
        print(f'normal_magnitudes MIN:{np.amin(normal_magnitudes)}   MAX:{np.amax(normal_magnitudes)}')
        # lon_boundary_WT_list.append(lon_boundary)
        # lat_boundary_WT_list.append(lat_boundary)
        # ROS_fr_WT_list.append(normal_magnitudes)
        
        # pointsROS[time_step] = np.vstack((lon_boundary, lat_boundary))
        lat_pointsROS[time_step]=lat_boundary
        lon_pointsROS[time_step]=lon_boundary
        magnitudesROS[time_step] = normal_magnitudes
        # normal_magnitudes = np.linalg.norm(normal_gradients, axis=0)
        # print("Normal Gradient Array:\n", normal_gradient_array)
        ds.close()
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
        
        smoke_unsc = dsM1["SVT002"][0,:,:,:]
        
        smoke=smoke_unsc*smoke_scalefactor #relation btw 1kg BRatio and PM2.5
        lat_center_M1 = dsM1["SVT002"].coords["latitude"].values
        lon_center_M1 = dsM1["SVT002"].coords["longitude"].values
        z_vertex = ds["SVT002"].coords["level"].values
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
        
        # plt.imshow(z_plumebottom_flat)
        # plt.imshow(smokeground_thr)
        plt.show()
        
        z_plumebottom_flat=z_plumebottom_flat+smokeground_thr
        altitude_plume_bottom_WT_list.append(z_plumebottom_flat)
        # plt.imshow(z_plumebottom_flat)
        # plt.imshow(smokeground_thr)
        plt.show()
        # -----------------------------------------------------------------------------
        #                          Plume Top
        # -----------------------------------------------------------------------------
        # 
        #------------------------------------------------------------------------------
        
        #   Store altitude corresponding to plume top
        z_plumetop=np.where(smoke>=thr,altitude_3D_array,np.min(altitude_3D_array))
        z_plumetop_flat = np.max(z_plumetop, axis=0)    
        # plt.imshow(z_plumetop_flat)
        plt.show()
        altitude_plume_top_WT_list.append(z_plumetop_flat)
        dsM1.close()
        flagfirststep=False
    ###############################################################################
    
    # Creation of the Datarrays 
    
    #::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::  
    ##-----------------------------------------------------------------------------
    #Inizialize empty lists for late stacking
    # time_XA=np.stack(time_XA,axis=0)
    
    wind_u_WT = np.stack(wind_u_WT_list ,axis=0)
    wind_v_WT = np.stack(wind_v_WT_list ,axis=0)
    wind_w_WT = np.stack(wind_w_WT_list ,axis=0)
    w_up_TW = np.stack(w_up_TW_list ,axis=0)
    tke_WT = np.stack(tke_WT_list ,axis=0)
    smoke_ground_WT = np.stack(smoke_ground_WT_list ,axis=0)
    timearr_WT = np.stack(timearr_WT_list ,axis=0)
    # lon_boundary_WT = np.stack(lon_boundary_WT_list ,axis=0)
    # lat_boundary_WT = np.stack(lat_boundary_WT_list ,axis=0)
    # ROS_fr_WT = np.stack(ROS_fr_WT_list ,axis=0)
    altitude_plume_bottom_WT = np.stack(altitude_plume_bottom_WT_list ,axis=0)
    altitude_plume_top_WT = np.stack(altitude_plume_top_WT_list ,axis=0)
    
    print("CHACK  ",wind_u_WT.shape)
    timesteps=np.arange(0,wind_u_WT.shape[0])
    print(f'timesteps is {timesteps}')
    # Convert the dictionary to a DataArray
    # times = list(points.keys())
    lon_points_data = list(lat_pointsROS.values())
    lat_points_data = list(lon_pointsROS.values())
    magnitudes_data = list(magnitudesROS.values())
    
    # Find the maximum number of points
    max_points = max(len(p) for p in lon_points_data)
    
    # Create arrays with NaNs to hold the data
    lat_data_array = np.full((len(timesteps), max_points), np.nan)
    lon_data_array = np.full((len(timesteps), max_points), np.nan)
    coords_points_ROS=np.full((len(timesteps), max_points,2), np.nan)
    magnitude_array = np.full((len(timesteps), max_points), np.nan)
    
    # Fill the data arrays with actual points and magnitudes
    for timestep in timesteps_array:
        # timestepok=timestep+1
        n_points = len(lat_pointsROS[timestep])
        print(timestep,n_points)
        lat_data_array[timestep-1, :n_points] = lat_pointsROS[timestep]
        lon_data_array[timestep-1, :n_points] = lon_pointsROS[timestep]
        coords_points_ROS[timestep-1, :n_points, 0] =lon_pointsROS[timestep]
        coords_points_ROS[timestep-1, :n_points, 1] =lat_pointsROS[timestep]
        magnitude_array[timestep-1, :n_points] = magnitudesROS[timestep]
    
    
    ##-----------------------------------------------------------------------------
    ##-----------------------------------------------------------------------------
    #----------------------------  Fire Generalities   ----------------------------
    if namefire==None:
        namefire = filenameM2.split('/')[-1].split('.')[0]+"___"+first_day_obj.strftime('%m/%d/%Y')
    name_da =xr.DataArray(namefire, name="WildfireName")
    
    ignition_da = xr.DataArray(  ignition,  # Replace with your high-resolution data array
        dims=['y', 'x'],
        coords={'lat_bmap': (['y', 'x'], lat_high_res_interp), 'lon_bmap': (['y', 'x'], lon_high_res_interp)},
        name="Ignition")
        
    #-------------------------------- MODEL 1 -------------------------------------
    # store velocities at the ground
    time_da = xr.DataArray(time_XA,
                        dims=["timestep"],
                        coords={
                            #"altitude": z_vertex,
                            "timestep":timesteps,
                            },
                        name="Time")
    u_da = xr.DataArray(wind_u_WT,
                        dims=["timestep","lat", "lon"],
                        coords={
                            #"altitude": z_vertex,
                            "timestep":timesteps,
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="Wind_U")
    v_da = xr.DataArray(wind_v_WT,
                        dims=["timestep","lat", "lon"],
                        coords={
                            "timestep":timesteps,
                            #"altitude": z_vertex,
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="Wind_V")
    w_da = xr.DataArray(wind_w_WT,                    
                        dims=["timestep","lat", "lon"],
                        coords={
                            "timestep":timesteps,
                            #"altitude": z_vertex,
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="Wind_W")    
    
    # store upward_air_velocity above 3 m s-1 
    wup_da = xr.DataArray(w_up_TW,                    
                        dims=["timestep","altitude_wup", "lat", "lon"],
                        coords={
                            "timestep":timesteps,
                            "altitude_wup": z_vertex[:16],
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="W_Upward")
    
    # storeTKE above 1.5 m s-1
    tke_da = xr.DataArray(tke_WT,                    
                        dims=["timestep","altitude_tke", "lat", "lon"],
                        coords={
                            "timestep":timesteps,
                            "altitude_tke": z_vertex[:16],
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="TKE")
    
    # store smoke density at the ground   
    smoke_da = xr.DataArray(smoke_ground_WT,                    
                            dims=["timestep","lat", "lon"],
                            coords={
                                # "altitude": z_vertex,
                                "timestep":timesteps,
                                "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                                "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                                },
                            name="GroundSmoke")
    # Create a high-resolution DataArray for time of arrival
    ta_da = xr.DataArray(
        timearr_WT,  # Replace with your high-resolution data array
        dims=["timesteps",'y', 'x'],
        coords={"timesteps":timesteps,'lat_bmap': (['y', 'x'], lat_high_res_interp), 'lon_bmap': (['y', 'x'], lon_high_res_interp)},
        name="ArrivalTime"
    )
    # Create the DataArray for the points of the fire front
    coords_points_da = xr.DataArray(
        coords_points_ROS,
        dims=['timesteps', 'points', 'coord_points'],
        coords={'timesteps': timesteps, 'points': range(max_points), 'coord_points': ['lon_points', 'lat_points']}
    )
    
    # Create the DataArray for magnitudes
    magnitudes_da = xr.DataArray(
        magnitude_array,
        dims=['timesteps', 'points'],
        coords={'timesteps': timesteps, 'points': range(max_points)}
    )
    # # store velocities of the fire front
    # front_velocity_da = xr.DataArray(
    #     ROS_fr_WT,  
    #     dims=["time",'n_points_coords'],
    #     coords={"time":(time_XA),'lon_point': (['n_points_coords'],lon_boundary_WT), 'lat_point': (['n_points_coords'],lat_boundary_WT)},
    #     name="FrontVelocity"
    # )
    
    #::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    #-------------------------------- MODEL 2 -------------------------------------
    
    #   Store altitude corresponding to plume bottom
    zplumebottom_da = xr.DataArray(altitude_plume_bottom_WT,                    
                        dims=["timestep","large_scale_lat", "large_scale_lon"],
                        coords={
                            "timestep":timesteps,
                            "large_scale_latitude": (["large_scale_lat", "large_scale_lon"], lat_center_M1),  # Ensure correct dims here
                            "large_scale_longitude": (["large_scale_lat", "large_scale_lon"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="Z_FirePlumeBottom")
    
    #   Store altitude corresponding to plume top
    zplumetop_da = xr.DataArray(altitude_plume_top_WT,                    
                        dims=["timestep","large_scale_lat", "large_scale_lon"],
                        coords={
                            "timestep":timesteps,
                            "large_scale_latitude": (["large_scale_lat", "large_scale_lon"], lat_center_M1),  # Ensure correct dims here
                            "large_scale_longitude": (["large_scale_lat", "large_scale_lon"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="Z_FirePlumeTop")
    
    ###############################################################################
    #------------------------------------------------------------------------------
    # Create the reduced Dataset
    #------------------------------------------------------------------------------
    ###############################################################################
    dsnew = xr.Dataset({
        "Ignition": ignition_da,
        "WildfireName": name_da,
        "Time": time_da,
        "Wind_U": u_da,
        "Wind_V": v_da,
        "Wind_W": w_da,
        "W_Upward": wup_da,
        "TKE": tke_da,
        "GroundSmoke": smoke_da,
        "ArrivalTime": ta_da,
        "FrontVelocity": magnitudes_da,
        "FrontCoords": coords_points_da,
        "Z_FirePlumeBottom": zplumebottom_da,
        "Z_FirePlumeTop": zplumetop_da})
    
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
    dsnew["Wind_W"].attrs["comment"] = "upward_air_velocity at the sueface"
    
    dsnew["TKE"].attrs["units"] = "m2 s-2"
    dsnew["TKE"].attrs["standard_name"] = "specific_turbulent_kinetic_energy_of_air"
    dsnew["TKE"].attrs["comment"] = "TKE above 1.5 m2s-2 in the first ~1000 m above the ground"
    
    dsnew["W_Upward"].attrs["units"] = "m s-1"
    # dsnew["W_Upward"].attrs["standard_name"] = "specific_turbulent_kinetic_energy_of_air"
    dsnew["W_Upward"].attrs["comment"] = "upward_air_velocity above 3 m s-1 in the first ~1000 m above the ground"
    
    dsnew["GroundSmoke"].attrs["units"] = "adimensional kg kg-1"
    dsnew["GroundSmoke"].attrs["comment"] = "Smoke density at the ground"
    
    dsnew["ArrivalTime"].attrs["units"] = "seconds from midnight"
    dsnew["ArrivalTime"].attrs["comment"] = "time of arrival of the fire fronts, measured in seconds from midnight of the date of ignition"
    
    dsnew["FrontVelocity"].attrs["units"] = "m s-1"
    dsnew["FrontVelocity"].attrs["comment"] = "Velocity of each point of the firefront "
    
    dsnew["latitude"].attrs["units"] = "degrees_north"
    dsnew["latitude"].attrs["comment"] = "latitudes of the 160 resolution, small scale domain"
    dsnew["longitude"].attrs["units"] = "degrees_east"
    dsnew["longitude"].attrs["comment"] = "longitudes of the 160 resolution, small scale domain"
    dsnew["altitude_wup"].attrs["units"] = "m"
    dsnew["altitude_tke"].attrs["units"] = "m"
    
    dsnew["lat_bmap"].attrs["units"] = "degrees_north"
    dsnew["lat_bmap"].attrs["comment"] = "latitudes of the high resolution (10 m) fire propagation model"
    dsnew["lon_bmap"].attrs["units"] = "degrees_east"
    dsnew["lon_bmap"].attrs["comment"] = "longitudes of the high resolution (10 m) fire propagation model"
    dsnew["coord_points"].attrs["units"] = "degrees east, degrees_north"
    dsnew["coord_points"].attrs["comment"] = "longitudes and latitudes of fire front (10 m resolution) points"
    # dsnew["lon_point"].attrs["units"] = "degrees_east"
    # dsnew["lon_point"].attrs["comment"] = "longitudes of fire front(10 m resolution) points"
    #------------------------------------------------------------------------------
    # MODEL 1 attributes
    
    dsnew["Z_FirePlumeBottom"].attrs["units"] = "m"
    dsnew["Z_FirePlumeBottom"].attrs["comment"] = "altitude of the lower boundary of the fireplume in meters"
    
    dsnew["Z_FirePlumeTop"].attrs["units"] = "m"
    dsnew["Z_FirePlumeTop"].attrs["comment"] = "altitude of the upper boundary of the fireplume in meters"
    
    
    dsnew["large_scale_latitude"].attrs["units"] = "degrees_north"
    dsnew["large_scale_latitude"].attrs["comment"] = "latitudes of the 800 resolution, large scale domain"
    dsnew["large_scale_longitude"].attrs["units"] = "degrees_east"
    dsnew["large_scale_longitude"].attrs["comment"] = "longitudes of the 800 resolution, large scale domain"
    ###############################################################################
    # -----------------------------------------------------------------------------
    # Save to a NetCDF file
    # -----------------------------------------------------------------------------
    ###############################################################################
    
    
    # ds.close()
    # dsM1.close()
    # Write the reduced NetCDF file
    

    dsnew.to_netcdf(savepath+f"{nameout}.nc")
    print(f'NetCDF file {savepath}/{nameout}.nc created successfully.')
##-----------------------------------------------------------------------------
# Read the available NetCDF files
###  Aquitaine
# filepath = "/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Aquitaine/LaTeste/"
# filepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Canaries/"

# dirmesnhfilesM1="Netcdf_files/MOD1t/" #Netcdf_files
# dirmesnhfilesM2="Netcdf_files/MOD2/"
# savepath=filepath
# filebmap = filepath+"ForeFire.0.nc"
# filepath ="/scratch/baggio_r/fcouto/KDATABASE/TesteDeBuch_20220712/2NEST/005_runff/"
# namef="TESTE"
### Gran Canaria
# filepath ="/scratch/baggio_r/fcouto/KDATABASE/Valleseco_20190817/2NEST/005_runff/"
### Portugal-Riodades
filepath= "/scratch/baggio_r/fcouto/KDATABASE/SaoJoao_20200710/2NEST/005_runff/"

dirmesnhfilesM1=""
dirmesnhfilesM2=""
# savepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Canaries/"
# savepath="/scratch/baggio_r/fcouto/KDATABASE/TesteDeBuch_20220712/2NEST/RESULTS/"
# savepath = "/scratch/baggio_r/fcouto/KDATABASE/Valleseco_20190817/2NEST/RESULTS/"
savepath = "/scratch/baggio_r/fcouto/KDATABASE/SaoJoao_20200710/2NEST/RESULTS/"
# savepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/Canaries/"
filebmap = "ForeFire/Outputs/ForeFire.0.nc"
paramsfile= "ForeFire/Params.ff"
namefire = None
namef=None
# namef = "test"

to_MNHfiles_to_FFNC(filepath, dirmesnhfilesM1=dirmesnhfilesM1,dirmesnhfilesM2=dirmesnhfilesM2,bmappath="ForeFire/Outputs/ForeFire.0.nc",paramsfile=paramsfile,savepath=savepath,namefire=None,nameout=None)

##-----------------------------------------------------------------------------
# The end
##-----------------------------------------------------------------------------


###############################################################################
# -----------------------------------------------------------------------------
# Some plotting stuff
# -----------------------------------------------------------------------------
###############################################################################
# # matrix1 = w_up[6,:,:]
# for j in range (0,u.shape[1]):
#     matrix2 = tke[:,j,:]
#     matrix1= tke_vertex[:,j,:]
# # matrix2=ta
# # Determine the common color scale limits
#     vmin = min(matrix1.min(), matrix2.min())
#     vmax = max(matrix1.max(), matrix2.max())
    
#     # Create the subplots
#     fig, axes = plt.subplots(1, 3, figsize=(12, 6))
    
#     # Plot the first matrix
#     im1 = axes[0].imshow(matrix1, vmin=vmin, vmax=vmax, cmap='viridis')
#     axes[0].set_title('Matrix 1')
#     fig.colorbar(im1, ax=axes[0],fraction=0.015,aspect=30)
    
#     # Plot the second matrix
#     # m2 = axes[1].imshow(matrix2,cmap="Greys")
#     im2 = axes[1].imshow(matrix2, vmin=vmin, vmax=vmax, cmap='viridis')
#     axes[1].set_title('Matrix 2')
#     fig.colorbar(im2, ax=axes[1],fraction=0.015,aspect=30)
    
#     im3=axes[2].imshow(np.absolute(matrix2-matrix1), cmap='cool')
#     fig.colorbar(im3, ax=axes[2],fraction=0.015,aspect=30)
#     # Show the plot
#     plt.tight_layout()
#     plt.show()
##-----------------------------------------------------------------------------
