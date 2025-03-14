import numpy as np 
import pyvista as pv
import geopandas as gpd
from shapely.geometry import Point, Polygon
import pandas as pd
import matplotlib.pyplot as plt
import sys
import cv2 
import os 
import xarray as xr
from scipy import ndimage 
import numpy as np
import xarray as xr
from scipy.ndimage import distance_transform_edt
import dask.array as da
from scipy.ndimage import gaussian_filter
import glob 
from affine import Affine
from rasterio.features import rasterize
from scipy import interpolate
import rasterio
import xarray as xr
from rasterio.crs import CRS
import pyproj

################################
def fill_missing_nearest_2d(da):
    """
    Fill NaN values in a 2D xarray DataArray with the value of the nearest valid cell (in Euclidean distance).

    Parameters
    ----------
    da : xr.DataArray
        A 2D DataArray (dims might be something like ("y", "x")) that contains NaNs.

    Returns
    -------
    xr.DataArray
        A new DataArray of the same shape and coordinates, but with NaN values filled
        by the nearest non-NaN neighbor in Euclidean distance.

    Notes
    -----
    1. This approach uses scipy.ndimage.distance_transform_edt to compute, for each NaN cell,
       the indices of the closest non-NaN cell. 
    2. If your grid is very large, this can be memory-intensive. Consider chunking or a 
       KD-tree approach if needed.
    3. This method does not differentiate between “inside” or “outside” convex regions
       of valid data; it simply finds the closest valid cell for every NaN location.
    """
    if da.ndim != 2:
        raise ValueError("This function is designed for 2D DataArrays only.")

    # Get raw numpy array and create a mask of where values are NaN
    arr = da.values
    mask_nan = np.isnan(arr)

    # distance_transform_edt returns:
    #   dist : array of distances
    #   idx  : array of indices of nearest non-NaN cell
    dist, idx = distance_transform_edt(mask_nan, return_indices=True)

    # Copy original data so we can fill in
    arr_filled = arr.copy()

    # Fill in NaN values by indexing the nearest non-NaN cell
    arr_filled[mask_nan] = arr_filled[idx[0, mask_nan], idx[1, mask_nan]]

    # Return a new xarray DataArray with the same dims/coords
    return xr.DataArray(arr_filled, coords=da.coords, dims=da.dims)


simuName = os.getcwd().split('/')[-1]

#load fartsite tif ros
#####################
# Open the .tif file with rasterio
dirfarsite = '/data/paugam/Data/ElPontDeVilamora/FARSITE-UPC/'
with rasterio.open(dirfarsite+'ros_farsite.tif') as src:
    # Read the image data into a numpy array
    data = src.read(1)  # Assuming single-band data
    
    # Create an xarray DataArray
    farsite = xr.DataArray(data, dims=["y", "x"], coords={"y": src.bounds.top - src.res[1] * np.arange(src.height),
                                                     "x": src.bounds.left + src.res[0] * np.arange(src.width)})

# Assign CRS using the EPSG code 28831
farsite.attrs['crs'] = CRS.from_epsg(25831).to_string()
farsite = farsite.where(farsite != -9999, np.nan) 

non_nan_mask = farsite.notnull()

# Get the indices of the non-NaN values (bounding box)
y_minP, y_maxP = farsite.coords["y"][non_nan_mask.any(dim="x")].min(), farsite.coords["y"][non_nan_mask.any(dim="x")].max()
x_minP, x_maxP = farsite.coords["x"][non_nan_mask.any(dim="y")].min(), farsite.coords["x"][non_nan_mask.any(dim="y")].max()
buffer = 1000


#laod mnh output in lat lon 
###########################
mnh = xr.open_dataset('../006_mnhSolo/Postproc/outputFile/Netcdf/FCAST_model3.nc', decode_times=False)
refdate = pd.Timestamp('2022-07-17 00:00:00')


# Load the vtu file and rasterize velocity
###################################
filesPVD = glob.glob("./vtkFront/*.vtu")
rosmap = farsite.copy()
rosmap.data[:,:] = -999

##################################
def get_interp(x,y,z,mask=None):
    coord_pts = np.vstack((x.flatten(), y.flatten())).T
    data      = z.flatten()
    fill_val  = -999
    return interpolate.LinearNDInterpolator(coord_pts, data, fill_value=fill_val)
   
y2d,x2d = np.meshgrid(mnh.y, mnh.x)
lon_interpolator     = get_interp(x2d,y2d, mnh.lon.data)
lat_interpolator     = get_interp(x2d,y2d, mnh.lat.data)
# Définir les systèmes de coordonnées : WGS84 (lat, lon) et UTM Zone 31N (EPSG:25831)
wgs84 = pyproj.CRS('EPSG:4326')  # Système de référence pour latitude/longitude
utm31n = pyproj.CRS('EPSG:25831')  # Système de projection UTM Zone 31N

# Créer un transformateur pour effectuer la conversion entre les deux CRS
transformer = pyproj.Transformer.from_crs(wgs84, utm31n, always_xy=True)

