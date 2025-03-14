import numpy as np 
import matplotlib as mpl
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
import glob
import rasterio
from skimage import exposure
import cv2
import pdb 

#homebrewed
import create_data_tools as tools
from ffToGeo import FFGEO
from satpy import Scene
from satpy.writers import get_enhanced_image

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
    tools.ensure_dir('./ForeFire/')
    tools.ensure_dir('./ForeFire/Outputs/')

    mydatanc = './ForeFire/mydata_ll.nc'
    s2filedir = '/data/paugam/Data/ElPontDeVilamora/Sentinel/S2/S2A_MSIL1C_20220716T103641_N0400_R008_T31TDG_20220716T155924.SAFE/'

    #load fuelmap
    #-----------
    mydata = xr.open_dataset(mydatanc)
    xxmnh = np.linspace(mydata.domain.attrs['SWx'], mydata.domain.attrs['NEx'], mydata.sizes['DIMX'])
    yymnh = np.linspace(mydata.domain.attrs['SWy'], mydata.domain.attrs['NEy'], mydata.sizes['DIMY'])
    
    #fuelmap_f = xr.DataArray(
    #                    mydata.fuel[0,0],
    #                    coords={"x": xx, "y": yy},
    #                    dims=["y", "x"],  # Order of dimensions matches the data array shape
    #                )
    fuelmap_ll = mydata.fuel[0,0,:,:]
    #fuelmap_f.name = 'fuel map'
    #fuelmap_f = fuelmap_f.rio.write_crs('epsg:{:d}'.format(mydata.domain.attrs['epsg']))
    #fuelmap_ll = fuelmap_f.rio.reproject(4326)
    
    '''
    N = len(np.unique(fuelmap_ll.values)) -1  # Number of colors
    colors = np.random.rand(N, 3) #plt.cm.hsv(np.linspace(0, 1, N))
    cmap_fm = mcolors.ListedColormap(colors)
    colors[0,:] = [1,0,0]

    # Define the boundaries of each color segment
    bounds = np.unique(fuelmap_ll.values)[1:]  # from 1 to 10
    bounds = np.append(bounds, 9999)
    norm_fm = mcolors.BoundaryNorm(bounds, cmap_fm.N)

    #set no data to not be ploted
    nodata_value = -9999
    fuelmap_ll = fuelmap_ll.where(fuelmap_ll != nodata_value)
    nx,ny = fuelmap_ll.shape
    #plot fuelmap
    mpl.rcParams['figure.subplot.left'] = .0
    mpl.rcParams['figure.subplot.right'] = 1.
    mpl.rcParams['figure.subplot.top'] = 1.
    mpl.rcParams['figure.subplot.bottom'] = .0
    fig = plt.figure(figsize=(10,10*ny/nx))
    fuelmap_ll.plot(cmap=cmap_fm,norm=norm_fm,add_colorbar=False)

    fig.savefig('fuelmap.png',dpi=200)
    plt.close(fig)

    #load RGB sentinel2
    ###################
    postfire_rgb_path = s2filedir+'preFireRGB-s2.tif'
    if ~os.path.isfile(postfire_rgb_path):
        # Open b2, b3 and b4
        band2=rasterio.open(glob.glob(s2filedir+'GRANULE/L1C_T31TDG_A036899_20220716T104613/IMG_DATA/'+'*B02.jp2')[0])
        band3=rasterio.open(glob.glob(s2filedir+'GRANULE/L1C_T31TDG_A036899_20220716T104613/IMG_DATA/'+'*B03.jp2')[0])
        band4=rasterio.open(glob.glob(s2filedir+'GRANULE/L1C_T31TDG_A036899_20220716T104613/IMG_DATA/'+'*B04.jp2')[0])

        # Extract and update the metadata
        meta = band2.meta
        meta.update({"count": 3})

        # Write the natural colour composite image with metadata
        postfire_rgb_path = s2filedir+'preFireRGB-s2.tif'

        with rasterio.open(postfire_rgb_path, 'w', **meta) as dest:
            dest.write(band4.read(1),1)
            dest.write(band3.read(1),2)
            dest.write(band2.read(1),3)
    
    # Transpose and rescale the image 
    s2 = rioxarray.open_rasterio(postfire_rgb_path)
    s2 = s2.rio.reproject(fuelmap_ll.rio.crs)
    s2 =s2.rename({'x':'DIMX'})
    s2 =s2.rename({'y':'DIMY'})
    s2_pgd3 = s2.interp(DIMX=fuelmap_ll.DIMX, DIMY=fuelmap_ll.DIMY)    

    # Assuming 'image' is your 3-band RGB image
    def equalize_histogram(image):
        # Split the image into its color channels
        b, g, r = cv2.split(image)

        # Apply histogram equalization on each channel
        b_eq = cv2.equalizeHist((255*b).astype(np.uint8))
        g_eq = cv2.equalizeHist((255*g).astype(np.uint8))
        r_eq = cv2.equalizeHist((255*r).astype(np.uint8))

        # Merge the equalized channels back
        image_eq = cv2.merge((b_eq, g_eq, r_eq))
        
        return image_eq

    ir = 0
    ig = 1
    ib = 2
    #image = np.array([s2_pgd3[0],s2_pgd3[1],s2_pgd3[2]]).transpose(1,2,0)
    image = np.array([s2_pgd3[ir][::-1,:]/s2_pgd3[ir].max(),
                      s2_pgd3[ig][::-1,:]/s2_pgd3[ig].max(),
                      s2_pgd3[ib][::-1,:]/s2_pgd3[ib].max()]
                     ).transpose(1,2,0)
    
    #p2, p98 = np.percentile(image, (2, 98))
    #image_contrast_stretched = np.clip((image - p2) / (p98 - p2), 0, 1)

    # Equalize the histogram of the image
    image_equalized = equalize_histogram(image)

    # Plot the resulting image
    ny,nx,_ = image_equalized.shape
    fig = plt.figure(figsize=(10,10*ny/nx))
    plt.imshow(np.transpose(image_equalized,[1,0,2])) 
    fig.savefig('s2RGB.png',dpi=200)
    plt.close(fig)
    '''
    ################### ... run forefire 

    ff = forefire.ForeFire()
    outputsUpdate    = 10
    spatialIncrement = 2 
    perimeterResolution = 10 
    minimalPropagativeFrontDepth = 10
    bmapOutputUpdate = 10
    InitTime = 0

    #set param
    ff.setString('ForeFireDataDirectory','ForeFire')
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
    ff.execute("loadData[mydata.nc;2022-07-16T00:00:00Z]")
    #ff.execute("startFire[loc=(60390,76090,0.);t=0]")
    ff.execute("startFire[loc=(59601.3,77044.1,0.);t=39600]")
    print(ff.execute("print[]"))

    ff.setString('dumpMode','FF')
    
    N_step = 10
    step = 20
    t0 = 39000
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
   
    
    #fig = plt.figure()
    #ax = plt.subplot(111)
    #ax.pcolormesh(xx,yy,mydata.fuel[0,0])
    
    plt.figure()
    ax = plt.subplot()
    ax.pcolormesh(xxmnh,yymnh,mydata.fuel[0,0])
    ffgeo.pd.plot(ax=ax,facecolor='None')
    plt.show()
