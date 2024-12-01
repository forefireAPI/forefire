import numpy as np 
import matplotlib.pyplot as plt 
import xarray as xr
import rioxarray 
import pyforefire as forefire
import os 
import sys
import argparse
import importlib
#import ffToGeoJson
import pyproj
#homebrewed
import create_data_tools as tools
from ffToGeo import FFGEO

from forefire_helper import *

###################################################
def getDomainExtent(line):
    print(line)
    llv = line.split("sw=(")
    llr = llv[1].split("ne=(");
    return( float( llr[0].split(",")[0]), float(llr[1].split(",")[0]), float(llr[0].split(",")[1]), float(llr[1].split(",")[1]) 
           )
###################################################
if __name__ == '__main__':
###################################################
    
    importlib.reload(tools)
    print('check env variable:')
    print('ntask =', os.environ['ntask'])
    

    #create dir
    tools.ensure_dir('./Inputs/')
    tools.ensure_dir('./ForeFire/')
    tools.ensure_dir('./ForeFire/Outputs/')

    ################### ... run forefire 

    ff = forefire.ForeFire()
    outputsUpdate    = 10
    mydatanc = './Inputs/mydata.nc'
    spatialIncrement = 2 
    perimeterResolution = 10 
    minimalPropagativeFrontDepth = 10
    bmapOutputUpdate = 10
    InitTime = 0

    #set param
    ff.setString('ForeFireDataDirectory','Inputs')
    ff.setString('fireOutputDirectory','ForeFire/Outputs')
    ff.setInt('outputsUpdate',outputsUpdate)
    
    ff.setString('fuelsTableFile','fuels.ff') # end dur
    ff["propagationModel"]="Andrews"

    ff.setDouble("spatialIncrement",spatialIncrement)
    ff.setDouble("perimeterResolution",perimeterResolution)
    ff.setDouble("minimalPropagativeFrontDepth",minimalPropagativeFrontDepth)

    ff.setDouble("bmapOutputUpdate",bmapOutputUpdate)
    ff.setInt("defaultHeatType",0)
    ff.setInt("defaultVaporType",1)

    ff.setDouble("InitTime",InitTime)
    ff.execute("loadData[mydata.nc;2023-07-08T10:00:00Z]")
    ff.execute("startFire[loc=(60390,76090,0.);t=0]")
    
    print(ff.execute("print[]"))

    ff.setString('dumpMode','FF')
    
    N_step = 100
    step = 20 
    t0 = 0
    pathes = []
    ffgeo = FFGEO('pdv',4326)
    ffgeo.addFront(ff)

    for i in np.arange(1,N_step):
        
        ff_time = t0 + i*step

        print("goTo[t=%f]"%(ff_time), end='\n')
        ff.execute("goTo[t=%f]"%(ff_time))
        
        pathes  += printToPathe(ff.execute("print[]"))
        #print(pathes)
        ffgeo.addFront(ff)
   
    
    #plot 
    mydata = xr.open_dataset(mydatanc)
    xx = np.linspace(mydata.domain.attrs['SWx'], mydata.domain.attrs['NEx'], mydata.sizes['DIMX'])
    yy = np.linspace(mydata.domain.attrs['SWy'], mydata.domain.attrs['NEy'], mydata.sizes['DIMY'])
    ax = plt.subplot()
    ax.pcolormesh(xx,yy,mydata.fuel[0,0])
    ffgeo.pd.plot(ax=ax,facecolor='None')
    plt.show()
