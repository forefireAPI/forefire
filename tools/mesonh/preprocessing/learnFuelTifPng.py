#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Oct  3 14:14:56 2023

@author: filippi_j
"""
from PIL import Image, ImageDraw, ImageFont
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
import tensorflow as tf
# Charger l'image

import geopandas as gpd
import contextily as cx

import xarray as xr  
from fiona.crs import from_epsg 
from shapely.geometry import box
import rasterio
from rasterio.mask import mask
import pycrs
import json


from . import get_WSEN_LBRT_ZS_From_Pgd, arrayToPng, create_kml

def webMapsToTif(west, south, east, north, outF, providerSRC=cx.providers.GeoportailFrance.orthos, zoomLevel=12):
    tempOUT = outF+"_temp.tif"
    print("extracting ", west, south, east, north, outF)
    
    
    cx.bounds2raster(west, south, east, north ,
                         ll=True,
                         path=tempOUT,
                                         zoom=zoomLevel,
                                         source=providerSRC
     
                        )
    
    
    data = rasterio.open(tempOUT) 
    
    bbox = box(west, south, east, north)
    
    geo = gpd.GeoDataFrame({'geometry': bbox}, index=[0], crs=from_epsg(4326))
    geo = geo.to_crs(crs=data.crs.data)

    coords = [json.loads(geo.to_json())['features'][0]['geometry']]
      
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
    
    with rasterio.open(outF, "w", **out_meta) as dest:
        dest.write(out_img)
    
    print("Extracted image from contextily bounds:",west, south, east, north," zoom ", zoomLevel, " out files ",outF," and temporary ",tempOUT)
    
# def getFilteredPaletteAndSize(png_path):
#     png_image = Image.open(png_path)
#     png_array = np.array(png_image)
#     palette = png_image.getpalette()
#     palette = [tuple(palette[i:i+3]) for i in range(0, len(palette), 3)]
#     unique_indices = np.unique(png_array)
#     filtered_palette = [palette[i] for i in unique_indices]
#     return filtered_palette, png_image.size

def display_images(png_path, tiff_path):
    # Charger l'image PNG
    png_image = Image.open(png_path)
    png_array = np.array(png_image)
    palette = png_image.getpalette()
    palette = [tuple(palette[i:i+3]) for i in range(0, len(palette), 3)]
    unique_indices = np.unique(png_array)
    filtered_palette = [palette[i] for i in unique_indices]
    mapped_png_array = np.array([[filtered_palette[unique_indices.tolist().index(idx)] for idx in row] for row in png_array])
    
    # Charger et redimensionner l'image TIFF
    tiff_image = Image.open(tiff_path)
    tiff_resized = tiff_image.resize(png_image.size, Image.ANTIALIAS)
    
    # Créer une image PIL pour la palette de couleurs filtrée
    palette_image = Image.new('RGB', (len(filtered_palette), 1))
    palette_image.putdata([color for color in filtered_palette for _ in range(1)])
    
    # Affichage
    fig, axes = plt.subplots(1, 3, figsize=(18, 6))
    
    # Afficher le TIFF redimensionné
    axes[0].imshow(np.array(tiff_resized))
    axes[0].set_title(tiff_path)
    axes[0].axis('off')
    
    # Afficher le PNG avec les couleurs mappées
    axes[1].imshow(mapped_png_array)
    axes[1].set_title(png_path)
    axes[1].axis('off')
    
    # Afficher la palette de couleurs
    axes[2].imshow(np.array(palette_image), aspect='auto')
    axes[2].set_title('Palette de couleurs filtrée')
    for i, index in enumerate(unique_indices):
        color = filtered_palette[i]
        axes[2].text(i, 0, str(index), color='white' if sum(color) < 400 else 'black', ha='center', va='center')
    axes[2].axis('off')
    
    plt.show()
 
# Tester la fonction
#display_images(OUTIMGS[IIM], INIMGS[IIM])

def prepare_data(image_tuples):
    # Listes pour stocker les données concaténées
    tiff_data_list = []
    y_train_list = []
    
    for tif_path, png_path in image_tuples:
        print("loading ", tif_path, png_path)
        # Charger l'image TIFF
        tiff_image = Image.open(tif_path)
        tiff_array = np.array(tiff_image)[:,:,:3]  # Prendre uniquement les canaux RGB
        
        # Charger et redimensionner l'image PNG
        png_image = Image.open(png_path)
        png_resized = png_image.resize(tiff_image.size, Image.NEAREST)
        png_array = np.array(png_resized)
        
        # Aplatir et ajouter aux listes
        tiff_data_list.append(tiff_array.reshape(-1, 3))
        y_train_list.append(png_array.reshape(-1))
    
    # Concaténer les données
    tiff_data = np.vstack(tiff_data_list)
    y_train = np.concatenate(y_train_list)
    
    # Identifier le nombre unique de classes (indices de couleurs)
    num_classes = len(np.unique(y_train))
    
    # One-hot encoder les étiquettes
    y_train_onehot = tf.keras.utils.to_categorical(y_train, num_classes=num_classes)
    
    return tiff_data, y_train_onehot

 

def train_and_save_model(tiff_data, y_train_onehot, out_file_path,subTrain = 500):
    # Créer un modèle simple de réseau neuronal
    random_indices = np.random.choice(tiff_data.shape[0], int(tiff_data.shape[0]/subTrain), replace=False)
    X_subsample = tiff_data[random_indices]
    y_subsample = y_train_onehot[random_indices]
    
    model = tf.keras.Sequential([
        tf.keras.layers.Dense(32, activation='relu', input_shape=(tiff_data.shape[1],)),
        tf.keras.layers.Dense(32, activation='relu'),
        tf.keras.layers.Dense(inimagedata.shape[1], activation='softmax')
    ])
    model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])
    
    # Entraîner le modèle
    model.fit(X_subsample, y_subsample, epochs=10)
    print(model.evaluate(tiff_data[:1000], y_train_onehot[:1000]))
 
    # Sauvegarder le modèle dans le fichier spécifié
    model.save(out_file_path)
    

def recreate_and_save_indexed_image_with_palette( tiff_path, output_png_path, output_size, filtered_palette = [(0, 178, 31), (188, 0, 165), (112, 105, 97), (255, 255, 255)],  model_path = '/Users/filippi_j/soft/firefront/tools/preprocessing/3categories.h5',):
    # Charger et redimensionner l'image TIFF
    model = tf.keras.models.load_model(model_path)
    tiff_image = Image.open(tiff_path)

    tiff_array = np.array(tiff_image)[:,:,:3]  # Prendre uniquement les canaux RGB
    print(tiff_array.shape[:-1])
    # Aplatir les données du TIFF
    tiff_data = tiff_array.reshape(-1, 3)
    
    # Utiliser le modèle pour prédire les indices de couleurs
    print("Image generation")
    predictions = model.predict(tiff_data)
    print("Image generated, categorizing")
    predicted_indices = np.argmax(predictions, axis=1)
    
    print("Image categorized, reshaping")
    # Reshaper les indices prédits pour correspondre à la taille de sortie
    predicted_image_array = predicted_indices.reshape(tiff_array.shape[:-1])
    
    # Créer une nouvelle image indexée avec la palette filtrée
    new_image = Image.fromarray(np.uint8(predicted_image_array), 'P')
    new_image.putpalette([component for color in filtered_palette for component in color])
    
    # Redimensionner l'image indexée à la taille finale
    new_image_resized = new_image.resize(output_size, Image.NEAREST)
    
    # Sauvegarder l'image en format PNG
    new_image_resized.save(output_png_path)



def makeFuelMapFromPgd(pgd_path,tifout_path,pngout_path,kmlout_path, resolution=10 ):

    WSEN, LBRT, ZS = get_WSEN_LBRT_ZS_From_Pgd(pgd_path)

    west, south, east, north = WSEN
#    pgd = xr.load_dataset(pgd_path)    
 #   arrayToPng(np.array(ZS.values, dtype=np.float32), pngout_path+"ZS.png",output_cbar_png=pngout_path+"ZSCBAR.png", vx=pgd.UT[0,0,::4,::4].values, vy=pgd.VT[0,0,::4,::4].values)
    arrayToPng(np.array(ZS.values, dtype=np.float32), pngout_path+"ZS.png",output_cbar_png=pngout_path+"ZSCBAR.png")

    create_kml(west, south, east, north,"Altitude", pngout_path+"ZS.png",  kmlout_path+"ZS.kml", pngcbarfile=pngout_path+"ZSCBAR.png" )
    
    webMapsToTif(west, south, east, north, tifout_path,providerSRC=cx.providers.Esri.WorldImagery )
    
    out_size = (int((LBRT[2]-LBRT[0])/resolution),int((LBRT[3]-LBRT[1])/resolution))
    
    recreate_and_save_indexed_image_with_palette( tifout_path, pngout_path, out_size)
    
    create_kml(west, south, east, north,"Fuel", pngout_path,  kmlout_path )
    





# #OUTIMGS = "/Users/filippi_j/data/2023/baseFeux/pedrogao/80mDOMCUT.png", "/Users/filippi_j/data/2023/baseFeux/DomDB46.png"#,  "/Users/filippi_j/data/2023/baseFeux/DomDB59.png"
# #INIMGS = "/Users/filippi_j/data/2023/baseFeux/pedrogao/80mDOMCUT.tif", "/Users/filippi_j/data/2023/baseFeux/DomDB46.tif"#,  "/Users/filippi_j/data/2023/baseFeux/DomDB59.tif"
# INIMGS = "/Users/filippi_j/data/2023/baseFeux/DomDB46.tif", "/Users/filippi_j/data/2023/baseFeux/DomDB07.tif",  "/Users/filippi_j/data/2023/baseFeux/DomDB59.tif",  "/Users/filippi_j/data/2023/baseFeux/DomDB05.tif"
# AIOUTIMGS = "/Users/filippi_j/data/2023/baseFeux/AIDomDB46.png", "/Users/filippi_j/data/2023/baseFeux/AIDomDB07.png",  "/Users/filippi_j/data/2023/baseFeux/AIDomDB59.png",  "/Users/filippi_j/data/2023/baseFeux/AIDomDB05.png"
# #tiff_data, y_train_onehot = prepare_data(list(zip(INIMGS,OUTIMGS)))
# #train_and_save_model(tiff_data, y_train_onehot, model_path, subTrain=10)

#dataInPath = "/Users/filippi_j/Volumes/fcouto/KDATABASE/KTEST_Pedrogao/KTEST/KTEST_3nest/001_pgd/"
#dataOutPath = "/Users/filippi_j/data/2023/testPProc/"
#pgd_path = "%s/PGD_D80mA.nc"%dataInPath

# pgd_path = "/Users/filippi_j/Volumes/fcouto/KDATABASE/KTEST_Pedrogao/KTEST/KTEST_3nest/002_real/AND1.20170617.06.nc"
# pgd_path = "/Users/filippi_j/Volumes/fcouto/KDATABASE/KTEST_Pedrogao/KTEST/KTEST_3nest/002_real/AND1.20170617.06.nc"
#pgd_path = "/Users/filippi_j/Volumes/orsu//firecaster/2023/toulouse20190814/001_pgd/PGD_D80mA.nc"

#tifout_path = "%s/test.tif"%dataOutPath
#pngout_path = "%s/test.png"%dataOutPath
#kmlout_path = "%s/test.kml"%dataOutPath

#makeFuelMapFromPgd(pgd_path,tifout_path,pngout_path,kmlout_path)
#INIMGS = "/Users/filippi_j/data/2023/baseFeux/pedrogao/120mDOMCUT.tif",#,"/Users/filippi_j/data/2023/baseFeux/pedrogao/120mDOMCUT.tif","/Users/filippi_j/data/2023/baseFeux/pedrogao/80mDOMCUT.tif"
#AIOUTIMGS = "/Users/filippi_j/data/2023/baseFeux/pedrogao/AI120mDOMCUT.png",#,"/Users/filippi_j/data/2023/baseFeux/pedrogao/AI120mDOMCUT.png","/Users/filippi_j/data/2023/baseFeux/pedrogao/AI80mDOMCUT.png"

#for pngf, tiff in list(zip(OUTIMGS,INIMGS)):
#    display_images(pngf, tiff)
#for tifin, pngout in list(zip(INIMGS,AIOUTIMGS)):
#    recreate_and_save_indexed_image_with_palette( tifin,pngout, (2424, 2424))
    

 