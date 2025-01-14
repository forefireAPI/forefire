#!//gpfs/home/UDCPP/filippi_j/soft/bin/python2.7
'''
Created on 4 avr. 2011

@author: filippi
'''
 
from pyevtk.vtk import *  
import os 
import numpy as np 
import struct
import csv
import shutil
from copy import copy
import glob
import netCDF4 as nc4
class LaserShot():

    valueName = "PR2"
    rangeName = "R"
    azimuthKey = "AngleAzimuth"
    zenithKey = "AngleZenith"
        
    def __init__(self, filename=None, rangeName="R", valueName="PR2",origin =(0,0,0,0)):
 
        self.valueName=valueName
        self.parameters = {}
        self.parametersIndices = {}
        self.values = {}
        self.valuesIndices = {}
        self.fname=filename
        self.time = 0
        f = open(filename)
        self.origin = origin
        # Read from the object, storing the page's contents in 's'.
        s = f.read()
        f.close()
        lines = s.split("\r")
        indiceResults = -1
        for indice, line in enumerate(lines):
            if (indiceResults < 0):
                if "=" in line:
                    KeyVal = line.split("=")
                    self.parameters[KeyVal[0]] = KeyVal[1]
                    self.parametersIndices[indice] = KeyVal[0]
                else:
                    self.parametersIndices[indice] = line
                
                if (len(line) == 0 ) :
                    indiceResults = indice
        varnames = lines[indiceResults+1].split("\t")
    
       
        vals=np.zeros((len(varnames),(len(lines)-indiceResults+2)))
        for indice, line in enumerate(lines[indiceResults+2:len(lines)]):
            vLine=line.split("\t")
            for inCol, valS in enumerate(vLine):
                if(len(valS) > 0):
                    vals[inCol][indice] = float(valS)
        self.time = 0
        timinfos = self.parameters["Time"].split("_")[1].split("-")
 
        self.time = int(timinfos[0])*3600
        self.time += int(timinfos[1])*60
        self.time += int(timinfos[2])
        self.time += origin[3]
        
 
        for ind,name in enumerate(varnames):
            self.values[name] = vals[ind]
            self.valuesIndices[ind] = name
            
    def getTime(self):
        return self.time
    def saveAs(self, filename):
        fout = open(filename,'w')
        
        for key in list(self.parametersIndices.keys()):
            if self.parametersIndices[key] in self.parameters:
                fout.write( self.parametersIndices[key]+ "="+self.parameters[self.parametersIndices[key]]+"\r")
            else:
                fout.write( self.parametersIndices[key]+"\r")
        fout.write( ("\t").join(list(self.valuesIndices.values()))+"\r")

        for i in range(len(list(self.values.values())[0])):
            for key in list(self.valuesIndices.values()):
                fout.write( "%.6f\t"%self.values[key][i])
            fout.write("\r")
        fout.close()
        
    def getValues(self):
        return self.values[self.valueName]
    
    def getShotAsLatLonHeightValue(self):
        return 0
    
    def getParameters(self):
        return self.parameters
    
    def getNumPoints(self):    
        return len(self.values[self.rangeName])
    
    def setNumPointsWithSameOrigin(self,npoints):
        newValues = {}
        stepInRange = self.values[self.rangeName][1]-self.values[self.rangeName][0]
        for name in list(self.values.keys()):
            newValues[name] = np.zeros(npoints)
            niter = min(npoints,len(self.values[name]))
            for i in range(niter):
                newValues[name][i] = self.values[name][i]
        newValues[self.rangeName] = np.arange(self.values[self.rangeName][0],self.values[self.rangeName][0]+stepInRange*npoints,stepInRange)
        self.values = newValues
    
    def getShotAsXYZValue(self):
        ranges = np.array(self.values[self.rangeName])
        A= np.deg2rad(float(self.parameters[self.azimuthKey]))
        Z = np.deg2rad(float(self.parameters[self.zenithKey]))
        z = ranges*np.cos(Z)
        x = (ranges*np.sin(Z))*np.sin(A)
        y = (ranges*np.sin(Z))*np.cos(A)
        
        return (x+self.origin[0],y+self.origin[1],z+self.origin[2],np.array(self.values[self.valueName]))
    
    def getShotLocationAsXYZ(self):
        ranges = np.array(self.values[self.rangeName])
        A= np.deg2rad(float(self.parameters[self.azimuthKey]))
        Z = np.deg2rad(float(self.parameters[self.zenithKey]))
        z = ranges*np.cos(Z)
        x = (ranges*np.sin(Z))*np.sin(A)
        y = (ranges*np.sin(Z))*np.cos(A)
        retVals = []
        for i in range(len(x)):
            retVals.append( (x[i]+self.origin[0],y[i]+self.origin[1],z[i]+self.origin[2])  )
        return retVals 
       
    def setValues(self, values):
        self.values[self.valueName] = values


def pointsToVTK(path, x, y, z, data):
    """
        Export points and associated data as an unstructured grid.

        PARAMETERS:
            path: name of the file without extension where data should be saved.
            x, y, z: 1D arrays with coordinates of the points.
            data: dictionary with variables associated to each point.
                  Keys should be the names of the variable stored in each array.
                  All arrays must have the same number of elements.

        RETURNS:
            Full path to saved file.

    """
    assert (x.size == y.size == z.size)
    npoints = x.size
 
    # create some temporary arrays to write grid topology
    offsets = np.arange(start = 1, stop = npoints + 1, dtype = 'int32')   # index of last node in each cell
    connectivity = np.arange(npoints, dtype = 'int32')                   # each point is only connected to itself
    cell_types = np.empty(npoints, dtype = 'uint8') 
   
    cell_types[:] = VtkVertex.tid

    w = VtkFile(path, VtkUnstructuredGrid)
    w.openGrid()
    w.openPiece(ncells = npoints, npoints = npoints)
    
    w.openElement("Points")
    w.addData("points", (x,y,z))
    w.closeElement("Points")
    w.openElement("Cells")
    w.addData("connectivity", connectivity)
    w.addData("offsets", offsets)
    w.addData("types", cell_types)
    w.closeElement("Cells")
    
    _addDataToFile(w, cellData = None, pointData = data)

    w.closePiece()
    w.closeGrid()
    w.appendData( (x,y,z) )
    w.appendData(connectivity).appendData(offsets).appendData(cell_types)

    _appendDataToFile(w, cellData = None, pointData = data)

    w.save()
    return w.getFileName()

