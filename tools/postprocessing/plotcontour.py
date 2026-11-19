#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jan 11 15:53:46 2023

@author: filippi_j
"""
 
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import matplotlib.colors as mcolors
from mpl_toolkits.axes_grid1 import make_axes_locatable

import matplotlib.image as mpimg
import os.path
import struct
#plt.rcParams['figure.figsize'] = [10.0, 10.0]
#plt.rcParams['figure.dpi'] = 100

def readBMAP(fname, vals= None):
    if os.path.isfile(fname):
        fsize = os.path.getsize(fname)
        c_file = open(fname,"rb")
        formatMeta = "4Q"
        formatMetaRecord = "6Q"
        
        
        FDCnx,FDCny,gsx,gsy = struct.unpack(formatMeta,c_file.read(struct.calcsize(formatMeta)))
        
        step = int(gsx/FDCnx);
        
        formatData = "%dd" % int(step*step)
        
        fleft = fsize-struct.calcsize(formatMeta)
        
        
        if (vals is None):
           
            vals = np.zeros((gsx,gsy))
        else:
            vals = np.transpose(vals)
        
        ncells = int(fleft/(struct.calcsize(formatMetaRecord)+struct.calcsize(formatData)))
       # if (ncells > 0):
       # print(ncells, " read ",fname)
         
        for n in range(ncells):
            iNX,iNY,nx,ny,nz,nt = struct.unpack(formatMetaRecord,c_file.read(struct.calcsize(formatMetaRecord)))
            items = struct.unpack(formatData, c_file.read(struct.calcsize(formatData)))
            localV = np.reshape(np.array(items), (nx, ny))
            vals[iNX*step:(1+iNX)*step,iNY*step:(1+iNY)*step] = localV
           # print(n,atmoNX*step,(atmoNY+1)*step,nx,ny, np.shape(localV))
      
        
     
        vals[vals == np.inf] = 0
        
        return np.transpose(vals)
    print(fname, " not found")
    return None
def read2DBin(fname):
    if os.path.isfile(fname):
        fsize = os.path.getsize(fname)
        c_file = open(fname,"rb")
        format = "%iQ" % 4
        nx,ny,nz,nt = struct.unpack(format,c_file.read(struct.calcsize(format)))
        
        
        dlenGrid = (nx*ny*nz*nt)
        format = "%dd" % dlenGrid  
    
        items = struct.unpack(format, c_file.read(struct.calcsize(format)))
                            
        c_file.close()
        
        return np.transpose(np.reshape(np.array(items), (nx, ny)))
    return None
 
    
def calcAngle(x,y, px,py):
    """
    calculates the angle between 2 vectors in radians.
    @param x,y,px,py the direction of the 2 vectors in cartesinan space
    @return angle in radian
    """
 
    theta1 = np.arctan2(y,x);
    theta2 = np.arctan2(py,px);
    dtheta = theta2-theta1;
    while ( dtheta > np.pi ):
        dtheta -= 2.* np.pi;
    while ( dtheta < -np.pi ):
        dtheta += 2.* np.pi;
    return dtheta;

def isInsidePolyHard(poly, x, y):
    """
    calculates the angle between 2 vectors in radians.
    @param poly the list of points that de fines the polygon, left winded
    @param x, y the point corrdinates
    @return True id poly contains the point
    """
    angle = 0
    lpx = poly[0][0] -x
    lpy = poly[0][1] -y
    rpx = poly[1][0] -x
    rpy = poly[1][1] -y
    angle += calcAngle(lpx,lpy,rpx,rpy) 
  
    for ip in range(2,len(poly)-2):
        lpx = rpx
        lpy = rpy
        rpx = poly[ip][0] -x
        rpy = poly[ip][1] -y
        angle += calcAngle(lpx,lpy,rpx,rpy) 
        
    if (np.abs(angle) > np.pi) :
        return True
    
    return False

def isInsidePoly(poly, x, y):
    #try:
    #    import matplotlib.nxutils as nx    
    #    return nx.pnpoly(x,y,poly)
    #except ImportError:
        return isInsidePolyHard(poly, x, y)
    
    


def fillPoly(poly, matrix,value):
    """
    Fills a polygon inside a matrix 
    @param poly the list of points that defines the polygon, left winded
    @param matrix the matrix to be filled
    @param value the value to be used to fill the matrix
    """ 
  
    polyX = [c[0] for c in poly]
    polyY = [c[1] for c in poly]
    
    IMAGE_TOP = min(matrix.shape[0],int(max(polyY)))
    IMAGE_BOT = max(0,int(min(polyY)))
    IMAGE_RIGHT =  min(matrix.shape[1],int(max(polyX)))
    IMAGE_LEFT = max(0,int(min(polyX)))
  
    
    
    for i in range(IMAGE_LEFT,IMAGE_RIGHT):
        for j in range(IMAGE_BOT,IMAGE_TOP):
            if isInsidePoly(poly,i,j):
                matrix[j,i] = value
    return matrix  


def getDomainExtent(line):
  
    llv = line.split("sw=(")
    llr = llv[1].split("ne=(");
    return( float( llr[0].split(",")[0]), float(llr[1].split(",")[0]), float(llr[0].split(",")[1]), float(llr[1].split(",")[1]) )

def readNode(nodeline):
#            FireNode[domain=0;id=5930;fdepth=20;kappa=0.00172342;loc=(633147,4.81278e+06,16.8554);vel=(0.00480158,-0.0157864,-0.00105307);t=399931;state=moving;frontId=2]
    splitted =  nodeline.split("FireNode[")
    node = {}
    node["tabs"] = splitted[0]
    parames = (splitted[1][:-1]).split(";")
    for param in parames:
        keyval=param.split("=")
        node[keyval[0]] = keyval[1]
    return node

def makeNode(ns):
    fnodeString = "%sFireNode["%ns["tabs"]
    for p in ("domain","id","fdepth","kappa","loc","vel","t","state","frontId"):
 
        fnodeString = fnodeString + "%s=%s;"%(p,ns[p])
    return fnodeString[:-1]+"]"

def getLocationFromLine(line):

    llv = line.split("loc=(")
    if len(llv) < 2: 
        return None
    llr = llv[1].split(",");
    if len(llr) < 3: 
        return None
    return (float(llr[0]),float(llr[1]))

def dist(a,b):
    return np.sqrt(np.power(a[0]-b[0],2)+np.power(a[1]-b[1],2))

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



basePath="/Users/filippi_j/soft/MNH-V5-5-1/MY_RUN/KTEST/016_PEDROGAO/002_mesonh/"
basePath="//Users/filippi_j/soft/MNH-V5-6-0/MY_RUN/KTEST/016_PEDROGAO/002_mesonh/"
inistep =0
NumDomains =1
for nid in range(1000):
    fname = basePath+"ForeFire/Outputs/output.%d.%d"%(nid,inistep) 
    if os.path.isfile(fname):
        NumDomains = nid 
print(NumDomains, " domains found ")
    

RunDuration=300000
domains = range(0 ,NumDomains+1)
steps =  range(inistep,inistep+RunDuration,10) 
domColor = ['red','blue','black','pink','yellow','green','green','green','green','red','blue','black','pink',]
#list(mcolors.TABLEAU_COLORS.keys())
print(domColor)
fig, ax = plt.subplots() 

simTime = 10

def onclick(event):
    global ix, iy
    global simTime
    ix, iy = event.xdata, event.ydata
    print("startFire[loc=(%d.,%d.,0.);t=%d]"%(ix, iy,simTime))
    simTime = simTime +50
    coords = []
    coords.append((ix, iy))

    if len(coords) == 2:
        fig.canvas.mpl_disconnect(cid)


#cid = fig.canvas.mpl_connect('button_press_event', onclick)
pathes = []
domPatches= []
testarr = None
quivWind = {}

bmImage = {}

extentsImage = {}
XW = {}
YW = {}
res = 0;
sctest = 0.05
quivNorm = mcolors.Normalize(vmin=0, vmax=10)
arrBMAP = {}
for domainID in domains:
    
    fname = basePath+"ForeFire/Outputs/output.%d.%d"%(domainID,inistep)
    fnameWindV = basePath+"parallel/0/%d.windV"%(domainID)
    fnameWindU = basePath+"parallel/0/%d.windU"%(domainID)
    fnameBMAP =basePath+"parallel/0/%d.bmapcells"%(domainID)
    arrBMAP[domainID] = None
    bmImage[domainID] = None
    if os.path.isfile(fname):
        f= open(fname,'r')  
        square = getDomainExtent(f.readline())
        extentsImage[domainID]=square
        print (square)
        domPatches.append(mpatches.Rectangle((square[0], square[2]), square[1]-square[0], square[3]-square[2],edgecolor ='black',fill=False,lw=2))
        f.close()
 

        arrWindu = read2DBin(fnameWindU)
        arrWindv = read2DBin(fnameWindV)
        if(arrWindu is not None) and (arrWindv is not None) :
            res = (square[1]-square[0])/(np.shape(arrWindu)[1]-2)
            square2=(square[0]+(1*res)/2,square[1]-(1*res)/2,square[2]+(1*res)/2,square[3]-(1*res/2))
            
            #arr2 = arrWindu[1:-2,1:-2]
            
            xW = np.arange(square2[0], square2[1], res)
            yW = np.arange(square2[2], square2[3], res)
            XW[domainID], YW[domainID] = np.meshgrid(xW, yW)
            
            
            aru = ((arrWindu[2:-1,2:-1]))
            arv = ((arrWindv[2:-1,2:-1]))
            speedW = np.sqrt((aru*aru)+(arv*arv)).flatten()
            quivWind[domainID] = None#ax.quiver(XW[domainID], YW[domainID] , aru, arv, speedW,norm=quivNorm, cmap='jet',scale_units='dots',scale=sctest)
            
    
        
      
        arrBMAP[domainID] = None#readBMAP(fnameBMAP,arrBMAP[domainID])
        if(arrBMAP[domainID] is not None)  :
           bmImage[domainID] =  ax.imshow(arrBMAP[domainID],extent=square,origin='lower')
#ax.imshow(imgRR,extent=extentsImage[0])
#ax.quiverkey(quivWind[domains[0]], X=0.3, Y=1.1, U=10, label='Quiver key, length = 10', labelpos='E')
 


inited = False

for doms in domPatches:
    ax.add_patch(doms)

lastFoundOutputs = 0
oneshot = True     

while(oneshot):
   #ax.clear()
    pathes = []
    for fi, originstep in enumerate(steps):
        for domainID in domains:
            fname = basePath+"ForeFire/Outputs/output.%d.%d"%(domainID,originstep)
            if os.path.isfile(fname):
                lastFoundOutputs = fi
                
    for doms in domPatches:
        ax.add_patch(doms)      
      
    originstep = steps[lastFoundOutputs]   
  
    for domainID in domains:        
        fname = basePath+"ForeFire/Outputs/output.%d.%d"%(domainID,originstep)
        fnameBMAP =basePath+"parallel/0/%d.bmapcells"%(domainID)
        arrBMAP[domainID] = readBMAP(fnameBMAP,arrBMAP[domainID])

             
        if(arrBMAP[domainID] is not None):
            if(bmImage[domainID] is not None):
                bmImage[domainID].remove()
            bmImage[domainID] = None          
          
            bmImage[domainID] = ax.imshow(arrBMAP[domainID],extent=extentsImage[domainID],origin='lower')  
        if domainID == 0:
            if os.path.isfile(fname):
                f= open(fname,'r') 
                print(fname)
                newPathes= printToPathe(f.read())
                pathes += newPathes
                f.close()
            for path in pathes:
                patch = mpatches.PathPatch(path,edgecolor=domColor[domainID], facecolor='none', alpha=1)
                ax.add_patch(patch)
 
 
        fnameWindV = basePath+"parallel/0/%d.windV"%(domainID)
        fnameWindU = basePath+"parallel/0/%d.windU"%(domainID)
            
        arWu = read2DBin(fnameWindU)
        arWv = read2DBin(fnameWindV)
        if(arWu is not None) and (arWv is not None) :
            aru = arWu[1:-2,1:-2]
            arv = arWv[1:-2,1:-2]
            speedW = np.sqrt((aru*aru)+(arv*arv)).flatten()
            if(quivWind[domainID] is not None):
                quivWind[domainID].remove()
            quivWind[domainID] = None
            if (domainID==0):
                quivWind[domainID] = None#ax.quiver(XW[domainID], YW[domainID], aru, arv, speedW,norm=quivNorm, cmap='Spectral_r',scale_units='dots',scale=sctest)
            else:
                quivWind[domainID] = None# ax.quiver(XW[domainID], YW[domainID], aru, arv, speedW,norm=quivNorm, cmap='jet',scale_units='dots',scale=sctest)


    ax.set_title(f"step {originstep}")
    if not inited:    
        ax.grid()
        ax.axis('equal')
    inited = True
    
    
    plt.pause(1)
    #oneshot = False    

 

 



