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
from scipy import ndimage

#from skimage import measure
import os
import re
import sys 
home_dir = os.path.expanduser("~")

sys.path.append(home_dir+'/soft/firefront/tools/')
from preprocessing.ffToGeoJson import get_WSEN_LBRT_ZS_From_Pgd

##-----------------------------------------------------------------------------
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
                    # print(f"entry is {entry}")
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
from collections import defaultdict
# Define the pattern
# pattern = re.compile(r'^[a-zA-Z]{5}\.1\.[a-zA-Z]{3,5}.*\.nc')
# casepath = "/Users/baggio_r/Documents/mount/scratchorsu/fcouto/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
# casepath = "/Users/baggio_r/Documents/mount/scratchorsu/fcouto/KDATABASE/Valleseco_20190817/2NEST/005_runff/"

# dirmesnhfilesM1=""

def to_MNHfiles_to_FFNC(casepath, dirmesnhfilesM1="",dirmesnhfilesM2="",bmappath="ForeFire/Outputs/ForeFire.0.nc",paramsfile="ForeFire/Params.ff",savepath="",namefire=None,nameout=None):

    """
    casepath="main directory with the MNH-Forefire simulation" 
            example="/Users/tiziocaio/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
    dirmesnhfilesM1: path (relative to casepath) to the directory containing the nc files for the larger MNH model (MODEL1)
    dirmesnhfilesM2: path (relative to casepath) to the directory containing the nc files of the small scale MNH model (MODEL2)
    bmappath: path (relative to casepath) to the Forefire time of Arrival matrix
    paramsfile: path (relative to casepath) to the Forefire params file
    savepath: path to save the reduced .nc file (with name nameout.nc)
    namefire: Name of the wildife that will be noted on the output .nc file (if None, it is taken from the name of the experiment directory)
                --> namefire = filenameM2.split('/')[-1].split('.')[0]+"___"+first_day_obj.strftime('%m/%d/%Y')
    nameout: Name of the output nc file, if None gives:
                --> nameout = mnhcexp (the mesonh experiment name parameter)
    savepath: Output directory 
    """
    # List to store matching files
    matching_files = []
    # matching_files = list_NetCDFfiles_in_directory(casepath+dirmesnhfilesM1)
    matching_files = list_NetCDFfiles_in_directory(casepath+dirmesnhfilesM2)
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
    ffsettings=casepath+paramsfile    
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
    bratio_for_msquared=burning_pow*burning_time #Watt s m-2 => j m-2 #SVFF001 corresponding to a buened m2
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
    surface_altitudeM2_list=[]
    surface_altitudeM1_list=[]
    # A dictionary is needed for front velocities:
    lon_pointsROS = {}
    lat_pointsROS = {}
    magnitudesROS = {}
    ##-----------------------------------------------------------------------------
    # Satr looping on available time steps
    time_XA=[]
    flagfirststep=True
    for formatted_number in formatted_numbers:
    
        filenameM1 = casepath+dirmesnhfilesM1+mnhcexp+".1."+mnhcseg+"."+formatted_number+".nc"
        filenameM2 = casepath+dirmesnhfilesM2+mnhcexp+".2."+mnhcseg+"."+formatted_number+".nc"   
        
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
        filebmap=casepath+bmappath
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
        print(f" z_vertex {z_vertex}")
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
        if formatted_number==formatted_numbers[0]:
            alt=ds["ZS"]
            alt_vertex = np.zeros_like(alt)
            # -------------------------------
            # Handle Columns (j-direction)
            # -------------------------------
            
            # First column (j=0): Assign the value from the second column (j=1)
            alt_vertex[:, 0] = alt[:, 1]
            # Middle columns (j=1 to j=-2): Average of current and next column
            alt_vertex[:, 0:-1] = (alt[:, 0:-1] + np.roll(alt, -1, axis=1)[:, 0:-1]) / 2            
            # Last column (j=-1): Average of the last two columns
            alt_vertex[:, -1] = (alt[:, -2] + alt[:, -1]) / 2
            
            # -------------------------------
            # Handle Rows (i-direction)
            # -------------------------------
            
            # First row (i=0): Assign the value directly
            alt_vertex[0, :] = alt[0, :]          
            # Middle rows (i=1 to i=-2): Average of current and next row
            alt_vertex[0:-1, :] = (alt[0:-1, :] + np.roll(alt, -1, axis=0)[0:-1, :]) / 2          
            # Last row (i=-1): Average of the last two rows
            alt_vertex[-1, :] = (alt[-2, :] + alt[-1, :]) / 2
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
        plt.imshow(firefront)
        plt.show()
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


        #------------------------------------------------------------------------------
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
        
        smoke_unsc = dsM1["SVFF001"][0,:,:,:]
        
        smoke=smoke_unsc*smoke_scalefactor #relation btw 1kg BRatio and PM2.5
        lat_center_M1 = dsM1["SVFF001"].coords["latitude"].values
        lon_center_M1 = dsM1["SVFF001"].coords["longitude"].values
        z_vertex = ds["SVFF001"].coords["level"].values
        
        if formatted_number==formatted_numbers[0]:
            ZSM1 = dsM1["ZS"].data
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

        #------------------------------------------------------------------------------
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
    ZSM2= alt_vertex#surface_altitudeM2_list
    # ZSM1= #surface_altitudeM1_list
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
    alt_da = xr.DataArray(ZSM2,                    
                        dims=["lat", "lon"],
                        coords={
                            #"altitude": z_vertex,
                            "latitude": (["lat", "lon"], lat_vertex),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_vertex)  # Ensure correct dims here
                            },
                        name="ZS")        
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
                        dims=["timestep","lat", "lon"],
                        coords={
                            "timestep":timesteps,
                            "latitude": (["lat", "lon"], lat_center_M1),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="Z_FirePlumeBottom")
    
    #   Store altitude corresponding to plume top
    zplumetop_da = xr.DataArray(altitude_plume_top_WT,                    
                        dims=["timestep","lat", "lon"],
                        coords={
                            "timestep":timesteps,
                            "latitude": (["lat", "lon"], lat_center_M1),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="Z_FirePlumeTop")
    altM1_da = xr.DataArray(ZSM1,                    
                        dims=["lat", "lon"],
                        coords={
                            "latitude": (["lat", "lon"], lat_center_M1),  # Ensure correct dims here
                            "longitude": (["lat", "lon"], lon_center_M1)  # Ensure correct dims here
                            },
                        name="ZS")
    
    ###############################################################################
    #------------------------------------------------------------------------------
    # Create the reduced Dataset
    #------------------------------------------------------------------------------
    ###############################################################################
    ### MODEL 2
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
        "ZS":alt_da})
    ### MODEL 1
    dsnewM1 = xr.Dataset({
        "WildfireName": name_da,
        "Time": time_da,
        "Z_FirePlumeBottom": zplumebottom_da,
        "Z_FirePlumeTop": zplumetop_da,
        "ZS":altM1_da})
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
    
    dsnew["ZS"].attrs["standard_name"] = "surface_altitude"
    dsnew["ZS"].attrs["units"] = "m"
    dsnew["ZS"].attrs["comment"] = "surface altitude"
    # dsnew["lon_point"].attrs["units"] = "degrees_east"
    # dsnew["lon_point"].attrs["comment"] = "longitudes of fire front(10 m resolution) points"
    #------------------------------------------------------------------------------
    # MODEL 1 attributes
    
    dsnewM1["Z_FirePlumeBottom"].attrs["units"] = "m"
    dsnewM1["Z_FirePlumeBottom"].attrs["comment"] = "altitude of the lower boundary of the fireplume in meters"
    
    dsnewM1["Z_FirePlumeTop"].attrs["units"] = "m"
    dsnewM1["Z_FirePlumeTop"].attrs["comment"] = "altitude of the upper boundary of the fireplume in meters"
    
    dsnewM1["ZS"].attrs["standard_name"] = "surface_altitude"
    dsnewM1["ZS"].attrs["units"] = "m"
    dsnewM1["ZS"].attrs["comment"] = "surface altitude"
    
    dsnewM1["latitude"].attrs["units"] = "degrees_north"
    dsnewM1["latitude"].attrs["comment"] = "latitudes of the 800 resolution, large scale domain"
    dsnewM1["longitude"].attrs["units"] = "degrees_east"
    dsnewM1["longitude"].attrs["comment"] = "longitudes of the 800 resolution, large scale domain"
    ###############################################################################
    # -----------------------------------------------------------------------------
    # Save to a NetCDF file
    # -----------------------------------------------------------------------------
    ###############################################################################
    
    
    # ds.close()
    # dsM1.close()
    # Write the reduced NetCDF file
    

    dsnew.to_netcdf(savepath+f"{nameout}_small.nc")
    print(f'NetCDF file {savepath}/{nameout}_small.nc created successfully.')
    dsnewM1.to_netcdf(savepath+f"{nameout}_large.nc")
    print(f'NetCDF file {savepath}/{nameout}_large.nc created successfully.')
    