def imageToVTK(path, origin = (0.0,0.0,0.0), spacing = (1.0,1.0,1.0), cellData = None, pointData = None, start =   (0,0,0), end = None):

    assert (cellData is not None or pointData is not None)
    
    # Extract dimensions
 
    if cellData is not None:
        keys = list(cellData.keys())
        data = cellData[keys[0]]
        if end ==None:
            end = data.shape
    elif pointData is not None:
        keys = list(pointData.keys())
        data = pointData[keys[0]]
        if end ==None:
            end = data.shape
            end = (end[0] - 1, end[1] - 1, end[1] - 1)

    # Write data to file
    w = VtkFile(path, VtkImageData)
    w.openGrid(start = start, end = end, origin = origin, spacing = spacing)
    w.openPiece(start = start, end = end)
    _addDataToFile(w, cellData, pointData)
    w.closePiece()
    w.closeGrid()
    _appendDataToFile(w, cellData, pointData)
    w.save()
    return w.getFileName()

def gridToCdf(path, x, y, z, dataDict = None):
    import xarray as xr
    encoding = {}
    helperF = {}
    for key in dataDict.keys():
        encoding[key] = {'_FillValue': -9999.,
              'zlib': True}
        helperF[key] = {"description":key,
                        "units":"all",
            }
    
    
    scalCoords=dict(
        xat=(["x", "y", "z"], x),
        yat=(["x", "y", "z"], y),
        zat=(["x", "y", "z"], z),
    ) 
    
 
    
    ds = xr.Dataset()
 
    for pkey in dataDict.keys(): 
        ds[pkey] = xr.DataArray(
            data=dataDict[pkey].astype(np.float32),
            dims=["x", "y", "z"],
            coords=dict(
                xat=(["x", "y", "z"], x.astype(np.float32)),
                yat=(["x", "y", "z"], y.astype(np.float32)),
                zat=(["x", "y", "z"], z.astype(np.float32)),
            ),
            attrs=dict(
                description=pkey,
                units="all",
            ),
        )
        
 
    print("writing", path)
    ds.to_netcdf(path+".nc",encoding=encoding)

def gridToVTK(path, x, y, z, cellData = None, pointData = None, vectData = None,start = None, end = None):

    if(start == None):
        start = (0,0,0)
    

    ftype = VtkStructuredGrid
 
    s = x.shape
    nx, ny, nz = s[0] - 1, s[1] - 1, s[2] - 1
  

    if(end == None):
        end = (nx, ny, nz)
  

    w = VtkFile(path, ftype)
    w.openGrid(start = start, end = end)
    w.openPiece(start = start, end = end)

    w.openElement("Points")
    w.addData("points", (x,y,z))
    w.closeElement("Points")
    

    _addDataToFile(w, cellData, pointData)
    w.closePiece()
    w.closeGrid()
    # Write coordinates
    w.appendData( (x,y,z) )
    # Write data
    _appendDataToFile(w, cellData, pointData)
    w.save()
    return w.getFileName()

def formatBinS(fname, offset = 0):
  
    fsize = os.path.getsize(fname)
 
    c_file = open(fname,"rb")
    format = "%iQ" % 2
    items = struct.unpack(format,c_file.read(struct.calcsize(format)))
    nxt = items[0]
    
    nyt= items[1]
    if (nyt > 1000) :
        nyt = nxt
    nzt = ((fsize-8-8)/(nxt*nyt))/8
    
  #  print fname, nxt, nyt, nzt

def numberOfFields(fname):
    fsize = os.path.getsize(fname)
 
    c_file = open(fname,"rb")
   
    sizeofOfI4 = struct.calcsize("i")
    sizeofOfF8 = struct.calcsize("d")
    
    nxt = struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nyt= struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nzt = struct.unpack("i",c_file.read(sizeofOfI4))[0] 
    c_file.read(sizeofOfI4)
    tstep = struct.unpack("d",c_file.read(sizeofOfF8))[0]
    
    #3 ints plus padding plus tstep
    sizeOfHeader =  sizeofOfI4*3 + sizeofOfI4*3 + sizeofOfF8
    sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)
    sizeOfRecord = sizeOfHeader + sizeOfData
    
    numrec = fsize/sizeOfRecord
    
    return int(numrec)

def readAllSteps(fname, offset = 0, bestshape=None):
    fsize = os.path.getsize(fname)
 
    c_file = open(fname,"rb")
   
    sizeofOfI4 = struct.calcsize("i")
    sizeofOfF8 = struct.calcsize("d")
    
    nxt = struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nyt= struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nzt = struct.unpack("i",c_file.read(sizeofOfI4))[0] 
    c_file.read(sizeofOfI4)
    tstep = struct.unpack("d",c_file.read(sizeofOfF8))[0]
    
    #3 ints plus padding plus tstep
    sizeOfHeader =  sizeofOfI4*3 + sizeofOfI4*3 + sizeofOfF8
    sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)
    sizeOfRecord = sizeOfHeader + sizeOfData
    
    numrec = fsize/sizeOfRecord
    
  #  print numrec , " records nx:", nxt," ny:",nyt," nz:",nzt," step:",tstep," filesize:", fsize, " filename:",fname, " sizerec:",sizeOfRecord, " I4:", sizeofOfI4, " F8:",sizeofOfF8, " header:",sizeOfHeader
    
    tsteps = {}   
    for i in range(int(numrec)):
        c_file.seek(i*sizeOfRecord+sizeOfHeader-sizeofOfF8)
        tstep = struct.unpack("d",c_file.read(struct.calcsize("d")))
        tsteps[tstep[0]] = i
      
    c_file.close()
    return tsteps

