from __future__ import print_function
from __future__ import division
from builtins import zip
from builtins import range
from past.utils import old_div
import sys
import os
import numpy as np
import matplotlib as mpl
mpl.use('Qt5Agg')
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import pdb 
#import imp 
from scipy import io
import glob
import netCDF4
import f90nml
import datetime
import socket 
import warnings
warnings.filterwarnings("ignore",".*GUI is implemented.*")
from mpl_toolkits.axes_grid1 import make_axes_locatable
import pickle 
import argparse
from osgeo import gdal,osr,ogr
from scipy import interpolate 
import importlib 
import pandas
import math 

##################################################
def ensure_dir(f):
    d = os.path.dirname(f)
    if not os.path.exists(d):
        os.makedirs(d)


##################################################
def read_MNH_domain( expr_name, dir_mnh = '../02_mnh/', domainNumber=None, specificInputMNHFile=None):

    if specificInputMNHFile == None:
        expr_numb = '.{:d}.'.format(domainNumber) if (domainNumber is not None) else ''
        outfiles_mnh = sorted(list(set(glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.nc'))-\
                                   set(glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.OUT.*.nc')))+\
                              glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.nc4'))
        #print(outfiles_mnh)
        if len(outfiles_mnh) == 0:
            print('missing mnh files')
            pdb.set_trace()
        outfiles_mnh_inputTime = outfiles_mnh[0]
        outfiles_mnh_inputGrid = outfiles_mnh[1]
    
    else:
        outfiles_mnh_inputGrid = specificInputMNHFile 
        outfiles_mnh_inputTime = specificInputMNHFile 

    print('get time ref from :', outfiles_mnh_inputTime)
    print('get grid from     :', outfiles_mnh_inputGrid)


    #read geometry of the MNH domain
    nc = netCDF4.Dataset(outfiles_mnh_inputGrid,'r') # get the 001 file
    
    MNH_properties= {}
    
    MNH_properties['lat00'] = np.array(nc.variables['LAT'])[0,0]
    MNH_properties['lon00'] = np.array(nc.variables['LON'])[0,0]
    MNH_properties['lat2d'] = np.array(nc.variables['LAT'])
    MNH_properties['lon2d'] = np.array(nc.variables['LON'])

    DeltaY = nc.variables['YHAT'][1]-nc.variables['YHAT'][0]
    DeltaX = nc.variables['XHAT'][1]-nc.variables['XHAT'][0]
   
    MNH_properties['nx']  = len(nc.dimensions[u'ni'])
    MNH_properties['ny']  = len(nc.dimensions[u'nj'])
    MNH_properties['nz']  = len(nc.dimensions[u'level_w'])

    MNH_properties['SWx']  = nc.variables['XHAT'][0]+0.
    MNH_properties['SWy']  = nc.variables['YHAT'][0]+0.
    MNH_properties['SWz']  = 0
    MNH_properties['NEx']  = nc.variables['XHAT'][-1]+DeltaX
    MNH_properties['NEy']  = nc.variables['YHAT'][-1]+DeltaY
    MNH_properties['NEz']  = 0

    MNH_properties['Lx']   = float(nc.variables['XHAT'][-1]+DeltaX-nc.variables['XHAT'][0])
    MNH_properties['Ly']   = float(nc.variables['YHAT'][-1]+DeltaY-nc.variables['YHAT'][0])
    MNH_properties['Lz']   = 0
    nc.close()

    nc = netCDF4.Dataset(outfiles_mnh_inputTime,'r')
    MNH_properties['date']   = '_'.join(nc.variables['DTCUR'].units.split(' ')[2:4]) 
         
    
    MNH_properties['t0']   = float(nc.variables['DTCUR'][0])
    MNH_properties['Lt']   = np.Inf
    
    
    nc.close()
   

    return MNH_properties

#######################################
def offset_in_degrees(distance_m, lat):
    """
    Convert a distance in meters to an offset in degrees for both latitude and longitude.
    
    Parameters:
        distance_m (float): Distance in meters.
        lat (float): Latitude in degrees.

    Returns:
        lat_offset (float): Latitude offset in degrees.
        lon_offset (float): Longitude offset in degrees.
    """
    # Earth's radius approximation for latitude degrees
    lat_offset = distance_m / 111320

    # Longitude offset depends on latitude
    lon_offset = distance_m / (111320 * math.cos(math.radians(lat)))

    return lat_offset, lon_offset

