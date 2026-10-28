# Copyright (C) 2012
# Author(s): Jean Baptiste Filippi, Vivien  Mallet
#
# This file is part of pyFireScore, a tool for scoring wildfire simulation
#
# pyFireScore is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# pyFireScore is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.

"""Writes the NetCDF landscape file ForeFire reads.

ForeFire needs one file describing the terrain a fire runs on: which fuel
sits in each cell, the elevation, and optionally the wind and the flux models.
This builds it from numpy arrays.

    from genForeFireCase import FiretoNC

    FiretoNC("landscape.nc", domainProperties, parametersProperties,
             fuelModelMap, elevation=..., wind=...)

then point a ForeFire script at it with

    setParameter[NetCDFfile=landscape.nc]

Field arrays are indexed the way ForeFire reads them, outermost axis first:
(NY, NX) for a 2-D field, (NZ, NY, NX) for a 3-D one, (NT, NZ, NY, NX) for a
4-D one. This is the convention prealCF2Case.py in this directory already
uses.

Arguments to FiretoNC:

              filename: where to write.
      domainProperties: the domain extent, matching the ForeFire parameters
                        SWx, SWy, SWz, Lx, Ly, Lz, t0, Lt.
  parametersProperties: simulation date and duration. See REQUIRED_PARAMETERS.
          fuelModelMap: integer array of fuel indices into the fuel table.
             elevation: real array of ground elevation, metres.
                  wind: dict with "zonal" and "meridian" real arrays.
          fluxModelMap: list of dicts, each with "name", "data" (integer array
                        of model indices) and "table" (name -> index).

prealCF2Case.py in this directory carries its own copy of FiretoNC and
addFieldToNcFile, which it fixed independently while this file was missing.
The two should be merged, but that is a change to a working script and is left
for its own commit.
"""

import numpy as np
from netCDF4 import Dataset

# parametersProperties has no defaults: every one of these ends up as an
# attribute ForeFire reads back, and guessing a date for someone is worse than
# telling them which key is missing.
REQUIRED_PARAMETERS = ('date', 'duration', 'refYear', 'refDay',
                       'year', 'month', 'day')


def addFieldToNcFile(ncfile, field, fieldname, typeName, dvartype):
    """Writes one field as a (NT, NZ, NY, NX) variable.

    `field` is indexed (NY, NX), (NZ, NY, NX) or (NT, NZ, NY, NX): the same
    order, with the leading axes dropped when they are not used.

    The dimensions used to be read off the array as (NY, NX, NZ, NT) while the
    variable was created as (NT, NZ, NY, NX) and assigned without transposing,
    so only the 2-D path was right. A 4-D field raised a broadcast error, and a
    3-D one never got its NT dimension created at all.
    """
    sp = np.shape(field)
    if len(sp) < 2 or len(sp) > 4:
        raise ValueError(
            "field '%s' has %d dimensions, expected 2, 3 or 4 "
            "([NT, ][NZ, ]NY, NX)" % (fieldname, len(sp)))

    # Pad to (NT, NZ, NY, NX); the leading axes are 1 when not supplied.
    nt, nz, ny, nx = (1,) * (4 - len(sp)) + tuple(sp)

    ncfile.createDimension('%sNX' % fieldname, nx)
    ncfile.createDimension('%sNY' % fieldname, ny)
    ncfile.createDimension('%sNZ' % fieldname, nz)
    ncfile.createDimension('%sNT' % fieldname, nt)

    variable = ncfile.createVariable(
        fieldname, dvartype,
        ('%sNT' % fieldname, '%sNZ' % fieldname,
         '%sNY' % fieldname, '%sNX' % fieldname))

    variable[:, :, :, :] = np.reshape(field, (nt, nz, ny, nx))

    variable.type = typeName

    return variable


def FiretoNC(filename, domainProperties, parametersProperties, fuelModelMap,
             elevation=None, wind=None, fluxModelMap=None, bmap=None,
             cellMap=None):
    """Writes a ForeFire landscape file. See the module docstring."""

    if parametersProperties is not None:
        missing = [k for k in REQUIRED_PARAMETERS
                   if k not in parametersProperties]
        if missing:
            # Checked before the file is opened: a half-written landscape that
            # ForeFire then fails to load is harder to diagnose than this.
            raise KeyError(
                "parametersProperties is missing %s. All of %s are required."
                % (', '.join(missing), ', '.join(REQUIRED_PARAMETERS)))

    # NETCDF3_CLASSIC because that is what ForeFire has always been given here,
    # and the reader on the C++ side is the legacy netcdf-cxx4 API.
    ncfile = Dataset(filename, 'w', format='NETCDF3_CLASSIC')

    ncfile.version = "FF.1.0"
    domain = ncfile.createVariable('domain', 'S1', ())
    domain.type = "domain"
    domain.SWx = float(domainProperties['SWx'])
    domain.SWy = float(domainProperties['SWy'])
    domain.SWz = float(domainProperties['SWz'])
    domain.Lx = float(domainProperties['Lx'])
    domain.Ly = float(domainProperties['Ly'])
    domain.Lz = float(domainProperties['Lz'])
    domain.t0 = float(domainProperties['t0'])
    domain.Lt = float(domainProperties['Lt'])

    parameters = ncfile.createVariable('parameters', 'S1', ())
    parameters.type = "parameters"

    if parametersProperties is not None:
        parameters.date = parametersProperties['date']
        parameters.duration = parametersProperties['duration']
        parameters.refYear = parametersProperties['refYear']
        parameters.refDay = parametersProperties['refDay']
        parameters.year = parametersProperties['year']
        parameters.month = parametersProperties['month']
        parameters.day = parametersProperties['day']

    if fuelModelMap is not None:
        addFieldToNcFile(ncfile, fuelModelMap, 'fuel', 'fuel', 'i4')

    if elevation is not None:
        addFieldToNcFile(ncfile, elevation, 'altitude', 'data', 'f8')

    if wind is not None:
        addFieldToNcFile(ncfile, wind["zonal"], 'windU', 'data', 'f8')
        addFieldToNcFile(ncfile, wind["meridian"], 'windV', 'data', 'f8')

    if fluxModelMap is not None:
        for fMap in fluxModelMap:
            fVar = addFieldToNcFile(ncfile, fMap["data"], fMap["name"],
                                    'flux', 'i4')
            for entry in fMap["table"].keys():
                setattr(fVar, "model%dname" % fMap["table"][entry], entry)
            fVar.indices = np.array(list(fMap["table"].values()), dtype='i4')

    print("writing ", filename)
    ncfile.sync()
    ncfile.close()