def readBinNS(inpattern,domnum,stepV,vkey, appendedDict, zkey=None, bestshape=None, decay=0):

    fname =  "%s.%d.%s"%(inpattern,domnum,vkey)
    if (appendedDict == None):
        fname =  "%s.%d.%d.%s"%(inpattern,domnum,int(stepV),vkey)
        
    #print(fname)
    nxt = 0
    nyt= 0
    nzt = 0
    c_file = None
    
     
    sizeOfData = 0
  
    
    
  
    sizeofOfI4 = struct.calcsize("i")
    sizeofOfF8 = struct.calcsize("d")
    c_file = open(fname,"rb")
    
    nxt = struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nyt= struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nzt = struct.unpack("i",c_file.read(sizeofOfI4))[0] 
    c_file.read(sizeofOfI4)
    tstep = struct.unpack("d",c_file.read(sizeofOfF8))[0]
    
    #3 ints plus padding plus tstep
    sizeOfHeader =  sizeofOfI4*6 + sizeofOfF8
    sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)
    sizeOfRecord = sizeOfHeader + sizeOfData
   

    
    if nzt> 1000: 
        nzt=66
        print("no good nz for ", fname)
    
    recordNum = appendedDict[stepV];
    
    if(zkey != None) :
        recordNum = 0;
 #   print nxt,nyt,nzt, sizeOfData, " record ", recordNum  
    
    c_file.seek(recordNum*sizeOfRecord+sizeOfHeader)

    nx =  nxt
    ny = nyt
    nz =  nzt 

    D = np.zeros((nx, ny , nz ), dtype=np.float32)
    
    items = struct.unpack("%dd" % (nxt*nyt*nzt) , c_file.read(sizeOfData))
 
    for j in range(0,nyt-1):
        for i in range(0,nxt-1):
            for k in range(0,nzt-1): 
                indice =  (k*(nyt-decay)*(nxt))+(j*(nxt+decay))+i
                D[i,j,k]=items[indice]
    
    #print  fname," max ", np.max(D) ," minimum ", np.min(D), "  val"
    c_file.close()
    return D    

def readBinS(inpattern,domnum,stepV,vkey, appendedDict, zkey=None, bestshape=None):
    fname =  "%s.%d.%s"%(inpattern,domnum,vkey)
    if (appendedDict == None):
        fname =  "%s.%d.%d.%s"%(inpattern,domnum,int(stepV),vkey)
    nxt = 0
    nyt= 0
    nzt = 0
    c_file = None
    sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)
    if (appendedDict == None):
        fsize = os.path.getsize(fname)
        c_file = open(fname,"rb")
        formatBin = "%iQ" % 2
        items = struct.unpack(formatBin,c_file.read(struct.calcsize(formatBin)))
        nxt = items[0]
        nyt= items[1]
        if (nyt > 1000) :
            if(bestshape == None):
                nyt = nxt
            else:
                nxt = bestshape[0]
                nyt= bestshape[1]
                
        nzt = ((fsize-8-8)/(nxt*nyt))/8
        sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)
    else:
        sizeofOfI4 = struct.calcsize("i")
        sizeofOfF8 = struct.calcsize("d")
        c_file = open(fname,"rb")
        
        nxt = struct.unpack("i",c_file.read(sizeofOfI4))[0]
        c_file.read(sizeofOfI4)
        nyt= struct.unpack("i",c_file.read(sizeofOfI4))[0]
        c_file.read(sizeofOfI4)
        nzt = struct.unpack("i",c_file.read(sizeofOfI4))[0] 
        c_file.read(sizeofOfI4)
        tstep = struct.unpack("d",c_file.read(sizeofOfF8))[0]
        
        #3 ints plus padding plus tstep
        sizeOfHeader =  sizeofOfI4*6 + sizeofOfF8
        sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)
        sizeOfRecord = sizeOfHeader + sizeOfData
        
        if nzt> 1000: 
            nzt=66
            print("no good nz for ", fname)
        
        recordNum = appendedDict[stepV];
        
        if(zkey != None) :
            recordNum = 0;
     #   print nxt,nyt,nzt, sizeOfData, " record ", recordNum  
        c_file.seek(recordNum*sizeOfRecord+sizeOfHeader)

    nx =  nxt-2
    ny = nyt-2
    nz =  nzt-2 

    #D = np.zeros((nx + 1, ny + 1, nz + 1), dtype=np.float32)
    #items = struct.unpack("%dd" % (nxt*nyt*nzt) , c_file.read(sizeOfData))
    #for j in range(1,nyt):
    #    for i in range(1,nxt):
    #        for k in range(1,nzt): 
    #            indice =  (k*nyt*nxt)+(j*nxt)+i
    #            D[i-1,j-1,k-1]=items[indice]
    #c_file.close()
    #return D

    rawDoublesInFloats = np.frombuffer(c_file.read(sizeOfData), dtype=np.float64).astype(np.float32)
    c_file.close()
    data_reshaped_and_ordered = rawDoublesInFloats.reshape((nzt, nyt, nxt)).transpose(2, 1, 0)
    return data_reshaped_and_ordered[1:nxt, 1:nyt, 1:nzt]



