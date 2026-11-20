import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import matplotlib.image as mpimg
from os import path
import json

def dist(a,b):
    return np.sqrt(np.power(a[0]-b[0],2)+np.power(a[1]-b[1],2))

spottingInfo = {}

spottingInfo["resolution"] = None        # number of meters par pixel
spottingInfo["reference"] = None         # textwith where data was found, ideally a DOI
spottingInfo["BottomLeftLatLon"] = None  # Coordinates in latlon of the bottomleft image corner
spottingInfo["TopRightLatLon"] = None    # Coordinates in latlon of the topright image corner
spottingInfo["date"] = None              # UTC datetime string on when the picture is taken
spottingInfo["spotPoints"] = []        # Coordinates in meters relatives to the bottomleft of the image of spotting points
spottingInfo["frontPoints"] = []       # Coordinates in meters relatives to the bottomleft of the image of various points of the burned area
spottingInfo["activeFrontPoints"] = [] # Coordinates in meters relatives to the bottomleft of the image of various points of active fire

#######  HERE mettre le nom de fichier du png
spottingInfo["filename"] ="australia1.png"
#######  HERE mettre le nom du fichier, si il n'existe pas il sera créé
datafilename  ="australia1.json"
#######  HERE mettre la longueur de l'echelle dans figure.. 2km.. 1km...
legendLength  = 5000.

RESOLUTION = 0
SPOT = 1
ACTIVEFIRE = 2
BURNED = 3
 
iMode = SPOT

mI = spottingInfo
if path.exists(datafilename):
    with open(datafilename) as json_file:
        mI = json.load(json_file)
    print("loaded json ",datafilename, mI)
else:
    with open(datafilename, 'w') as outfile:
        outfile.write(json.dumps(mI))   
  
if(mI["resolution"] is None):
   iMode = RESOLUTION
   
fig, ax = plt.subplots()
plt.subplots_adjust(bottom=0.2)


firstResPoint=(0,0)
lastResPoint=(0,0)



def paintSpots(ax, sI):
    ax.clear()
    print("imode is json ",iMode)
    imgRR = mpimg.imread(sI["filename"] )
    extentL= (0, np.shape(imgRR)[1],0, np.shape(imgRR)[0])


    if(iMode == RESOLUTION):
        ax.set_title('First, click on the scale to compute (from last 2 clicks, click as needed) \n scale length is here expected as %d m) \nthen save and relaunch'%legendLength)
    else:
        extentL= (0, np.shape(imgRR)[1]*sI["resolution"],0, np.shape(imgRR)[0]*sI["resolution"])
    
    ax.imshow(imgRR,extent=extentL)
    if(iMode == SPOT):
        ax.set_title('DoubleClick to add spot')
    
    if(iMode == ACTIVEFIRE):
        ax.set_title('DoubleClick to add active fire area')
    
    if(iMode == BURNED):
        ax.set_title('DoubleClick to add burnt contour area')
        
    for p in sI["spotPoints"]:
        ax.add_patch(plt.Circle((p[0],p[1]),4*sI["resolution"],color='blue')) 
        
    for p in sI["activeFrontPoints"]:
        ax.add_patch(plt.Circle((p[0],p[1]),4*sI["resolution"],color='red')) 
        
    for p in sI["frontPoints"]:
        ax.add_patch(plt.Circle((p[0],p[1]),4*sI["resolution"],color='black')) 
        
    plt.draw()
    
        
    

def onclick(event):
    global ax
    global firstResPoint
    global lastResPoint
    global mI
    global legendLength
    
    p = (event.xdata,event.ydata)
    if(iMode == RESOLUTION):
        firstResPoint = lastResPoint
        lastResPoint = p
        mI["resolution"] = float(legendLength)/float(dist(lastResPoint,firstResPoint))
        print("resolution set to %.2f pixel per meter"%mI["resolution"],lastResPoint,firstResPoint,dist(lastResPoint,firstResPoint),legendLength)

    if(iMode == SPOT):
        mI["spotPoints"].append(p)
        
    if(iMode == ACTIVEFIRE):
        mI["activeFrontPoints"].append(p)
        
    if(iMode == BURNED):         
        mI["frontPoints"].append(p)
        

    paintSpots(ax, mI) 



def spot( event):
    global iMode
    iMode =SPOT
def burn( event):
    global iMode
    iMode =BURNED
def fire( event):
    global iMode
    iMode = ACTIVEFIRE
        
def save( event):
    print("saving", datafilename , mI)
    with open(datafilename, 'w') as outfile:
        outfile.write(json.dumps(mI))   


cid = fig.canvas.mpl_connect('button_press_event',onclick)

axspots = plt.axes([0.5, 0.05, 0.1, 0.075])
axline = plt.axes([0.6, 0.05, 0.1, 0.075])
axfire = plt.axes([0.7, 0.05, 0.1, 0.075])
axsave = plt.axes([0.85, 0.05, 0.1, 0.075])

b1 = Button(axspots, 'Spots')
b1.on_clicked(spot)
b2 = Button(axline, 'Burned')
b2.on_clicked(burn)
b3 = Button(axfire, 'Fire')
b3.on_clicked(fire)
b4 = Button(axsave, 'Save')
b4.on_clicked(save)

paintSpots(ax, mI)  



plt.show()