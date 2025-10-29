# -*- coding: utf-8 -*-
"""
Created on Thu Jul 18 10:23:07 2024
Polished on Wed OCt 29 08:35:00 2025

@author: Batti Filippi
@co-author: Fernando Veiga
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import pyforefire as forefire
import math as math
import pyforefire as pyff
import struct, os
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.patches as patches
import numpy as np
import matplotlib
import math
import datetime
import pyforefire.helpers as ffhelpers

def plot_test(pathes, myExtents, bg_array=None):
    """
    Plot 4 axis graph with:
      - A blue square corresponding to the given extents.
      - A grid to aid in measurements.
      - Each path drawn in a different color sampled from the inferno colormap.
      - An optional background image (grey colormap) spread over the entire extent.
        NaN or infinity in the background array are made transparent.
    
    Parameters:
      pathes   : list of matplotlib.path.Path objects to be drawn.
      myExtents: [xmin, xmax, ymin, ymax] specifying the spatial extents.
      bg_array : (optional) 2D numpy array for background image.
    """
    fig, ax = plt.subplots(figsize=(10, 7), dpi=120)

    # If a background array is provided, plot it
    if bg_array is not None:
        # Mask invalid data (NaN or inf)
        bg_masked = np.ma.masked_invalid(bg_array)
        print(np.min(bg_masked), " max ", np.max(bg_masked))
        ax.imshow(bg_masked, extent=myExtents, cmap='gray', origin='lower')

    # Add a blue square representing the extent
    xmin, xmax, ymin, ymax = myExtents
    width = xmax - xmin
    height = ymax - ymin
    blue_square = patches.Rectangle((xmin, ymin), width, height,
                                    linewidth=2, edgecolor='blue', facecolor='none')
    ax.add_patch(blue_square)
    
    # Set the grid for measurements
    ax.grid(True)

    # Create a colormap instance from inferno and determine colors for each path
    cmap = matplotlib.colormaps['inferno']
    
    for idx, path in enumerate(pathes):
        # Use a different color for each path
        color = cmap(idx)
        patch = mpatches.PathPatch(path, edgecolor=color, facecolor='none', lw=2)
        ax.add_patch(patch)
    
    ax.axis('equal')
    ax.set_xlim(xmin, xmax)
    ax.set_ylim(ymin, ymax)
    #plt.show()

def load_weather_file(file_path, default_step_size=1800):
    """
    Parses a weather file and runs a simulation using wind speed and direction data.
    
    Parameters:
      file_path (str): Path to the weather file.
      default_step_size (float): Step size in seconds used for the last record if no next time is available.
      
    Returns:
      pathes (list): A list of simulation path data (as returned by pyff.helpers.printToPathe).
    """
    pathes = []
    data_records = []
    
    # Open the file and skip metadata lines until the header is found.
    with open(file_path, 'r') as f:
        for line in f:
            if line.strip().startswith("Year"):
                header = line.strip().split()
                print(header)
                break
        
        # Read the remaining lines as data rows.
        for line in f:
            if not line.strip():
                continue
            parts = line.strip().split()
            # Expecting: Year, Mth, Day, Time, Temp, RH, HrlyPcp, WindSpd, WindDir, CloudCov
            if len(parts) < 10:
                continue
            
            try:
                year = int(parts[0])
                month = int(parts[1])
                day = int(parts[2])
                # Parse time assuming a HHMM format (e.g. "0130" -> 1:30 AM)
                time_str = parts[3]
                hour = int(time_str[:2])
                minute = int(time_str[2:])
                dt_obj = datetime.datetime(year, month, day, hour, minute)
                
                wind_spd = float(parts[7])
                wind_dir = float(parts[8])
            except ValueError:
                continue  # Skip lines with parsing issues

            data_records.append({
                'datetime': dt_obj,
                'wind_spd': wind_spd,
                'wind_dir': wind_dir
            })
    return data_records

def estimate_utm_zone(easting, latitude):
    """
    Estimate the UTM zone given the easting coordinate and approximate latitude.
    UTM zones are 6 degrees wide, starting from -180° longitude.
    """
    # Approximate longitude from UTM easting (assuming central meridian at 500,000m false easting)
    longitude = ((easting - 500000) / 0.9996) / 111320  # Approximate inverse scaling

    # Estimate UTM zone from longitude
    utm_zone = int((longitude + 180) / 6) + 1
    
    return longitude, utm_zone

def readLCPFile(filename):
    result = {"filename": filename}
    
    # Define the header fields: (key, struct format, number of bytes)
    header_fields = [
        ("CrownFuels", "<i", 4),
        ("GroundFuels", "<i", 4),
        ("latitude", "<i", 4),
        ("loeast", "<d", 8),
        ("hieast", "<d", 8),
        ("lonorth", "<d", 8),
        ("hinorth", "<d", 8),
        ("loelev", "<i", 4),
        ("hielev", "<i", 4),
        ("numelev", "<i", 4),
        ("elevs", "<100i", 400),
        ("loslope", "<i", 4),
        ("hislope", "<i", 4),
        ("numslope", "<i", 4),
        ("slopes", "<100i", 400),
        ("loaspect", "<i", 4),
        ("hiaspect", "<i", 4),
        ("numaspect", "<i", 4),
        ("aspects", "<100i", 400),
        ("lofuel", "<i", 4),
        ("hifuel", "<i", 4),
        ("numfuel", "<i", 4),
        ("fuels", "<100i", 400),
        ("locover", "<i", 4),
        ("hicover", "<i", 4),
        ("numcover", "<i", 4),
        ("covers", "<100i", 400),
        ("loheight", "<i", 4),
        ("hiheight", "<i", 4),
        ("numheight", "<i", 4),
        ("heights", "<100i", 400),
        ("lobase", "<i", 4),
        ("hibase", "<i", 4),
        ("numbase", "<i", 4),
        ("bases", "<100i", 400),
        ("lodensity", "<i", 4),
        ("hidensity", "<i", 4),
        ("numdensity", "<i", 4),
        ("densities", "<100i", 400),
        ("loduff", "<i", 4),
        ("hiduff", "<i", 4),
        ("numduff", "<i", 4),
        ("duffs", "<100i", 400),
        ("lowoody", "<i", 4),
        ("hiwoody", "<i", 4),
        ("numwoody", "<i", 4),
        ("woodies", "<100i", 400),
        ("numeast", "<i", 4),
        ("numnorth", "<i", 4),
        ("EastUtm", "<d", 8),
        ("WestUtm", "<d", 8),
        ("NorthUtm", "<d", 8),
        ("SouthUtm", "<d", 8),
        ("GridUnits", "<i", 4),
        ("XResol", "<d", 8),
        ("YResol", "<d", 8),
        ("EUnits", "<h", 2),
        ("SUnits", "<h", 2),
        ("AUnits", "<h", 2),
        ("FOptions", "<h", 2),
        ("CUnits", "<h", 2),
        ("HUnits", "<h", 2),
        ("BUnits", "<h", 2),
        ("PUnits", "<h", 2),
        ("DUnits", "<h", 2),
        ("WOptions", "<h", 2),
        ("ElevFile", "<256s", 256),
        ("SlopeFile", "<256s", 256),
        ("AspectFile", "<256s", 256),
        ("FuelFile", "<256s", 256),
        ("CoverFile", "<256s", 256),
        ("HeightFile", "<256s", 256),
        ("BaseFile", "<256s", 256),
        ("DensityFile", "<256s", 256),
        ("DuffFile", "<256s", 256),
        ("WoodyFile", "<256s", 256),
        ("Description", "<512s", 512)
    ]
    
    header = {}
    total_file_size = os.path.getsize(filename)
    with open(filename, "rb") as f:
        # Read header fields sequentially.
        for key, fmt, size in header_fields:
            data = f.read(size)
            if len(data) != size:
                raise ValueError(f"Could not read {size} bytes for {key}")
            value = struct.unpack(fmt, data)
            if fmt.endswith("s"):
                value = value[0].decode("utf-8", errors="ignore").rstrip("\x00")
            else:
                value = value[0] if len(value) == 1 else list(value)
            
            header[key] = value
        
        result["header"] = header
        # Determine the header size from the current file pointer.
        headsize = f.tell()
        
        # Compute NumVals based on CrownFuels and GroundFuels values.
        crown = (header["CrownFuels"] - 20) > 0
        ground = (header["GroundFuels"] - 20) > 0


        if crown:
            NumVals = 10 if ground else 8
        else:
            NumVals = 7 if ground else 5
       
        NumVals = NumVals 
        result["NumVals"] = NumVals
        
        # Read the landscape array.
        numnorth = header["numnorth"]
        numeast = header["numeast"]
        total_length = (numnorth * numeast * NumVals) 
        
        l_fmt = "<" + str(total_length) + "h"
        l_bytes = total_length * 2
        
         
        row_values = struct.unpack(l_fmt, f.read(l_bytes))
        current_pos = f.tell()
        
        remaining_bytes = total_file_size - current_pos

        #print("Remaining bytes to read:", remaining_bytes, " one line was ", numnorth, numeast, NumVals, " and ",l_bytes," remaining bytes", remaining_bytes )
        # Convert the flat list of rows to a NumPy array and reshape it.
        # Read the flat landscape data into a 1D array.
        

        # Convert to a NumPy array, reshaping as (NumVals, numnorth, numeast)
        temp = np.array(row_values, dtype=np.int16).reshape(( numnorth, numeast, NumVals ))

        # Then transpose to shape (numnorth, numeast, NumVals)
        landscape_np = np.transpose(temp, (0, 1, 2))


        landscape_dict = {}
        landscape_dict["elevation"] = np.flipud(landscape_np[:, :, 0])
        landscape_dict["slope"]     = np.flipud(landscape_np[:, :, 1])
        landscape_dict["aspect"]    = np.flipud(landscape_np[:, :, 2])
        landscape_dict["fuel"]      = np.flipud(landscape_np[:, :, 3])
        landscape_dict["cover"]     = np.flipud(landscape_np[:, :, 4])

        if crown:
            if ground:
                landscape_dict["CrownHeight"] = np.flipud(landscape_np[:, :, 5])
                landscape_dict["CrownBase"]   = np.flipud(landscape_np[:, :, 6])
                landscape_dict["CrownDensity"] = np.flipud(landscape_np[:, :, 7])
                landscape_dict["duff"]        = np.flipud(landscape_np[:, :, 8])
                landscape_dict["woody"]       = np.flipud(landscape_np[:, :, 9])
            else:
                landscape_dict["CrownHeight"]  = np.flipud(landscape_np[:, :, 5])
                landscape_dict["CrownBase"]    = np.flipud(landscape_np[:, :, 6])
                landscape_dict["CrownDensity"] = np.flipud(landscape_np[:, :, 7])
        else:
            if ground:
                landscape_dict["duff"]  = np.flipud(landscape_np[:, :, 5])
                landscape_dict["woody"] = np.flipud(landscape_np[:, :, 6])
        
        
        result["landscape"] = landscape_dict

        
        result["CantAllocLCP"] = False

    return result


def plot_landscape_with_header(lcp_data):
    """
    Plots all raster bands from the landscape dictionary (expected 8 keys)
    in a 4x2 grid on the left, and displays header information on the right.
    Cells with a value of -9999 are masked and not plotted.
    """
    import matplotlib.pyplot as plt
    import numpy as np
    import matplotlib.gridspec as gridspec

    header = lcp_data["header"]
    # Build the header text to display.
    keys_to_show = ["CrownFuels", "GroundFuels", "latitude", "loeast", "hieast", 
                    "lonorth", "hinorth", "numelev", "numeast", "numnorth", "XResol", "YResol"]
    header_text = "\n".join([f"{k}: {header[k]}" for k in keys_to_show if k in header])

    # Use the landscape dictionary keys in a specific order.
    # Change the order if needed.
    landscape_dict = lcp_data["landscape"]
    # Ensure the order is as desired.
 
    # Use only those keys that exist.
    keys = landscape_dict.keys()
    num_keys = len(keys)
    
    # Set up a gridspec: 4 rows x 3 columns, with the last column reserved for header text.
    fig = plt.figure(figsize=(18, 14))
    gs = gridspec.GridSpec(4, 3, width_ratios=[1, 1, 0.5])
    
    # Create axes for each raster plot in the first 2 columns.
    raster_axes = []
    for i in range(4):
        for j in range(2):
            ax = fig.add_subplot(gs[i, j])
            raster_axes.append(ax)
    
    # Plot each raster (if fewer than 8 keys, remaining axes will be turned off).
    for idx, key in enumerate(keys):
        ax = raster_axes[idx]
        data = np.array(landscape_dict[key])
        masked_data = np.ma.masked_equal(data, -9999)
        im = ax.imshow(masked_data, cmap="viridis", interpolation="nearest")
        # Calculate min and max, ignoring masked elements.
        vmin = masked_data.min()
        vmax = masked_data.max()
        ax.set_title(f"{key}\n(min: {vmin}, max: {vmax})")
        ax.axis("off")
        fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    
    # Turn off any unused raster subplots.
    for ax in raster_axes[len(keys):]:
        ax.axis("off")
    
    # Create an axes in the third column spanning all rows for header information.
    ax_header = fig.add_subplot(gs[:, 2])
    ax_header.axis("off")
    ax_header.text(0.05, 0.95, header_text, fontsize=12, verticalalignment="top",
                   transform=ax_header.transAxes)
    ax_header.set_title("Header Information")
    
    plt.tight_layout()
    plt.show()

## Simulation Parameters ##
nb_steps = 3               # The number of step the simulation will execute
step_size =   1000              # The duration (in seconds) between each step

# Initialize pyforefire module
ff = pyff.ForeFire()
mylcpfile = "./flatland.lcp"
mylWeatherFile = './flatland_3mph0deg7hr.raws'
a = readLCPFile(mylcpfile)

#plot_landscape_with_header(a)

ff["propagationModel"] = "Farsite"
ff["fuelsTable"] = "STDfarsiteFuelsTable"
ff["FarsiteLoggerCSVPath"] = "FF_FS4roslog_2.csv"

ff["moistures.ones"] = 0.032
ff["moistures.liveh"] =  0.7
ff["moistures.tens"] =  0.04
ff["moistures.livew"] =  1
ff["moistures.hundreds"] =  0.06

ff["spatialIncrement"] = 1
ff["minimalPropagativeFrontDepth"] = 30
ff["perimeterResolution"] = float(ff["spatialIncrement"]) * 20

ff["minSpeed"] = 0
ff["relax"] = 0.5
ff["bmapLayer"] = 1

S = float(a["header"]["SouthUtm"])
N = float(a["header"]["NorthUtm"])
W = float(a["header"]["WestUtm"])
E = float(a["header"]["EastUtm"])

ff["SWx"] = W
ff["SWy"] = S
ff["Lx"] = E-W
ff["Ly"] = N-S

dwidth = E-W
dheight =  N-S

wind_map = np.zeros((2,2,20,20))
windU=wind_map[0:1,:,:,:]  
windU[0,0,:,:].fill(1.0)
windU[0,1,:,:].fill(0.0)
windV=wind_map[1:2,:,:,:] 
windV[0,0,:,:].fill(0.0)
windV[0,1,:,:].fill(1.0)
#print('wind_map',wind_map, wind_map.shape)

fuel = a["landscape"]["fuel"]
fuel_map = np.zeros((1, 1) + np.shape(fuel))
fuel_map[0, 0, ...] = fuel
#print('fuelmap',fuel_map, fuel_map.shape)

altitude = a["landscape"]["elevation"]
altitude_map = np.zeros((1, 1) + np.shape(altitude))
altitude_map[0, 0, ...] = altitude
#print('altitudemap',altitude_map, altitude_map.shape)

# Set computation
domain_string = f'FireDomain[sw=({W},{S},0);ne=({E},{N},0);t=0.]'
ff.execute(domain_string)
ff.addLayer("propagation",ff["propagationModel"],"propagationModel")
ff.addIndexLayer("table", "fuel", W,S, 0, dwidth, dheight,0, fuel_map)
ff.addScalarLayer("windScalDir", "windU", W,S, 0, dwidth, dheight,0, windU)
ff.addScalarLayer("windScalDir", "windV", W,S, 0, dwidth, dheight,0,  windV)
ff.addScalarLayer("data", "altitude", W,S, 0, dwidth, dheight,0,  altitude_map)

startx, starty = W + dwidth/2, S + dheight/2
dx = 2*float(ff["perimeterResolution"])
fireString =f"startFire[loc=({startx},{starty},100.);t=0.]"
ff.execute("    FireFront[id=2;domain=0;t=0]")
ff.execute(f"        FireNode[domain=0;id=4;fdepth=20;kappa=0;loc=({startx},{starty+dx},100);vel=(0,0.1,0);t=0;state=init;frontId=2]")
ff.execute(f"        FireNode[domain=0;id=6;fdepth=20;kappa=0;loc=({startx+dx},{starty},100);vel=(0.1,0,0);t=0;state=init;frontId=2]")
ff.execute(f"        FireNode[domain=0;id=8;fdepth=20;kappa=0;loc=({startx},{starty-dx},100);vel=(0,-0.1,0);t=0;state=init;frontId=2]")
ff.execute(f"        FireNode[domain=0;id=10;fdepth=20;kappa=0;loc=({startx-dx},{starty},100);vel=(-0.1,0,0);t=0;state=init;frontId=2]")

pathes = []

data_records = load_weather_file(mylWeatherFile)
print('Data_weather!',data_records)
for i, record in enumerate(data_records):
        current_dt = record['datetime']
        wind_speed = record['wind_spd'] * 0.44704 * 0.5#mph
        wind_dir = record['wind_dir']+180.0
        
        # Convert wind direction (degrees) to radians.
        angle_rad = math.radians(wind_dir)
        # Compute wind vector components.
        wind_x = wind_speed * math.sin(angle_rad)
        wind_y = wind_speed * math.cos(angle_rad)
        
        # Here, we use the current date-time in ISO format as a simulation time identifier.
        sim_time = current_dt.isoformat()
        print(sim_time)
        
        # Trigger the wind in the simulation.
        ff.execute(f"trigger[wind;loc=(0.,0.,0.);vel=({wind_x},{wind_y},0);t={sim_time}]")
        
        # Determine time step (in seconds) between current record and next record.
        if i < len(data_records) - 1:
            next_dt = data_records[i+1]['datetime']
            dt_seconds = (next_dt - current_dt).total_seconds()
        else:
            dt_seconds = 3600
        
        # Advance the simulation.
        ff.execute("step[dt=%f]" % dt_seconds)
        
        # Retrieve and accumulate the simulation path information.
        #if(i%2==0):
        pathes += pyff.helpers.printToPathe(ff.execute("print[]"))
        print(f"Simulated for {current_dt} with wind speed {wind_speed} and direction {wind_dir}°, step {dt_seconds} sec")
    
ff.execute("save[filename=data.nc;fields=fuel,altitude,wind]")

ffplotExtents =(W,E,S,N)
pyff.helpers.plot_simulation(pathes,ff.getDoubleArray("fuel")[0,0,:,:], ff.getDoubleArray("altitude")[0,0,:,:],  ffplotExtents)
