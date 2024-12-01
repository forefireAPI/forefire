from __future__ import print_function
from __future__ import division
from builtins import zip
from builtins import range
from past.utils import old_div
import sys
import os
import numpy as np
import matplotlib as mpl
mpl.use('Qt5Agg')
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import pdb 
#import imp 
from scipy import io
import glob
import netCDF4
import f90nml
import datetime
import socket 
import warnings
warnings.filterwarnings("ignore",".*GUI is implemented.*")
from mpl_toolkits.axes_grid1 import make_axes_locatable
import pickle 
import argparse
from osgeo import gdal,osr,ogr
from scipy import interpolate 
import importlib 
import pandas


path_src = os.environ['PATH_SRC_PYTHON_LOCAL']

try:
    path_ForeFire =  os.environ["PATH_FOREFIRE"]
except:
    print('PATH_FOREFIRE is not defined. stop here')
    sys.exit()

sys.path.append(path_src+'/Regrid/')
#sys.path.append(path_ForeFire)
#sys.path.append(path_ForeFire+'/swig/')
#sys.path.append(path_ForeFire+'tools/')
sys.path.append(path_ForeFire+'tools/preprocessing/')
try: 
    import pyforefire as forefire
    flag_run_ff = True
except ImportError:
    print('could not find ForeFire API')
    print('check variable path_ForeFire in path_ForeFire/tools/create_data_bmap_nc.py')
    print('currently, path_ForeFire = ', path_ForeFire)
    print('flag_run_ff set to False')
    flag_run_ff = False
import regrid
importlib.reload(regrid)
import parametersProperties as attribute


##################################################
def ensure_dir(f):
    d = os.path.dirname(f)
    if not os.path.exists(d):
        os.makedirs(d)


##################################################
def read_MNH_domain( expr_name, dir_mnh = '../02_mnh/', domainNumber=None, specificInputMNHFile=None):

    if specificInputMNHFile == None:
        expr_numb = '.{:d}.'.format(domainNumber) if (domainNumber is not None) else ''
        outfiles_mnh = sorted(list(set(glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.nc'))-\
                                   set(glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.OUT.*.nc')))+\
                              glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.nc4'))
        print(outfiles_mnh)
        if len(outfiles_mnh) == 0:
            print('missing mnh files')
            pdb.set_trace()
        outfiles_mnh_inputTime = outfiles_mnh[0]
        outfiles_mnh_inputGrid = outfiles_mnh[1]
    
    else:
        outfiles_mnh_inputGrid = specificInputMNHFile 
        outfiles_mnh_inputTime = specificInputMNHFile 

    print('get time ref from :', outfiles_mnh_inputTime)
    print('get grid from     :', outfiles_mnh_inputGrid)


    #read geometry of the MNH domain
    nc = netCDF4.Dataset(outfiles_mnh_inputGrid,'r') # get the 001 file
    
    if domainNumber is not None:
        expr_numb = '.{:d}.'.format(1)
        outfiles_mnh_dad = sorted(list( set(glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.nc'))-\
                                        set(glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.OUT.*.nc')))+\
                                  glob.glob(dir_mnh+expr_name[:5]+expr_numb+'*.nc4'))
        ncDad = netCDF4.Dataset(outfiles_mnh_dad[1],'r') 

    MNH_properties= {}
    
    try:
        if domainNumber is not None:
            MNH_properties['lat00'] = np.array(ncDad.variables['LAT'])[0,0]
            MNH_properties['lon00'] = np.array(ncDad.variables['LON'])[0,0]
            ncDad.close()
        else:
            MNH_properties['lat00'] = np.array(nc.variables['LAT'])[0,0]
            MNH_properties['lon00'] = np.array(nc.variables['LON'])[0,0]
    except: 
        print('no ref point found, assume IDEAL RUN')
        MNH_properties['lat00'] = 'ideal'

    DeltaY = nc.variables['YHAT'][1]-nc.variables['YHAT'][0]
    DeltaX = nc.variables['XHAT'][1]-nc.variables['XHAT'][0]
   
    if nc.variables['MASDEV'][0].data==53: 
        MNH_properties['nx']  = len(nc.dimensions[u'X'])
        MNH_properties['ny']  = len(nc.dimensions[u'Y'])
        MNH_properties['nz']  = len(nc.dimensions[u'Z'])
    else:
        MNH_properties['nx']  = len(nc.dimensions[u'ni'])
        MNH_properties['ny']  = len(nc.dimensions[u'nj'])
        MNH_properties['nz']  = len(nc.dimensions[u'level_w'])

    MNH_properties['SWx']  = nc.variables['XHAT'][0]+0.
    MNH_properties['SWy']  = nc.variables['YHAT'][0]+0.
    MNH_properties['SWz']  = 0
    MNH_properties['NEx']  = nc.variables['XHAT'][-1]+DeltaX
    MNH_properties['NEy']  = nc.variables['YHAT'][-1]+DeltaY
    MNH_properties['NEz']  = 0

    MNH_properties['Lx']   = nc.variables['XHAT'][-1]+DeltaX-nc.variables['XHAT'][0]
    MNH_properties['Ly']   = nc.variables['YHAT'][-1]+DeltaY-nc.variables['YHAT'][0]
    MNH_properties['Lz']   = 0
    nc.close()

    nc = netCDF4.Dataset(outfiles_mnh_inputTime,'r')
    if nc.variables['MASDEV'][0].data==53:
        MNH_properties['date']   = datetime.datetime(*nc.variables['DTCUR__TDATE'][:]).strftime("%Y-%m-%d_%H:%M:%S")
    else:
        MNH_properties['date']   = '_'.join(nc.variables['DTCUR'].units.split(' ')[2:4]) 
         
    
    if nc.variables['MASDEV'][0].data==53:
        MNH_properties['t0']   = nc.variables['DTCUR__TIME'][0]
    else:
        MNH_properties['t0']   = nc.variables['DTCUR'][0]
    MNH_properties['Lt']   = np.Inf
    
    nc.close()
   

    return MNH_properties


###################################################
def get_dimension_name_and_value(key,nc_variable, nco):
    
    dim_var  = np.array(                                 list(nc_variable[key].keys())  )
    dim_var_ = np.array([dimension[:-1] for dimension in nc_variable[key] ])
    idx_dim_var = np.where(dim_var_ == 'dim')

    dimensions_dimX = tuple(sorted(dim_var[idx_dim_var])[::-1])
    dimensions_name = []
    for dim in dimensions_dimX:
        dimensions_name.append(nc_variable[key][dim])

    nco.createVariable(key,nc_variable[key]['type'], dimensions_name )

    dimensions_value = [ nco.dimensions[dim] for dim in dimensions_name ]
   
    return  dimensions_name, dimensions_value


###################################################
def merge_two_dicts(x, y):
    '''Given two dicts, merge them into a new dict as a shallow copy.'''
    z = x.copy()
    z.update(y)
    return z


#################################
def getLocationFromLine(line):
    llv = line.split("loc=(")
    if len(llv) < 2: 
        return None
    llr = llv[1].split(",");
    if len(llr) < 3: 
        return None
    return (float(llr[0]),float(llr[1]))


#################################
def dist(a,b):
    return np.sqrt(np.power(a[0]-b[0],2)+np.power(a[1]-b[1],2))


