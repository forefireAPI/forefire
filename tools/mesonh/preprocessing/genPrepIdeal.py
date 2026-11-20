#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Feb 24 14:23:51 2023

@author: filippi_j
"""

import matplotlib.pyplot as plt
from matplotlib import cm
from matplotlib.ticker import LinearLocator
import numpy as np
import json

def patchFile(pMap, fileNameIn, fileNameOut):
    s = ""
    with open(fileNameIn) as f:
        s = f.read()
    for k in pMap.keys():
        s = s.replace(k,str(pMap[k]))
    with open(fileNameOut, 'w') as f:
        f.write(s)
        
fig, ax = plt.subplots()
# Make data.
X = np.arange(-5, 1, 0.1)
Y = np.arange(-2, 2, 0.1)

X, Y = np.meshgrid(X, Y)
R = np.sqrt(X**2 + Y**2)
Z = np.sin(R)*400
resolution = 100
montab=""
for i in range(np.shape(Z)[0]):
    x_arrstr = np.char.mod('%.02f', Z[i,:])
    montab+=" ".join(x_arrstr)+"\n"


#print(montab)
ax.imshow(Z)
plt.plot()

patch={}
patch["$NIMAX$"] = np.shape(Z)[1]
patch["$NJMAX$"] = np.shape(Z)[0]
patch["$XDELTAX$"] = resolution
patch["$XDELTAY$"] = resolution
patch["$V$"] = 0
patch["$U$"] = 4
patch["$ZSDATA$"] = montab
patch["$INIFILE$"] = "RELIEF3D"
print (json.dumps(patch, indent=2))


fnameI = "/Users/filippi_j/soft/MNH-V5-5-1/MY_RUN/KTEST/016_PEDROGAO/001_prep_ideal_case/PRE_IDEA1.pattern"
fnameO = "/Users/filippi_j/soft/MNH-V5-5-1/MY_RUN/KTEST/016_PEDROGAO/001_prep_ideal_case/PRE_IDEA1.nam"

patchFile(patch, fnameI, fnameO)