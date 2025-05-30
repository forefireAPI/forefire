#!//gpfs/home/UDCPP/filippi_j/soft/bin/python2.7
# -*- coding: utf-8 -*-
"""
Created on 4 avr. 2011

@author: filippi
"""

import os
import numpy as np
import struct
import glob
import json
# Make sure vtk is available in your environment:
import vtk
import xml.etree.ElementTree as ET

def numberOfFields(fname):
    """Return the number of time records in a concatenated binary file."""
    fsize = os.path.getsize(fname)
    with open(fname, "rb") as c_file:
        sizeofOfI4 = struct.calcsize("i")
        sizeofOfF8 = struct.calcsize("d")
        
        nxt = struct.unpack("i", c_file.read(sizeofOfI4))[0]
        c_file.seek(sizeofOfI4, 1)  # skip padding
        nyt = struct.unpack("i", c_file.read(sizeofOfI4))[0]
        c_file.seek(sizeofOfI4, 1)
        nzt = struct.unpack("i", c_file.read(sizeofOfI4))[0]
        c_file.seek(sizeofOfI4, 1)
        _tstep = struct.unpack("d", c_file.read(sizeofOfF8))[0]

        # 3 ints + padding + tstep
        sizeOfHeader = sizeofOfI4 * 3 + sizeofOfI4 * 3 + sizeofOfF8
        sizeOfData = struct.calcsize("d") * (nxt * nyt * nzt)
        sizeOfRecord = sizeOfHeader + sizeOfData

        numrec = fsize // sizeOfRecord

    return int(numrec)
def readAllSteps(fname, offset=0, bestshape=None):
    """Scan a concatenated file to build a dictionary of {tstep: record_index}."""
    fsize = os.path.getsize(fname)
    c_file = open(fname, "rb")

    sizeofOfI4 = struct.calcsize("i")
    sizeofOfF8 = struct.calcsize("d")
    
    nxt = struct.unpack("i", c_file.read(sizeofOfI4))[0]
    c_file.seek(sizeofOfI4, 1)
    nyt = struct.unpack("i", c_file.read(sizeofOfI4))[0]
    c_file.seek(sizeofOfI4, 1)
    nzt = struct.unpack("i", c_file.read(sizeofOfI4))[0]
    c_file.seek(sizeofOfI4, 1)
    _tstep = struct.unpack("d", c_file.read(sizeofOfF8))[0]
    
    sizeOfHeader = sizeofOfI4 * 3 + sizeofOfI4 * 3 + sizeofOfF8
    sizeOfData = struct.calcsize("d") * (nxt * nyt * nzt)
    sizeOfRecord = sizeOfHeader + sizeOfData
    numrec = fsize // sizeOfRecord
    
    tsteps = {}
    for i in range(int(numrec)):
        c_file.seek(i*sizeOfRecord + sizeOfHeader - sizeofOfF8)
        tstep = struct.unpack("d", c_file.read(struct.calcsize("d")))
        tsteps[tstep[0]] = i

    c_file.close()
    return tsteps
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
def readBinShape(fname):

    nxt = 0
    nyt= 0
    nzt = 0
    c_file = None
    sizeofOfI4 = struct.calcsize("i")
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