#################################
def printToPathe(linePrinted):
    fronts = linePrinted.split("FireFront")
    pathes = []
    for front in fronts[1:]:
        nodes =front.split("FireNode")[1:]
        if len(nodes) > 0: 
            Path = mpath.Path
           
            codes = []
            verts = []
            lastNode = getLocationFromLine(nodes[0])
            firstNode = getLocationFromLine(nodes[0])
            
 
            codes.append(Path.MOVETO)
            verts.append(firstNode)      

            for node in nodes[:]:
                newNode = getLocationFromLine(node)
                codes.append(Path.LINETO)
                verts.append(newNode)         
                lastNode = newNode
                
    
            codes.append(Path.LINETO)
            verts.append(firstNode)          
           
            pathes.append(mpath.Path(verts, codes))

    return pathes;


###################################################
def getDomainExtent(line):
    print(line)
    llv = line.split("sw=(")
    llr = llv[1].split("ne=(");
    return( float( llr[0].split(",")[0]), float(llr[1].split(",")[0]), float(llr[0].split(",")[1]), float(llr[1].split(",")[1]) )


###################################################
if __name__ == '__main__':
###################################################

    print('check env variable:')
    print('ntask =', os.environ['ntask'])
    

    parser = argparse.ArgumentParser(description='create data for MNH et forefire')
    parser.add_argument('-i','--input', help='Input MNH dir',required=False)
    parser.add_argument('-d','--domainNumber', help='domain number',required=False)
    parser.add_argument('-sf','--specificFile', help='to use in case of nested domain',required=False)
    args = parser.parse_args()

    ################### ... start input param

    #ff param
    input_ff_param_file = './Inputs/ff-param.nam'

    ################### ... end input param

    #create dir
    ensure_dir('./Inputs/')
    ensure_dir('./ForeFire/')
    ensure_dir('./ForeFire/Outputs/')
    ensure_dir('./MODEL1/')

    dir_input = args.input
    if dir_input is None: dir_input = '../02_mnh/'
    domainNumber = int(args.domainNumber) if (args.domainNumber is not None) else None

    #read MNH info
    MNH_namelist = f90nml.read(dir_input+'/EXSEG1.nam')
    expr_name = MNH_namelist['NAM_CONF']['CEXP']
    MNH_domain   = read_MNH_domain(expr_name, dir_mnh = dir_input, domainNumber = domainNumber, specificInputMNHFile=args.specificFile)
    '''
    try: 
    
    except IOError:
        print '************** set test run ****************'
        expr_name = 'test'
        MNH_domain= {}
        MNH_domain['nx']  = 10 
        MNH_domain['ny']  = 10
        MNH_domain['nz']  = 10

        MNH_domain['SWx']  = 0 
        MNH_domain['SWy']  = 0
        MNH_domain['SWz']  = 0
        MNH_domain['NEx']  = 100
        MNH_domain['NEy']  = 100
        MNH_domain['NEz']  = 100

        MNH_domain['Lx']   = None 
        MNH_domain['Ly']   = None
        MNH_domain['Lz']   = None

        MNH_domain['t0']   = 0. 
        MNH_domain['Lt']   = np.Inf

        MNH_domain['date']   = datetime.datetime(1970,1,1).strftime("%Y-%m-%d_%H:%M:%S")
    '''

    #read FF info
    nmls = f90nml.read('./Inputs/ff-param.nam')
    
    #ronan ff info
    ronan_param = nmls['FF_RONAN']
    print('############')
    if not(ronan_param['flag_freshStart']):
        print(' this is not a fresh start')
    print(' init mod is :', ronan_param['init_mode'])
    print('############')


    if ronan_param['init_mode'] == 'ideal':
        print('initialization in ideal mode')
        
        flag_rotation  = False
        shiftTheta = -999
        rotce = -999
        rotcn = -999
        
        #ideal fix FireLine
        x_location_fireLine = ronan_param['x_location_fireLine']  
        length_fireLine     = ronan_param['length_fireLine'] 
        depth_fireLine      = ronan_param['depth_fireLine'] 
        time_start_fireLine = ronan_param['time_start_fireLine']
        residence_time      = ronan_param['residence_time']
        burning_time      = ronan_param['burning_time']

        moisture            = ronan_param['moisture']
        nominalHeatFlux_f     = ronan_param['nominalHeatFlux_flaming']
        nominalHeatFlux_s     = ronan_param['nominalHeatFlux_smoldering']

        if type(time_start_fireLine) is float:
            nbre_fire_line = 1
            x_location_fireLine = [x_location_fireLine]
            length_fireLine     = [length_fireLine]
            depth_fireLine      = [depth_fireLine]
            time_start_fireLine = [time_start_fireLine]
            residence_time      = [residence_time]
            burning_time      = [burning_time]
            moisture            = [moisture]
            nominalHeatFlux_f    = [nominalHeatFlux_f]
            nominalHeatFlux_s    = [nominalHeatFlux_s]
        else:
            nbre_fire_line      = len(time_start_fireLine)
        flag_extra_contour = ronan_param['flag_extra_contour'] #extra parameter 
        shiftx = 0 
        shifty = 0
        shiftz = 0


    if ronan_param['init_mode'] == 'fromOverHeadImg':
       
        firedata = np.load(ronan_param['init_file'])
        firedata = firedata.view(np.recarray)
        if MNH_domain['lat00'] == 'ideal':
            shiftx = firedata.grid_e[0,0] - ronan_param['init_file_xshift']
            shifty = firedata.grid_n[0,0] - ronan_param['init_file_yshift']
        else:
            prj_info = open(ronan_param['init_file'].replace('.npy','.prj')).readline()
            wgs84 = osr.SpatialReference( ) # Define a SpatialReference object
            wgs84.ImportFromEPSG( 4326 ) # And set it to WGS84 using the EPSG code
            utm = osr.SpatialReference()
            utm.ImportFromWkt(prj_info)
            print('********* WARNING **********: need to change grid UTM zone in next obs data, hard reset to Zone 36N to correct error')
            utm.SetUTM(36)
            conv_ll2utm = osr.CoordinateTransformation(wgs84, utm)
            conv_utm2ll = osr.CoordinateTransformation(utm,wgs84)

            e_00, n_00, _ = conv_ll2utm.TransformPoint(MNH_domain['lon00'], MNH_domain['lat00'])
        
            shiftx =  e_00
            shifty =  n_00
        #shiftx = firedata.grid_e[0,0] - ronan_param['init_file_xshift'] - e_00
        #shifty = firedata.grid_n[0,0] - ronan_param['init_file_yshift'] - n_00
        shiftz = 0

        firedata.grid_e = firedata.grid_e - shiftx
        firedata.grid_n = firedata.grid_n - shifty 

        #apply rotation
        if 'init_file_thetashift' in ronan_param.keys():
            flag_rotation  = True
            shiftTheta = ronan_param['init_file_thetashift']
           
            idx = np.where(firedata.arrivalTime>0)
            rotce, rotcn = firedata.grid_e[idx].mean(), firedata.grid_n[idx].mean()

            firedata.grid_e -= rotce
            firedata.grid_n -= rotcn

            rot = np.zeros([2,2])
            theta = 3.14*shiftTheta/180.
            rot[0,0] =   np.cos(theta)
            rot[0,1] = - np.sin(theta)
            rot[1,0] =   np.sin(theta)
            rot[1,1] =   np.cos(theta)

            nn = firedata.grid_e.flatten().shape[0]
            ptsRot = np.zeros([nn,2])
            for ii_, [xx,yy] in enumerate( zip(firedata.grid_e.flatten(), firedata.grid_n.flatten())):
                ptsRot[ii_] = rot.dot(np.array([xx,yy]))
            firedata.grid_e = ptsRot[:,0].reshape(firedata.grid_e.shape)
            firedata.grid_n = ptsRot[:,1].reshape(firedata.grid_e.shape)

            firedata.grid_e += rotce
            firedata.grid_n += rotcn
            
        else:
            flag_rotation  = False
            shiftTheta = -999
            rotce = -999
            rotcn = -999


        dx = np.sqrt( (firedata.grid_e[1,0]-firedata.grid_e[0,0])**2+ (firedata.grid_n[1,0]-firedata.grid_n[0,0])**2 )
        dy = np.sqrt( (firedata.grid_e[0,1]-firedata.grid_e[0,0])**2+ (firedata.grid_n[0,1]-firedata.grid_n[0,0])**2 )
        firedata_reso = np.ones_like(firedata.grid_e)*dx*dy

        #flag no fire pixel as -999
        idx = np.where((firedata.fre_f+firedata.fre_s)<=0)
        firedata.fre_f[idx]           = 0
        firedata.fre_s[idx]           = 0
        firedata.residenceTime[idx] = -999
        firedata.burningTime[idx] = -999
        firedata.arrivalTime[idx]   = -999

        flag_extra_contour = False # not use here 

    if flag_run_ff != None: 
        flag_run_ff        = ronan_param['flag_run_ff'] 
    evaporation_time      = ronan_param['evaporation_time']

    #general ff info
    ff_param = nmls['FF_PARAM']
    minimalPropagativeFrontDepth = ff_param['minimalPropagativeFrontDepth']
    spatialIncrement             = ff_param['spatialIncrement']
    perimeterResolution          = ff_param['perimeterResolution']
    outputsUpdate                = ff_param['outputsUpdate']
    bmapOutputUpdate             = ff_param['bmapOutputUpdate']
    # end time of the bmap. if only forcing from the bmap use large value > simulation time
    InitTime                     = ff_param['InitTime']

    # dimension
    nx = MNH_domain['nx']
    ny = MNH_domain['ny']
   
    #load attribute
    for key in ['Lx','Ly','Lz','NEx','NEy','NEz','SWx','SWy','SWz','t0','Lt']:
        attribute.domain[key] = MNH_domain[key]
    #attribute.parameters['date'] =  MNH_domain['date']
    date_mnh = datetime.datetime.strptime(MNH_domain['date'], '%Y-%m-%d_%H:%M:%S')
    #time_mnh = attribute.domain['t0']
    attribute.parameters['refYear']  = date_mnh.year
    #attribute.parameters['refMonth'] = date_mnh.month
    attribute.parameters['refDay']   =  date_mnh.timetuple().tm_yday  #date_mnh.day
    
    #'projection','duration' are not defined here

    #add domain length to attribute 
    Lx = 1.*(attribute.domain['NEx']-attribute.domain['SWx'])
    Ly = 1.*(attribute.domain['NEy']-attribute.domain['SWy'])
    attribute.domain['Lx'] = Lx
    attribute.domain['Ly'] = Ly
    
    # create atmo grid
    ##################
    atmo_dx = old_div(Lx,nx)
    atmo_dy = old_div(Ly,ny)
    y_center = .5 * (Ly - 2 * atmo_dy) #of the domain without the grid pt on the side
    atmo_grid_x = np.arange(attribute.domain['SWx'],attribute.domain['NEx']+atmo_dx, atmo_dx)
    atmo_grid_y = np.arange(attribute.domain['SWy'],attribute.domain['NEy']+atmo_dy, atmo_dy)
    #2D grid
    atmo_grid_y2d, atmo_grid_x2d = np.meshgrid(atmo_grid_y, atmo_grid_x)


    # create bmap grid
    ##################
    #this need to be defined here for bmap matrix
    BMapsResolution = max(old_div(spatialIncrement,np.sqrt(2)), minimalPropagativeFrontDepth)
    print('BMapsResolution = ', BMapsResolution)

    nbre_firePt_per_atmoCell_x = (int(old_div(atmo_dx,BMapsResolution)) ) 
    bmap_dx = old_div(atmo_dx, nbre_firePt_per_atmoCell_x)
    bmap_grid_x = np.arange(attribute.domain['SWx'],attribute.domain['NEx']+bmap_dx, bmap_dx)
    
    nbre_firePt_per_atmoCell_y = (int(old_div(atmo_dy,BMapsResolution)) ) 
    bmap_dy = old_div(atmo_dy, nbre_firePt_per_atmoCell_y)
    bmap_grid_y = np.arange(attribute.domain['SWy'],attribute.domain['NEy']+bmap_dy, bmap_dy)
    
    #2D grid
    bmap_grid_y2d, bmap_grid_x2d = np.meshgrid(bmap_grid_y, bmap_grid_x)

    if ronan_param['init_mode'] == 'ideal':
        #create firedata array from input info
        firedata_grid_x = np.arange(attribute.domain['SWx'],attribute.domain['NEx']+ronan_param['firedata_res'], ronan_param['firedata_res'])
        firedata_grid_y = np.arange(attribute.domain['SWy'],attribute.domain['NEy']+ronan_param['firedata_res'], ronan_param['firedata_res'])
        firedata_grid_y2d, firedata_grid_x2d = np.meshgrid(firedata_grid_y, firedata_grid_x)
        firedata = np.zeros([firedata_grid_y2d.shape[0]-1,firedata_grid_y2d.shape[1]-1],dtype=np.dtype([('grid_e', '<f8'), ('grid_n', '<f8'), ('plotMask', '<f8'), 
                                                                                                        ('fre_f', '<f8'),('fre_s', '<f8'), ('arrivalTime', '<f8'), 
                                                                                                        ('residenceTime', '<f8'), ('burningTime', '<f8'), 
                                                                                                        ('moisture', '<f8')])) 
        firedata = firedata.view(np.recarray)
       
        print('fire data dimension = ',firedata.shape)

        firedata.grid_e = firedata_grid_x2d[:-1,:-1]
        firedata.grid_n = firedata_grid_y2d[:-1,:-1]
        dx,dy = firedata.grid_e[1,1]-firedata.grid_e[0,0], firedata.grid_n[1,1]-firedata.grid_n[0,0]
        firedata_reso = np.ones_like(firedata.grid_e)*dx*dy
     
        for key in ['arrivalTime','residenceTime','moisture','burningTime']:
            firedata[key][:,:] = -999
        firedata.fre_f[:,:] = 0.
        firedata.fre_s[:,:] = 0.
        for i_fireLine in range(nbre_fire_line):
            xlocfire =  firedata.grid_e[np.abs(firedata.grid_e[:,0]-x_location_fireLine[i_fireLine]).argmin(),0]
            idx = np.where( (firedata.grid_e >= xlocfire)                            &\
                            (firedata.grid_e <  xlocfire+depth_fireLine[i_fireLine]) &\
                            (firedata.grid_n >= y_center-.5*length_fireLine[i_fireLine])                    &\
                            (firedata.grid_n <  y_center+.5*length_fireLine[i_fireLine])                     )
       
            firedata.residenceTime[idx] = residence_time[i_fireLine]
            firedata.burningTime[idx]   = burning_time[i_fireLine]
            firedata.moisture[idx]      = moisture[i_fireLine]
            firedata.arrivalTime[idx]   = time_start_fireLine[i_fireLine]
            firedata.fre_f[idx]           = old_div(nominalHeatFlux_f[i_fireLine]*ff_param['fractionRadiation_f'],(1.-ff_param['fractionRadiation_f']))\
                                          *ronan_param['firedata_res']*ronan_param['firedata_res']*residence_time[i_fireLine] * 1.e-6 # MJ
            firedata.fre_s[idx]           = old_div(nominalHeatFlux_s[i_fireLine]*ff_param['fractionRadiation_s'],(1.-ff_param['fractionRadiation_s']))\
                                          *ronan_param['firedata_res']*ronan_param['firedata_res']*(burning_time[i_fireLine]-residence_time[i_fireLine]) * 1.e-6 # MJ


    #match pixels from firedata grid and atmo grid
    #######################
    regrid_name = 'fireData2atmo'
    wkdir = './'
    gridin_polygon,  gridin_xx,  gridin_yy  = regrid.get_polygon_from_grid(firedata.grid_e, firedata.grid_n, flag_add_1extra_rowCol=True)
    gridout_polygon, gridout_xx, gridout_yy = regrid.get_polygon_from_grid(atmo_grid_x2d,   atmo_grid_y2d                               )
    
    in_outA_grid_list, outA_in_grid_list = regrid.grid2grid_pixel_match(regrid_name,                                  \
                                                                        gridin_xx,  gridin_yy, gridin_polygon,None,   \
                                                                        gridout_polygon,                              \
                                                                        wkdir,                                        \
                                                                        flag_parallel=ronan_param['flag_parallel'],   \
                                                                        flag_freshstart=ronan_param['flag_freshStart'])


    #init param
    windU_in = 10 # cte wind speed
    windV_in = 0
    mydatanc = './Inputs/mydata_'+expr_name+'.nc'
    mybmapnc = './Inputs/mybmap_'+expr_name+'.nc'

    #set model name
    attribute.heatFlux['model0name'] = ronan_param['heatFluxModel']
    attribute.vaporFlux['model1name'] = ronan_param['vaporFluxModel']
    attribute.scalarFlux['model2name'] = ronan_param['scalarFluxModel']


    #---------------------------------------------
    #create data nc file for ForeFire Layer Models
    #---------------------------------------------
    print('write data file: ', mydatanc.split('/')[-1])
    fnameout = mydatanc
    nc_dimension = {'DIMX': -999, 'DIMY': -999, 'DIMZ': -999, 'DIMT': -999, 'domdim': -999}


    var4dim = {'dim1': 'DIMX',   'dim2': 'DIMY',  'dim3': 'DIMZ', 'dim4': 'DIMT', 'type': 'float' }
    nc_variable_data  = {'vaporFlux' : var4dim, \
                    'heatFlux'  : var4dim, \
                    'scalarFlux'       : var4dim, \
                    'fuel'      : var4dim, \
                    'windU'     : var4dim, \
                    'windV'     : var4dim, \
                    'altitude'  : var4dim, \
                    #fire regime control time
                    'FromObs_residenceTime'    : var4dim,\
                    'FromObs_burningTime'    : var4dim,\
                    'FromObs_evaporationTime'    : var4dim,\
                    #sensible heat and Vapor flux
                    'FromObs_NominalHeatFlux_flaming'  : var4dim,\
                    'FromObs_NominalHeatFlux_smoldering'  : var4dim,\
                    'FromObs_VaporFlux_evaporation' : var4dim,\
                    #Emission Factor
                    'FromObs_EFScalar_flaming': var4dim,\
                    'FromObs_EFScalar_smoldering': var4dim,\
                    #radiation Fraction
                    'FromObs_radiationFraction_flaming' : var4dim,\
                    'FromObs_radiationFraction_smoldering' : var4dim,\
                    #convsersion factor
                    'FromObs_conversionFactor' : var4dim,\
                    #misc
                    'parameters': {'dim1': 'domdim', 'type': 'c'},  
                    'domain'    : {'dim1': 'domdim', 'type': 'c'}  }

    nc_attribute  = {'vaporFlux': attribute.vaporFlux,
                    'heatFlux'  : attribute.heatFlux,\
                    'scalarFlux'       : attribute.scalarFlux,\
                    'fuel'      : attribute.fuel, \
                    'windU'     : attribute.windU, \
                    'windV'     : attribute.windV, \
                    'altitude'  : attribute.altitude, \
                    #fire regime control time
                    'FromObs_residenceTime'    : attribute.FromObs_residenceTime, \
                    'FromObs_burningTime'    : attribute.FromObs_burningTime, \
                    'FromObs_evaporationTime'    : attribute.FromObs_evaporationTime, \
                    #sensible heat & vapor8flux
                    'FromObs_NominalHeatFlux_flaming'  : attribute.FromObs_NominalHeatFlux_flaming, \
                    'FromObs_NominalHeatFlux_smoldering'  : attribute.FromObs_NominalHeatFlux_smoldering, \
                    'FromObs_VaporFlux_evaporation' : attribute.FromObs_VaporFlux_evaporation, \
                    #Emission Factor
                    'FromObs_EFScalar_flaming': attribute.FromObs_EFScalar_flamning, \
                    'FromObs_EFScalar_smoldering': attribute.FromObs_EFScalar_smoldering, \
                    #radiation fraction
                    'FromObs_radiationFraction_flaming': attribute.FromObs_radiationFraction_flaming, \
                    'FromObs_radiationFraction_smoldering': attribute.FromObs_radiationFraction_smoldering, \
                    #conversion factor
                    'FromObs_conversionFactor': attribute.FromObs_conversionFactor, \
                    #misc
                    'parameters': attribute.parameters,                
                    'domain'    : attribute.domain}


    #open nc file
    nco = io.netcdf_file(fnameout, 'w')

    #set up dimension
    for dim in list(nc_dimension.keys()):
        if dim == 'DIMX':
            nco.createDimension(dim, nx )
        elif dim == 'DIMY':
            nco.createDimension(dim, ny )
        elif dim == 'DIMZ':
            nco.createDimension(dim, 1 )
        elif dim == 'DIMT':
            nco.createDimension(dim, 1 )
        elif dim == 'domdim':
            nco.createDimension(dim, 1 )
        else:
            print('should not be here')
            pdb.set_trace()

    #set sensible heat flux at the resoluton of mnh
    #-------------
    _, dimensions_value = get_dimension_name_and_value('FromObs_residenceTime',nc_variable_data, nco) # get dimension
    residenceTime_out, _ = regrid.map_data(outA_in_grid_list,dimensions_value[2:][::-1],firedata.residenceTime,flag='average') #[::-1] as in nc dimension are stored in inverse order 
    burningTime_out, _   = regrid.map_data(outA_in_grid_list,dimensions_value[2:][::-1],firedata.burningTime,flag='average') 
    evaporationTime_out  = np.zeros_like(burningTime_out) + ronan_param['evaporation_time']

    #afArea_in  = np.where(firedata.fre>0,firedata_reso,np.zeros_like(firedata.fre))
    #afArea_out, _ = regrid.map_data(outA_in_grid_list,dimensions_value[2:][::-1],afArea_in,flag='sum',gridReso_in=firedata_reso)

    fre_f_out, _ = regrid.map_data(outA_in_grid_list,dimensions_value[2:][::-1],firedata.fre_f,flag='sum',gridReso_in=firedata_reso)
    fre_f_out *= 1.e6 # J
   
    
    fre_s_out, _ = regrid.map_data(outA_in_grid_list,dimensions_value[2:][::-1],firedata.fre_s,flag='sum',gridReso_in=firedata_reso)
    fre_s_out *= 1.e6 # J
 
    '''
    #remove pixel with low burningTime and residenceTime
    idx = np.where( (residenceTime_out<1) ) #& (fre_f_out>0))
    fre_f_out[idx] = 0
    residenceTime_out[idx] = 0
    
    idx = np.where( ((burningTime_out-residenceTime_out)<1) )#& ((fre_f_out+fre_s_out)>0))  # smouldering time < 1
    fre_f_out[idx] = 0
    fre_s_out[idx] = 0
    burningTime_out[idx] = 0
    residenceTime_out[idx] = 0
    '''

    #clip value of residence and burning time that do not fit fre 
    idx = np.where((residenceTime_out<0)|(burningTime_out<=residenceTime_out))
    for ii,jj in zip(idx[0],idx[1]):
        if (fre_f_out[ii,jj] > 0) & (fre_s_out[ii,jj] > 0) : 
            fre_s_out[ii,jj] += fre_f_out[ii,jj] # pass flaming fre to smouldering
            fre_f_out[ii,jj] = 0
            residenceTime_out[ii,jj] = 0 #.
        elif (fre_f_out[ii,jj] > 0) & (fre_s_out[ii,jj] == 0) : 
            residenceTime_out[ii,jj] =  residenceTime_out[np.where(residenceTime_out>0)].max()
            burningTime_out[ii,jj]   =  residenceTime_out[ii,jj] 
        elif (fre_f_out[ii,jj] == 0) & (fre_s_out[ii,jj] > 0) : 
            burningTime_out[ii,jj]   =  burningTime_out[np.where(burningTime_out>0)].max()
            residenceTime_out[ii,jj] =  0. 
        elif (fre_f_out[ii,jj] == 0) & (fre_s_out[ii,jj] == 0) : 
            burningTime_out[ii,jj]   =  0.
            residenceTime_out[ii,jj] =  0. 


    #mass comsumption
    mass_comsumption_out = ff_param['conversionFactor'] * 1.e-6 * (fre_f_out + fre_s_out) # kg
    
                           

    #flaming sensible heat flux
    print('compute flaming senible heat flux')
    idx = np.where(fre_f_out>0)
    sensible_heat_flux_f = np.zeros_like(fre_f_out) 
    sensible_heat_flux_f[idx] = old_div((old_div((1.-ff_param['fractionRadiation_f']),ff_param['fractionRadiation_f']))\
                               *fre_f_out[idx],(residenceTime_out[idx]* atmo_dx * atmo_dy)) # W/m2
    if len(np.where(sensible_heat_flux_f<0)[0])>0:
        print('issue in flaming sensible heat flux. found <0 values')
        sys.exit()
    if sensible_heat_flux_f.max() > 1.e10: 
        pdb.set_trace()


    #patch to remove high flux
    #idx = np.where(sensible_heat_flux_f>7.e5)
    #sensible_heat_flux_f[idx]=0
    #residenceTime_out[idx]=0
    

    #smoldering sensible heat flux
    print('compute smoldering senible heat flux')
    idx = np.where(fre_s_out>0)
    sensible_heat_flux_s = np.zeros_like(fre_s_out) 
    sensible_heat_flux_s[idx] = old_div((old_div((1.-ff_param['fractionRadiation_s']),ff_param['fractionRadiation_s']))\
                               *fre_s_out[idx],((burningTime_out[idx]-residenceTime_out[idx])* atmo_dx * atmo_dy)) # W/m2
    if len(np.where(sensible_heat_flux_s<0)[0])>0:
        print('issue in smoldering sensible heat flux. found <0 values')
        sys.exit()
    
    #patch to remove high flux
    #idx = np.where(sensible_heat_flux_s>=4.e5)
    #sensible_heat_flux_s[idx]=0
    #burningTime_out[idx]= residenceTime_out[idx]


    #Vapor flux during evaporation
    print('compute vapor flux')
    moisture_out, _ = regrid.map_data(outA_in_grid_list,dimensions_value[2:][::-1],firedata.moisture,flag='average') 
    moisture_out = np.where(moisture_out<0,np.zeros_like(moisture_out),moisture_out)
    
    vapor_flux = np.zeros_like(moisture_out) 
    idx = np.where(moisture_out*mass_comsumption_out > 0)
    vapor_flux[idx] = old_div((1.e-2*moisture_out[idx]*mass_comsumption_out[idx]),(evaporationTime_out[idx]* atmo_dx * atmo_dy)) # kg/m2


    #create variables
    #--------------
    for key in nc_variable_data:
        dimensions_name, dimensions_value = get_dimension_name_and_value(key,nc_variable_data, nco)
        #nco.variables[key][:] = np.zeros(dimensions_value)

        if   key == 'windU': 
            nco.variables[key][:] = np.zeros(dimensions_value) + windU_in
        elif key == 'windV': 
            nco.variables[key][:] = np.zeros(dimensions_value) + windV_in
        elif key == 'vaporFlux': 
            nco.variables[key][:] = np.zeros(dimensions_value) + 1
        elif key == 'heatFlux': 
            nco.variables[key][:] = np.zeros(dimensions_value) + 0
        elif key == 'scalarFlux': 
            nco.variables[key][:] = np.zeros(dimensions_value) + 2
        
        elif key == 'FromObs_residenceTime':
            print('   map residence time:') 
            nco.variables[key][:] = np.array(residenceTime_out.T,dtype=np.float32).reshape(dimensions_value)
        
        elif key == 'FromObs_evaporationTime':
            print('   **********  WARNING ***********: map evaporation time is set constant') 
            nco.variables[key][:] = np.array(evaporationTime_out.T,dtype=np.float32).reshape(dimensions_value)
        
        elif key == 'FromObs_burningTime':
            print('   map burning time:') 
            nco.variables[key][:] = np.array(burningTime_out.T,dtype=np.float32).reshape(dimensions_value)
        
        elif key == 'FromObs_NominalHeatFlux_flaming': 
            print('   map flaming nominal heat flux:') 
            nco.variables[key][:] = np.array(sensible_heat_flux_f.T,dtype=np.float32).reshape(dimensions_value)
        
        elif key == 'FromObs_NominalHeatFlux_smoldering': 
            print('   map smoldering nominal heat flux:') 
            nco.variables[key][:] = np.array(sensible_heat_flux_s.T,dtype=np.float32).reshape(dimensions_value)

        elif key == 'FromObs_VaporFlux_evaporation': 
            print('   map Vapor flux:')
            nco.variables[key][:] = np.array(vapor_flux.T,dtype=np.float32).reshape(dimensions_value)
            #nco.variables[key][:] = np.array(np.zeros_like(vapor_flux).T,dtype=np.float32).reshape(dimensions_value)
            dimensions_value_EVAP = dimensions_value

        elif key == 'FromObs_EFScalar_flaming':
            print('   **********  WARNING ***********:  EF scalar flaming is set constant')
            nco.variables[key][:] = np.zeros(dimensions_value) + 0.01
        
        elif key == 'FromObs_EFScalar_smoldering':
            print('   **********  WARNING ***********:  EF scalar smoldering is set constant')
            nco.variables[key][:] = np.zeros(dimensions_value) + 0.02
        
        elif key == 'FromObs_radiationFraction_flaming': 
            print('   map fraction radiation for smoldering:')
            nco.variables[key][:] = np.zeros(dimensions_value) + ff_param['fractionRadiation_f']
        
        elif key == 'FromObs_radiationFraction_smoldering': 
            print('   map fraction radiation for smoldering:')
            nco.variables[key][:] = np.zeros(dimensions_value) + ff_param['fractionRadiation_s']
        
        elif key == 'FromObs_conversionFactor': 
            print('   map consersion factor:')
            nco.variables[key][:] = np.zeros(dimensions_value) + ff_param['conversionFactor']
        
        elif key == 'fuel': 
            print('   map fuel:')
            nco.variables[key][:] = np.zeros(dimensions_value) + 1
        
        else:
            nco.variables[key][:] = np.zeros(dimensions_value)

        for attvar in nc_attribute[key]: 
            nco.variables[key]._attributes[attvar] =  nc_attribute[key][attvar]
             

    #close nc file
    nco.close()


    #---------------------------------------------
    # create bmap nc file to force fire propapation
    #---------------------------------------------
    print('write bmap file: ', mybmapnc.split('/')[-1])
    fnameout = mybmapnc
    nc_dimension = {'DIMX': -999, 'DIMY': -999, 'C_DIMX': -999, 'C_DIMY': -999, 'domdim': -999}

    var2dim_atm  = {'dim1': 'C_DIMX',   'dim2': 'C_DIMY',  'type': 'int32' }
    var2dim_fire = {'dim1': 'DIMX',     'dim2': 'DIMY',    'type': 'float' }

    nc_variable_bmap  = {'arrival_time_of_front' : var2dim_fire, \
                         'cell_active'           : var2dim_atm,  
                         'domain'                : {'dim1': 'domdim', 'type': 'c'}  }

    nc_attribute  = {'arrival_time_of_front': None,
                    'cell_active'           : None, \
                    'domain'                : merge_two_dicts(attribute.domain,attribute.parameters)}
            


    #sys.exit()
    #open nc file
    nco = io.netcdf_file(fnameout, 'w')

    #set up dimension
    for dim in list(nc_dimension.keys()):
        if dim == 'C_DIMX':
            nco.createDimension(dim, nx )
        elif dim == 'C_DIMY':
            nco.createDimension(dim, ny )
        elif dim == 'DIMX':
            nco.createDimension(dim, nbre_firePt_per_atmoCell_x * (nx))
        elif dim == 'DIMY':
            nco.createDimension(dim, nbre_firePt_per_atmoCell_y * (ny))
        elif dim == 'domdim':
            nco.createDimension(dim, 1 )
        else:
            print('should not be here')
            pdb.set_trace()


    #match pixels from firedata grid and atmo grid
    #######################
    regrid_name = 'fireData2ff'
    wkdir = './'
    gridin_polygon, firein_polygon, gridin_xx, gridin_yy  = regrid.get_polygon_from_grid(firedata.grid_e, firedata.grid_n, \
                                                                         flag_add_1extra_rowCol=True, firedata=firedata)
    gridout_polygon, gridout_xx, gridout_yy = regrid.get_polygon_from_grid(bmap_grid_x2d, bmap_grid_y2d)

    in_outFF_grid_list, outFF_in_grid_list = regrid.grid2grid_pixel_match(regrid_name,                         \
                                                                          gridin_xx, gridin_yy, gridin_polygon, firein_polygon,\
                                                                          gridout_polygon,                     \
                                                                          wkdir, \
                                                                          flag_parallel=ronan_param['flag_parallel'], \
                                                                          flag_freshstart=ronan_param['flag_freshStart'])

    #regrid evaporation time if needed
    if evaporationTime_out.size != dimensions_value_EVAP[0]*dimensions_value_EVAP[1]:
        grid_x_ = .5*(gridout_xx[:-1,:-1]+gridout_xx[1:,1:])
        grid_y_ = .5*(gridout_yy[:-1,:-1]+gridout_yy[1:,1:])
        points = np.dstack(  ( .5*(atmo_grid_x2d[:-1,:-1]+atmo_grid_x2d[1:,1:]).flatten(), .5*(atmo_grid_y2d[:-1,:-1]+atmo_grid_y2d[1:,1:]).flatten()  ) )[0]
        values = evaporationTime_out.flatten()
        evaporationTime_out_regrid = interpolate.griddata(points, values, (grid_x_, grid_y_), method='nearest')
    else: 
        evaporationTime_out_regrid = evaporationTime_out


    #create variables
    for key in nc_variable_bmap:
        dimensions_name, dimensions_value = get_dimension_name_and_value(key,nc_variable_bmap, nco)
        nco.variables[key][:] = np.zeros(dimensions_value)

        if   key == 'arrival_time_of_front': 
            print('map arrival time to bmap:') 
            arrivalTime_out, _ = regrid.map_data(outFF_in_grid_list,dimensions_value[::-1],firedata.arrivalTime,flag='average') 
            idx = np.where(arrivalTime_out>=0)
            arrivalTime_out[idx] -= evaporationTime_out_regrid[idx] # time when evaporation start 
          
            #adjust arrival time to residence time map
            #idx_ = np.where(residenceTime_out<=0)
            #arrivalTime_out[idx_] = -999

            if ronan_param['init_mode'] == 'fromOverHeadImg':
                print('**********  WARNING ***********:  applying shift on arrivalTime')
                idx = np.where(arrivalTime_out!=-999)
                shifTime = 0 if 'ignitiontime' not in list(ronan_param.keys()) else \
                           -1*(attribute.domain['t0']+arrivalTime_out[idx].min())+ronan_param['ignitiontime']
                arrivalTime_out[idx] = (arrivalTime_out[idx] + attribute.domain['t0'] + shifTime) 
            else:
                shifTime = 0.
            
            nco.variables[key][:] = np.array(arrivalTime_out.T,dtype=np.float32).reshape(dimensions_value)
        
        elif key == 'cell_active': 
            nco.variables[key][:] = np.ones(dimensions_value,dtype=int) 
        
        else:
            nco.variables[key][:] = np.zeros(dimensions_value)

        if nc_attribute[key] is not None:
            for attvar in nc_attribute[key]: 
                nco.variables[key]._attributes[attvar] =  nc_attribute[key][attvar]
             

    #close nc file
    nco.close()

    print('')
    print('Time Info:')
    print('ref MNH time             : ', attribute.domain['t0'])
    print('shift time               : ', shifTime)
    print('min firedata arrival time: ', firedata.arrivalTime[np.where(firedata.arrivalTime>=0)].min())
    idx = np.where(arrivalTime_out!=-999)
    print('nbre points = {:d}'.format(idx[0].shape[0]))
    print('Final arrivalTime min={:.1f} max={:.1f}'.format(arrivalTime_out[idx].min(),arrivalTime_out[idx].max()))   
    print('')
    #save shift in space and time to combine later the original 2D firescene and the atmospheric domain
    atmosDomainReference = {'time': attribute.domain['t0'] + shifTime, 
                            'x': shiftx, 
                            'y': shifty,
                            'z': shiftz, 
                            'flag_rotation': flag_rotation,
                            'shiftTheta'   : shiftTheta,
                            'rotce'        : rotce,
                            'rotcn'        : rotcn,
                            }
    with open( 'Inputs/'+'atmosDomainReference.p', "wb" ) as f :
        pickle.dump( atmosDomainReference,f )
   
    #save "_out" array for use in runff
    out = np.zeros_like(fre_f_out,dtype=np.dtype([('fre_f',float),('fre_s',float),
                                                  ('burningTime',float),('residenceTime',float)]))
    out = out.view(np.recarray)
    out.fre_f = fre_f_out
    out.fre_s = fre_s_out
    out.burningTime = burningTime_out
    out.residenceTime = residenceTime_out

    np.save('Inputs/'+'out_{:s}.npy'.format(expr_name), out)

    #---------------------------------------------
    # set ForeFire simulation
    #---------------------------------------------
    sys.exit()
    if flag_run_ff:
        ff = forefire.ForeFire()

        
        #set param
        ff.setString('ForeFireDataDirectory','Inputs')
        ff.setString('fireOutputDirectory','ForeFire/Outputs')
        ff.setInt('outputsUpdate',outputsUpdate)
        
        ff.setString('NetCDFfile',    mydatanc.split('/')[-1])
        ff.setString('fluxNetCDFfile',mydatanc.split('/')[-1])
        ff.setString('fuelsTableFile','fuels.ff')
        ff.setString('BMapFiles', mybmapnc.split('/')[-1])
        
        ff.setDouble("spatialIncrement",spatialIncrement)
        ff.setDouble("perimeterResolution",perimeterResolution)
        ff.setDouble("minimalPropagativeFrontDepth",minimalPropagativeFrontDepth)
       
        if ronan_param['heatFluxModel'] == 'heatFluxNominal':
            ff.setDouble("nominalHeatFlux", ff_param['nominalHeatFlux'])
        if ronan_param['vaporFluxModel'] == 'vaporFluxNominal':
            ff.setDouble("nominalVaporFlux", ff_param['nominalVaporFlux'])
        if ronan_param['scalarFluxModel'] == 'scalarFluxNominal':
            ff.setDouble("nominalScalarFlux", ff_param['nominalScalarFlux'])

        #ff.setDouble("nominalHeatFlux",nominalHeatFlux)
        #ff.setDouble("nominalVaporFlux",nominalVaporFlux)
        #ff.setDouble("burningDuration",burningDuration)

        ff.setDouble("bmapOutputUpdate",bmapOutputUpdate)
        ff.setInt("defaultHeatType",0)
        ff.setInt("defaultscalarType",2)
        ff.setInt("defaultVaporType",1)
        
        #ff.setInt('bmapLayer',1)
        ff.setDouble("InitTime",InitTime)

        #set domain
        ff.setInt("atmoNX",nx)
        ff.setInt("atmoNY",ny)
        ff.execute("FireDomain[sw=(%f,%f,0.);ne=(%f,%f,0.);t=%f]"%(attribute.domain['SWx'],attribute.domain['SWy'],\
                                                                   attribute.domain['NEx'],attribute.domain['NEy'],  \
                                                                   attribute.domain['t0']))

        extentLocal= getDomainExtent(ff.execute("print[]").split("\n")[0]);


        #set propagation model
        ff.addLayer("propagation","TroisPourcent","propagationModel")
        
        #residenceTime = np.zeros((nx,ny,1)) + 60
        #ff.addScalarLayer("double","residenceTime",0 , 0, 0,extentLocal[1]-extentLocal[0], extentLocal[3]-extentLocal[2] , 0, residenceTime)
        
        fuelmap = ff.getDoubleArray("fuel").astype("int32")
        ff.addIndexLayer("table","fuel", extentLocal[0], extentLocal[2],0,  extentLocal[1]-extentLocal[0], extentLocal[3]-extentLocal[2], 0, fuelmap)

        print("resolution of bmap is ", ff.getString("bmapResolution"))

        #set fire line
        if flag_extra_contour:
            print('add an extra coutour')
            ff.execute("\tFireFront[t={%f}]".format(attribute.domain['t0']))
            ff.execute("\t\tFireNode[loc=(090,100,0.);vel=(0.,0.,0.);t=%f]".format(attribute.domain['t0']))
            ff.execute("\t\tFireNode[loc=(090,300,0.);vel=(0.,0.,0.);t=%f]".format(attribute.domain['t0']))
            ff.execute("\t\tFireNode[loc=(100,300,0.);vel=(0.,0.,0.);t=%f]".format(attribute.domain['t0']))
            ff.execute("\t\tFireNode[loc=(100,100,0.);vel=(0.,0.,0.);t=%f]".format(attribute.domain['t0']))

        sys.exit()
        #---------------------------------------------
        # run ForeFire simulation
        #---------------------------------------------
        plt.figure(1)
        plt.ion()
        pathes = []
        step = 50
        time_duration = 2000
        N_step = int(old_div(time_duration, step)) + 1
        #step = 10
        #N_step = 80/step
        flux_out_ff_history = []
        fre_ff = np.zeros_like(fre_f_out)
        h2o_ff = np.zeros_like(fre_f_out)
        scalar_ff = np.zeros_like(fre_f_out)
        chf_ff = np.zeros_like(fre_f_out)
        for i in np.arange(1,N_step):
            
            ff_time = attribute.domain['t0'] + i*step

            print("goTo[t=%f]"%(ff_time), end=' ')
            ff.execute("goTo[t=%f]"%(ff_time))
            

            #HeatFLux
            Hflux2d = ff.getDoubleArray('heatFlux')[0,0,:,:] * 1.e-6  # MW/m2 (sensible heat flux)
            flux_out_ff =  (Hflux2d * atmo_dx * atmo_dy).sum() #MW
       
            frp_ff_add = old_div(ff_param['fractionRadiation_f'],(1.-ff_param['fractionRadiation_f'])) * Hflux2d * atmo_dx * atmo_dy  # MW FRP
            fre_ff_add = frp_ff_add * step # MJ FRE
          
            fre_ff += (fre_ff_add).T
            chf_ff += (Hflux2d * step).T

            #Scalar Flux
            Sflux2d = ff.getDoubleArray('scalarFlux')[0,0,:,:]
            scalar_ff += (Sflux2d * atmo_dx * atmo_dy * step).T

            #Vapor Flux
            Vflux2d = ff.getDoubleArray('vaporFlux')[0,0,:,:]
            h2o_ff += (Vflux2d * atmo_dx * atmo_dy * step).T

            '''
            fig = plt.figure(figsize=(15,8))
            ax = plt.subplot(131)
            ax.imshow(Sflux2d.T,origin='lower',interpolation='nearest')
            ax.set_title('Scalar')
            bx = plt.subplot(132)
            bx.imshow(Hflux2d.T,origin='lower',interpolation='nearest')
            bx.set_title('Heat')
            cx = plt.subplot(133)
            cx.imshow(Vflux2d.T,origin='lower',interpolation='nearest')
            cx.set_title('Vapor')
            plt.show()
            '''
            
            if ronan_param['init_mode'] == 'ideal':
                idx_fire_line_f = np.where( (np.array(time_start_fireLine) <= ff_time) & 
                                            (np.array(time_start_fireLine)+np.array(residence_time) >= ff_time) )
                idx_fire_line_s = np.where( (np.array(time_start_fireLine)+np.array(residence_time) < ff_time) & 
                                            (np.array(time_start_fireLine)+np.array(burning_time) >= ff_time) )
                flux_expected = 0
                for idx_ in idx_fire_line_f[0]:
                    flux_expected += depth_fireLine[idx_]*length_fireLine[idx_]*nominalHeatFlux_f[idx_] * 1.e-6 
                for idx_ in idx_fire_line_s[0]:
                    flux_expected += depth_fireLine[idx_]*length_fireLine[idx_]*nominalHeatFlux_s[idx_] * 1.e-6 
            
                pathes += printToPathe( ff.execute("print[]"))
                if flux_expected != 0:
                    flux_out_ff_history.append(old_div(flux_out_ff,flux_expected))
                else:
                    if flux_out_ff < 1.e-6: 
                        flux_out_ff_history.append(1)
                    else:
                        flux_out_ff_history.append(0)

                print('| flux out of ff = ', flux_out_ff, '| expected = ', flux_expected, '| ratio = ', flux_out_ff_history[-1])
           
            if ronan_param['init_mode'] == 'fromOverHeadImg':
                print('| flux out of ff = ', flux_out_ff , 1.e-6*fre_ff_add.max())

            
            plt.clf()
            ax = plt.subplot(111)
            ax.imshow(np.ma.masked_where(Hflux2d<=0,Hflux2d).T,origin='lower',interpolation='nearest',vmin=0.001,vmax=1.e-1)
            ax.set_title('t={:.1f}'.format(ff_time))
            #plt.show()
            plt.draw()
            plt.pause(.001)
            
            if np.isnan(fre_ff.sum()):
                pdb.set_trace()


        plt.ioff()
        plt.close()

        
        print('conservation FRE')
        print(fre_ff.sum())
        print((fre_f_out+fre_s_out)[np.where(fre_f_out+fre_s_out>0)].sum()*1.e-6)
        print((firedata.fre_f+firedata.fre_s)[np.where((firedata.fre_f+firedata.fre_s>0))].sum())
        print((old_div(ff_param['fractionRadiation_f'],(1.-ff_param['fractionRadiation_f'])))*(sensible_heat_flux_f * residenceTime_out * atmo_dx * atmo_dy).sum() * 1.e-6 + \
              (old_div(ff_param['fractionRadiation_s'],(1.-ff_param['fractionRadiation_s'])))*(sensible_heat_flux_s * (burningTime_out-residenceTime_out) * atmo_dx * atmo_dy).sum() * 1.e-6)   

        print('')
        print('conservation Vapor')
        print(h2o_ff.sum())
        print((mass_comsumption_out  * 1.e-2 * moisture_out ).sum())
   
        print('')
        print('conservation Scalar')
        print(scalar_ff.sum())
        print(ff_param['conversionFactor'] * 1.e-6 * ( 0.01*fre_f_out + .02*fre_s_out).sum())
        
        print('')
        print('mean nominal flux')
        fuelparams = pandas.read_csv('./Inputs/fuels.ff', sep=';')
        firedurationmap = np.zeros_like(fuelmap)
        for fuelidx,tau0,sd in zip(fuelparams['Index'], fuelparams['Tau0'], fuelparams['sd']):
            idx_ = np.where(fuelmap==fuelidx)
            firedurationmap[idx_] = tau0/sd
        fireduration_mean = firedurationmap[0,0,:,:][np.where(chf_ff>0)].mean()
        
        print('fireduration_mean', fireduration_mean)

        meanNominalHeatFlux = chf_ff.sum()/(np.where(chf_ff>0)[0].shape[0])  /                        fireduration_mean
        meanNominalVaporFlux = h2o_ff.sum()/(np.where(chf_ff>0)[0].shape[0]) /  (atmo_dx * atmo_dy) / fireduration_mean
        meanNominalScalarFlux = scalar_ff.sum()/(np.where(chf_ff>0)[0].shape[0]) /  (atmo_dx * atmo_dy) / fireduration_mean
    
        print('meanNominalHeatFlux (W/m2)', meanNominalHeatFlux*1.e6)
        print('meanNominalVaporFlux', meanNominalVaporFlux)
        print('meanNominalScalarFlux', meanNominalScalarFlux)
        
        lines=[]
        lines.append( 'meanNominalHeatFlux (W/m2) = {:.8f}\n'.format( meanNominalHeatFlux*1.e6) )
        lines.append( 'meanNominalVaporFlux (kg/m2) = {:.8f}\n'.format( meanNominalVaporFlux) )
        lines.append( 'meanNominalScalarFlux (-/m2) = {:.8f}\n'.format( meanNominalScalarFlux) )

        with open('equivalentNominalFlux.txt','w') as f: 
            f.writelines(lines)


        if ronan_param['init_mode'] == 'ideal':
            ax = plt.subplot(111)
            ax.plot(step*np.arange(1,N_step),np.array(flux_out_ff_history))
            ax.set_ylabel('ratio:  heat_ff  /  heat_expected')
            ax.set_xlabel('time (s)')
            plt.show()

        if ronan_param['init_mode'] == 'fromOverHeadImg':
            fig = plt.figure(figsize=(12,6))
            ax = plt.subplot(131)
            im = ax.imshow(np.ma.masked_where(fre_ff<=0,fre_ff).T,origin='lower',interpolation='nearest')
            ax.set_title('fre_ff = \n rad_frac x flux_conv_FF')
            divider = make_axes_locatable(ax)
            cbaxes = divider.append_axes("bottom", size="5%", pad=0.05)
            cbar = fig.colorbar(im ,cax = cbaxes,orientation='horizontal')
            cbar.set_label('FRE (MW)',labelpad=10)

            ax = plt.subplot(132)
            im = ax.imshow(np.ma.masked_where( (fre_f_out+fre_s_out)<=0,1.e-6*(fre_f_out+fre_s_out)).T,origin='lower',interpolation='nearest')
            ax.set_title('fre_obs \n from Obs at resolution of MNH')
            divider = make_axes_locatable(ax)
            cbaxes = divider.append_axes("bottom", size="5%", pad=0.05)
            cbar = fig.colorbar(im ,cax = cbaxes,orientation='horizontal')
            cbar.set_label('FRE (MW)',labelpad=10)

            ax = plt.subplot(133)
            ratio = np.zeros_like(fre_f_out)
            idx = np.where(fre_f_out+fre_s_out>0)
            ratio[idx] = old_div(fre_ff[idx],((fre_f_out+fre_s_out)[idx]*1.e-6))
            im = ax.imshow(np.ma.masked_where((fre_f_out+fre_s_out)<=0,ratio).T,origin='lower',interpolation='nearest')
            ax.set_title('ratio fre_ff/fre_obs')
            divider = make_axes_locatable(ax)
            cbaxes = divider.append_axes("bottom", size="5%", pad=0.05)
            cbar = fig.colorbar(im ,cax = cbaxes,orientation='horizontal')
            cbar.set_label('ratio (-)',labelpad=10)
   
            fig.savefig(expr_name+'_fre_conservation.png')
            plt.show()


        sys.exit()
        #---------------------------------------------
        # plot ForeFire perimeter
        #---------------------------------------------
        fig, ax = plt.subplots()
         
        #tab = np.transpose(ff.getDoubleArray("BMap"))[0]
        #CS = ax.imshow(tab, origin='lower', cmap=plt.cm.gray, interpolation='nearest',\
        #               extent=(0,tab.shape[1]*np.float(ff.getString("bmapResolution")),0,tab.shape[0]*np.float(ff.getString("bmapResolution"))))
        #cbar = plt.colorbar(CS)
        #cbar.ax.set_ylabel('v')
        cmap = mpl.cm.get_cmap('jet')
        for i, path in enumerate(pathes):
            rgba = cmap(old_div(1.*i,(N_step-1)))
            patch = mpatches.PathPatch(path,edgecolor=rgba, facecolor='none', alpha=1)
            ax.add_patch(patch)

        ax.grid()
        ax.axis('equal')
        ax.set_xlim(attribute.domain['SWx']+atmo_dx,attribute.domain['NEx']-atmo_dx)
        ax.set_ylim(attribute.domain['SWy']+atmo_dy,attribute.domain['NEy']-atmo_dy)
        plt.show()


