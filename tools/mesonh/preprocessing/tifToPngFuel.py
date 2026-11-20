#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Aug 30 15:37:24 2023

@author: filippi_j
"""

from PIL import Image
import numpy as np
from sklearn.cluster import KMeans
import matplotlib.image as mpimg 

def train_and_save_kmeans(png_path, model_save_path):
    """Train a KMeans model on a PNG and save the model."""
    # Load the PNG
    img_png = mpimg.imread(png_path)
    
    # Extract unique colors
    unique_colors = np.unique(img_png.reshape(-1, img_png.shape[2]), axis=0)[:, :3] * 255
    colors_float = unique_colors.astype(float)
    
    # Train the KMeans model
    kmeans = KMeans(n_clusters=len(unique_colors))
    kmeans.fit(colors_float)
    
    # Save the KMeans model
    return kmeans

def quantize_image_using_model(image_path, kmeans, output_path):
    """Quantize an image using a pre-trained KMeans model."""
    # Load the image
    img = mpimg.imread(image_path)
    
 
    
    # Predict using the KMeans model
    pixels_float = (img[:, :, :3] * 255).astype(float).reshape(-1, 3)
    labels = kmeans.predict(pixels_float)
    quantized_image = kmeans.cluster_centers_[labels].reshape(img.shape[:2] + (3,)).astype(np.uint8)
    
    # Save the quantized image
    Image.fromarray(quantized_image).save(output_path)

# Example usage:
    
png_path = '/Users/filippi_j/soft/firefront/Examples/villa/DomDB05.png'
model_save_path = 'kmeans_model.pkl'
km = train_and_save_kmeans(png_path, model_save_path)

input_image_path = 'path_to_input_image.jpg'
output_png_path = 'path_for_output_png_file.png'
quantize_image_using_model(input_image_path, km, output_png_path)