def genVoxelRemap(
    xcoord, ycoord, zcoord,   # 3D arrays of shape (nx, ny, nz)
    targetNz
):
    """
    Precompute a uniform 1D array 'zNew' of length targetNz spanning [z_min..z_max],
    plus a 4D array 'zCoeffs' of shape (nx, ny, targetNz, 2).

    For each (i, j, kNew), zCoeffs[i,j,kNew] = (k0, alpha),
    meaning the interpolated value is:
       val = data[i,j,k0]*(1-alpha) + data[i,j,k0+1]*alpha.

    Returns:
      x1D, y1D : 1D arrays (length nx, ny) derived from the first row/col of xcoord,ycoord
                 (assuming uniform X and Y).
      zNew : 1D array, shape (targetNz,)
      zCoeffs : array, shape (nx, ny, targetNz, 2)
    """

    nx, ny, nz = zcoord.shape

    # 1) Build x1D, y1D from the assumption that xcoord, ycoord are uniform
    #    in the i, j directions. We take slices along the dimension we trust.
    #    E.g. xcoord[i,0,0] for i in [0..nx-1], ycoord[0,j,0] for j in [0..ny-1].
    x1D = np.array([xcoord[i, 0, 0] for i in range(nx)], dtype=float)
    y1D = np.array([ycoord[0, j, 0] for j in range(ny)], dtype=float)

    # 2) Find min and max of zcoord for the bounding range
    z_min = float(zcoord.min())
    z_max = float(zcoord.max())
    if z_max <= z_min:
        raise ValueError(f"Invalid z-range: z_min={z_min}, z_max={z_max}")

    # 3) Create a new uniform 1D array zNew of length targetNz
    zNew = np.linspace(z_min, z_max, targetNz)
    print("altitude min max =",z_min, z_max)
    # 4) Build the coefficient array
    #    zCoeffs[i,j,kNew,0] = k0 (int index)
    #    zCoeffs[i,j,kNew,1] = alpha (float in [0..1])
    zCoeffs = np.zeros((nx, ny, targetNz), dtype=float)

    # For each (i, j), we assume zcoord[i,j,:] is sorted ascending
    # If not sorted, you'd need to sort it first or handle it differently.
    for i in range(nx):
        for j in range(ny):
            old_z = zcoord[i, j, :]  # shape (nz,)

            # If not sorted, do:
            # idx_sorted = np.argsort(old_z)
            # old_z = old_z[idx_sorted]

            k_current = 0  # pointer for old_z
            for kNew in range(targetNz):
                zTarget = zNew[kNew]

                # Advance until old_z[k_current+1] >= zTarget or we reach end
                while (k_current < nz-2) and (old_z[k_current+1] < zTarget):
                    k_current += 1

                # old_z[k_current] <= zTarget <= old_z[k_current+1] typically
                zLo = old_z[k_current]
                zHi = old_z[k_current+1] if (k_current+1 < nz) else zLo

                denom = zHi - zLo
                if denom <= 1e-12:  # degenerate or out of range => no interpolation
                    alpha = 0.0
                else:
                    alpha = (zTarget - zLo) / denom

                # Store k0 and alpha
                zCoeffs[i, j, kNew] = k_current+alpha
                if(zTarget<old_z[0]):
                    zCoeffs[i, j, kNew] = -1

    return x1D, y1D, zNew, zCoeffs
def saveJsonVdbIndex(json_filename, file_list, time_list):
    """
    Create a JSON meta-file so that ParaView can recognize
    multiple .vdb files as a time series. The output file
    has the format:

    {
      "file-series-version": "1.0",
      "files": [
        { "name": "step0.vdb", "time": 0 },
        { "name": "step10.vdb", "time": 10 },
        ...
      ]
    }

    Parameters
    ----------
    json_filename : str
        Path to the output JSON file (e.g. "vdbgroups.json").
    file_list : list of str
        Each element is a .vdb filename, e.g. "output.full.0.vdb".
    time_list : list of float or int
        Time or step values matching each file.

    Example
    -------
    appendedOutName = ["output.full.0.vdb", "output.full.10.vdb", "output.full.550.vdb"]
    stepVals = [0, 10, 550]
    saveJsonVdbIndex("vdbgroups.json", appendedOutName, stepVals)
    """
    if len(file_list) != len(time_list):
        raise ValueError("file_list and time_list must have the same length.")

    file_entries = []
    for fname, tval in zip(file_list, time_list):
        file_entries.append({
            "name": fname,
            "time": float(tval)  # ensure we store a numeric time
        })

    meta_dict = {
        "file-series-version": "1.0",
        "files": file_entries
    }

    with open(json_filename, "w") as f:
        json.dump(meta_dict, f, indent=4)
    print(f"[saveJsonVdbIndex] Wrote meta file: {json_filename}")