for iifile, file_ in enumerate(filesPVD):
    print(file_)
    front = pv.read(file_)

    points = front.points
    vmod = front.point_data['vmod']
    velx = front.point_data['velocity'][:,0]
    vely = front.point_data['velocity'][:,1]
    velz = front.point_data['velocity'][:,2]

    data = {
        'geometry': [Point(p) for p in points],  # Create Point geometries
        'vmod': vmod,  # vmod data,
        'u': velx,  # velocity data
        'v': vely,  # velocity data
        'w': velz,  # velocity data
    }

    gdf = gpd.GeoDataFrame(data)  # You can adjust the CRS if needed

    # Define the grid and extraction bounds from the MNH dataset
    x_min, x_max = farsite.x.min().item(), farsite.x.max().item()
    y_min, y_max = farsite.y.min().item(), farsite.y.max().item()
    resolution_x = abs(farsite.x[1] - farsite.x[0]).item()
    resolution_y = abs(farsite.y[1] - farsite.y[0]).item()

    # Create the transformation matrix for mapping coordinates to pixel locations
    transform = Affine(resolution_x, 0.0, x_min, 0.0, -resolution_y, y_max)


    # Create a list of shapes (geometries and associated value)
    
    shapes = []
    for geom,val in zip(gdf.geometry,gdf.vmod): 
        lon_interpolator(geom.x, geom.y)
        x, y = transformer.transform(lon_interpolator(geom.x, geom.y), lat_interpolator(geom.x, geom.y))
        shapes.append( (Point(x,y), val ) ) 


    # Rasterize the shapes onto the grid defined by bmap_po
    rasterized = rasterize(
        shapes=shapes,
        out_shape=farsite.shape[::-1],
        transform=transform,
        fill=-999,  # Background value (no data value)
        dtype=np.float32
    )

    # Adjust the raster for any necessary flipping (this might depend on how your grid is oriented)
    tmp = rasterized.T[::-1,::-1]  # Flip vertically if needed

    # Update the bmap_po data where the raster has non-background values
    rosmap.data = np.where( tmp > rosmap.data, tmp,  rosmap.data )


rosmap = rosmap.where(rosmap != -999, np.nan) * 60 # m/min
#rosmap = rosmap.assign_coords(lat=mnh.lat, lon=mnh.lon)
#rosmap = rosmap.drop_vars(['x', 'y'])  # Drop the old x and y coordinates
#rosmap = rosmap.rename({'x': 'lon', 'y': 'lat'})

#set to 1d grid
#points = np.array([rosmap.lon.data.flatten(), rosmap.lat.data.flatten()]).T  # (n_points, 2)
#values = rosmap.data.flatten() 

#new_lat_1d = np.linspace(np.min(mnh.lat), np.max(mnh.lat), mnh.lat.shape[0])
#new_lon_1d = np.linspace(np.min(mnh.lon), np.max(mnh.lon), mnh.lat.shape[1])
#new_lat_grid, new_lon_grid = np.meshgrid(new_lat_1d, new_lon_1d)

# Perform the interpolation using griddata
#rosmap_interp = interpolate.griddata(points, values, (new_lon_grid,new_lat_grid), method='nearest')

#rosmap = xr.DataArray(
#    rosmap_interp.T,  # The interpolated data
#    coords={'lat': new_lat_1d, 'lon': new_lon_1d},  # 1D coordinates
#    dims=['lat', 'lon'],  # Dimension names
#)
#rosmap.rio.write_crs("EPSG:4326", inplace=True)
#rosmap = rosmap.rename({'lat':'y', 'lon':'x'})
#rosmap = rosmap.rio.reproject(25831)



ax = plt.subplot(121)
rosmap.plot(ax=ax, ) #vmax=15)
ax.set_title('ff')
ax.set_xlim(x_minP-buffer, x_maxP+buffer)
ax.set_ylim(y_minP-buffer, y_maxP+buffer)

ax = plt.subplot(122)
farsite.plot(ax=ax, vmax=15)
ax.set_title('farsite')
ax.set_xlim(x_minP-buffer, x_maxP+buffer)
ax.set_ylim(y_minP-buffer, y_maxP+buffer)


plt.show()
sys.exit()
    
    


# Load the vts file
fileVTU = "./vtkmap3.vts"
mesh = pv.read(fileVTU)

cell_centers = mesh.cell_centers()
x = cell_centers.points[:, 0]
y = cell_centers.points[:, 1]
bmap_cell = mesh.cell_data['atime']

# Get the unique x and y values to reshape data into a 2D grid
unique_x = np.unique(x)
unique_y = np.unique(y)

# Create a grid (2D array) to map the cell data
# We use the unique x and y values to determine the grid shape
bmap2d = np.full((len(unique_y), len(unique_x)), np.nan)

# Map the cell data onto the grid by matching cell centers to grid coordinates
for i, (xi, yi) in enumerate(zip(x, y)):
    # Find the closest grid point in x and y
    x_idx = np.abs(unique_x - xi).argmin()
    y_idx = np.abs(unique_y - yi).argmin()
    
    # Set the cell data at the corresponding grid point
    bmap2d[y_idx, x_idx] = bmap_cell[i]