def findCSEGandCEXP(casepath, dirmesnhfilesMNH, nMod ):
    
    """
    Given the output directory of MNH for Model nMod, returns the latest time step, CSEG and CEXP
    CSEG and CEXP are the strings which compose a MesNH output file (CEXP.nMod.CSEG.000.nc)
    Parameters:
    -----------
    casepath=str, main directory with the MNH-Forefire simulation"
            example="/Users/tiziocaio/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
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
    print(formatted_numbers)
    formatted_numbers=formatted_numbers[1:]
    print(f"There are {len(formatted_numbers)} time steps available")
    return max(formatted_numbers),mnhcexp, mnhcseg
    
def findSmokeScaleFactor(casepath,paramsfile="ForeFire/Params.ff"):
    """
    casepath=str, main directory with the MNH-Forefire simulation
            example="/Users/tiziocaio/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
    paramsfile: path (relative to casepath) to the Forefire params file
    -------
    smokescalefator (float)

    """
    #Find the perimeter resolution:
    ffsettings=casepath+paramsfile    
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
    bratio_for_msquared=burning_pow*burning_time #Watt s m-2 => j m-2 #SVFF001 corresponding to a buened m2
    biomasskg=bratio_for_msquared/(18*10**6)#Bratio which corresponds to a kg of biomass burned (kg m-2)
    pm25kgform2=biomasskg*(0.015)  #(kg m-2)
    bratio_for_msquared=burning_time #(kg m-2) by costruction
    smoke_scalefactor=pm25kgform2/bratio_for_msquared
    print(f"smoke_scalefactor is {smoke_scalefactor}")
    return smoke_scalefactor


def Tstep2MNHnc(timestep,casepath,dirmesnhfiles,nMod):
    """
    Outputs the filenmae corresp to time step timestep of model nMod
    timestep = int, timestep (if none the last timestep is taken)
    casepath = str, main directory with the MNH-Forefire simulation
            example="/Users/tiziocaio/scratch/KTEST_PEDROGAO/001_2FIRES/006_Runff/"
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
    filenameM2: str, path to MNH file
    first_day_date str, date of ignition in format  DDMMYYYY
    namefire : name of wildfire case
    Returns
    -------
    a DataArray with the varaibles to be plotted

    """

    # print(mnhfile)
    ds=xr.load_dataset(filenameM2,engine="netcdf4")
    # print(ds.v)
    # plt.imshow(ds['ZS'][:,:])
    # plt.show()
    # plt.imshow(ds["SVFF001"][0,1,:,:])
    # plt.show()
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
    
    # plt.imshow(u_vertex_surf[:,:])
    # plt.show()
    # plt.imshow(v_vertex_surf[:,:])
    # plt.show()
    # plt.imshow(w_vertex_surf[:,:])
    # plt.show()
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
    # plt.imshow(smoke_vertex_ground)
    # plt.show()
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
    w_up=np.where(smoke_vertex[:level,:,:] >= thr, w_up, 0)
    w_up_th=np.where(w_up>=3,w_up,0)
    # Compute the max along the vertical (first) axis
    w_up_2D = np.max(w_up_th, axis=0)

    # -----------------------------------------------------------------------------
    #                               TKE m2 s-2 (or j/kg)
    # -----------------------------------------------------------------------------
    # Take TKE above 1.5 m2s-2 in the first ~1000 m above the ground
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
    tke_vertex=np.where(smoke_vertex[:level,:,:]>=thr,tke_vertex,0)
    #keep only tke > 1.5 m2 s-2  ( 3 m2 s-2  is indicative of highly turbolent boundary layer Heilman Bian 2010)
    tke_vertex_th=np.where(tke_vertex>=1.5,tke_vertex,0)
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
    # print(f"z_vertex is {z_vertex}")

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
    # altitude_plume_bottom_WT_list.append(z_plumebottom_flat)
    # plt.imshow(z_plumebottom_flat)
    # # plt.imshow(smokeground_thr)
    # plt.show()
    # -----------------------------------------------------------------------------
    #                          Plume Top
    # -----------------------------------------------------------------------------
    # 
    #------------------------------------------------------------------------------
    
    #   Store altitude corresponding to plume top
    z_plumetop=np.where(smoke>=thr,altitude_3D_array,np.min(altitude_3D_array))
    z_plumetop_flat = np.max(z_plumetop, axis=0)    
    plt.imshow(z_plumetop_flat)
    plt.show()
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
    #------------------------------------------------------------------------------
    dsnewM1 = xr.Dataset({
        "WildfireName": name_da,
        "Time": datetime_str,
        "Z_FirePlumeBottom": zplumebottom_da,
        "Z_FirePlumeTop": zplumetop_da})
    #------------------------------------------------------------------------------
    # MODEL 1 attributes
    
    dsnewM1["Z_FirePlumeBottom"].attrs["units"] = "m"
    dsnewM1["Z_FirePlumeBottom"].attrs["comment"] = "altitude of the lower boundary of the fireplume in meters"
    
    dsnewM1["Z_FirePlumeTop"].attrs["units"] = "m"
    dsnewM1["Z_FirePlumeTop"].attrs["comment"] = "altitude of the upper boundary of the fireplume in meters"
    
    
    dsnewM1["latitudeM1"].attrs["units"] = "degrees_north"
    dsnewM1["latitudeM1"].attrs["comment"] = "latitudes of the 800 resolution, large scale domain"
    dsnewM1["longitudeM1"].attrs["units"] = "degrees_east"
    dsnewM1["longitudeM1"].attrs["comment"] = "longitudes of the 800 resolution, large scale domain"
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
    print(f'firstday: {first_day},  day : {day}  and secs : {seconds_since_midnight}')
    ##-----------------------------------------------------------------------------
    #Find the perimeter resolution:
    #ffsettings=casepath+paramsfile    
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
    print("lon_high_res_grid.shape",lon_high_res_grid.shape)
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
    print(f"lat_high_res_interp {lat_high_res_interp}")
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
    fire_velocity=np.divide(1,sqrdiv)

    fire_velocity=np.where(fire_velocity==np.nan,0.0,fire_velocity)
    fire_velocity=np.where(fire_velocity==np.inf,0.0,fire_velocity)
    print(f"fire_velocity min max: {np.amin(fire_velocity)}  {np.amax(fire_velocity)}")
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
    
    
    # if the fire has already started
    if slices: 
    # Extract the boundary of the largest region (if there are multiple regions)
        largest_region = slices[0]  # Assuming the first region is the largest
    ## if the fire has noot started yet take the ignition point
        # if flagfirststep:
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
########################################################
    boundary = ndimage.binary_dilation(boundary_mask) ^ boundary_mask  # Get the boundary (fire front)
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
    
    # Convert the dictionary to a DataArray
    # times = list(points.keys())

    
    # Find the maximum number of points
    print(f"shape of lat lon boundary: {len(lat_boundary)}  {len(lon_boundary)}")
    max_points =len(lat_boundary)
    
    # Create arrays with NaNs to hold the data
    lat_data_array = np.full((max_points), np.nan)
    lon_data_array = np.full((max_points), np.nan)
    coords_points_ROS=np.full(( max_points,2), np.nan)
    magnitude_array = np.full((max_points), np.nan)
    
    # Fill the data arrays with actual points and magnitudes
    n_points = len(lat_boundary)
    lat_data_array[ :n_points] = lat_boundary
    lon_data_array[ :n_points] = lon_boundary
    coords_points_ROS[:n_points, 0] =lat_boundary
    coords_points_ROS[:n_points, 1] =lon_boundary
    magnitude_array[:n_points] = normal_magnitudes
    
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
        coords={'points': range(max_points), 'coord_points': ['lat_points', 'lon_points']}
    )
    
    # Create the DataArray for magnitudes
    magnitudes_da = xr.DataArray(
        magnitude_array,
        dims=[ 'points'],
        coords={'points': range(max_points)}
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
    time_range = pd.date_range(start=ignitiondate, end=enddate, freq=f'{N}T')
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
        #print(f'firstday: {first_day},  day : {day}  and secs : {seconds_since_midnight}')
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
        fire_velocity=np.divide(1,sqrdiv)

        fire_velocity=np.where(fire_velocity==np.nan,0.0,fire_velocity)
        fire_velocity=np.where(fire_velocity==np.inf,0.0,fire_velocity)
        # print(f"fire_velocity min max: {np.amin(fire_velocity)}  {np.amax(fire_velocity)}")
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
        
        
        # if the fire has already started
        if slices: 
        # Extract the boundary of the largest region (if there are multiple regions)
            largest_region = slices[0]  # Assuming the first region is the largest
        ## if the fire has noot started yet take the ignition point
            # if flagfirststep:
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
    ########################################################
        boundary = ndimage.binary_dilation(boundary_mask) ^ boundary_mask  # Get the boundary (fire front)
        # Find the coordinates of the boundary points
        boundary_coords = np.argwhere(boundary)

        # Create an array to store the normal gradients
        
        # Calculate the magnitude of each normal vector

        for coord in boundary_coords:
            y, x = coord
            normal_y, normal_x = compute_normal(gradient_y[y, x], gradient_x[y, x])
            normal_vectors.append((normal_y, normal_x))
            normal_magnitudes.append(fire_velocity[y,x])
        # print(f"max ROS {np.amax(normal_magnitudes)}")   
        BurnedA.append(total_area)
        maxRos.append(np.amax(normal_magnitudes))
        timestepsN.append(timestamp)
    return BurnedA,maxRos,timestepsN
    
    
def BuildDS4AllTimes(timesteps,casepath,mnfiles,pathBMAP,pathffParams,smoke_scalefactor,ignitiondate,namefire,savepath):
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
        smoke_scalefactor=findSmokeScaleFactor(casepath)
        ds=BuildDSfromAllSources(casepath+mnhFilenameM1,casepath+mnhFilenameM2,pathBMAP,pathffParams,
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
    listnoExp = ['WildfireName']
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
    combined_ds['IgnitionTime']=np.datetime64(datetime.strptime(ignitiondate, "%Y-%m-%dT%H:%M:%S").isoformat())
    ignitionlat=np.mean(combined_ds.Ignition.coords["lat_bmap"].values[~np.isnan(combined_ds.Ignition.values)])
    ignitionlon=np.mean(combined_ds.Ignition.coords["lon_bmap"].values[~np.isnan(combined_ds.Ignition.values)])
    combined_ds['IgnitionCoords']=[ignitionlat,ignitionlon]
    combined_ds["IgnitionCoords"].attrs["units"] = "degrees_north, degrees_east"
    print("check ignition time",combined_ds['IgnitionTime'].values)
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
    # print(combined_ds.data_vars)
    # print(combined_ds["Time"].values)
    # print(maxRos)
    # print(maxPlumeHeight)
    combined_ds.to_netcdf(savepath+f"{namefire}_small.nc")
    print(f'NetCDF file {savepath}/{namefire}_small.nc created successfully.')
    return combined_ds

# casepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/newtestcases/005_runFIRE/"
# mnfiles=""
# savepath="/Users/baggio_r/Documents/DocUbuntu/FIRERES/reports/newtestcases/"
# smoke_scalefactor=findSmokeScaleFactor(casepath)
# pathBMAP=casepath+"/ForeFire/Outputs/ForeFire.0.nc"
# pathffParams=casepath+"/ForeFire/Params.ff"
# list_nc=list_NetCDFfiles_in_directory(casepath+mnfiles)
# formatted_numbers = [s[-6:-3] for s in list_nc]
# # Sort the list of strings based on the extracted numbers
# formatted_numbers = sorted(formatted_numbers, key=extract_number)
# formatted_numbers =[int(s) for s in formatted_numbers]
# timesteps=formatted_numbers[:5] 
# ignition="2017-06-17T13:30:00"
# endfire="2017-06-17T23:30:00"
# N=6
# ExtractROSeveryNmin(N,ignition,endfire,pathBMAP,pathffParams,namefire=None)


