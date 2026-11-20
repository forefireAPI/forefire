#!/usr/bin/python
# Copyright (C) 2012
# Author(s): Jean Baptiste Filippi, Vivien  Mallet
#
# This file is part of pyFireScore, a tool for scoring wildfire simulation
#
# pyFireScore is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# pyFireScore is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.

import numpy as np
import sys
import xarray as xr
import netCDF4 as nc4
import matplotlib.pyplot as plt
from PIL import Image
from scipy.spatial import distance


def forceDim(arrIn, newD=None, kind='edge'):
    if(newD==None):
        return arrIn
   
    if (newD[0][1] < 0):
        newr = arrIn[newD[0][0]:newD[0][1],newD[1][0]:newD[1][1]]
        print("new shape ",np.shape(newr))
        return(newr)
    

    return np.pad(arrIn, newD, kind)



#if(len(sys.argv)==1):
#    print "Usage case2init inMNH_PGD_File.nc lat lon seconds"
#    exit(0)


if(len(sys.argv)<5):
    print("Usage PGD2Init.py inMNH_PGD_File.nc lat lon startTimeInseconds")
    sys.exit(0)
   
    
#fname = sys.argv[1] #"'/Users/filippi_j/Volumes/fcouto/KDATABASE/DB59/001_pgd/PGD_D80mDB59A.nested.nc'"
fname = sys.argv[1]

#P = (39.7556,-8.1736)

P = (float(sys.argv[2]),float(sys.argv[3]))
seconds = int(sys.argv[4])
geomCDFBin = fname 
nc = nc4.Dataset(geomCDFBin, 'r') 
DeltaY = nc['YHAT'][1]-nc['YHAT'][0]
DeltaX = nc['XHAT'][1]-nc['XHAT'][0]


closest = []
# now find 4 closest points....
lats = nc["latitude"]
lons = nc["longitude"]
sh = np.shape(lats)

sDist = distance.euclidean((lats[-1,-1], lons[-1,-1]),(lats[-3,-3], lons[-3,-3]) ) 

for i in range(sh[1]):
    for j in range(sh[0]):
       
        dst = distance.euclidean((lats[j,i], lons[j,i]),P)
        if dst < sDist:
            closest.insert(0,[dst,j,i])
            sDist=dst 


dom= "FireDomain[sw=(%d,%d,0);ne=(%d,%d,0);t=%d]"%(nc['XHAT'][0],nc['YHAT'][0],nc['XHAT'][-1]+DeltaX,nc['YHAT'][-1]+DeltaY,seconds)
dom+= "\nstartFire[loc=(%d,%d,0.);t=%d]"%( nc['XHAT'][closest[0][2]],nc['YHAT'][closest[0][1]],seconds)  

print(dom)

