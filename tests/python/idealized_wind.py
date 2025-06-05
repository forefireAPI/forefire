# -*- coding: utf-8 -*-
"""
Created on Thu Jul 18 10:23:07 2024

@author: baggio_r2
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import pyforefire as forefire
import math as math
import pyforefire as pyff
###############################################################################
##:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#                    IDEALIZED TEST CASE WIND
##::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 
## Front propagates driven by a turning wind
##-----------------------------------------------------------------------------
## Propagation model "WindDriven"-> ROS=vv_coeff*NormalWind;
## - change the coefficient vv_coeff by editing "VVCoeffTable()"
##
## To simulate inhomogeneous propagation:
##     1) add rows to the fuel table VVCoeffTable():
##    ________________  
##    |Index|vv_coeff|
##    |-----|--------|  
##    |1    |1.0     |
##    |-----|--------|
##    |2    |0.5     |
##    |-----|--------|
##      ...    ...
##
##     2) edit the fuelmap using a pattern of "Index" values
##     Ex:
##         fuel_map[:,:,:,:int(.5*sim_shape[1])].fill(valueofIndex1)    
##         fuel_map[:,:,:,int(.5*sim_shape[1]):].fill(valueofIndex2)
##-----------------------------------------------------------------------------
##  Routine for plotting
##-----------------------------------------------------------------------------
def plot_test(pathes, myExtents):
    """
    Used for plot 4 axis graph, with Heatflux, Fuels, Altitude plotted under simulation, 
    and Statistics for the last axis.
    """
    # Create a figure with 2 axis (2 subplots)
    fig, ax = plt.subplots(figsize=(10,7), dpi=120)

    # Plot current firefronts to the first 3 subplots
    for p, path in enumerate(pathes):
        patch = mpatches.PathPatch(path, edgecolor="Red", facecolor='none', alpha=1, lw=2)
        ax.add_patch(patch)
        # ax.grid()
        ax.axis('equal')

    plt.show()
##-----------------------------------------------------------------------------
##  Fuel type table
##-----------------------------------------------------------------------------
def VVCoeffTable():
    return """Index;vv_coeff;Kcurv;beta
1;1.0;1.0;1.0
2;1.0;1.0;1.0"""

##-----------------------------------------------------------------------------
##  Simulation parameters and settings ##
##-----------------------------------------------------------------------------
nb_steps_tot=15          # The number of steps the simulation will execute
step_size = 5            # The duration (in seconds) of each time step
fuel_type = 1            # The type of used fuel by default (Index in VVCoeffTable() )
norm=80                  # Module of the turning speed
angle_deg=24             # Angle spanned for every time step
Ray=(norm*(nb_steps_tot*step_size)/(2*np.pi)) #Ray of the circle




# Initialize pyforefire module
ff = pyff.ForeFire()


#Fuel settings
ff["fuelsTable"] = VVCoeffTable()
ff["defaultFuelType"]=1.

#ForeFire settings
ff["spatialIncrement"]=0.5
ff["minimalPropagativeFrontDepth"]=10
ff["perimeterResolution"]=5
ff["initialFrontDepth"]=0.1
ff["relax"]=1.0
ff["smoothing"]=0
ff["minSpeed"]=0.2*norm
ff["bmapLayer"]=1
##  Propagation model settings
ff["windReductionFactor"]=1. 
ff["propagationModel"] = "WindDriven"

## total size of sim
sim_shape = (2500, 2500)
## domain_width & domain_height
domain_height = sim_shape[1]
domain_width = sim_shape[0]

##  Init fuel
fuel_map = np.zeros((1, 1) + sim_shape)
fuel_map.fill(fuel_type)
##   Wind arrays
wind_map = np.zeros((2,2,domain_width,domain_height))
windU=wind_map[0:1,:,:,:] #Should have shape (1, 2, domain_width, domain_height)
windU[0,0,:,:].fill(1.0)
windU[0,1,:,:].fill(0.0)
windV=wind_map[1:2,:,:,:] #Should have shape (1, 2, domain_width, domain_height)
windV[0,0,:,:].fill(0.0)
windV[0,1,:,:].fill(1.0)
ff["SWx"] = 0.0
ff["SWy"] = 0.0
ff["Lx"] = domain_width
ff["Ly"] = domain_height
time=0.
domain_string = f'FireDomain[sw=({ff["SWx"]},{ff["SWy"]},0);ne=({ff["SWx"] + ff["Lx"]},{ff["SWy"] + ff["Ly"]},0);t={time}]'
ff.execute(domain_string)


ff.addLayer("propagation",ff["propagationModel"],"propagationModel")
ff.addIndexLayer("table", "fuel", float(ff["SWx"]), float(ff["SWy"]), 0, float(ff["Lx"]), float(ff["Ly"]), 0, fuel_map)
ff.addScalarLayer("windScalDir", "windU", float(ff["SWx"]), float(ff["SWy"]), 0, float(ff["Lx"]), float(ff["Ly"]), 0, windU)
ff.addScalarLayer("windScalDir", "windV", float(ff["SWx"]), float(ff["SWy"]), 0, float(ff["Lx"]), float(ff["Ly"]), 0, windV)

## "Ignition" point
pointxcenter=domain_width//2
pointycenter=domain_height//2
startx,starty=pointxcenter,domain_height//5
ff.execute(f"startFire[loc=({startx},{starty},0.);t={time}]")  

pathes = []


for i in range(nb_steps_tot+1):
    # print("ciao2")
    rotation_angle_rad = math.radians(i * angle_deg)
    herex = norm * math.cos(rotation_angle_rad)
    herey = norm * math.sin(rotation_angle_rad)
    #angle_deg = angle_deg-#(0.5)
    angleprint=rotation_angle_rad*180/np.pi
    try:
        new_out = ff.execute("print[]")
        newPathes = pyff.helpers.printToPathe(new_out)
        ff.execute(f"trigger[wind;loc=(0.,0.,0.);vel=({herex},{herey},0);t={time}]")
        ff.execute("step[dt=%f]" % (step_size))
        print("Iteration ",i,"/",nb_steps_tot, " angle: ", angleprint,"   time: ",time)
        pathes += newPathes
        time+=step_size

        
    except KeyboardInterrupt:
        break
# Finally, plot simulations and show stats
ffplotExtents =(float(ff["SWx"]),float(ff["SWx"]) + float(ff["Lx"]),float(ff["SWy"]),float(ff["SWy"]) + float(ff["Ly"]))
ff.execute("plot[parameter=speed;filename=360wind.png;range=(0,2);cmap=viridis]")
#plot_test(pathes,ffplotExtents)