def read2DBinS(fname, offset = 0, fatherDom = (0,0,0)):
    fsize = os.path.getsize(fname)
    c_file = open(fname,"rb")
    format = "%iQ" % 2
    items = struct.unpack(format,c_file.read(struct.calcsize(format)))
    nxt = items[0]
    nyt= items[1]
    xsub = 0
    if fatherDom[0]>0 :
        xsub= nxt/fatherDom[0]
    ysub = 0
    if fatherDom[1]>0 :
        ysub= nyt/fatherDom[1]
    
    nx =  nxt-2*xsub
    ny =  nyt-2*ysub

    D = np.ones((nx , ny ,1), dtype=np.float32)
    D =D*5
    dlenGrid = (nxt*nyt)
    format = "%dd" % dlenGrid  

    items = struct.unpack(format, c_file.read(struct.calcsize(format)))
                          
    for j in range(ysub,nyt-ysub):
        for i in range(xsub,nxt-xsub):
            indice =  (i*nyt)+j
             
            D[i-xsub,j-ysub,0]=items[indice]
    
    return np.where(D == np.Infinity, 0., D)
    c_file.close()
 
def readBinShape(fname):

    nxt = 0
    nyt= 0
    nzt = 0
    c_file = None

    sizeOfData = struct.calcsize("d") * (nxt*nyt*nzt)

    sizeofOfI4 = struct.calcsize("i")
    sizeofOfF8 = struct.calcsize("d")
    c_file = open(fname,"rb")
 
    nxt = struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nyt= struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)
    nzt = struct.unpack("i",c_file.read(sizeofOfI4))[0]
    c_file.read(sizeofOfI4)

    nx =  nxt
    ny = nyt
    nz =  nzt

    c_file.close()

    return nx,ny,nz
   
def _addDataToFile(vtkFile, cellData, pointData):
    # Point data
    if pointData is not None:
        keys = list(pointData.keys())

        vtkFile.openData("Point", scalars = keys[0])
        for key in keys:
            data = pointData[key]
            vtkFile.addData(key, data)
        vtkFile.closeData("Point")

    # Cell data
    if cellData is not None:
        keys = list(cellData.keys())
        vtkFile.openData("Cell", scalars = keys[0])
        for key in keys:
            data = cellData[key]
            vtkFile.addData(key, data)
        vtkFile.closeData("Cell")

def _appendDataToFile(vtkFile, cellData, pointData):
    # Append data to binary section
    if pointData is not None:
        keys = list(pointData.keys())
        for key in keys:
            data = pointData[key]
            vtkFile.appendData(data)

    if cellData is not None:
        keys = list(cellData.keys())
        for key in keys:
            data = cellData[key]
            vtkFile.appendData(data)

def iround(x):
    return int(round(x) - .5) + (x > 0)
 
 
 
def getLocationAndCoeffsInGridPoints(probeLoc, domlocation,nx,ny,nz,largCell, zzd):
   
    zcoeff=0
    xcoeff=0
    ycoeff=0
    probeLocation=(probeLoc[0]-domlocation[0],probeLoc[1]-domlocation[1],probeLoc[2])
    ploc = ((probeLocation[0]/largCell),(probeLocation[1]/largCell))

    
    if ploc[0] > 0 and ploc[0] < nx and ploc[1] > 0 and ploc[1] < ny:
        zprobeloc = 0
        for zind in range(nz):
  
            if(zzd[ploc[0],ploc[1],zind]>(probeLocation[2])):
                zprobeloc = zind                           
                break
        xcoeff = ((((np.ceil(ploc[0]+0.5))*largCell)-largCell/2)-probeLocation[0])/largCell
        ycoeff = ((((np.ceil(ploc[1]+0.5))*largCell)-largCell/2)-probeLocation[1])/largCell
        zcoeff = (zzd[ploc[0],ploc[1],zprobeloc]-(probeLocation[2]))/(zzd[ploc[0],ploc[1],zprobeloc]-zzd[ploc[0],ploc[1],zprobeloc-1])

        return (np.ceil(ploc[0]+0.5),np.ceil(ploc[1]+0.5),zprobeloc,xcoeff,ycoeff,zcoeff)
    return ()


def getProbeValue(varArray,ploc):
    if(len(ploc) != 6):
        return 0
     
    if ploc[0] < 1 or ploc[1] <1 or ploc[2] <1 :
        return 0
    if ploc[0] >= np.shape(varArray)[0] or ploc[1] >= np.shape(varArray)[1] or ploc[2] >= np.shape(varArray)[2] :
        return 0

    val = varArray[ploc[0],ploc[1],ploc[2]]*ploc[3]*ploc[4]*(1-ploc[5])
    val += varArray[ploc[0],ploc[1],ploc[2]-1]*ploc[3]*ploc[4]*(ploc[5])
    val += varArray[ploc[0],ploc[1]-1,ploc[2]]*ploc[3]*(1-ploc[4])*(1-ploc[5])
    val += varArray[ploc[0],ploc[1]-1,ploc[2]-1]*ploc[3]*(1-ploc[4])*(ploc[5])
    val += varArray[ploc[0]-1,ploc[1],ploc[2]]*(1-ploc[3])*ploc[4]*(1-ploc[5])
    val += varArray[ploc[0]-1,ploc[1],ploc[2]-1]*(1-ploc[3])*ploc[4]*(ploc[5])
    val += varArray[ploc[0]-1,ploc[1]-1,ploc[2]]*(1-ploc[3])*(1-ploc[4])*(1-ploc[5])
    val += varArray[ploc[0]-1,ploc[1]-1,ploc[2]-1]*(1-ploc[3])*(1-ploc[4])*(ploc[5])
    return val


