import numpy as np 
import pyforefire as forefire
#from ffToGeo import FFGEO
#from forefire_helper import *

###################################################
if __name__ == '__main__':
###################################################

    ff = forefire.ForeFire()
    outputsUpdate    = 10
    spatialIncrement = 2 
    perimeterResolution = 10 
    minimalPropagativeFrontDepth = 10
    bmapOutputUpdate = 10
    InitTime = 0

    #set param
    ff.setString('ForeFireDataDirectory','ForeFire')
    ff.setString('fireOutputDirectory','ForeFire/Outputs')
    ff.setInt('outputsUpdate',outputsUpdate)
    
    ff.setString('fuelsTableFile','fuels.ff') # end dur
    ff["propagationModel"]="Andrews"

    ff.setDouble("spatialIncrement",spatialIncrement)
    ff.setDouble("perimeterResolution",perimeterResolution)
    ff.setDouble("minimalPropagativeFrontDepth",minimalPropagativeFrontDepth)

    ff.setDouble("bmapOutputUpdate",bmapOutputUpdate)
    ff.setInt("defaultHeatType",0)
    ff.setInt("defaultVaporType",1)

    ff.setDouble("InitTime",InitTime)
    ff.execute("loadData[mydata.nc;2022-07-16T01:30:00Z]")
    ff.execute("startFire[loc=(59601.3,77044.1,0.);t=39600]")
    print(ff.execute("print[]"))

    ff.setString('dumpMode','FF')
    
    N_step = 10
    step = 20
    t0 = 39000
    pathes = []
    #ffgeo = FFGEO('pdv',4326)
    #ffgeo.addFront(ff)

    for i in np.arange(1,N_step):
        
        ff_time = t0 + i*step

        print("goTo[t=%f]"%(ff_time), end='\n')
        ff.execute("goTo[t=%f]"%(ff_time))
        
        #pathes  += printToPathe(ff.execute("print[]"))
        #print(pathes)
        #ffgeo.addFront(ff)
   
    
    #fig = plt.figure()
    #ax = plt.subplot(111)
    #ax.pcolormesh(xx,yy,mydata.fuel[0,0])
    

