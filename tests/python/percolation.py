import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
from matplotlib.colors import ListedColormap
import pyforefire as forefire
 
import pyforefire.helpers as ffhelpers

## Simulation Parameters ##
nb_steps = 3                # The number of step the simulation will execute
step_size =   2400              # The duration (in seconds) between each step

fuel_type = 82

data_resolution = 50
sub_sim_shape = (80,60)
k_coeffs = [.10,.3,.4,.5]   

# Initialize pyforefire module
ff = forefire.ForeFire()
#fuel_file_path = './fuels.ff' 
ff["fuelsTable"] = ffhelpers.extendedRothermelFuelTable()

ff["spatialIncrement"] = 0.5
ff["minimalPropagativeFrontDepth"] = 20
ff["perimeterResolution"] = 10
ff["initialFrontDepth"] = 50
ff["relax"] = 0.2
ff["smoothing"] = 1000000

ff["z0"] = 0.0

ff["defaultHeatType"] = 0
ff["nominalHeatFlux"] = 100000
ff["burningDuration"] = 100
ff["maxFrontDepth"] = 50
ff["minSpeed"] = 0.005
ff["propagationModel"] = "Rothermel"
ff["bmapLayer"]=1

sim_shape = (sub_sim_shape[0], sub_sim_shape[1]*len(k_coeffs))
domain_height = sim_shape[0] * data_resolution
domain_width = sim_shape[1] * data_resolution

# setting the velues for the fluxed models diagnostics
ff["atmoNX"] = sim_shape[1]
ff["atmoNY"] = sim_shape[0]

fuel_map = np.zeros((1, 1) + sim_shape)

for i, k in enumerate(k_coeffs):
    fuel_map[0, 0, :, sub_sim_shape[1] *i:sub_sim_shape[1] * (i + 1)] = ffhelpers.fill_random((sub_sim_shape[0], sub_sim_shape[1]), k, fuel_type)
print("creating domain of ",domain_width,domain_height, " size of data ",sim_shape[1],sim_shape[0]) 
wind_map = np.zeros((2,2,sim_shape[1],sim_shape[0]))

windU=wind_map[0:1,:,:,:]  
windU[0,0,:,:].fill(1.0)
windU[0,1,:,:].fill(0.0)
windV=wind_map[1:2,:,:,:]  
windV[0,0,:,:].fill(0.0)
windV[0,1,:,:].fill(1.0)


ff["SWx"] = 0.
ff["SWy"] = 0.
ff["Lx"] = domain_width
ff["Ly"] = domain_height
#ff["RothermelLoggerCSVPath"] = "rothermel.csv"

domain_string = f'FireDomain[sw=({ff["SWx"]},{ff["SWy"]},0);ne=({ff["SWx"] + ff["Lx"]},{ff["SWy"] + ff["Ly"]},0);t=0]'
ff.execute(domain_string)
ff.addLayer("BRatio","BRatio","BRatio")
ff.addLayer("flux","heatFluxBasic","defaultHeatType")
ff.addLayer("propagation",ff["propagationModel"],"propagationModel")
ff.addIndexLayer("table", "fuel", float(ff["SWx"]), float(ff["SWy"]), 0, float(ff["Lx"]), float(ff["Ly"]), 0, fuel_map)
ff.addScalarLayer("windScalDir", "windU", float(ff["SWx"]), float(ff["SWy"]), 0, float(ff["Lx"]), float(ff["Ly"]), 0, windU)
ff.addScalarLayer("windScalDir", "windV", float(ff["SWx"]), float(ff["SWy"]), 0, float(ff["Lx"]), float(ff["Ly"]), 0, windV)


GlobalWindScalerU = 0                     # The Horizontal wind
GlobalWindScalerV = 3                     # The Vertical wind
ff.execute(f"trigger[wind;loc=(0.,0.,0.);vel=({GlobalWindScalerU},{GlobalWindScalerV},0);t={0}]")

# Start ignitions with previoulsy computed ignitions points
for i in range(len(k_coeffs)):
    ignition_string = "startFire[loc=(%i,%i,0.);vel=(10.,10.,0.);t=0.]" % ( data_resolution * (0.5+i) * sub_sim_shape[1] , 5*data_resolution)
    print(ignition_string)
    ff.execute(ignition_string)
outS = ff.execute("print[]")

 
pathes = []
for i in range(1, nb_steps+1):
    try:
        ff.execute("goTo[t=%f]" % (i*step_size))
        outS = ff.execute("print[]")
        #print(outS)
        pathes  += ffhelpers.printToPathe(outS)

    except KeyboardInterrupt:
        break
ff.execute("save[filename=percolation.nc;fields=altitude,wind,fuel]")
ffplotExtents =(float(ff["SWx"]),float(ff["SWx"]) + float(ff["Lx"]),float(ff["SWy"]),float(ff["SWy"]) + float(ff["Ly"]))
#bmap = ff.getDoubleArray("BMap")[0,0,:,:]
ff.execute("save[]")

#ffhelpers.plot_simulation(pathes,ff.getDoubleArray("fuel")[0,0,:,:] ,None,  ffplotExtents ,scalMap=bmap)

