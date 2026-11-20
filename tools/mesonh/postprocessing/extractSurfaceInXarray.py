import vtk
import numpy as np
from vtkmodules.util import numpy_support
import matplotlib.pyplot as plt
import os
import glob
import xarray as xr

def read_vtk(file_name):
    # Read the VTS file
    reader = vtk.vtkXMLStructuredGridReader()
    reader.SetFileName(file_name)
    reader.Update()

    # Get the extent of the data
    extent = reader.GetOutput().GetExtent()

    # Extract the specified variable
    data = reader.GetOutput().GetPointData().GetArray("Wind")

    # Convert to a numpy array
    data_np = numpy_support.vtk_to_numpy(data)

    # Reshape the array according to the grid dimensions
    ni, nj, nk = extent[1] + 1, extent[3] + 1, extent[5] + 1
    data_np = data_np.reshape((nk, nj, ni, -1))
    
    U = np.array(data_np[0,:,:,0])
    V = np.array(data_np[0,:,:,1])
    W = np.array(data_np[0,:,:,2])
    
    data = reader.GetOutput().GetPointData().GetArray("TKE")

    # Convert to a numpy array
    data_np = numpy_support.vtk_to_numpy(data)

    # Reshape the array according to the grid dimensions
    ni, nj, nk = extent[1] + 1, extent[3] + 1, extent[5] + 1
    data_np = data_np.reshape((nk, nj, ni, -1))
    
    TKE = np.array(data_np[0,:,:,0])
    
    newShape = (ni-1,nj-1)
    
    U_resized = np.zeros(newShape)
    V_resized = np.zeros(newShape)
    W_resized = W[1:,1:]
    TKE_resized = TKE[1:,1:]
        
    # Perform the averaging
    
    for i in range(newShape[0]):
        U_resized[i, :] = (U[i,:newShape[1]] + U[i+1,:newShape[1]]) / 2

    for j in range(newShape[1]):
        V_resized[:, j] = (V[:newShape[0],j] + V[:newShape[0], j+1]) / 2
        
    
    
    
    
    # Extract the U and V components for the specified extent
    #u_component = data_np[0, :extent[1]+1, :extent[3]+1, 0]
    #v_component = data_np[0, :extent[1]+1, :extent[3]+1, 1]

    return U_resized,V_resized,W_resized,TKE_resized

def process_files(file_dict, min_id, max_id, output_filename, PGD_path):
    # Open PGD file and extract the zsmt_slice
    pgd_ds = xr.open_dataset(PGD_path)
    zsmt_slice = pgd_ds['ZSMT'][1:-1, 1:-1]

    # Filter and sort the dictionary based on the specified range
    filtered_sorted_files = sorted(
        ((id, path) for id, path in file_dict.items() if min_id <= id <= max_id),
        key=lambda x: x[0]
    )

    # Initialize lists to store data for each variable
    U_data_all = []
    V_data_all = []
    W_data_all = []
    TKE_data_all = []
    time_coords = []
    fcount = 0
    for id, file_name in filtered_sorted_files:
        print("reading",id, "  num ", fcount ,"of ",len(filtered_sorted_files))
        U, V, W, TKE = read_vtk(file_name)
        U_data_all.append(U)
        V_data_all.append(V)
        W_data_all.append(W)
        TKE_data_all.append(TKE)
        time_coords.append(id)
        fcount = fcount +1

    # Convert lists to numpy arrays
    U_data = np.array(U_data_all)
    V_data = np.array(V_data_all)
    W_data = np.array(W_data_all)
    TKE_data = np.array(TKE_data_all)

    # Create an xarray Dataset with the same spatial coordinates as zsmt_slice
    ds = xr.Dataset({
        "U": (["time", "nj", "ni"], U_data),
        "V": (["time", "nj", "ni"], V_data),
        "W": (["time", "nj", "ni"], W_data),
        "TKE": (["time", "nj", "ni"], TKE_data),
        "altitude": (["nj", "ni"], zsmt_slice.data)  # Use the .data attribute
    }, coords={
        "time": time_coords,
        "ni": zsmt_slice.coords['ni'],
        "nj": zsmt_slice.coords['nj'],
        "latitude": (("nj", "ni"), zsmt_slice.coords['latitude'].data),
        "longitude": (("nj", "ni"), zsmt_slice.coords['longitude'].data)
    })

    # Save the dataset to a NetCDF file
    ds.to_netcdf(output_filename)
    return ds

def generate_file_list(path_files):
    # List all .vts files in the directory
    vts_files = glob.glob(os.path.join(path_files, '*.vts'))

    # Extract the unique identifiers from the filenames
    file_list = {}
    for file_path in vts_files:
        # Assuming file format is "output.full.<identifier>.vts"
        identifier = int(os.path.basename(file_path).split('.')[2])
        file_list[identifier] = file_path

    # Sort the dictionary by key (identifier)
    sorted_file_list = dict(sorted(file_list.items()))

    return sorted_file_list
sliceVTK = True
base_path = "/scratch/baggio_r/fcouto/KTEST_PEDROGAO/nest15020200809/"
pgdX =  base_path+"/001_pgd/PGD_D80mA.nc"
vtk_f_path = base_path+"/006_runff/vtkout3/"
file_list = generate_file_list(vtk_f_path)

output_filename = "processed_data.nc" 

process_files(file_list, 55000, 55030, output_filename, pgdX)
newds = xr.open_dataset(output_filename)