def genATMapImage(fname, pdgFile, outFile):
    from scipy import interpolate
    with nc4.Dataset(fname, 'r') as nc_file:
        at = nc_file.variables['arrival_time_of_front'][:]
        shpAT = at.shape
        nx, ny, nz = shpAT[1], shpAT[0], 1 

    with nc4.Dataset(pdgFile, 'r') as nc_file:
        zgrid = np.transpose(nc_file.variables['ZS'][:, :])
        
        zgridres = (nc_file.variables['XHAT'][-1]-nc_file.variables['XHAT'][0])/(len(nc_file.variables['XHAT'][:])-1)
  
        ox = nc_file.variables['XHAT'][0]
        oy = nc_file.variables['YHAT'][0]
        
        ex = nc_file.variables['XHAT'][-1]+zgridres
        ey = nc_file.variables['YHAT'][-1]+zgridres
        
    
    res = (ex - ox) / (nx)
    
    shpZ = zgrid.shape
    znx, zny = shpZ[0], shpZ[1] 
    
    kernelIn = zgrid
    inKSize = len(kernelIn)
    
    kernelOut = np.zeros((znx,zny),np.float64)
    
    projx = np.array(list(range(znx)))
    projy = np.array(list(range(zny)))
    
    projz = kernelIn
    xx = np.linspace(projx.min(),projx.max(),nx+1)
    yy = np.linspace(projy.min(),projy.max(),ny+1)
    
    newKernel = interpolate.RectBivariateSpline(projx,projy,projz, kx=2,ky=2)
    
    kernelOut = newKernel(xx,yy)
    
 
    
    X = np.arange(ox, ex+res, res, dtype='float64') 
    Y = np.arange(oy, ey+res, res, dtype='float64') 
    
    
    x = np.zeros(( nx+1 ,ny +1, nz )) 
    y = np.zeros(( nx +1,ny +1, nz )) 
    z = np.zeros(( nx +1,ny +1, nz )) 
    
    for k in range(nz ): 
        for j in range(ny+1 ):
            for i in range(nx+1): 
                x[i,j,k] = X[i]
                y[i,j,k] = Y[j]
                z[i,j,k] = kernelOut[i,j]
    atime = np.zeros((nx , ny , nz )) 
    atime[:,:,0] = np.transpose(at)
    gridToVTK(outFile, x, y, z, cellData = {"atime" : atime}, pointData = None)
    
 


def ffFrontsToVtk(inFFpattern = "", outPath = ""):

    timedContourFiles = sorted(glob.glob(inFFpattern+"*"))
    
    gp = VtkGroup(f"{outPath}/fronts")
   
    for ffrontFileName in timedContourFiles:
        fronts=[]
        ntotPoints = 0

       
        if os.path.isfile(ffrontFileName) :
            fDomFile = open(ffrontFileName, 'r')
            fDomFile.readline()
            fDomFile.readline()
            domainLines = fDomFile.read().split("\n")
            cfront = []
            
            for dline in domainLines:
                if "FireFront" in dline:
                    if (len(cfront)> 5):
                        fronts.append(cfront)
                    cfront = []
                if "FireNode" in dline:
                    loc = dline.split("loc=(")[1].split(")")[0].split(',');
                    vel = dline.split("vel=(")[1].split(")")[0].split(',');    
                    velMod = np.sqrt((float(vel[0])*float(vel[0]))+(float(vel[1])*float(vel[1]))+(float(vel[2])*float(vel[2])))
                    ntotPoints = ntotPoints +1
                    if np.abs(velMod) > 0.00:    
                        cfront.append(((float(loc[0]),float(loc[1]),float(loc[2])),(float(vel[0]),float(vel[1]),float(vel[2]))))
                  
            if (len(cfront)> 5):
                 
                fronts.append(cfront)       
   
            if (len(fronts)> 0):
                stepV = int(ffrontFileName.split(".")[-1])
                 
                xp = np.zeros((ntotPoints))
                yp=np.zeros((ntotPoints))
                zp=np.zeros((ntotPoints))
                pvx=np.zeros((ntotPoints))
                pvy=np.zeros((ntotPoints))
                pvz=np.zeros((ntotPoints))
                datap=np.zeros((ntotPoints))
                ptcount = 0
           
                for front in fronts:
                    npoints = 0
                    npoints = len(front)*2-1
                 
                    ptplus = 1
                    ptleft = None
                    ptright = None
                    
                    for point,vel in front:
                        ptleft = ptright
                        ptright = point
                        velMod = np.sqrt((vel[0]*vel[0])+(vel[1]*vel[1])+(vel[2]*vel[2]))
                        if(ptcount < ntotPoints):
                            datap[ptcount] = velMod
                            pvx[ptcount] = vel[0]
                            pvy[ptcount] = vel[1]
                            pvz[ptcount] = vel[2]
                            xp[ptcount]= point[0]
                            yp[ptcount]=point[1]
                            zp[ptcount]=point[2]
                         
                        ptcount += 1
                         
                fpoutGname = f"{outPath}/front.{stepV}"
                print("saving ", fpoutGname)
                pointsToVTK(fpoutGname, xp, yp, zp, {"vmod":datap,"velocity":(pvx,pvy,pvz)})
                gp.addFile(filepath = "%s.vtu"%fpoutGname, sim_time = stepV)
                    
 
    gp.save()
 

