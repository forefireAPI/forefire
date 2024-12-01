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
    tools.ensure_dir('./Inputs/')
    tools.ensure_dir('./ForeFire/')
    tools.ensure_dir('./ForeFire/Outputs/')
    #tools.ensure_dir('./MODEL1/')

    dir_input = args.input
    if dir_input is None: dir_input = '../006_mnhSolo/'
    domainNumber = int(args.domainNumber) if (args.domainNumber is not None) else None

    #read MNH info
    MNH_namelist = f90nml.read(dir_input+'/EXSEG1.nam')
    expr_name = MNH_namelist['NAM_CONF']['CEXP']
    MNH_domain   = tools.read_MNH_domain(expr_name, dir_mnh = dir_input, domainNumber = domainNumber, specificInputMNHFile=args.specificFile)

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
    altitude.rio.write_crs(lcp.rio.crs)
    altitude = altitude

    fuelmap = lcp.isel(band=3).drop_vars(['band','spatial_ref'])['band_data'] # dataarray
    fuelmap.attrs["type"] = "fuel"
    fuelmap.rio.write_crs(lcp.rio.crs)
    fuelmap = fuelmap

    heatFlux = fuelmap.copy()
    heatFlux[:,:]=0
    heatFlux.attrs["type"] = "flux"
    heatFlux.attrs["indices"] = 0
    heatFlux.attrs["model1name"] = "heatFluxBasic"

    moisture = fuelmap.copy()
    moisture[:,:]=0.1
    moisture.attrs["type"] = "data"

    vaporFlux = fuelmap.copy()
    vaporFlux[:,:]=1
    vaporFlux.attrs["type"] = "flux"
    vaporFlux.attrs["indices"] = 1
    vaporFlux.attrs["model1name"] = "vaporFluxBasic"

    windU = fuelmap.copy()
    windU[:,:]=0
    windU.attrs["type"] = "data"

    windV = fuelmap.copy()
    windV[:,:]=4
    windV.attrs["type"] = "data"

    ds = xr.Dataset({"altitude":altitude, "windU":windU, "windV":windV, "fuel": fuelmap, "heatFlux": heatFlux, "vaporFlux": vaporFlux, "moisture": moisture})
    #ds = xr.Dataset({"altitude":altitude, "fuel": fuelmap, "heatFlux": heatFlux, "vaporFlux": vaporFlux, "moisture": moisture})
    ds = ds.rio.write_crs(lcp.rio.crs)
    ds = ds.rio.reproject(4326)
 
    latmean = MNH_domain['lat2d'].mean()
    lonmean = MNH_domain['lon2d'].mean()
    distance_m = 10  # 1000 meters
    lat_offset, lon_offset =tools.offset_in_degrees(distance_m, latmean)

    nlon =int(np.ceil((MNH_domain['lon2d'][0,:].max()-MNH_domain['lon2d'][0,:].min())/lon_offset))
    nlat =int(np.ceil((MNH_domain['lat2d'][:,0].max()-MNH_domain['lat2d'][:,0].min())/lat_offset))

    print('dimension mydata.nc: lon lat')
    print(nlon, nlat)
    lon1d = np.linspace(MNH_domain['lon2d'][0,:].min(), MNH_domain['lon2d'][0,:].max(),nlon)
    lat1d = np.linspace(MNH_domain['lat2d'][:,0].min(), MNH_domain['lat2d'][:,0].max(),nlat)
    
    #to MNH domain
    ds = ds.interp(x=lon1d, y=lat1d, method='nearest')
    

    # then we to some cleaning 
    ds = ds.rename({'x':'DIMX'})
    ds = ds.rename({'y':'DIMY'})
    ds = ds.expand_dims(DIMZ=1)
    ds = ds.expand_dims(DIMT=1)
    ds = ds.transpose("DIMT", "DIMZ", "DIMY", "DIMX")

    ds['fuel'] = ds.fuel.astype('int32')
    ds['altitude'] = ds.altitude.astype('float64')

    date_ = datetime.strptime(MNH_domain['date'], "%Y-%m-%d_%H:%M:%S")


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
            "t0": np.float32(0.0),
            "Lt": np.float32(np.inf),  # Use numpy's representation of infinity
        },
    )

    ds = ds.drop_vars('DIMX')
    ds = ds.drop_vars('DIMY')
    ds = ds.drop_vars('domdim')
    ds = ds.drop_vars('spatial_ref')
    

    #print('###### no saving ########')
    #and save
    print('save ./ForeFire/mydata.nc')
    ds.to_netcdf('./ForeFire/mydata.nc')
    
