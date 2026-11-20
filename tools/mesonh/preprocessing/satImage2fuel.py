#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Sep 21 09:05:01 2023

@author: filippi_j
"""

import rasterio
import numpy as np
from osgeo import gdal
from pyproj import Transformer
from PIL import Image
import matplotlib.pyplot as plt

 
def get_tiff_extent(file_path):
    # Ouvrir le fichier TIFF avec GDAL
    ds = gdal.Open(file_path)
    wkt_projection = ds.GetProjection()
    # Obtenir les paramètres géospatiaux
    gt = ds.GetGeoTransform()
    
    # Calculer les coordonnées de l'emprise
    x_min = gt[0]
    y_max = gt[3]
    x_max = x_min + (gt[1] * ds.RasterXSize)
    y_min = y_max + (gt[5] * ds.RasterYSize)
    
    return x_min, y_min, x_max, y_max, wkt_projection

 
def cutToDimentions(lat_sw, lon_sw, lat_ne, lon_ne, inF, outF):
    with rasterio.open(inF) as src:
    
     
        # Obtenir le système de projection de l'image
        src_crs = src.crs.to_string()
        
        # Transformer pour convertir du système lat/lon (EPSG:4326) vers le système de l'image
        transformer = Transformer.from_crs("EPSG:4326", src_crs, always_xy=True)
        
        # Transformer les coordonnées
        xoff_sw, yoff_sw = transformer.transform(lon_sw, lat_sw)
        xoff_ne, yoff_ne = transformer.transform(lon_ne, lat_ne)
    
        # Convertir les coordonnées en indices de pixels
        xoff_sw, yoff_sw = ~src.transform * (xoff_sw, yoff_sw)
        xoff_ne, yoff_ne = ~src.transform * (xoff_ne, yoff_ne)
    
        xoff_sw, yoff_sw, xoff_ne, yoff_ne = map(int, (xoff_sw, yoff_sw, xoff_ne, yoff_ne))
        
        # Calculer la largeur et la hauteur de la fenêtre
        width = xoff_ne - xoff_sw
        height = yoff_sw - yoff_ne
        
        # Créer une fenêtre en utilisant les indices des pixels
        window = rasterio.windows.Window(col_off=xoff_sw, row_off=yoff_ne, width=width, height=height)
        
        # Lire la sous-image
        sub_image = src.read(window=window)
        
        # Profil pour la nouvelle image
        new_profile = src.profile
        new_profile.update({
            "height": height,
            "width": width,
            "transform": src.window_transform(window)
        })
        
        # Sauvegarder la sous-image
        # Sauvegarder la sous-image
        with rasterio.open(outF, 'w', **new_profile) as dst:
            dst.write(sub_image)
         
        x_min, y_min, x_max, y_max, proj = get_tiff_extent(outF)
        print(f"Emprise SW-NE : ({x_min}, {y_min}) - ({x_max}, {y_max}, {proj})")
        
def plot_band_histograms(src):
 
    num_bands = src.count

    # Créer une figure pour les histogrammes
    fig, axes = plt.subplots(1, num_bands, figsize=(15, 5))

    # Parcourir chaque bande et afficher l'histogramme
    for band_index in range(1, num_bands + 1):
        prin
        band_data = src.read(band_index)
        
        # Pour des raisons de performance, aplatir le tableau en 1D
        band_data = band_data.flatten()

        # Plotter l'histogramme
        if num_bands == 1:
            ax = axes  # Si une seule bande, axes n'est pas un tableau
        else:
            ax = axes[band_index - 1]
            
        ax.hist(band_data, bins=50, color='blue', edgecolor='black')
        ax.set_title(f'Histogramme de la bande {band_index}')
        ax.set_xlabel('Valeur du pixel')
        ax.set_ylabel('Fréquence')

    plt.tight_layout()
    plt.show()

def toPng(ndvi_array, ndvi_min,ndvi_max, outPNG):
 
    ndvi_normalized = ((ndvi_array - ndvi_min) / (ndvi_max - ndvi_min) * 255).astype(np.uint8)
    
    # Créer une image en niveaux de gris à partir du tableau normalisé
    img = Image.fromarray(ndvi_normalized, 'L')
 

    img.save(outPNG)
        
def plot_band_histogram(band):

    # Pour des raisons de performance, aplatir le tableau en 1D
    band_data = band.flatten()
    fig, axes = plt.subplots(1, 1, figsize=(15, 5))
    # Plotter l'histogramme
    ax = axes  # Si une seule bande, axes n'est pas un tableau

        
    ax.hist(band_data, bins=50, color='blue', edgecolor='black')


    plt.tight_layout()
    plt.show()

def toNDVI(inF, ndviF):
    blue_band_index = 1  # exemple
    green_band_index = 2  # exemple
    red_band_index = 3  # exemple
    nir_band_index = 4  # exemple
    
    with rasterio.open(inF) as src:
        num_bands = src.count
        #plot_band_histograms(src)
        
        # Indices des bandes Rouge et NIR
        # À ajuster selon votre image satellite
        
        
        print(f"Le fichier TIFF contient {num_bands} bandes. de type {src.dtypes[0]}")
        
        # Lire les bandes Rouge et NIR
        blue_band = src.read(blue_band_index).astype('float64')/4096.0
        green_band = src.read(green_band_index).astype('float64')/4096.0
        red_band = (src.read(red_band_index).astype('float64'))/4096.0
        nir_band = (src.read(nir_band_index).astype('float64'))/4096.0
        #toPng(blue_band, 0,1000, ndviF+"blue.png")
        #toPng(green_band, 0,1000, ndviF+"green.png")
        #toPng(red_band, 0,1000, ndviF+"red.png")
        #toPng(nir_band, 0,3000, ndviF+"nir.png")
        red_min = np.nanmin(red_band)
        red_max = np.nanmax(red_band)
        
        print("red",red_min,red_max,"nir", np.nanmin(nir_band), np.nanmax(nir_band))
        
        
        # Éviter la division par zéro
        np.seterr(divide='ignore', invalid='ignore')
        
 
        # NDVI
        ndvi = (nir_band - red_band) / (nir_band + red_band)
        L = 0.5
        savi = ((nir_band - red_band) / (nir_band + red_band + L)) * (1 + L)
        evi2 = 2.5 * ((nir_band - red_band) / (nir_band + 2.4 * red_band + 1))

        ndwi = (green_band - nir_band) / (green_band + nir_band)
        # MNDWI (Suppose une bande MIR)
        #mndwi = (green_band - mir_band) / (green_band + mir_band)
        evi = 2.5 * ((nir_band - red_band) / (nir_band + 6 * red_band - 7.5 * blue_band + 1))
        # NDBI (Suppose une bande MIR)
        # ndbi = (mir_band - nir_band) / (mir_band + nir_band)
        arvi = (nir_band - (2 * red_band) + blue_band) / (nir_band + (2 * red_band) + blue_band)
        ci = (nir_band / red_band) - 1
        toPng(arvi, 0,1, ndviF+"arvi.png")
        toPng(evi, 0,1, ndviF+"evi.png")
        toPng(ndwi, 0,1, ndviF+"ndwi.png")
        toPng(ndvi, 0,1, ndviF+"ndvi.png")
        toPng(savi, 0,1, ndviF+"savi.png") 
        toPng(evi2, 0,1, ndviF+"evi2.png")
        toPng(ndvi, 0,1, ndviF+"ndvi.png")
        toJpg(nir_band,red_band*4,green_band*4,ndviF+"rvb.png")
        # Profil pour la nouvelle image NDVI
        new_profile = src.profile
        new_profile.update({
            "dtype": 'float32',
            "count": 1  # NDVI est une seule bande
        })
 
        # Sauvegarder l'image NDVI
        with rasterio.open(ndviF, 'w', **new_profile) as dst:
            dst.write(nir_band, 1)  # Écrire dans la première bande
 

from sklearn.cluster import KMeans
from sklearn.cluster import DBSCAN
def toJpg(red,green,blue, outF):
 
    
    # Convert the normalized bands to 8-bit (0-255)
    red_8bit = (red * 255).astype('uint8')
    green_8bit = (green * 255).astype('uint8')
    blue_8bit = (blue * 255).astype('uint8')
    
    # Create an RGB image
    rgb_array = np.stack((red_8bit, green_8bit, blue_8bit), axis=2)
    rgb_image = Image.fromarray(rgb_array, 'RGB')
    
    # Save the image
    rgb_image.save(outF)
    
def classify_central_area_dbscan(blue, green, red, nir, eps=0.1, min_samples=5, areaHW=10000000):
    # ... (le reste du code est identique jusqu'à la préparation des données pour le clustering)
        
    # Get image dimensions
    rows, cols = blue.shape
    row_start =  0
    row_end = rows 
    col_start = 0
    col_end = cols
    
    if areaHW < rows :
    # Define central 500x500 window
        row_start = (rows - areaHW) // 2
        row_end = row_start + areaHW
        col_start = (cols - areaHW) // 2
        col_end = col_start + areaHW

       
    
    # Extract central 500x500 pixels from each band
    central_blue = blue[row_start:row_end, col_start:col_end]
    central_green = green[row_start:row_end, col_start:col_end]
    central_red = red[row_start:row_end, col_start:col_end]
    central_nir = nir[row_start:row_end, col_start:col_end]
    
    # Prepare data for clustering
    flattened_data = np.vstack([central_blue.flatten(), 
                                central_green.flatten(), 
                                central_red.flatten(), 
                                central_nir.flatten()]).T
    
    # Appliquer DBSCAN
    dbscan = DBSCAN(eps=5, min_samples=50, metric = 'euclidean',algorithm ='auto')
    dbscan_labels = dbscan.fit_predict(flattened_data)
    
    # Remodeler le résultat pour obtenir une image d'étiquettes
    label_image = dbscan_labels.reshape(central_blue.shape)
    
    return label_image

def classify_central_area(blue, green, red, nir, n_clusters=20, areaHW=10000000):
 
    
    # Get image dimensions
    rows, cols = blue.shape
    row_start =  0
    row_end = rows 
    col_start = 0
    col_end = cols
    
    if areaHW < rows :
    # Define central 500x500 window
        row_start = (rows - areaHW) // 2
        row_end = row_start + areaHW
        col_start = (cols - areaHW) // 2
        col_end = col_start + areaHW

       
    
    # Extract central 500x500 pixels from each band
    central_blue = blue[row_start:row_end, col_start:col_end]
    central_green = green[row_start:row_end, col_start:col_end]
    central_red = red[row_start:row_end, col_start:col_end]
    central_nir = nir[row_start:row_end, col_start:col_end]
    
    # Prepare data for clustering
    flattened_data = np.vstack([central_blue.flatten(), 
                                central_green.flatten(), 
                                central_red.flatten(), 
                                central_nir.flatten()]).T
    
    # Apply K-Means
    kmeans = KMeans(n_clusters=n_clusters,n_init='auto', max_iter=50000)
    kmeans_labels = kmeans.fit_predict(flattened_data)
    
    # Reshape the result to get a label image
    label_image = kmeans_labels.reshape(central_blue.shape)
    
    return label_image

def downsample_max(band, N=10):
    """
    Downsample a 2D array representing a single band by taking the maximum value in each NxN block.
    
    Parameters:
        band: 2D numpy array representing the image band
        N: Size of the block (default is 10)
        
    Returns:
        downsampled_band: 2D numpy array with reduced resolution
    """
    
    # Get dimensions of the original band
    rows, cols = band.shape
    
    # Calculate dimensions of the downsampled band
    down_rows = rows // N
    down_cols = cols // N
    
    # Initialize the downsampled band
    downsampled_band = np.zeros((down_rows, down_cols))
    
    for i in range(0, down_rows):
        for j in range(0, down_cols):
            # Extract NxN block from the original band
            block = band[i*N:(i+1)*N, j*N:(j+1)*N]
            
            # Take the maximum value in the block and assign it to the downsampled band
            downsampled_band[i, j] = np.max(block)
            
    return downsampled_band

# Ouvrir l'image source
inF = '/Users/filippi_j/data/2023/prunelli/6731213101-2/IMG_PHR1B_PMS_001/IMG_PHR1B_PMS_202106131023055_ORT_6731213101_R2C2.TIF'
outF = "/Users/filippi_j/data/2023/prunelli/6731213101-2/sub/subsetFDS.TIF"
ndviF = "/Users/filippi_j/data/2023/prunelli/6731213101-2/sub/subFDS.TIF"
 
# domaine MNH
lat_sw, lon_sw = 41.955008091611084, 9.272822878548226
lat_ne, lon_ne = 42.063626286569516, 9.418036449457915
#domaine FDS
lat_sw, lon_sw = 42.0063067 , 9.3263082
lat_ne, lon_ne = 42.0104249 ,  9.3327945

cutToDimentions(lat_sw, lon_sw, lat_ne, lon_ne, inF, outF)

toNDVI(outF, ndviF)

#toNDVI(outF, ndviF)