factorNiceFront = 4
'''
unique_x2 = np.linspace(unique_x.min(),unique_x.max(),unique_x.shape[0]*factorNiceFront)
unique_y2 = np.linspace(unique_y.min(),unique_y.max(),unique_y.shape[0]*factorNiceFront)
bmap2d2 =ndimage.zoom(bmap2d, factorNiceFront, order=0)
bmap2d2[np.where(bmap2d2>bmap2d.max())] = bmap2d.max()
bmap2d2[np.where(bmap2d2<0)] = -9999
sigma = 2  # The standard deviation of the Gaussian kernel
bmap2d2 = ndimage.gaussian_filter(bmap2d2, sigma=sigma)
'''

dabmap = xr.DataArray(bmap2d, coords=[("y", unique_y), ("x", unique_x)])

new_x = np.linspace(dabmap.x.min(), dabmap.x.max(), dabmap.sizes['x'] * factorNiceFront)
new_y = np.linspace(dabmap.y.min(), dabmap.y.max(), dabmap.sizes['y'] * factorNiceFront)

mnhlon = mnhrosmap.lon.interp( x=new_x, y=new_y, method='linear' )
mnhlat = mnh.lat.interp( x=new_x, y=new_y, method='linear' )

# Interpolate to the new grid
dabmap_high_res = dabmap.where(dabmap>0).interp(x=new_x, y=new_y, method='linear')

mask = da.isnan(dabmap_high_res)
dabmap_high_res = fill_missing_nearest_2d(dabmap_high_res)

# Apply Gaussian smoothing
sigma = 1.5  # Standard deviation for Gaussian kernel
dabmap_smoothed = xr.DataArray(
                                gaussian_filter(dabmap_high_res.where(dabmap_high_res>0), sigma=sigma),
                                coords=dabmap_high_res.coords,
                                dims=dabmap_high_res.dims,
                              )

dabmap_smoothed = dabmap_smoothed.where(~mask, np.nan)
dabmap_smoothed = dabmap_smoothed.fillna(-9999)

bmap2d   = dabmap_smoothed.values
unique_x = dabmap_smoothed.x.values
unique_y = dabmap_smoothed.y.values

times = np.unique(bmap2d)
gdf = None
dt = 600.

timerange = np.arange(times[np.where(times>0)].min(), times[np.where(times>0)].max()+dt, dt)
for it, time in enumerate(timerange): 
    if time < 0 : continue
    contours,hierarchy  = cv2.findContours(np.where((bmap2d<=time)&(bmap2d>0),1,0).astype(np.uint8),cv2.RETR_TREE,cv2.CHAIN_APPROX_NONE)
    
    #convert contours to lat lon
    contours_ll = []
    for ic, contour in enumerate(contours):
        if len(contour)<=3: continue
        x_ = unique_x[contour[:,0,0]]
        y_ = unique_y[contour[:,0,1]]
    
        lat_ = []; lon_ = []
        for ii,jj in zip(contour[:,0,0],contour[:,0,1]): 
            lat_.append(mnhlat[jj,ii])
            lon_.append(mnhlon[jj,ii])
        
        contours_ll_ = np.array([lon_, lat_]).T.reshape(-1,1,2)
        contours_ll.append(contours_ll_)
     
        #contours_ll_ = np.array([x_, y_]).T.reshape(-1,1,2)
        #contours_ll.append(contours_ll_)

    # Create a list of polygons
    # this codes shoyld only work for main polygon with holes indise. new polygon inside hole are not tackle
    polygons = []
    for i, contour in enumerate(contours_ll):
        if len(contour)<=3: continue
        contour_points = contour.reshape(-1, 2)
        
        if hierarchy[0][i][3] == -1:  # Has a hole
            
            holes = []
            # Find the child contours (holes)
            for j, child_contour in enumerate(contours_ll):
                if hierarchy[0][j][3] == i:
                    holes.append(child_contour.reshape(-1, 2))
           
            if len(holes) == 0 : 
                polygon = Polygon(contour_points)
            else:
                polygon = Polygon(contour_points, holes=holes)
        
        polygons.append(polygon)

    # Create a GeoDataFrame from the list of polygons
    if len(polygons) == 0 : 
        continue
    gdf_ = gpd.GeoDataFrame({'geometry': polygons})
    gdf_ = gdf_.set_crs(4326)
    #gdf_.geometry = gdf_.buffer(0)
    gdf_['timestamp'] = refdate + pd.Timedelta(seconds=time)
    gdf_['time_seconds'] = time
        
    if gdf is None: 
        gdf = gdf_
    else: 
        gdf = pd.concat([gdf, gdf_]).reset_index(drop=True)

    print( '{:.1f} | {:}'.format(100.*it/len(timerange), gdf_['timestamp'].iloc[0]) , end='\r')

os.makedirs('./GPKG/', exist_ok=True)
if os.path.isfile("./GPKG/frontModel3_{:s}.gpkg".format(simuName)):
    os.remove("./GPKG/frontModel3_{:s}.gpkg".format(simuName))
gdf.to_file("./GPKG/frontModel3_{:s}.gpkg".format(simuName), driver="GPKG")

    