def savePvdIndex(pvd_filename, file_list, time_list):
    """
    Create a .pvd meta-file so that ParaView can recognize
    multiple .vti files as a time series. The output file
    has the format:

    <VTKFile type="Collection" version="0.1" byte_order="LittleEndian">
      <Collection>
        <DataSet timestep="0" group="" part="0" file="step0.vti"/>
        <DataSet timestep="10" group="" part="0" file="step10.vti"/>
        ...
      </Collection>
    </VTKFile>
    """
    if len(file_list) != len(time_list):
        raise ValueError("file_list and time_list must have the same length.")

    root = ET.Element("VTKFile")
    root.set("type", "Collection")
    root.set("version", "0.1")
    root.set("byte_order", "LittleEndian")

    collection = ET.SubElement(root, "Collection")
    for fname, tval in zip(file_list, time_list):
        ds = ET.SubElement(collection, "DataSet")
        ds.set("timestep", str(float(tval)))
        ds.set("group", "")
        ds.set("part", "0")
        ds.set("file", fname)

    tree = ET.ElementTree(root)
    tree.write(pvd_filename)
    print(f"[savePvdIndex] Wrote meta file: {pvd_filename}")

def gridToVTK(
    outname,
    data,             # shape (nx, ny, nz)
    x1D, y1D, zNew,   # 1D arrays
    zCoeffs,          # shape (nx, ny, nzNew)
    rangeVdb=(0.002, 1.0),
    fallbackVoxelSize=80.0
):
    """
    Interpolate data along z, apply threshold, and write as a .vti (VTK XML Image) file.
    """
    # Prepare dims
    nx, ny, nz_orig = data.shape
    newNz = len(zNew)
    lower_threshold, upper_threshold = rangeVdb

    # Derive dx, dy, dz from x1D, y1D, zNew or fallback
    try:
        if len(x1D) > 1:
            dx = float(x1D[1] - x1D[0])
        else:
            dx = fallbackVoxelSize

        if len(y1D) > 1:
            dy = float(y1D[1] - y1D[0])
        else:
            dy = fallbackVoxelSize

        if newNz > 1:
            dz = float(zNew[1] - zNew[0])
        else:
            dz = fallbackVoxelSize

        origin = (x1D[0], y1D[0], zNew[0])
        print(f"[gridToVTK] Using dx={dx:.3f}, dy={dy:.3f}, dz={dz:.3f}, "
              f"origin=({origin[0]:.3f},{origin[1]:.3f},{origin[2]:.3f}).")
    except Exception as e:
        print(f"[gridToVTK] transform creation failed: {e}")
        dx = dy = dz = fallbackVoxelSize
        origin = (0.0, 0.0, 0.0)
        print(f"Falling back to uniform voxelSize={fallbackVoxelSize}")

    # Create a vtkImageData
    imageData = vtk.vtkImageData()
    # VTK dimensions: (nx, ny, nz) => we say newNz in the z dimension
    imageData.SetDimensions(nx, ny, newNz)
    imageData.SetSpacing(dx, dy, dz)
    imageData.SetOrigin(origin)

    # Create scalar array
    vtkArr = vtk.vtkFloatArray()
    vtkArr.SetName("scalar_data")
    vtkArr.SetNumberOfComponents(1)
    vtkArr.SetNumberOfTuples(nx * ny * newNz)

    # Interpolate & threshold
    index = 0
    for kNew in range(newNz):
        for j in range(ny):
            for i in range(nx):
                val_index = zCoeffs[i, j, kNew]
                if val_index < 0.0:
                    # below min altitude => 0 or background
                    vtkArr.SetValue(index, 0.0)
                    index += 1
                    continue
                k0 = int(val_index)
                alpha = val_index - k0

                if k0 < 0:
                    val = data[i, j, 0]
                elif k0 >= nz_orig - 1:
                    val = data[i, j, nz_orig - 1]
                else:
                    vLo = data[i, j, k0]
                    vHi = data[i, j, k0+1]
                    val = (1 - alpha) * vLo + alpha * vHi

                # threshold
                if val < lower_threshold:
                    vtkArr.SetValue(index, 0.0)
                else:
                    if val > upper_threshold:
                        val = upper_threshold
                    vtkArr.SetValue(index, val)
                index += 1

    imageData.GetPointData().SetScalars(vtkArr)

    # Write to .vti (a .vti variant)
    writer = vtk.vtkXMLImageDataWriter()
    writer.SetFileName(outname + ".vti")
    # optional: use compression
    writer.SetCompressorTypeToZLib()
    writer.SetInputData(imageData)
    writer.Write()

    print(f"[gridToVTK] Wrote {outname}.vti with shape=({nx},{ny},{newNz}).")
    return imageData

