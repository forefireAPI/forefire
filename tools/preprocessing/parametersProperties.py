heatFlux = {'type':'flux',                   \
            'indices': [0],              \
            'model0name': 'na',   \
            }
vaporFlux= {'type':'flux',                   \
            'indices': [1],              \
            'model1name': 'na',\
            }
scalarFlux = {'type':'flux',                   \
            'indices': [2],              \
            'model2name': 'na',   \
            }

fuel     = {'type' : "fuel"} 
windU    = {'type' : "data"} 
windV    = {'type' : "data"} 
altitude = {'type' : "data"} 
#fire regime control time
FromObs_evaporationTime = {'type' : "data"} 
FromObs_residenceTime = {'type' : "data"} 
FromObs_burningTime = {'type' : "data"} 
#sensible heat & vapor flux
FromObs_VaporFlux_evaporation = {'type' : "data"}
FromObs_NominalHeatFlux_flaming = {'type' : "data"}
FromObs_NominalHeatFlux_smoldering = {'type' : "data"}
#Emission Factor
FromObs_EFScalar_flamning = {'type' : "data"} 
FromObs_EFScalar_smoldering = {'type' : "data"} 
#radiation fraction
FromObs_radiationFraction_flaming = {'type' : "data"} 
FromObs_radiationFraction_smoldering = {'type' : "data"} 
#conversion factor
FromObs_conversionFactor = {'type' : "data"} 

domain   = {'type': 'domain',
            'NEx'  : None ,
            'NEy'  : None, 
            'NEz'  : None, 
            'SWx' :  None,
            'SWy' :  None, 
            'SWz' :  None,
            'Lx' :  None,
            'Ly' :  None, 
            'Lz' :  None,
            't0'  : None, 
            'Lt'  : None 
            }

parameters= {'type' : "parameters" ,
            #'date' : None  ,               #"2000-01-01_00:00:00",
            'refYear' : None,              #2000 ,
            #'refMonth' : None,             #1,
            'refDay' : None,               #1,
            'duration' : None,             #360000 ,
    	    'projection' : None,           #"OPENMAP" ,
            'projectionproperties' : None, #"41.551998,8.828396,41.551998,8.828396" ,
            }
