#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Jun 22 18:36:51 2023

@author: filippi_j
"""



import geopandas as gpd
import contextily as cx

import xarray as xr
import numpy as np
import os.path
from fiona.crs import from_epsg
import matplotlib.pyplot as plt
from shapely.geometry import box
import rasterio
from rasterio.mask import mask
import pycrs



pgdFile = "/Users/filippi_j/soft/firefront/Examples/villa/DB05.nc"




#fireFile = "/Users/filippi_j/soft/firefront/Examples/villa/villa/progression_ViladeRei_200719.shp"
pgdFile = "/Users/filippi_j/Volumes/orsu/firecaster/2023/nest150Ref/001_pgd/PGD_D2000mA.nc"
outBG = "/Users/filippi_j/data/2023/prunelli/pignaV.tif"
out_tif = "/Users/filippi_j/data/2023/prunelli/pignaBGM1DOMCUT.tif";




#pgdFile = "/Users/filippi_j/Volumes/fcouto/KDATABASE/KTEST_Pedrogao/KTEST/KTEST_2nest/001_pgd/PGD_D160mA.nc"
#pgdFile = "/Users/filippi_j/Volumes/fcouto/KDATABASE/KTEST_Pedrogao/KTEST/KTEST_1nest/001_pgd/PGD_D120mA.nc"
pgdFile = "/Users/filippi_j/Volumes/fcouto/KDATABASE/KTEST_Pedrogao/KTEST/KTEST_3nest/001_pgd/PGD_D80mA.nc"

outBG = "/Users/filippi_j/data/2023/baseFeux/pedrogao/160m.tif"
out_tif = "/Users/filippi_j/data/2023/baseFeux/pedrogao/160mDOMCUT.tif"



def wsenFromPgd(pgdFile):
    pgd = xr.load_dataset(pgdFile)
    return (
        float(pgd.longitude.min()),
        float(pgd.latitude.min()),
        float(pgd.longitude.max()),
        float(pgd.latitude.max())
                 )
 

#DeltaY = float(pgd.YHAT[1]-pgd.YHAT[0])
#DeltaX = float(pgd.XHAT[1]-pgd.XHAT[0])

#SWx =  float(pgd.XHAT[0])
#SWy =  float(pgd.YHAT[0])
#Nex =  float(pgd.XHAT[-1]+DeltaX)
#Ney =  float(pgd.YHAT[-1]+DeltaY)

#Lx = float( Nex-SWx)
#Ly =  float(Ney-SWy)
 
lat_sw, lon_sw = 42.0063067 , 9.3263082
lat_ne, lon_ne = 42.0104249 ,  9.3327945

west, south, east, north = lon_sw,lat_sw, lon_ne,lat_ne

west, south, east, north = wsenFromPgd(pgdFile)

#fig, ax = plt.subplots(figsize=(8, 8))
#ax.axis(bounds)
#cx.add_basemap(ax)

def makeBGFile(west, south, east, north , outF):
    
    cx.bounds2raster(west, south, east, north ,
                         ll=True,
                         path=outF,
                                         zoom=12,
                                         source=cx.providers.GeoportailFrance.orthos
     
                        )
    
    
makeBGFile(west, south, east, north, outBG)
    
data = rasterio.open(outBG) 

bbox = box(west, south, east, north)

geo = gpd.GeoDataFrame({'geometry': bbox}, index=[0], crs=from_epsg(4326))
geo = geo.to_crs(crs=data.crs.data)

def getFeatures(gdf):
    """Function to parse features from GeoDataFrame in such a manner that rasterio wants them"""
    import json
    return [json.loads(gdf.to_json())['features'][0]['geometry']]

coords = getFeatures(geo)
print(coords)


out_img, out_transform = mask(data, shapes=coords, crop=True)
epsg_code = int(data.crs.data['init'][5:])
out_meta = data.meta.copy()
print(out_meta)


out_meta.update({"driver": "GTiff",
                    "height": out_img.shape[1],
                    "width": out_img.shape[2],
                     "transform": out_transform,
                    "crs": pycrs.parse.from_epsg_code(epsg_code).to_proj4()}
                           )

with rasterio.open(out_tif, "w", **out_meta) as dest:
    dest.write(out_img)

#f, ax = plt.subplots(1, figsize=(9, 9))

#df = geopandas.read_file(fireFile)  

#df_wm = df.to_crs(epsg=3857)
#ax.imshow(ghent_img, extent=ghent_ext)

#ax = df_wm.plot(figsize=(10, 10), alpha=0.5, edgecolor="k")




#ax.axis(bounds)



#cx.add_basemap(ax, source=cx.providers.GeoportailFrance.orthos, zoom=12)
#cx.add_basemap(ax, source=cx.providers.AzureMaps.MicrosoftImagery, zoom=12)


# read domain properties
# once i have area, make a cut of BG
# once i Have that open the 