def ffmnhFileToVtk(inpattern="", pgdFile="", outPath="", cleanFile=False, lidarIn=None, lidarOut=None,
                   startStep=-1, endStep=-1, genDomainOrigin=None, genDomainExtent=None, norecompute=False,
                   quitAfterCompute=False, xcfdName=None, inputcdfvars=(), vect_vars=("U", "V", "W")):
    
    # Obtenir le préfixe du fichier
    fprefix = inpattern.split("/")[-1]

    # Clés de grille et bmap
    gridKey = "ZGRID"
    bmapKey = "bmap"

    # Variables scalaires et vectorielles
    scals = {}
    scals["cells"] = ()
    scals["points"] = []

    # Liste des fichiers dans le répertoire de sortie correspondant au motif output.*
    file_pattern =  f"{inpattern}.1.*"
    files = glob.glob(file_pattern)

    # Variables scalaires à exclure (vect_vars et gridKey)
    exclude_vars = set(vect_vars + (gridKey,))
    surface_vars = []
    # Parcourir les fichiers et déterminer les variables scalaires
    for file in files:
        # Extraire le nom de la variable du fichier
        variable = file.split(".")[-1]
        if variable not in exclude_vars:
            if variable.startswith("surf"):
                surface_vars.append(variable)
            else:
                scals["points"].append(variable)

    # Convertir en tuple (facultatif selon les besoins)
    scals["points"] = tuple(scals["points"])

    Vects={}
    Vects["Wind"] =vect_vars
    if quitAfterCompute:
        navail = numberOfFields(f"{inpattern}.1.{vect_vars[0]}")
        print(navail)
        return navail
     
    appendedSteps=None;
    
    domains=list(range(1,2)) 
    lidarList={}
    smokeLocList=[]
    probeFile = None 
    
    if lidarIn != None:
        for filename in os.listdir("%s"%(lidarIn)):
            if filename.endswith("txt"):
                shot = LaserShot("%s%s"%(lidarIn,filename),origin=(8150,2180,1440,8030-300))
                lidarList[shot.time] = (filename,shot)
        probeFile = open("%s/windInSmoke.csv"%lidarOut,'w')
        probeFile.write("time;u;v;w;average of speeds;average of modules\n") 
    
    
    tsteps = []
    stepzgrid = ""
    varsDataIn = Vects["Wind"] + scals["points"] + scals["cells"]

    
    appendedSteps=readAllSteps("%s.1.%s"%(inpattern,varsDataIn[0]))
    tsteps = list(appendedSteps.keys())
    stepzgrid = tsteps[0]
    
    if quitAfterCompute:
        print(len(appendedSteps))
        return len(appendedSteps)
    
    print(len(appendedSteps), "step found")
    
    Allsteps = np.sort(tsteps)
    
    
    steps = []
    
    
    gf = VtkGroup("%s/%s_all_fields"%(outPath,fprefix))
    for stepV in Allsteps[:]:
        outname = "%s/%s.full.%d.vts"%(outPath,fprefix,stepV) 
        if (cleanFile and os.path.isfile(outname)  ):
            print("Step %d already post-processed, cleaning up"%stepV)
            delCmd =  "rm %s.*.%d"%(inFFpattern,stepV)
            print(delCmd)
            #os.system(delCmd)
            for varName in varsDataIn:
                delCmd=  "rm %s.*.%d.%s"%(inpattern,stepV,varName)
                print(delCmd)
                #os.system(delCmd)
            
        if ((norecompute or cleanFile) and os.path.isfile(outname)):
            print(outname,"at %d, "%stepV, end = '') 
            gf.addFile(filepath = "%s"%outname, sim_time = stepV)
            
        else:
            if not os.path.isfile(outname):
                steps.append(stepV)
            else:
                print("Step %d already post-processed"%stepV)
                gf.addFile(filepath = "%s"%outname, sim_time = stepV)
        
    gf.save()
    
    if (norecompute or cleanFile) :
        print("Quit because not recomputing", norecompute, cleanFile)
        return
    
    if len(steps) < 1:
        print("nothing more to be done")
        return
        
    selectedSteps = steps[:]
    
    MNHSteptoLidarStep={}
    
    if len(list(lidarList.keys()))>0:
        sortedList = np.sort(list(lidarList.keys()))
        for val in list(lidarList.keys()):
            for stepV in steps[:]:
                if val <= stepV:
                    MNHSteptoLidarStep[stepV] = val
                    break;
        selectedSteps=np.sort(list(MNHSteptoLidarStep.keys()))
    
    
    domainID = 1
   
    numOfDomains = len(list(glob.glob(f"{inpattern}.*.{gridKey}")))
    print(f"Found {numOfDomains} domains")
    domains = np.zeros(shape=(4,numOfDomains),dtype=float)
    pgdInfo = pgdFile

    print(f"handling PGD info {pgdInfo} for domains")
    xhat=None
    yhat=None
    
    if pgdInfo.endswith(".nc"):
        with nc4.Dataset(pgdInfo, 'r') as nc_file:
            xhat = list(nc_file.variables['XHAT'][:])
            yhat = list(nc_file.variables['YHAT'][:])
        
            dx = xhat[1]-xhat[0]
            dy = xhat[1]-xhat[0]
            xhat.extend([xhat[-1]+dx, xhat[-1]+2*dx, xhat[-1]+3*dx])
            yhat.extend([yhat[-1]+dy, yhat[-1]+2*dy, yhat[-1]+3*dy])

    else:
        parts = pgdInfo.split(',')
        if len(parts) != 5:
            raise ValueError("Expected pgdInfo with 5 comma-separated values: 'res,ni,nj,x0,y0'")

        res = int(parts[0].strip())
        ni = int(parts[1].strip())
        nj = int(parts[2].strip())
        x0 = float(parts[3].strip())
        y0 = float(parts[4].strip())

        x_end = x0 + res * (ni - 1)
        y_end = y0 + res * (nj - 1)

        xhat = np.linspace(x0, x_end, ni)
        yhat = np.linspace(y0, y_end, nj)

    

    print(np.shape(xhat),xhat[0],xhat[-1])
    print(np.shape(yhat),yhat[0],yhat[-1])

    njt = len(xhat)

    xhati=0
    yhati=0
    for i in range(1,numOfDomains+1):
        nii,nji,z= readBinShape(f"{inpattern}.{i}.{gridKey}")  
        domains[0][i-1] = float(xhat[xhati])
        domains[1][i-1] = float(yhat[yhati])
        domains[2][i-1] = float(xhat[xhati+nii])
        domains[3][i-1] = float(yhat[yhati+nji])
        xhati = xhati+nii-2
        if (xhati > (njt-nii)):
            yhati = yhati+nji-2
            xhati=0

    
    
    if startStep > -1 and endStep > -1:
        selectedSteps=selectedSteps[startStep:endStep]
    
    print(scals["points"], surface_vars, " on ",len(selectedSteps), " time steps to be computed on ", len(Allsteps) , " list :", selectedSteps, " ")
    
    largCell = 0;
    
    if numOfDomains > 1:
        largCell = (domains[2][0] - domains[0][1])/2
        print(" \n\nMultiproc  CELL SIZE SET TO  ", largCell, " \n\n")
    if numOfDomains == 1:
        largCell = 200
        print(" \n\nWARNING  CELL SIZE SET TO  ", largCell, " \n\n")
    
    dx = dy = largCell
            
    largDom = (max(domains[2]))  - (min(domains[0]))   - largCell*2
    lonDom = (max(domains[3]))  - (min(domains[1]))  - largCell*2
    
    tx = int(largDom/largCell)
    ty =  int(lonDom/largCell)
    print("Domain ",tx, ty, " resolution ", dx,dy, " in  domains (%d)"%numOfDomains)
    
    domainID = 1               
    
    paralPiecesVtkStr=""
    tz = 0
    curStep = 0
    
    gf = VtkGroup("%s/%sgroupsfields"%(outPath,fprefix))
    
    
    
    xd = {}
    yd = {}
    zd = {}
    
    startd_D = {}
    endd_D = {}
    
    startd_F = {}
    endd_F = {}
    
    fd = {}
    xf = {}
    yf = {}
    zf = {}
    paralPiecesVtkStr=""
    
    nxf = 0
    nyf = 0
    
    probesLocation=[]
    
    zzd = None 
    domainshapesInPoint ={}
    dLocalShape = {}
    
     
    for inddom in range(0,numOfDomains):
        localZ = readBinS(inpattern,inddom+1,stepzgrid,gridKey,appendedSteps,zkey=gridKey)
        shZ = np.shape(localZ)
        nx =  shZ[0]-1
        ny = shZ[1]-1
        nz =  shZ[2]-1 
        domainshapesInPoint[inddom] = (nx,ny,nz)
        tz = nz
        if (zzd is None):
            zzd = np.zeros((tx+1, ty+1, tz+1), dtype=np.float32)
        datatop =int((domains[3][inddom] - domains[1][0])/dy -1)
        databottom = int((domains[1][inddom] - domains[1][0])/dy)
        dataleft =int((domains[0][inddom] - domains[0][0])/dx)
        dataright =int( (domains[2][inddom] - domains[0][0])/dx -1 )
       
        isdatatop = datatop == (ty+1)
        isdatabottom = databottom == 0
        isdataleft = dataleft == 0
        isdataright = dataright == (tx+1)
        
        dLocalShape[inddom] =((databottom if isdatabottom else databottom) ,(datatop if isdatatop else datatop-4), (dataleft if isdataleft else dataleft) ,(dataright if isdataright else dataright-4))
        
        for k in range(nz+1):
            for j in range(dLocalShape[inddom][0],dLocalShape[inddom][0]+shZ[1]):
        	    for i in range(dLocalShape[inddom][2],dLocalShape[inddom][2]+shZ[0]):
        		      zzd[i,j,k] = localZ[i-dLocalShape[inddom][2],j-dLocalShape[inddom][0],k] # if (isdatabottom or isdataleft) else 0
          
    
    xxd = np.zeros((tx+1, ty+1, tz+1), dtype=np.float32)
    yyd = np.zeros((tx+1, ty+1, tz+1), dtype=np.float32)
    X = np.arange(domains[0][0]+1.5*dx,domains[0][0]+(tx+1)*dx+1.5*dx, dx, dtype=np.float32) 
    Y = np.arange(domains[1][0]+1.5*dy,domains[1][0]+(ty+1)*dy+1.5*dy, dy, dtype=np.float32)
    
    for i in range(tx+1):
        for j in range(ty+1):
            for k in range(tz+1):
                xxd[i,j,k] = X[i] 
                yyd[i,j,k] = Y[j] 
    
    
    paralPiecesVtkStr = ""
    # selection of smoke points
    if len(list(MNHSteptoLidarStep.keys()))>0 :
        for myshotfname, myshot in list(lidarList.values()):
            for iShot, shotLoc in enumerate(myshot.getShotLocationAsXYZ()):
                if myshot.getValues()[iShot] > 0.1:
                    smokeLocList.append(getLocationAndCoeffsInGridPoints(shotLoc,(xxd[0,0,0],yyd[0,0,0]),tx,ty,tz, largCell, zzd))
        print(len(smokeLocList), "sample smoke point selected")
    midUVW = True # put the flow point at mass point
   
    for stepV in selectedSteps:
        paralPiecesVtkStr = ""
 
        varmapAll = {}
        
        for keycount, vkey in enumerate(varsDataIn):
            varmapAll[vkey] = np.zeros((tx+1, ty+1, tz+1), dtype=np.float32)
    
     

        for indom in range(0,numOfDomains):
            if (indom%100 == 0) : 
                print("processing sub-domains %d to %d at step %d"%(indom,indom+100, stepV))

            nx,ny,nz =     domainshapesInPoint[indom]
            i0 = dLocalShape[indom][2]
            i1 = i0 + nx + 1   # +1 because you store up to index nx
            j0 = dLocalShape[indom][0]
            j1 = j0 + ny + 1
            
            if (midUVW):
                for vkey in varsDataIn:
                    feed_data = readBinS(inpattern, indom+1, stepV, vkey, appendedSteps)
                    if vkey == "U":
                        u_mass = 0.5 * (feed_data[:-1, :, :] + feed_data[1:, :, :])
                        varmapAll[vkey][i0:i1-1, j0:j1, :nz+1] = u_mass

                    elif vkey == "V":
                        v_mass = 0.5 * (feed_data[:, :-1, :] + feed_data[:, 1:, :])
                        varmapAll[vkey][i0:i1, j0:j1-1, :nz+1] = v_mass

                    elif vkey == "W":
                        w_mass = 0.5 * (feed_data[:, :, :-1] + feed_data[:, :, 1:])
                        varmapAll[vkey][i0:i1, j0:j1, :nz] = w_mass

                    else:
                        varmapAll[vkey][i0:i1, j0:j1, :nz+1] = feed_data
            else:
                for keycount, vkey in enumerate(varsDataIn):
                        fname =  "%s.%d.%d.%s"%(inpattern,indom+1,stepV,vkey)
                        varmapAll[vkey][dLocalShape[indom][2]:dLocalShape[indom][2]+nx+1,dLocalShape[indom][0]:dLocalShape[indom][0]+ny+1,:nz+1] = readBinS(inpattern,indom+1,stepV,vkey,appendedSteps) 
        
        outname = "%s/%s.full.%d"%(outPath,fprefix,stepV) 
     
        ptsAll = {}
        for key in scals["points"]:
            ptsAll[key] = varmapAll[key]
            
        for key in inputcdfvars:
            ptsAll[key] = varmapAll[key]
            
        vects = {}
        for key in list(Vects.keys()):
            ptsAll[key] =(varmapAll[Vects[key][0]],varmapAll[Vects[key][1]],varmapAll[Vects[key][2]])
        
    ####    LIDAR DIAGNOSTICS ##
        if len(list(MNHSteptoLidarStep.keys()))>0 :
            myshotfname, myshot = lidarList[MNHSteptoLidarStep[stepV]]
            print("Start lidarsim with ", stepV, " here ",  myshot.time)
    
            shotVals = np.zeros(myshot.getNumPoints())
            
            
            for iShot, shotLoc in enumerate(myshot.getShotLocationAsXYZ()):
                shotAndCoeffLoc = getLocationAndCoeffsInGridPoints(shotLoc,(xxd[0,0,0],yyd[0,0,0]),tx,ty,tz, largCell, zzd)
                shotVals[iShot] = getProbeValue(varmapAll["BRatio"],shotAndCoeffLoc)
            
            windHere = []
            
            for smokeLoc in smokeLocList:
                if getProbeValue(varmapAll["BRatio"],smokeLoc) > 0:
                    windHere.append((getProbeValue(varmapAll["U"],smokeLoc),getProbeValue(varmapAll["V"],smokeLoc),getProbeValue(varmapAll["W"],smokeLoc)))
                
            avgU = avgV = avgW = avgMM = avgM = 0
            for wu,wv,ww in windHere:
                avgU += wu
                avgV += wv
                avgW += ww
                avgMM += np.sqrt(wu*wu+wv*wv+ww*ww)
            if len(windHere) >0:
                avgU = avgU / len(windHere)
                avgV = avgV / len(windHere)
                avgW = avgW / len(windHere)
                avgMM = avgMM / len(windHere)
            avgM = np.sqrt(avgU*avgU+avgV*avgV+avgW*avgW)
            probeFile.write("%f;%f;%f;%f;%f;%f\n"%(stepV,avgU,avgV,avgW,avgMM,avgM))
            
            myshot.setValues(shotVals)
            myshot.saveAs("%s%s"%(lidarOut,myshotfname))
            print("end lidarsim") 
            
        else:
            gridToVTK(outname, xxd, yyd, zzd,  cellData = None, pointData = ptsAll)  
            
            if(xcfdName is not None):
                gridToCdf("%s.%d"%(xcfdName,stepV) , xxd, yyd, zzd, varmapAll)  
                
            gf.addFile(filepath = "%s.vts"%outname, sim_time = stepV)
    
    if len(list(MNHSteptoLidarStep.keys()))==0 :
        gf.save()


 
