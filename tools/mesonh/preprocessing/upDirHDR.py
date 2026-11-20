#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Jan 14 19:25:06 2024

@author: filippi_j
"""
# A file to upgrade DIR/HDR files in order to stop having Halo problems in MesoNH

import numpy as np
import matplotlib.pyplot as plt
import rasterio
from rasterio.merge import merge
from rasterio.warp import calculate_default_transform, reproject, Resampling
import math
import os
import xarray as xr
import scipy.ndimage
import argparse

def load_and_plot_data(file_path, rows, cols):
    # Charger les données du fichier
    data = np.fromfile(file_path, dtype=np.int16)  # Assurez-vous que le dtype correspond au type de données
    data = data.reshape((rows, cols))  # Remodeler les données selon les dimensions fournies

    # Gérer les valeurs 'nodata'
    data = np.where(data == -9999, np.nan, data)  # Remplacer -9999 par NaN

    # Afficher les données
    plt.imshow(data, cmap='terrain')  # Choisir une colormap appropriée
    plt.colorbar(label='Altitude')
    plt.title('Visualisation du Fichier .dir')
    plt.show()

def get_SRTMDB_File_List(north=43.3, south=41.1, east=9.8, west=8.1, srtm_path="/path/to/srtm/data/", nodata_value=-32768):
    required_files = []
    for lat in range(math.floor(south), math.ceil(north) + 1):
        for lon in range(math.floor(west), math.ceil(east) + 1):
            lat_prefix = 'N' if lat >= 0 else 'S'
            lon_prefix = 'E' if lon >= 0 else 'W'
            filename = f"{lat_prefix}{abs(lat):02d}{lon_prefix}{abs(lon):03d}.tif"
            filepath = os.path.join(srtm_path, filename)
            if os.path.isfile(filepath):
                print(f"found {filepath} ")
                src = rasterio.open(filepath, nodata=nodata_value)
                required_files.append(src)
            else:
                print(f"#File not found: {filepath}" )

    if not required_files:
        print("No files were found for the specified area.")
        return None
    return required_files

def get_elevation_raster_from_srtm_NSEW(north=43.3, south=41.1, east=9.8, west=8.1, ni=None, nj=None, srtm_path="/path/to/srtm/data/", nodata_value=-32768):

    required_files = get_SRTMDB_File_List(north, south, east, west, srtm_path, nodata_value)

    if not required_files:
        print("No files were found for the specified area.")
        return None

    mosaic, out_trans = merge(required_files, bounds=(west, south, east, north), nodata=nodata_value)

    try:
        if ni is None or nj is None:
            # Use native resolution
            src = required_files[0]
            transform, width, height = calculate_default_transform(
                src.crs, src.crs,
                mosaic.shape[2], mosaic.shape[1],
                left=west, bottom=south, right=east, top=north
            )
            ni = width
            nj = height
        else:
            transform, width, height = calculate_default_transform(
                src_crs=required_files[0].crs, dst_crs=required_files[0].crs,
                width=mosaic.shape[2], height=mosaic.shape[1],
                left=west, bottom=south, right=east, top=north,
                dst_width=ni, dst_height=nj
            )
            print(f"Calculated size: width {width} height {height}")
    except Exception as e:
        print(f"Error calculating transform: {e}")
        return None

    final_array = np.empty((height, width), dtype=mosaic.dtype)
    reproject(
        source=mosaic,
        destination=final_array,
        src_transform=out_trans,
        src_crs=required_files[0].crs,
        dst_transform=transform,
        dst_crs=required_files[0].crs,
        resampling=Resampling.bilinear,
        src_nodata=nodata_value,
        dst_nodata=0
    )

    lon_res = (east - west) / ni
    lat_res = (north - south) / nj
    lon = np.linspace(west + lon_res/2, east - lon_res/2, ni)
    lat = np.linspace(south + lat_res/2, north - lat_res/2, nj)

    elevation = xr.DataArray(
        final_array[::-1, :],  # Reverse the array to match the latitude coordinates
        dims=['latitude', 'longitude'],
        coords={
            'latitude': lat,
            'longitude': lon
        },
        attrs={
                'description': 'Raster data from SRTM ',
                'units': 'unknown',  # Modify according to the actual units
                'nodata': nodata_value
        }
    )

    return elevation

def geotiff_to_xarray(filename):

    """ Load a GeoTIFF file as an xarray.DataArray, assuming WGS84 coordinates """
    with rasterio.open(filename) as src:
        data = src.read(1)  # Read the first band; adjust if necessary for multiple bands
        # Get the bounds from the file
        left, bottom, right, top = src.bounds.left, src.bounds.bottom, src.bounds.right, src.bounds.top
        # Calculate the resolution of the raster
        res_lat = (top - bottom) / src.height
        res_lon = (right - left) / src.width
        
        # Generate coordinates based on the bounds and resolution
        lats = np.linspace(bottom + res_lat / 2, top - res_lat / 2, src.height)
        lons = np.linspace(left + res_lon / 2, right - res_lon / 2, src.width)
        
        # Create the xarray DataArray
        da = xr.DataArray(
            np.flipud(data),
            dims=['latitude', 'longitude'],
            coords={
                'latitude': lats,
                'longitude': lons
            },
            attrs={
                'description': 'Raster data from {}'.format(filename),
                'units': 'unknown',  # Modify according to the actual units
                'nodata': src.nodatavals[0]
            }
        )
    return da

def read_hdr(filenamehdr):
    """ Lire le fichier .hdr et extraire les métadonnées """
    metadata = {}
    with open(filenamehdr, 'r') as file:
        for line in file:
            parts = line.strip().split(': ')
            if len(parts) == 2:
                key, value = parts
                metadata[key] = value
    return metadata

def dirHDRtoXarray(filenamedir, filenamehdr):
    """ Charger un fichier .dir en tant que xarray.DataArray """
    metadata = read_hdr(filenamehdr)
    print(metadata)
    rows = int(metadata['rows'])
    cols = int(metadata['cols'])
    data_type = np.int16  # Modifiez cela en fonction du type de données spécifié dans le .hdr

    data = np.fromfile(filenamedir, dtype=data_type).reshape((rows, cols))
    data = np.flipud(data)
    # Créer des coordonnées (exemple simple, ajustez selon vos besoins réels)
    lats = np.linspace(float(metadata['south']), float(metadata['north']), rows)
    lons = np.linspace(float(metadata['west']), float(metadata['east']), cols)

    return xr.DataArray(data, coords=[lats, lons], dims=['latitude', 'longitude'])

def xarrayToDirHdr(xrarray, filenamedir, filenamehdr):
    """ Enregistrer un xarray.DataArray dans un fichier .dir en tant que données 16 bits et créer un fichier .hdr """
    # Convertir les données en int16
    nparray = np.flipud(xrarray.values.astype(np.int16))

    # Enregistrer les données dans un fichier .dir
    nparray.tofile(filenamedir)

    # Créer les métadonnées pour le fichier .hdr
    metadata = {
        'nodata': '-9999',
        'north': str(xrarray.latitude.max().item()),
        'south': str(xrarray.latitude.min().item()),
        'east': str(xrarray.longitude.max().item()),
        'west': str(xrarray.longitude.min().item()),
        'rows': str(xrarray.shape[0]),
        'cols': str(xrarray.shape[1]),
        'recordtype': 'integer 16 bit'
    }

    with open(filenamehdr, 'w') as file:
        file.write("PROCESSED SRTM DATA VERSION 4.1, orography model\n")
        for key, value in metadata.items():
            file.write(f"{key}: {value}\n")

def makeSubset():
    dirin = '/Users/filippi_j/soft/Meso-NH/PGD/srtm_ne_250.dir'
    hdrin = '/Users/filippi_j/soft/Meso-NH/PGD/srtm_ne_250.hdr'
    dirout = '/Users/filippi_j/soft/Meso-NH/PGD/ffDEM.dir'
    hdrout = '/Users/filippi_j/soft/Meso-NH/PGD/ffDEM.hdr'

    # Charger les données
    data_xarray = dirHDRtoXarray(dirin, hdrin)
    print("data loaded")
    # Sélectionner le sous-ensemble
    subset = data_xarray.sel(latitude=slice(28, 60.02), longitude=slice(-15, 40))
    #subset.plot()
    # Augmenter la résolution
    # Le facteur 4 signifie que chaque dimension sera 4 fois plus grande
    zoom_factor = 4
    #resampled_data = scipy.ndimage.zoom(subset, (zoom_factor, zoom_factor), order=3)  # order=3 pour une interpolation cubique
    
    # Convert subset to float
    
    # Create a mask for the -9999 values
    mask = subset == -9999
    
    # Replace -9999 values with NaN using xarray's where method
    data_with_0 = subset.where(~mask, 0)
    
    resampled_data = scipy.ndimage.zoom(data_with_0, (zoom_factor, zoom_factor), order=3)
    print("resampled")
    resampled_mask = scipy.ndimage.zoom(mask, (zoom_factor, zoom_factor), order=0)
    # Reapply the mask to the resampled data
    resampled_data_masked = np.where(resampled_mask, -9999, resampled_data)
    print("masked")
    
    # Créer un nouvel xarray.DataArray avec les nouvelles coordonnées
    new_lats = np.linspace(subset.latitude.min(), subset.latitude.max(), resampled_data_masked.shape[0])
    new_lons = np.linspace(subset.longitude.min(), subset.longitude.max(), resampled_data_masked.shape[1])
    resampled_xarray = xr.DataArray(resampled_data_masked, coords=[new_lats, new_lons], dims=['latitude', 'longitude'])
    
    # Sauvegarder les données
    xarrayToDirHdr(resampled_xarray, dirout, hdrout)

def makeSubsetFromBounds():
    get_elevation_raster_from_srtm_NSEW(north=43.3, south=41.1, east=9.8, west=8.1, ni=1000, nj=1000, srtm_path="/path/to/srtm/data/", nodata_value = -32768 )

def main():
    parser = argparse.ArgumentParser(description="Generate DIR/HDR or NetCDF files from SRTM data.")
    parser.add_argument("--output", choices=["dirhdr", "netcdf"], required=True, help="Output format: dirhdr or netcdf.")
    parser.add_argument("--ni", type=int, help="Number of columns (default: native resolution).")
    parser.add_argument("--nj", type=int, help="Number of rows (default: native resolution).")
    parser.add_argument("--north", type=float, required=True, help="Northern boundary of the area.")
    parser.add_argument("--south", type=float, required=True, help="Southern boundary of the area.")
    parser.add_argument("--east", type=float, required=True, help="Eastern boundary of the area.")
    parser.add_argument("--west", type=float, required=True, help="Western boundary of the area.")
    parser.add_argument("--output_path", required=True, help="Path for saving the output file. Extension will be added based on output format.")

    args = parser.parse_args()

    # Set the hardcoded SRTM path
    srtm_path = "SRTM_GL1_srtm/"

    # Generate the data based on boundaries
    elevation_data = get_elevation_raster_from_srtm_NSEW(
        north=args.north,
        south=args.south,
        east=args.east,
        west=args.west,
        ni=args.ni,  # Will be None if not specified
        nj=args.nj,  # Will be None if not specified
        srtm_path=srtm_path,
        nodata_value=-32768
    )

    if elevation_data is None:
        print("No data found for the specified boundaries.")
        return

    # Save the output in the requested format
    if args.output == "dirhdr":
        dir_path = f"{args.output_path}.dir"
        hdr_path = f"{args.output_path}.hdr"
        xarrayToDirHdr(elevation_data, dir_path, hdr_path)
        print(f"DIR/HDR files saved at: {dir_path} and {hdr_path}")
    elif args.output == "netcdf":
        nc_path = f"{args.output_path}.nc"
        elevation_data.to_netcdf(nc_path)
        print(f"NetCDF file saved at: {nc_path}")

if __name__ == "__main__":
    main()
    
#SWd = dirHDRtoXarray("/Users/filippi_j/soft/Meso-NH/PGD/auDEM.dir","/Users/filippi_j/soft/Meso-NH/PGD/auDEM.hdr")
#NWd = dirHDRtoXarray("/Users/filippi_j/Volumes/dataorsu/landscape/auNWReduced.dir","/Users/filippi_j/Volumes/dataorsu/landscape/auNWReduced.hdr")
