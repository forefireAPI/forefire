import numpy as np 
import matplotlib.pyplot as plt 
import xarray as xr
import rioxarray 
import pyforefire as forefire
import os 
import sys
import argparse
import f90nml
import importlib
import cftime
from datetime import datetime
import geopandas as gpd
import glob
import shapely
import pdb 
import pandas as pd 


#homebrewed
import create_data_tools as tools

###################################################
if __name__ == '__main__':
###################################################
    
    importlib.reload(tools)
    print('check env variable:')
    print('ntask =', os.environ['ntask'])
    

    parser = argparse.ArgumentParser(description='create data for MNH et forefire')
    parser.add_argument('-i','--input', help='Input MNH dir',required=False)
    parser.add_argument('-d','--domainNumber', help='domain number',required=False)
    parser.add_argument('-sf','--specificFile', help='to use in case of nested domain',required=False)
    args = parser.parse_args()

    ################### ... start input param

    #ff param
    #input_ff_param_file = './Inputs/ff-param.nam'

    ################### ... end input param

    #create dir
    tools.ensure_dir('./ForeFire/')
    tools.ensure_dir('./ForeFire/Outputs/')
    #tools.ensure_dir('./MODEL1/')

    dir_input = args.input
    if dir_input is None: dir_input = '../006_mnh/'
    domainNumber = int(args.domainNumber) if (args.domainNumber is not None) else None

    #read MNH info
    MNH_namelist = f90nml.read(dir_input+'/EXSEG1.nam')
    expr_name    = MNH_namelist['NAM_CONF']['CEXP']
    dtoutput_mnh    = MNH_namelist['NAM_BACKUP']['XBAK_TIME_FREQ']
    MNH_domain   = tools.read_MNH_domain(expr_name, dtoutput_mnh, dir_mnh = dir_input, domainNumber = domainNumber, specificInputMNHFile=args.specificFile)

    mnhfile = dir_input + 'Postproc/outputFile/Netcdf/{:s}_model{:d}.nc'.format(expr_name,domainNumber)
    mnhdata = xr.open_dataset(mnhfile, decode_times=False)
    # Extract the time variable
    time_var = mnhdata["time"]
    #Get the time units, and remove the non-standard prefix
    units = time_var.attrs['units'].replace('fire ignition: ', '')
    # Convert the time variable using cftime
    times = cftime.num2date(time_var.values, units=units, calendar='standard')
    # Convert to pandas datetime if needed
    times_as_datetime = [datetime(year=t.year, month=t.month, day=t.day, 
                              hour=t.hour, minute=t.minute, second=t.second,microsecond=t.microsecond)
                         for t in times]
    # Replace the time variable in the dataset with the converted times
    mnhdata["time"] = ("time", times_as_datetime)
    #set mnhdata to have the size of the original domain. add 1pt on the left and 2 on the right
    # Calculate spacing for each dimension
    dx = mnhdata.x[1] - mnhdata.x[0]  # Spacing in the x dimension
    dy = mnhdata.y[1] - mnhdata.y[0]  # Spacing in the y dimension
    dz = mnhdata.z[1] - mnhdata.z[0]  # Spacing in the z dimension
    # Define new coordinates
    new_x = np.concatenate(([mnhdata.x[0] - dx], mnhdata.x, [mnhdata.x[-1] + dx, mnhdata.x[-1] + 2 * dx]))
    new_y = np.concatenate(([mnhdata.y[0] - dy], mnhdata.y, [mnhdata.y[-1] + dy, mnhdata.y[-1] + 2 * dy]))
    new_z = np.concatenate(([mnhdata.z[0] - dz], mnhdata.z, [mnhdata.z[-1] + dz, mnhdata.z[-1] + 2 * dz]))
    # Reindex the dataset, filling new values with -9999
    mnhdata = mnhdata.reindex(
        x=new_x, 
        y=new_y, 
        z=new_z, 
        method=None,  # Explicitly admnhdata NaNs to new positions
        fill_value=-9999
    )

    # Set nodata value explicitly for all variables
    for var in mnhdata.data_vars:
        mnhdata[var].attrs['_FillValue'] = -9999
    mnhdata.attrs['nodata'] = -9999
   
    dirLCP = '/data/paugam/MNH/Cat_PdV/lcp/'
    lcp = xr.open_dataset(dirLCP+'lcp_pgd80_pdV.tif')
    
    altitude = lcp.isel(band=0).drop_vars(['band','spatial_ref'])['band_data'] # dataarray
    altitude.attrs["type"] = "data"
    altitude = altitude

    #MERDE
    altitude.values[:,:] = 100

    fuelmap = lcp.isel(band=3).drop_vars(['band','spatial_ref'])['band_data'] # dataarray
    fuelmap.attrs["type"] = "fuel"
    fuelmap = fuelmap

    '''
    #MERDE
    center_x = 408389.97
    center_y = 4617415.01

    # define the square size (2km x 2km = 2000m x 2000m)
    square_size = 2000  # in meters
    half_square_size = square_size / 2

    # define the range for the square
    x_min = center_x - half_square_size
    x_max = center_x + half_square_size
    y_min = center_y - half_square_size
    y_max = center_y + half_square_size

    # assuming fuelmap is already defined as an xarray.dataarray
    # if not, this would be the time to load the data (replace with your actual fuelmap data)
    # fuelmap = xr.open_dataarray('your_fuelmap_file.nc')  # example to load a file

    # make a copy of the fuelmap
    fuelmap_copy = fuelmap.copy()

    # define the x and y indices within the square bounds
    x_indices = (fuelmap.coords['x'] >= x_min) & (fuelmap.coords['x'] <= x_max)
    y_indices = (fuelmap.coords['y'] >= y_min) & (fuelmap.coords['y'] <= y_max)

    # modify the values in fuelmap_copy inside the defined square region
    fuelmap_copy.loc[{'x': x_indices, 'y': y_indices}] = 99  # set value 99 inside the square area

    # visualize the modified fuelmap_copy to ensure the path is added correctly
    #fuelmap_copy.plot()
    fuelmap = fuelmap_copy
    '''
    #MERDE
    fuelmap.values[:,:] = 145

    heatFlux = fuelmap.copy()
    heatFlux[:,:]=0
    heatFlux.attrs["type"] = "flux"
    heatFlux.attrs["indices"] = 0
    heatFlux.attrs["model1name"] = "heatFluxBasic"

    #moisture = fuelmap.copy()
    #moisture[:,:]=0.1
    #moisture.attrs["type"] = "data"

    vaporFlux = fuelmap.copy()
    vaporFlux[:,:]=1
    vaporFlux.attrs["type"] = "flux"
    vaporFlux.attrs["indices"] = 1
    vaporFlux.attrs["model1name"] = "vaporFluxBasic"

    #windU = fuelmap.copy()
    #windU[:,:]=0
    #windU.attrs["type"] = "data"

    #windV = fuelmap.copy()
    #windV[:,:]=4
    #windV.attrs["type"] = "data"

    ds = xr.Dataset({"fuel": fuelmap, "heatFlux": heatFlux, "vaporFlux": vaporFlux, } )
    #ds = xr.Dataset({"altitude":altitude, "fuel": fuelmap, "heatFlux": heatFlux, "vaporFlux": vaporFlux, } )
    #ds = xr.Dataset({"altitude":altitude, "windU":windU, "windV":windV, "fuel": fuelmap, "heatFlux": heatFlux, "vaporFlux": vaporFlux, "moisture": moisture})
    #ds = xr.Dataset({"altitude":altitude, "fuel": fuelmap, "heatFlux": heatFlux, "vaporFlux": vaporFlux, "moisture": moisture})
    ds = ds.rio.write_crs(lcp.rio.crs)
    ds = ds.rio.reproject(4326)

    latmean = MNH_domain['lat2d'].mean()
    lonmean = MNH_domain['lon2d'].mean()
    distance_m = 20  # 1000 meters
    lat_offset, lon_offset =tools.offset_in_degrees(distance_m, latmean)

    nlon =int(np.ceil((MNH_domain['lon2d'][0,:].max()-MNH_domain['lon2d'][0,:].min())/lon_offset))
    nlat =int(np.ceil((MNH_domain['lat2d'][:,0].max()-MNH_domain['lat2d'][:,0].min())/lat_offset))

    print('dimension mydata.nc: lon lat')
    print(nlon, nlat)
    lon1d = np.linspace(MNH_domain['lon2d'][0,:].min(), MNH_domain['lon2d'][0,:].max(),nlon)
    lat1d = np.linspace(MNH_domain['lat2d'][:,0].min(), MNH_domain['lat2d'][:,0].max(),nlat)
    
    #to MNH domain
    ds = ds.interp(x=lon1d, y=lat1d, method='nearest')
    

    # then we do some cleaning 
    ds = ds.rename({'x':'DIMX'})
    ds = ds.rename({'y':'DIMY'})
    ds = ds.expand_dims(DIMZ=1)
    ds = ds.expand_dims(DIMT=1)
    ds = ds.transpose("DIMT", "DIMZ", "DIMY", "DIMX")

    ds['fuel'] = ds.fuel.astype('int32')
    #ds['altitude'] = ds.altitude.astype('float64')

    date_ = datetime.strptime(MNH_domain['date'], "%Y-%m-%d_%H:%M:%S")


    #add moisture
    dirFWI = '/data/paugam/MNH/Cat_PdV/006_mnhSolo/Postproc/FWI/'
    fwi = xr.open_dataset(dirFWI + 'FCAST_model3_fwi.nc')
    crs = tools.load_crs_from_prj(dirFWI + 'FCAST_model3_fwi.prj')
    fwi.rio.write_crs(crs,inplace=True)

    dx_fwi = fwi.x[1]-fwi.x[0]
    factor = np.round(dx_fwi/distance_m,0)
    lon1d_fwi = np.linspace(MNH_domain['lon2d'][0,:].min(), MNH_domain['lon2d'][0,:].max(),int(nlon/factor))
    lat1d_fwi = np.linspace(MNH_domain['lat2d'][:,0].min(), MNH_domain['lat2d'][:,0].max(),int(nlat/factor))
    
    fwi_ll = fwi.rio.reproject(4326)
    fwi_ = fwi_ll.interp(x=lon1d_fwi, y=lat1d_fwi, method='nearest')
    fwi_ = fwi_.rename({'time':'mnh_t','y':'mnh_y','x':'mnh_x', }) 

    #ds['mnh_x'] = (['mnh_x'], lon1d_fwi)
    #ds['mnh_y'] = (['mnh_y'], lat1d_fwi)
    #ds['mnh_z'] = (['mnh_z'], [0])
    #ds['mnh_t'] = (['mnh_t'], fwi_.mnh_t.values)
    #ds['mnh_t'] = (['mnh_t'], [0])

    #ds['moisture'] = 1.e-2* fwi_.FMC.isel(mnh_t = slice(0,1)).expand_dims(dim=["mnh_z",])
    moisture_ = 1.e-2* fwi_.FMC.expand_dims(dim=["mnh_z",]).transpose('mnh_t', 'mnh_z', 'mnh_y', 'mnh_x')
    moisture_ = moisture_.astype(np.float32).fillna(-9999)

    #take the mean 
    start_time = pd.to_datetime("2022-07-17T11:00:00.000000000")
    start_time_s = (start_time - date_).total_seconds()

    end___time = pd.to_datetime("2022-07-17T18:00:00.000000000")
    end___time_s = (end___time - date_).total_seconds()
    ds['moisture'] = moisture_.sel(mnh_t=slice(start_time, end___time)).mean(dim="mnh_t").expand_dims(dim=["mnh_t",])
    ds['moisture'].attrs["type"] = "data"
    
    #MERDE
    ds['moisture'].values = (np.zeros(ds['moisture'].values.shape)+0.06).astype(np.float32)

    # add wind
    dirMNH = '/data/paugam/MNH/Cat_PdV/006_mnhSolo/Postproc/outputFile/Netcdf/'
    mnh = xr.open_dataset(dirMNH + 'FCAST_model3_utm.nc',decode_times=False)
    crs = tools.load_crs_from_prj(dirMNH + 'FCAST_model3_utm.prj')
    mnh.rio.write_crs(crs,inplace=True)

    dx_mnh = mnh.x[1]-mnh.x[0]
    factor = np.round(dx_mnh/distance_m,0)
    lon1d_mnh = np.linspace(MNH_domain['lon2d'][0,:].min(), MNH_domain['lon2d'][0,:].max(),int(nlon/factor))
    lat1d_mnh = np.linspace(MNH_domain['lat2d'][:,0].min(), MNH_domain['lat2d'][:,0].max(),int(nlat/factor))
   
    #mnhu = mnh.u
    #mnh_ll = mnhu.rio.reproject(4326)
    #mnh_u = mnh_ll.u.interp(x=lon1d_mnh, y=lat1d_mnh, method='nearest')
    
    reprojected_u = []
    reprojected_v = []

    # Iterate over each time step
    for t in range(mnh.u.shape[0]):  # mnhu.shape[0] is the number of time steps
        # Reproject the u variable for the current time slice (t)
        reprojectedu_slice = mnh.u.isel(time=t).rio.reproject(4326)
        reprojectedv_slice = mnh.v.isel(time=t).rio.reproject(4326)
        
        # Append the reprojected slice to the list
        reprojected_u.append(reprojectedu_slice)
        reprojected_v.append(reprojectedv_slice)

    # After loop, stack the reprojected slices back together into a new xarray
    mnhu_ll = xr.concat(reprojected_u, dim="time")
    mnhv_ll = xr.concat(reprojected_v, dim="time")
    
    mnh_u = mnhu_ll.interp(x=lon1d_mnh, y=lat1d_mnh, method='nearest')
    mnh_v = mnhv_ll.interp(x=lon1d_mnh, y=lat1d_mnh, method='nearest')
    
    mnh_u = mnh_u.rename({'time':'mnh_t','z':'mnh_z', 'y':'mnh_y', 'x':'mnh_x', }) 
    mnh_v = mnh_v.rename({'time':'mnh_t','z':'mnh_z', 'y':'mnh_y', 'x':'mnh_x', }) 


    
    #UNCOMMENT BELOW MERDE
    #update altitude with the one of MNH
    #mnh_alt = mnh['altitude'].isel(z=0)
    #mnh_alt_ll = mnh_alt.rio.reproject(4326)
    #mnh_alt_reproj = mnh_alt_ll.interp(x=lon1d_mnh, y=lat1d_mnh, method='nearest')
    #ds['altitude'] = mnh_alt_reproj.rename({'y':'mnh_y', 'x':'mnh_x', }).drop_vars(['mnh_x','mnh_y','z']).astype('float32').expand_dims(dim=["mnh_t","mnh_z"]).fillna(-9999)
    #ds['altitude'].attrs["type"] = "data"
   

    # add extra attr 
    ds = ds.assign_coords(domdim=("domdim", [1]))  # Adding "domdim" as a new dimension
    ds["parameters"] = xr.DataArray(
                        data=["A"],  # You can replace "" with actual parameter strings
                        dims=["domdim"],
                        attrs={
                                "type": "parameters",
                                 "refYear": np.int32(date_.year),
                                 "refDay": np.int32(date_.timetuple().tm_yday),
                                 "duration": "None",
                                 "projection": "None",
                                 "projectionproperties": "None",
                                },
                        ) 
    ds["domain"] = xr.DataArray(
        data=["A"],  # Replace with actual data if needed
        dims=["domdim"],
        attrs={
            "type": "domain",
            "NEx": np.float32(MNH_domain['NEx']),
            "NEy": np.float32(MNH_domain['NEy']),
            "NEz": np.float32(0),
            "SWx": np.float32(MNH_domain['SWx']),
            "SWy": np.float32(MNH_domain['SWy']),
            "SWz": np.float32(0),
            "Lx": np.float32(MNH_domain['Lx']),
            "Ly": np.float32(MNH_domain['Ly']),
            "Lz": np.float32(0),
            "t0": np.float32(0), #(ds.mnh_t[0].values -np.datetime64(date_)).item()*1.e-9 , 
            "Lt": np.float32(np.inf), #(ds.mnh_t[-1].values-np.datetime64(date_)).item()*1.e-9,  # Use numpy's representation of infinity
            #"t0": np.float32( (ds.mnh_t[0].values -np.datetime64(date_)).item()*1.e-9 ), 
            #"Lt": np.float32( (ds.mnh_t[-1].values - ds.mnh_t[0].values).item()*1.e-9  ),  # Use numpy's representation of infinity
            "epsg": lcp.rio.crs.to_epsg(),  # Use numpy's representation of infinity
            },
    )


    #if ignition shp file present in the ForeFire directory, we give the coordinate to input in the Init.ff file
    try: 
        ingitionFile = glob.glob('./ForeFire/ignition/pointIgnition_t0/ignition-pdv.shp')[0]
        print('ignition file found')
        igni = gpd.read_file(ingitionFile)
        igni_utm = igni.to_crs(lcp.rio.crs.to_epsg())
        print(igni) 
        ref_ll  = shapely.Point((MNH_domain['lon2d'][0,0],MNH_domain['lat2d'][0,0]) )
        ref_ll  = gpd.GeoDataFrame([{'geometry': ref_ll}], crs="EPSG:4326")
        ref_utm = ref_ll.to_crs(lcp.rio.crs.to_epsg())
        
        xi = igni_utm.geometry.x - ref_utm.geometry.x + MNH_domain['SWx']
        yj = igni_utm.geometry.y - ref_utm.geometry.y + MNH_domain['SWy']
        
        print( "startFire[loc=({:.1f},{:.1f},0.);t=20]".format(xi[0],
                                                               yj[0] ) )

    except: 
        print('no ignition file')

    try: 
        referenceFile = glob.glob('./ForeFire/ignition/referencePoints/*.shp')
        if len(referenceFile)>0:
            print('ref file found')
            for referenceFile_ in referenceFile:
                print(referenceFile__.split('/')[-1])
                igni = gpd.read_file(referenceFile_)
                igni_utm = igni.to_crs(lcp.rio.crs.to_epsg())
                
                ref_ll  = shapely.Point((MNH_domain['lon2d'][0,0],MNH_domain['lat2d'][0,0]) )
                ref_ll  = gpd.GeoDataFrame([{'geometry': ref_ll}], crs="EPSG:4326")
                ref_utm = ref_ll.to_crs(lcp.rio.crs.to_epsg())
                
                xi = igni_utm.geometry.x - ref_utm.geometry.x + MNH_domain['SWx']
                yj = igni_utm.geometry.y - ref_utm.geometry.y + MNH_domain['SWy']
               
                print( "[loc=({:.1f},{:.1f},0.);t=20]".format(xi[0],
                                                                       yj[0] ) )

    except: 
        print('no reference file for point on map')




    windU = mnh_u.isel(mnh_z=slice(0,1), ).fillna(-9999).astype(np.float32)
    windU_m = windU.sel(mnh_t=slice(start_time_s, end___time_s)).mean(dim="mnh_t").expand_dims(dim=["mnh_t",])
    windU_m = windU_m.drop_vars('mnh_z')
    windU_m = windU_m.drop_vars('mnh_y')
    windU_m = windU_m.drop_vars('mnh_x')
    ds['windU'] = windU_m
    ds['windU'].attrs["type"] = "data"

    #MERDE
    #ds['windU'].values = (np.zeros(ds['windU'].values.shape)-0.302).astype(np.float32)
    ds['windU'].values = (np.zeros(ds['windU'].values.shape)).astype(np.float32)

    windV = mnh_v.isel(mnh_z=slice(0,1), ).fillna(-9999).astype(np.float32)
    windV_m = windV.sel(mnh_t=slice(start_time_s, end___time_s)).mean(dim="mnh_t").expand_dims(dim=["mnh_t",])
    windV_m = windV_m.drop_vars('mnh_z')
    windV_m = windV_m.drop_vars('mnh_y')
    windV_m = windV_m.drop_vars('mnh_x')
    ds['windV'] = windV_m
    ds['windV'].attrs["type"] = "data"

    #MERDE
    ds['windV'].values = (np.zeros(ds['windV'].values.shape)+1.9207).astype(np.float32)

    ds = ds.drop_vars('DIMX')
    ds = ds.drop_vars('DIMY')
    ds = ds.drop_vars('domdim')
    ds = ds.drop_vars('spatial_ref')
    ds = ds.drop_vars('mnh_x')
    ds = ds.drop_vars('mnh_y')
    #ds = ds.drop_vars('mnh_t')    
    
    #and save
    print('save ./ForeFire/mydata.nc')
    if os.path.isfile('./ForeFire/mydata.nc'):
        os.remove('./ForeFire/mydata.nc')
    ds.to_netcdf('./ForeFire/mydata.nc')
    
