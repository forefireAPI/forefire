#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Oct 30 11:38:26 2023

@author: filippi_j
"""

import rasterio
from rasterio.windows import from_bounds
from pyproj import Transformer
import fiona
import numpy as np
import os
import osmnx as ox  
from rasterio.features import geometry_mask
from rasterio.warp import calculate_default_transform, reproject, Resampling

from rasterio.transform import from_origin

import geopandas as gpd
from rasterio.mask import mask
from shapely.geometry import box
import json
 
from .ffToGeoJson import create_kml,generate_indexed_png_and_legend

 
from rasterio.features import rasterize
from shapely.geometry import mapping
 
attribute_widths_road_edge_full = {
    'secondary': 0.8,
    'track': 0.8,
    'tertiary': 0.8,
    'path': 0.8,
    'trunk': 0.8,
    'unclassified': 0.8,
    'service': 0.8,
    'living_street': 0.8,
    "['living_street', 'pedestrian']": 0.8,
    "['living_street', 'path']": 0.8,
    'residential': 0.8,
    "['track', 'residential']": 0.8,
    "['track', 'unclassified']": 0.8,
    'primary': 0.8,
    'trunk_link': 0.8,
    'primary_link': 0.8,
    "['steps', 'footway']": 0.8,
    "['unclassified', 'residential']": 0.8,
    'steps': 0.8,
    'pedestrian': 0.8,
    'secondary_link': 0.8,
    "['path', 'service']": 0.8,
    "['unclassified', 'path']": 0.8,
    'footway': 0.8,
    'tertiary_link': 0.8,
    "['pedestrian', 'footway']": 0.8,
    "['track', 'service']": 0.8,
    "['secondary', 'tertiary']": 0.8,
    "['residential', 'tertiary']": 0.8,
    "['pedestrian', 'residential']": 0.8,
    "['track', 'path']": 0.8,
    "['footway', 'residential']": 0.8,
    "['path', 'residential']": 0.8,
    "['track', 'path', 'residential']": 0.8,
    "['service', 'residential']": 0.8,
    "['path', 'pedestrian']": 0.8,
    "['service', 'pedestrian']": 0.8,
    "['track', 'unclassified', 'residential']": 0.8,
    "['residential', 'footway']": 0.8,
    "['unclassified', 'service']": 0.8,
    'cycleway': 0.8,
    "['steps', 'path', 'footway']": 0.8,
    "['service', 'footway']": 0.8,
    "['track', 'service', 'residential']": 0.8,
    "['steps', 'pedestrian', 'footway']": 0.8,
    "['service', 'steps', 'footway']": 0.8,
    "['path', 'footway']": 0.8,
}
attribute_widths_road_Edge_heavu = {
    'secondary': 3,
    'track': 1,
    'tertiary': 2,
    'path': 1,
    'trunk': 1,
    'service': 1,
    'living_street': 1,
    'residential': 1,
    'primary': 5,
    'trunk_link': 1,
    'primary_link': 3,
    'secondary_link': 2,
    'tertiary_link': 1,
    'cycleway': 1,
}
attribute_widths_road_Edge = {
    'secondary': 1,
    'track': 0.5,
    'tertiary': 0.7,
    'primary': 1.5,
}

# Example usage
attribute_widths_waterway = {
    'river': 1.5,
    'stream': 0.5,
    'canal': 1.0,
    'drain': 0.7,
    'generic': 0.5  # Set a generic width for all water features
}

def extract_subregion(input_tif, output_tif, westI, southI, eastI, northI):
    
    with rasterio.open(input_tif) as src:
        # Transformer pour convertir les coordonnées lat/lon (WGS84) en coordonnées du système du raster
        transformer = Transformer.from_crs("EPSG:4326", src.crs, always_xy=True)
        sw = transformer.transform( westI,southI)
        nw = transformer.transform( westI,northI)
        ne = transformer.transform( eastI,northI)
        se = transformer.transform( eastI,southI)
       
    #    print(sw,nw,se,ne)
        # Convertir les coordonnées
        west  = np.min([sw[0],nw[0]])
        north =  np.max([nw[1],ne[1]])
        east = np.max([se[0],ne[0]])
        south = np.min([sw[1],se[1]])
        
        print(west, south, east, north) 
        
        # Vérifications supplémentaires
        if west >= east or south >= north:
            print("Invalid bounds.",  west, south, east, north)
            return
        
        if west < src.bounds.left or east > src.bounds.right or \
           south < src.bounds.bottom or north > src.bounds.top:
            print("Bounds are outside the raster.")
            return
        
        # Créer une fenêtre (window) pour la sous-région
        window = from_bounds(west, south, east, north, src.transform)
        
        # Lire la sous-région
        data = src.read(1, window=window)
        
        # Calculer la nouvelle transformation affine pour la sous-région
        new_transform = src.window_transform(window)
        
        
        
        
        # Écrire la sous-région dans un nouveau fichier TIF
        with rasterio.open(output_tif, 'w', driver='GTiff',
                           height=data.shape[0], width=data.shape[1],
                           count=1, dtype=data.dtype,
                           crs=src.crs, transform=new_transform) as dst:
            dst.write(data, 1)



def warp_and_clip_raster(input_file_path: str, warped_file_path: str, output_file_path: str, west, south, east, north , target_width=2000, target_height = 2000):
    """
    Warps and clips a raster file based on the given bounds.

    Parameters:
        input_file_path (str): The path to the input raster file.
        output_file_path (str): The path to save the output raster file.
        bounds (dict): Dictionary containing the west, south, east, and north bounds.

    Returns:
        None: The function saves the output to a file.
    """
    # Define target CRS
    target_crs = 'EPSG:4326'
    
    # Warp raster to target CRS
    with rasterio.open(input_file_path) as src:
        transform, width, height = calculate_default_transform(
            src.crs, target_crs, src.width, src.height, *src.bounds)
        
        out_meta = src.meta.copy()
        out_meta.update({
            'crs': target_crs,
            'transform': transform,
            'width': int(width),
            'height': int(height)
        })

        temp_file_path = warped_file_path
        with rasterio.open(temp_file_path, 'w', **out_meta) as dst:
            for i in range(1, src.count + 1):
                reproject(
                    source=rasterio.band(src, i),
                    destination=rasterio.band(dst, i),
                    src_transform=src.transform,
                    src_crs=src.crs,
                    dst_transform=transform,
                    dst_crs=target_crs,
                    resampling=Resampling.nearest)
    
    # Create bounding box and GeoDataFrame 
    bbox = box(west, south, east, north)
    geo = gpd.GeoDataFrame({'geometry': [bbox]}, index=[0], crs="EPSG:4326")
    
    # Clip the raster
    with rasterio.open(temp_file_path) as src:
        coords = [json.loads(geo.to_json())['features'][0]['geometry']]
        out_img, out_transform = mask(src, shapes=coords, crop=True)
        out_meta = src.meta.copy()
        
        out_meta.update({
            "height": target_height,
            "width": target_width,
            "transform": out_transform,
        })

    # Save the clipped raster
    with rasterio.open(output_file_path, "w", **out_meta) as dest:
        dest.write(out_img)
    


def extract_roads_from_geotiff_original(west,south,east,north, road_shape_file):
    try:
        G = ox.graph_from_bbox(north, south, east, west, network_type='all')
    except Exception as e:
        return str(e)
    ox.save_graph_shapefile(G, filepath=road_shape_file )


def extract_roads_from_geotiff(west, south, east, north, road_shape_file, attribute_widths=attribute_widths_road_Edge):
 
    
    # Define the road types to extract based on the attribute_widths_road_Edge dictionary keys
    road_types = list(attribute_widths.keys())

    # Create a custom filter to fetch only the roads of interest
    custom_filter = f'["highway"~"{ "|".join(road_types) }"]'
    
    try:
        # Define the bounding box
        bbox = (north, south, east, west)
        
        # Retrieve the graph with the custom filter using the new bbox parameter
        G = ox.graph_from_bbox(bbox=bbox, network_type='drive', custom_filter=custom_filter)
    except Exception as e:
        return str(e)
    
    if len(G.edges) <= 0:
        return None
    # You might want to further process the graph here to assign or use your custom attributes
    # For example, adjusting edge widths as per your dictionary before saving
    for u, v, key, data in G.edges(data=True, keys=True):
        road_type = data.get('highway', None)
        if isinstance(road_type, list):  # Sometimes 'highway' can be a list of types
            road_type = road_type[0]    # Select the first type as the representative
        width = attribute_widths.get(road_type, None)
        if width is not None:
            data['width'] = width

    # Save the graph to a shapefile
    ox.save_graph_geopackage(G, filepath=road_shape_file)
    
    return road_shape_file



def extract_waterways_from_geotiff(west, south, east, north, waterway_shape_file, attribute_widths):
    
    # Define the waterway types to extract based on the attribute_widths dictionary keys
    waterway_types = list(attribute_widths.keys())

    # Create a custom filter to fetch only the waterways of interest
    custom_filter = f'["waterway"~"{ "|".join(waterway_types) }"]'
    

    try:    
        bbox = (north, south, east, west)  
        # Retrieve the graph with the custom filter using the new 'bbox' parameter
        G = ox.graph_from_bbox(bbox=bbox, network_type='none', custom_filter=custom_filter, retain_all=True)
    except Exception as e:
        return str(e)
    
    if len(G.edges) <= 0:
        return None
    
    # You might want to further process the graph here to assign or use your custom attributes
    # For example, adjusting widths as per your dictionary before saving
    for u, v, key, data in G.edges(data=True, keys=True):
        waterway_type = data.get('waterway', None)
        if isinstance(waterway_type, list):  # Sometimes 'waterway' can be a list of types
            waterway_type = waterway_type[0]    # Select the first type as the representative
        width = attribute_widths.get(waterway_type, None)
        if width is not None:
            data['width'] = width

    # Save the graph to a shapefile
    ox.save_graph_geopackage(G, filepath=waterway_shape_file)
    return waterway_shape_file

def rasterize_shapefiles_check(road_shapefile_path, water_shapefile_path, ref_tif, output_path,
                         roads_attribute_widths, water_attribute_widths,
                         default_width=0.3, imbounds=None, road_code=62, water_code=162):

    shapes = []

    if os.path.exists(road_shapefile_path):
        roads_gdf = gpd.read_file(road_shapefile_path)
        for _, feature in roads_gdf.iterrows():
            if('highway' in feature.keys()):
                highway_type = feature['highway']
                line_width = roads_attribute_widths.get(highway_type, default_width)
                if line_width > 0:
                    buffered_geom = feature['geometry'].buffer(line_width / 2)  # Adjust this buffer as needed
                    shapes.append((mapping(buffered_geom), road_code))
    else:
        print(f"Warning: Road shapefile {road_shapefile_path} not found.")

    if os.path.exists(water_shapefile_path):
        water_gdf = gpd.read_file(water_shapefile_path)
        generic_water_width = water_attribute_widths.get('generic', default_width)  # Assuming a generic type
        for _, feature in water_gdf.iterrows():
            buffered_geom = feature['geometry'].buffer(generic_water_width / 2)  # Adjust this buffer as needed
            shapes.append((mapping(buffered_geom), water_code))
    else:
        print(f"Warning: Water shapefile {water_shapefile_path} not found.")

    with rasterio.open(ref_tif) as refT:
        ref_crs = refT.crs
        width = refT.width
        height = refT.height
        transform = refT.transform  # Directly use the transform from the reference raster

        # Create a base raster array
        raster = refT.read(1)
        refT.close()
        
        if shapes:
            # Rasterize the shapes onto the raster array only if there are shapes to rasterize
            rasterized = rasterize(shapes, out_shape=(height, width), fill=0, transform=transform, dtype=np.uint8)
            raster[rasterized == road_code] = road_code
            raster[rasterized == water_code] = water_code

        # Save the raster data to a new GeoTIFF file
        with rasterio.open(output_path, 'w', driver='GTiff', width=width, height=height, count=1,
                           dtype=raster.dtype, crs=ref_crs, transform=transform) as dst:
            dst.write(raster, 1)

        refT.close()
        
def rasterize_shapefiles(road_shapefile_path, water_shapefile_path, ref_tif, output_path, 
                         roads_attribute_widths, water_attribute_widths, 
                         default_width=0.3, imbounds=None, road_code=62, water_code=162):
    
    roads_gdf = None
    water_gdf = None
    
    if road_shapefile_path is not None:
        if os.path.exists(road_shapefile_path):
            layers = fiona.listlayers(road_shapefile_path)
            if "edges" in layers:
                roads_gdf = gpd.read_file(road_shapefile_path,layer="edges")
                print(f"Loaded road 'edges' layer with {len(roads_gdf)} shapes.")
            
        
    if water_shapefile_path  is not None:
        if os.path.exists(water_shapefile_path):
            layers = fiona.listlayers(water_shapefile_path)
            if "edges" in layers:
                water_gdf = gpd.read_file(water_shapefile_path,layer="edges")
                print(f"Loaded water stream 'edges' layer with {len(water_gdf)} shapes.")

    # Use specified bounds if provided, otherwise derive bounds from both GeoDataFrames
    if imbounds is not None:
        bounds = imbounds
    else:
        print("WARNING getting bounds from roads")

        bounds = [
            roads_gdf.total_bounds[0], # minx
            roads_gdf.total_bounds[1],  # miny
            roads_gdf.total_bounds[2],  # maxx
            roads_gdf.total_bounds[3]  # maxy
        ]#

    x_min, y_min, x_max, y_max = bounds

    # Load the reference raster
    with rasterio.open(ref_tif) as refT:
        ref_crs = refT.crs
        width = refT.width
        height = refT.height

        # Calculate the resolution based on reference raster
        resolutionx = (x_max - x_min) / width
        resolutiony = (y_max - y_min) / height

        # Create a transformation from the bounds
        transform = rasterio.transform.from_origin(x_min, y_max, resolutionx, resolutiony)

        # Read the existing raster data
        raster = refT.read(1)
        #raster *= 0
        refT.close()

        # Prepare shapes and associated codes for roads and waterways
        shapes = []
        
        if roads_gdf is not None:
            print("using road shapefile :", road_shapefile_path)
            for _, feature in roads_gdf.iterrows():
                if('highway' in feature.keys()):
                    highway_type = feature['highway']
                   # print("YESSSSSSSSS HIGHWAY ")
                    line_width = roads_attribute_widths.get(highway_type, default_width)
                    if line_width > 0:
                        buffered_geom = feature['geometry'].buffer(line_width * resolutionx / 2)
                        shapes.append((mapping(buffered_geom), road_code))
                else:
                    print("NO HIGHWAY ", feature.keys())

        if water_gdf is not  None:
            print("using water shapefile :", water_shapefile_path)
            generic_water_width = water_attribute_widths.get('generic', default_width)  # Assuming a generic type if no specific 'waterway' key
            for _, feature in water_gdf.iterrows():
                buffered_geom = feature['geometry'].buffer(generic_water_width * resolutionx / 2)
                shapes.append((mapping(buffered_geom), water_code))
            

        # Rasterize the shapes onto the raster array
        if len(shapes) > 0:
            print("Number of shapes to resterize :", len(shapes))
            rasterized = rasterize(shapes, out_shape=(height, width), fill=0, transform=transform, dtype=np.uint8)
            raster[rasterized == road_code] = road_code
            raster[rasterized == water_code] = water_code

        
      #  for _, feature in roads_gdf.iterrows():
      #      highway_type = feature['highway']
      #      line_width = 10#attribute_widths.get(highway_type, default_width)  # Default line width is 1 pixel
      #      mask = geometry_mask([feature['geometry'].buffer(line_width * resolutionx / 2)],
      #                           transform=transform, invert=True, out_shape=(height, width))
      #      raster[mask] = road_code  # Set pixel value to 255 (white) where there is a feature
    # Save the raster data to a new GeoTIFF file
    with rasterio.open(output_path, 'w', driver='GTiff', width=width, height=height, count=1,
                       dtype=raster.dtype, crs=ref_crs, transform=transform) as dst:
        dst.write(raster, 1)

        
def rasterize_shapefile_original(shapefile_path, ref_tif, output_path, attribute_widths=attribute_widths_road_Edge, default_width = 0.3, imbounds =None,code=62):
    print(f"Rasterizing roads {shapefile_path} to {output_path}  with {attribute_widths}")
    reference_properties = {}
    
    gdf = gpd.read_file(shapefile_path)
    bounds = gdf.total_bounds  # minx, miny, maxx, maxy
   
 

    if imbounds is not None:
        bounds=imbounds
        
        
    x_min, y_min, x_max, y_max = bounds
    x_range = x_max - x_min
    y_range = y_max - y_min
    
    refT =  rasterio.open(ref_tif) 
    ref_crs = refT.crs
    width = refT.width
    height = refT.height  

    # Calculate the resolution
    resolutionx = x_range / width
    resolutiony = y_range / height 
    # Create a transform
    transform = from_origin(x_min, y_max, resolutionx, resolutiony)
    
    # Create an empty raster
    raster = refT.read(1)
    refT.close()
    
    # Rasterize each feature based on the "highway" attribute
    for _, feature in gdf.iterrows():
        highway_type = feature['highway']
        line_width = attribute_widths.get(highway_type, default_width)  # Default line width is 1 pixel
        mask = geometry_mask([feature['geometry'].buffer(line_width * resolutionx / 2)],
                             transform=transform, invert=True, out_shape=(height, width))
        raster[mask] = code  # Set pixel value to 255 (white) where there is a feature


    # Save the reprojected data to a new GeoTIFF file
    with rasterio.open(output_path, 'w', driver='GTiff',
                       width=width, height=height,
                       count=1, dtype=raster.dtype,
                       crs=ref_crs, transform=transform) as dst:
        dst.write(raster, 1)    

def fake_fuel(ref_tif, output_path,fuel_test,imbounds =None):
    print(f"faking fuels to {output_path}")
        
    bounds=imbounds
    x_min, y_min, x_max, y_max = bounds
    x_range = x_max - x_min
    y_range = y_max - y_min
    
    refT =  rasterio.open(ref_tif) 
    ref_crs = refT.crs
    width = refT.width
    height = refT.height  

    # Calculate the resolution
    resolutionx = x_range / width
    resolutiony = y_range / height 
    # Create a transform
    transform = from_origin(x_min, y_max, resolutionx, resolutiony)
    
    # Create an empty raster
    raster = refT.read(1)
    n_bandes = len(fuel_test)
    limites = np.linspace(0, width, n_bandes + 1).astype(int)
    # Remplissage de fuelMap
    for i in range(n_bandes):
        raster[:, limites[i]:limites[i + 1]] = fuel_test[i]
        
    

    
    refT.close()

    # Save the reprojected data to a new GeoTIFF file
    with rasterio.open(output_path, 'w', driver='GTiff',
                       width=width, height=height,
                       count=1, dtype=raster.dtype,
                       crs=ref_crs, transform=transform) as dst:
        dst.write(raster, 1)    

def fake_fuel(legend_file_path, WSEN, LBRT,output_dir,fuel_resolution = 10):
    fuel_indices_warped_clipped = f"{output_dir}/fuel_indices_warped_clipped.tif"
    fuel_road_indices = f'{output_dir}/fuel.tif'
    fuel_kml = f'{output_dir}/fuel.kml'
    fuel_png =f'{output_dir}/fuel.png'
    fuel_cbar_png =f'{output_dir}/fuel_bar.png'
      
    west, south, east, north =WSEN
    l,b,r,t = LBRT
    
    fake_fuel(fuel_indices_warped_clipped, fuel_road_indices,fuel_test,imbounds=(west, south,east ,north))   
    generate_indexed_png_and_legend(legend_file_path,fuel_road_indices, fuel_png, fuel_cbar_png)
    create_kml(west, south, east, north, "FUEL", fuel_png,fuel_kml , pngcbarfile=fuel_cbar_png) 
    


def rasterize_kml(kml_path, ref_tif, output_path, default_value=1, imbounds=None):
    print(f"Rasterizing KML {kml_path} to {output_path}")

    # Load reference GeoTIFF for spatial reference
    with rasterio.open(ref_tif) as refT:
        ref_crs = refT.crs
        width = refT.width
        height = refT.height
        x_min, y_min, x_max, y_max = refT.bounds
        if imbounds is not None:
            x_min, y_min, x_max, y_max = imbounds
        x_range = x_max - x_min
        y_range = y_max - y_min

        resolutionx = x_range / width
        resolutiony = y_range / height
        transform = from_origin(x_min, y_max, resolutionx, resolutiony)

        # Create an empty raster
        raster = refT.read(1, out_shape=(height, width))

    # Load the KML file
    gdf = gpd.read_file(kml_path, driver='KML')

    # Rasterize each feature based on its name
    for _, feature in gdf.iterrows():
        polygon_name = feature['Name']  # Assuming the name attribute holds the values
        polygon_value = int(polygon_name) if polygon_name.isdigit() else default_value
        mask = geometry_mask([feature['geometry'].buffer(resolutionx / 2)], transform=transform, invert=True, out_shape=(height, width))
        raster[mask] = polygon_value

    # Save the rasterized data to a new GeoTIFF file
    with rasterio.open(output_path, 'w', driver='GTiff',
                       width=width, height=height,
                       count=1, dtype=raster.dtype,
                       crs=ref_crs, transform=transform) as dst:
        dst.write(raster, 1)
        
def landcover_roads_to_fuel(S2GLC_tif,legend_file_path, WSEN, LBRT,output_dir,fuel_modifier=None, fuel_resolution = 10, no_fuel_code = 62):

    fuel_indices_origin = f"{output_dir}/fuel_indices_S2GLC.tif"
    fuel_indices_warped = f"{output_dir}/fuel_indices_warped.tif"
    fuel_indices_warped_clipped = f"{output_dir}/fuel_indices_warped_clipped.tif"
    
    roads_shape = f"{output_dir}/roads_shape.shp"
    water_shape = f"{output_dir}/water_shape.shp"
    fuel_road_indices = f'{output_dir}/fuel.tif'
     
    fuel_mod_road_indices = f'{output_dir}/fuelMod.tif'
    
    fuel_kml = f'{output_dir}/fuel.kml'
    fuel_png =f'{output_dir}/fuel.png'
    
    fuel_cbar_png =f'{output_dir}/fuel_bar.png'
    
    
      
    west, south, east, north =WSEN
    l,b,r,t = LBRT
    
    extract_subregion(S2GLC_tif, fuel_indices_origin, west, south, east, north)
    
    warp_and_clip_raster(fuel_indices_origin,fuel_indices_warped,fuel_indices_warped_clipped ,west, south, east, north,target_width = int((r-l)/fuel_resolution),target_height = int((t-b)/fuel_resolution))
    
    roads_shape = extract_roads_from_geotiff(west, south, east, north, roads_shape,attribute_widths=attribute_widths_road_Edge)
 

    water_shape = extract_waterways_from_geotiff(west, south, east, north, water_shape, attribute_widths=attribute_widths_waterway)
 
     
    rasterize_shapefiles(roads_shape,water_shape,fuel_indices_warped_clipped,fuel_road_indices,roads_attribute_widths=attribute_widths_road_Edge,water_attribute_widths=attribute_widths_waterway,imbounds=(west, south,east ,north), road_code=no_fuel_code, water_code=162)
    
    
    print("Rasterized roads and water")
    
    reftif = fuel_road_indices
    
    if fuel_modifier is not None:
        import fiona
        fiona.drvsupport.supported_drivers['KML'] = 'rw'
        rasterize_kml(fuel_modifier,fuel_road_indices,fuel_mod_road_indices)
        reftif = fuel_mod_road_indices
        print("applied ",fuel_modifier)

    generate_indexed_png_and_legend(legend_file_path,reftif, fuel_png, fuel_cbar_png)
    create_kml(west, south, east, north, "FUEL", fuel_png,fuel_kml , pngcbarfile=fuel_cbar_png)
    return fuel_png, fuel_kml, fuel_cbar_png