import argparse

def main():
    parser = argparse.ArgumentParser(
    description="Convert ffmnh files to VTK format.\n\n"
                "Examples:\n"
                "  For field conversion:\n"
                "    python pMNHFF2VTK.py -fields MODEL1/output PGD_D2000mA.nested.nc vtkout1 -steps 20 21\n"
                "  For front processing:\n"
                "    python pMNHFF2VTK.py -front ForeFire/Outputs/output.0. vtkFront/\n"
                "  For ATMap generation:\n"
                "    python pMNHFF2VTK.py -atmap ForeFire/Outputs/ForeFire.0.nc PGD_D80mA.nested.nc vtkmap",
    formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('-fields', nargs=3, help='Input : mnhdump file pattern. PGD nc file, VTK Output directory ')
    parser.add_argument('-clean', action='store_true', help='Enable this flag to clean the output files before processing.')
    parser.add_argument('-lidar', nargs=2, help='Lidar emulator input and output files. Provide two file paths, in and out csv.')
    parser.add_argument('-steps', nargs=2, type=int, help='Start and end generation steps for processing. Provide two integers, start and end')
    parser.add_argument('-norecompute', action='store_true', help='Enable this flag to skip recompute phase but make the index vtk file')
    parser.add_argument('-quit', action='store_true', help='Enable this flag to quit after computation without further processing but display number of steps in data')
    parser.add_argument('-cdf', help='Provide a prefix for CDF output (one file per step). Example: "CDFOUT/step"')
    parser.add_argument('-front', nargs=2, help='Input and output patterns for front processing. Provide two file paths, input ff front file pattern and VTK output directory.')
    parser.add_argument('-atmap', nargs=3, help='ATMap input file, pgd file, and output file. Provide three file paths, BMap nc file, PGD file and vtk file.')

    args = parser.parse_args()
    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)
        
    if args.fields:
        ffmnhFileToVtk(
            inpattern=args.fields[0],
            pgdFile=args.fields[1],
            outPath=args.fields[2],
            cleanFile=args.clean,
            lidarIn=args.lidar[0] if args.lidar else None,
            lidarOut=args.lidar[1] if args.lidar else None,
            startStep=args.steps[0] if args.steps else -1,
            endStep=args.steps[1] if args.steps else -1,
            norecompute=args.norecompute,
            quitAfterCompute=args.quit,
            xcfdName=args.cdf
        )
    
    if args.front:
        ffFrontsToVtk(inFFpattern=args.front[0], outPath=args.front[1])
    
    if args.atmap:
        genATMapImage(fname=args.atmap[0], pdgFile=args.atmap[1], outFile=args.atmap[2])

if __name__ == "__main__":
    main()
