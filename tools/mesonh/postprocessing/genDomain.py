#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Jun 14 11:29:56 2023

@author: filippi_j
"""

'''
Created on 4 avr. 2011

@author: filippi
'''

import os
import numpy as np
import struct
import csv
import shutil
import subprocess
import sys

def readBinShape(inpattern,domnum,vkey):

    fname =  "%s.%d.%s"%(inpattern,domnum,vkey)

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

#xhatS = subprocess.check_output("ncdump -vXHAT %s"%(ncFile))
def ncDumpVarToInt(ncFile,var):
    varS = "%s"%(subprocess.check_output('ncdump -v%s %s'%(var,ncFile),stderr=subprocess.STDOUT,shell=True))
    varS = varS.split("%s ="%(var))[-1].replace("\\n ", "")
    return list(map(int, varS.split(";")[0].split(",")))


ncFile="JUN17.2.EXP03.000.nc"
outputsFiles="ForeFire/Outputs450pMODEL2/output.%d.50400"

zgridDir = "MODEL2"
zkey="ZGRID"

if len(sys.argv) < 3:
    print("usage genDomain ncPGD ffMODELout outputDir")
else :
    ncFile= sys.argv[1]
    outputsFiles=sys.argv[3]
    zgridDir = sys.argv[2]


doms = []

xhat = ncDumpVarToInt(ncFile,"XHAT")
yhat = ncDumpVarToInt(ncFile,"YHAT")
dx = xhat[1]-xhat[0]
dy = xhat[1]-xhat[0]
xhat.extend([xhat[-1]+dx, xhat[-1]+2*dx, xhat[-1]+3*dx])
yhat.extend([yhat[-1]+dy, yhat[-1]+2*dy, yhat[-1]+3*dy])

for filename in os.listdir("%s"%(zgridDir)):
    fvars = filename.split(".")
    if fvars[2]== zkey:
        doms.append(int(fvars[1]))
nit = len(yhat)
njt = len(xhat)

assert(len(doms) == np.sort(doms)[-1])
swx = 0
swy = 0
nex= 0;
ney= 0;
xhati=0
yhati=0
for i in range(len(doms)+1)[1:]:
    nii,nji,z= readBinShape("%s/output"%(zgridDir),i,zkey)
    str0 = "FireDomain[sw=(%s,%s,0);ne=(%s,%s,0);t=50400]\n"%(xhat[xhati],yhat[yhati],xhat[xhati+nii],yhat[yhati+nji])
    fout = open(outputsFiles%(i), "w")
    fout.write(str0)
    fout.close()
    print(outputsFiles%(i), str0)
    xhati = xhati+nii-2
    if (xhati > (njt-nii)):
        yhati = yhati+nji-2
        xhati=0