def ffmnhFileToVTK(inpattern="", pgdInfo="", outPath="", cleanFile=False,
                   startStep=-1, endStep=-1, norecompute=False,
                   quitAfterCompute=False, vect_vars=("U", "V", "W"),
                   varName="BRatio", rangeVdb=(0.002, 1.0)):
    """
    Main conversion logic: read domain files, reconstruct data, and write
    them out to .vti using VTK. A .pvd index file is also generated
    so that ParaView can load the time series.
    """
    fprefix = inpattern.split("/")[-1]
    gridKey = "ZGRID"

    scals = {"cells": (), "points": []}
    file_pattern = f"{inpattern}.1.*"
    files = glob.glob(file_pattern)

    exclude_vars = set(vect_vars + (gridKey,))
    surface_vars = []
    for f in files:
        variable = f.split(".")[-1]
        if variable not in exclude_vars:
            if variable.startswith("surf"):
                surface_vars.append(variable)
            else:
                scals["points"].append(variable)
    scals["points"] = tuple(scals["points"])

    Vects = {"Wind": vect_vars}

    # If we only want the number of time steps, do so and quit.
    if quitAfterCompute:
        navail = numberOfFields(f"{inpattern}.1.{vect_vars[0]}")
        print(navail)
        return navail

    appendedSteps = readAllSteps(f"{inpattern}.1.{vect_vars[0]}")
    tsteps = list(appendedSteps.keys())
    stepzgrid = tsteps[0] if tsteps else 0

    if quitAfterCompute:
        print(len(appendedSteps))
        return len(appendedSteps)

    print(len(appendedSteps), "steps found")
    Allsteps = np.sort(tsteps)
    steps = []

    # Identify which steps remain to process
    varsDataIn = Vects["Wind"] + scals["points"] + scals["cells"]
    for stepV in Allsteps:
        outname = "%s/%s.full.%d.vti" % (outPath, fprefix, stepV)
        if cleanFile and os.path.isfile(outname):
            print("Step %d already post-processed, cleaning up" % stepV)
            # Optionally remove old partial data
            for varNameIter in varsDataIn:
                delCmd = "%s.*.%d.%s" % (inpattern, stepV, varNameIter)
                print("rm", delCmd)
            continue

        if (norecompute or cleanFile) and os.path.isfile(outname):
            print(outname, "at %d " % stepV)
        else:
            if not os.path.isfile(outname):
                steps.append(stepV)
            else:
                print("Step %d already post-processed" % stepV)

    if norecompute or cleanFile:
        print("Quit because not recomputing:", norecompute, cleanFile)
        return

    if len(steps) < 1:
        print("nothing more to be done")
        return

    selectedSteps = steps[:]

    # Collect domain bounding info
    numOfDomains = len(list(glob.glob(f"{inpattern}.*.{gridKey}")))
    print("Found %d domains" % numOfDomains)

    # Parse PGD info (res, ni, nj, x0, y0)
    parts = pgdInfo.split(',')
    if len(parts) != 5:
        raise ValueError("Expected pgdInfo='res,ni,nj,x0,y0' (5 comma-separated values)")

    res = int(parts[0].strip())
    ni = int(parts[1].strip())
    nj = int(parts[2].strip())
    x0 = float(parts[3].strip())
    y0 = float(parts[4].strip())

    x_end = x0 + res * (ni - 1)
    y_end = y0 + res * (nj - 1)
    xhat = np.linspace(x0, x_end, ni)
    yhat = np.linspace(y0, y_end, nj)

    # Initialize domain arrays
    domains = np.zeros(shape=(4, numOfDomains), dtype=float)

    # Read domain shapes
    xhati, yhati = 0, 0
    for i in range(1, numOfDomains + 1):
        nii, nji, _z = readBinShape(f"{inpattern}.{i}.{gridKey}")
        domains[0][i - 1] = float(xhat[xhati])
        domains[1][i - 1] = float(yhat[yhati])
        domains[2][i - 1] = float(xhat[xhati + nii])
        domains[3][i - 1] = float(yhat[yhati + nji])
        xhati += nii - 2
        if xhati > (len(xhat) - nii):
            yhati += nji - 2
            xhati = 0

    if startStep > -1 and endStep > -1:
        selectedSteps = selectedSteps[startStep:endStep]

    print(scals["points"], surface_vars, " on ", len(selectedSteps), " time steps")

    # Basic domain-level resolution
    if numOfDomains > 1:
        largCell = (domains[2][0] - domains[0][1]) / 2.0
        print("Multiproc  CELL SIZE SET TO ", largCell)
    else:
        largCell = 200
        print("WARNING  CELL SIZE SET TO ", largCell)

    dx = dy = largCell
    largDom = (max(domains[2])) - (min(domains[0])) - largCell * 2
    lonDom = (max(domains[3])) - (min(domains[1])) - largCell * 2
    tx = int(largDom // largCell)
    ty = int(lonDom // largCell)
    print("Domain ", tx, ty, " resolution ", dx, dy, " in domains (%d)" % numOfDomains)

    # Merge domain Z info
    zzd = None
    domainshapesInPoint = {}
    dLocalShape = {}
    for inddom in range(numOfDomains):
        localZ = readBinS(inpattern, inddom + 1, stepzgrid, gridKey, appendedSteps, zkey=gridKey)
        shZ = np.shape(localZ)
        nx = shZ[0] - 1
        ny = shZ[1] - 1
        nz = shZ[2] - 1
        domainshapesInPoint[inddom] = (nx, ny, nz)

        if zzd is None:
            zzd = np.zeros((tx + 1, ty + 1, nz + 1), dtype=np.float32)

        datatop = int((domains[3][inddom] - domains[1][0]) / dy - 1)
        databottom = int((domains[1][inddom] - domains[1][0]) / dy)
        dataleft = int((domains[0][inddom] - domains[0][0]) / dx)
        dataright = int((domains[2][inddom] - domains[0][0]) / dx - 1)

        dLocalShape[inddom] = (databottom, datatop - 4, dataleft, dataright - 4)

        for k in range(nz + 1):
            for j in range(dLocalShape[inddom][0],
                           dLocalShape[inddom][0] + shZ[1]):
                for i in range(dLocalShape[inddom][2],
                               dLocalShape[inddom][2] + shZ[0]):
                    zzd[i, j, k] = localZ[i - dLocalShape[inddom][2],
                                          j - dLocalShape[inddom][0],
                                          k]

    # Construct X/Y arrays
    xxd = np.zeros((tx + 1, ty + 1, nz + 1), dtype=np.float32)
    yyd = np.zeros((tx + 1, ty + 1, nz + 1), dtype=np.float32)
    X = np.arange(domains[0][0] + 1.5 * dx,
                  domains[0][0] + (tx + 1) * dx + 1.5 * dx, dx, dtype=np.float32)
    Y = np.arange(domains[1][0] + 1.5 * dy,
                  domains[1][0] + (ty + 1) * dy + 1.5 * dy, dy, dtype=np.float32)

    for i in range(tx + 1):
        for j in range(ty + 1):
            for k in range(nz + 1):
                xxd[i, j, k] = X[i]
                yyd[i, j, k] = Y[j]

    # Generate the voxel remap
    midUVW = True
    x1D, y1D, zNew, zCoeffs = genVoxelRemap(xxd, yyd, zzd, targetNz=200)
    print("REMM ", np.shape(zNew), np.shape(zCoeffs))

    appendedOutName = []
    stepsDone = []
    grids_to_write = []

    for stepV in selectedSteps:
        varmapAll = {}
        for vkey in varsDataIn:
            varmapAll[vkey] = np.zeros((tx + 1, ty + 1, nz + 1), dtype=np.float32)

        # Combine data from each domain
        for indom in range(numOfDomains):
            if (indom % 100 == 0):
                print("processing sub-domains %d to %d at step %d" % (indom, indom + 100, stepV))

            nx, ny, nzd = domainshapesInPoint[indom]
            i0 = dLocalShape[indom][2]
            j0 = dLocalShape[indom][0]
            feed_shape_x = nx + 1
            feed_shape_y = ny + 1

            for vkey in varsDataIn:
                feed_data = readBinS(inpattern, indom + 1, stepV, vkey, appendedSteps)
                if midUVW and vkey in ("U", "V", "W"):
                    if vkey == "U":
                        # average in x
                        u_mass = 0.5 * (feed_data[:-1, :, :] + feed_data[1:, :, :])
                        varmapAll[vkey][i0:i0+nx, j0:j0+feed_shape_y, :nzd+1] = u_mass
                    elif vkey == "V":
                        # average in y
                        v_mass = 0.5 * (feed_data[:, :-1, :] + feed_data[:, 1:, :])
                        varmapAll[vkey][i0:i0+feed_shape_x, j0:j0+ny, :nzd+1] = v_mass
                    elif vkey == "W":
                        # average in z
                        w_mass = 0.5 * (feed_data[:, :, :-1] + feed_data[:, :, 1:])
                        varmapAll[vkey][i0:i0+feed_shape_x, j0:j0+feed_shape_y, :nzd] = w_mass
                else:
                    varmapAll[vkey][i0:i0+feed_shape_x, j0:j0+feed_shape_y, :nzd+1] = feed_data

        # Write out VTK
        outname = "%s/%s.full.%d" % (outPath, fprefix, stepV)

        if varName not in varmapAll:
            print("Variable '%s' not found for step %d." % (varName, stepV))
            continue

        print("DATMM ", np.shape(varmapAll[varName]))
        imageData = gridToVTK(
            outname,
            varmapAll[varName],
            x1D=x1D,
            y1D=y1D,
            zNew=zNew,
            zCoeffs=zCoeffs,
            rangeVdb=rangeVdb,
            fallbackVoxelSize=80.0
        )

        # Keep track of output for the PVD index
        appendedOutName.append(outname + ".vti")
        stepsDone.append(stepV)

    # Build a single PVD file referencing all .vti outputs
    pvd_out = "%s/%sall.pvd" % (outPath, fprefix)
    savePvdIndex(pvd_out, appendedOutName, stepsDone)

    print("Processing finished.")

if __name__ == "__main__":
    import argparse
    import sys

    def main():
        parser = argparse.ArgumentParser(
            description=(
                "Convert ffmnh files to VTK .vti format. Example:\n"
                "  python pMNHFF2VTK.py -fields MODEL1/output 80,69,69,-80,-80 vdbout1"
                " -varName=BRatio -rangeVdb=0.002,1.0 -steps 10 21\n"
            ),
            formatter_class=argparse.RawTextHelpFormatter
        )
        parser.add_argument('--fields', nargs=3,
                            help='(1) mnhdump file pattern, (2) PGD info, (3) Output directory')
        parser.add_argument('--varName', help='Data variable to convert (e.g. BRatio)')
        parser.add_argument('--rangeVdb', help='Comma-separated lower,upper range for threshold', default="0.002,1.0")
        parser.add_argument('--steps', nargs=2, type=int,
                            help='Start/end steps for processing')
        parser.add_argument('--norecompute', action='store_true',
                            help='Skip re-compute phase if .vti files exist')
        parser.add_argument('--quit', action='store_true',
                            help='Quit after showing number of fields (no output files)')
        parser.add_argument('--cleanFile', action='store_true',
                            help='If set, remove the original partial data once processed')

        args = parser.parse_args()
        if len(sys.argv) == 1:
            parser.print_help(sys.stderr)
            sys.exit(1)

        startStep = -1
        endStep = -1
        if args.steps:
            startStep = args.steps[0]
            endStep = args.steps[1]

        if args.fields:
            range_pair = (0.002, 1.0)
            if args.rangeVdb:
                parts2 = args.rangeVdb.split(',')
                if len(parts2) == 2:
                    lo, hi = parts2
                    range_pair = (float(lo), float(hi))

            ffmnhFileToVTK(
                inpattern=args.fields[0],
                pgdInfo=args.fields[1],
                outPath=args.fields[2],
                cleanFile=args.cleanFile,
                startStep=startStep,
                endStep=endStep,
                norecompute=args.norecompute,
                quitAfterCompute=args.quit,
                varName=args.varName if args.varName else "BRatio",
                rangeVdb=range_pair
            )

    main()