.. _userguide-landscape-file:

Your own rasters
================

When the fire runs on a real place and you have the data yourself, such as a fuel map and an elevation model out of a GIS, put them in one NetCDF file. The file holds the domain and the three layers of the landscape (:doc:`simulation_inputs`): the fuel map, the elevation and the wind response. One :ref:`loadData <cmd-loadData>` reads it all and creates the domain.

Reading NetCDF needs a build that has it. The ``pip`` package and the default CMake build do. A build with ``-DFOREFIRE_ENABLE_NETCDF=OFF`` and the WebAssembly build do not; there, ``loadData`` stops with ``needs NetCDF, which this build has no support for``. Without your own data, use :doc:`tiles` or :doc:`python_arrays`.

The example file
----------------

The source tree has one: ``tests/runff/data.nc``, a 64 km square of Corsica, with the fuel table that goes with it, ``tests/runff/fuels.csv``. Copy both into a folder, and next to them this ``case.ff``:

.. code-block:: none

   setParameters[ForeFireDataDirectory=.;fuelsTableFile=fuels.csv;propagationModel=Rothermel]
   loadData[data.nc;2025-02-10T17:35:54Z]
   startFire[lonlat=(8.70,41.952,0);t=0]
   trigger[wind;vel=(10,5,0)]
   step[dt=600]
   setParameter[dumpMode=geojson]
   print[front.geojson]

Run it with ``forefire -i case.ff``. ``front.geojson`` holds the front after ten minutes, in longitude and latitude. There is no ``FireDomain``: ``loadData`` built the domain from the file. If ``data.nc`` is a few hundred bytes instead of about 4 MB, it is a Git LFS pointer; ``tests/runff/check_data_nc.bash`` downloads the real file.

What the file holds
-------------------

A variable named ``domain``, whose attributes give the domain. Its value is not read:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Attribute
     - Meaning
   * - ``SWx``, ``SWy``, ``SWz``
     - South-west corner, in metres.
   * - ``Lx``, ``Ly``, ``Lz``
     - Size of the domain, in metres.
   * - ``t0``
     - Simulation time at the start, in seconds. Usually ``0``.
   * - ``BBoxWSEN``
     - Optional. ``"west,south,east,north"`` in degrees, as a text attribute (see below).

And one variable per layer, each with a ``type`` attribute:

.. list-table::
   :header-rows: 1
   :widths: 18 14 68

   * - Variable
     - ``type``
     - Holds
   * - ``fuel``
     - ``"fuel"``
     - The fuel map: integers, each an ``Index`` of the fuel table (:doc:`fuels_and_models`).
   * - ``altitude``
     - ``"data"``
     - Elevation in metres. ForeFire derives the slope from it.
   * - ``wind``
     - ``"wind"``
     - The wind response. Optional.

The names must be exactly these. A variable with no ``type`` attribute is skipped, with ``Skipping variable ... because 'type' attribute is missing``. Any other variable of type ``"data"`` is read only if the propagation model asks for a layer of that name, so an elevation called ``elevation`` or ``dem`` is ignored and the ground stays flat. The fuel table belongs to the case, not to the file.

How the arrays are laid out
---------------------------

As in :doc:`python_arrays`, every layer is a 4-D array shaped ``(time, component, y, x)``. Fuel and elevation have one time and one component, so they are ``(1, 1, ny, nx)``. A 2-D ``(ny, nx)`` variable is not read correctly: the run prints ``out of bounds`` warnings.

The first row is the south edge and the first column the west edge. Each layer is stretched over the whole domain, so each has its own resolution. In ``data.nc`` the domain is 64 km wide, the fuel map 6400 cells (10 m), the elevation 640 (100 m) and the wind response 160 (400 m).

The wind response
-----------------

``wind`` is shaped ``(2, 2, ny, nx)`` and works as in :doc:`python_arrays`: each :ref:`trigger[wind;vel=(u,v,0)] <cmd-trigger>` sets ``u`` and ``v``, and in each cell

.. code-block:: none

   windU = wind[0, 0] × u + wind[0, 1] × v
   windV = wind[1, 0] × u + wind[1, 1] × v

``wind`` can instead be shaped ``(2, 8, ny, nx)``: eight precomputed wind fields, one per direction, with ``wind[0, k]`` the ``windU`` and ``wind[1, k]`` the ``windV`` of field ``k``. Field ``k`` is used for a trigger wind blowing toward ``k × 45°`` counter-clockwise from east: 0 toward the east, 2 toward the north, 4 toward the west, 6 toward the south. A wind between two directions mixes the two nearest fields linearly, and the result is scaled by the trigger speed divided by 10, so each field is what a 10 m/s wind gives. Before the first ``trigger[wind]``, field 0 is used as stored.

Without a ``wind`` variable, ``trigger[wind]`` does nothing. Give a uniform wind with the ``windU`` and ``windV`` parameters instead (:doc:`simulation_inputs`).

A file can also hold ``windU`` and ``windV`` variables, of type ``"data"`` and shaped like the elevation. They are the wind itself, in m/s, and stay as they are: ``trigger[wind]`` does not change them.

Writing the file from Python
----------------------------

With the ``netCDF4`` package (``pip install netCDF4``), a 2 km square with a fuel break, a slope rising to the north and the identity wind response:

.. code-block:: python

   import numpy as np
   from netCDF4 import Dataset

   fuel = np.full((1, 1, 200, 200), 2, dtype="i4")    # 10 m cells
   fuel[0, 0, :, 140:150] = 0                         # a fuel break, north to south

   altitude = np.zeros((1, 1, 20, 20))                # 100 m cells
   altitude[0, 0] = np.linspace(0, 400, 20)[:, None]  # rising to the north

   wind = np.zeros((2, 2, 20, 20))                    # the identity wind response
   wind[0, 0] = 1.0
   wind[1, 1] = 1.0

   with Dataset("landscape.nc", "w") as nc:
       domain = nc.createVariable("domain", "S1")
       domain.type = "domain"
       domain.SWx, domain.SWy, domain.SWz = 0.0, 0.0, 0.0
       domain.Lx, domain.Ly, domain.Lz = 2000.0, 2000.0, 0.0
       domain.t0 = 0.0
       domain.BBoxWSEN = "8.700,41.950,8.724,41.968"

       for name, data, kind in [("fuel", fuel, "fuel"),
                                ("altitude", altitude, "data"),
                                ("wind", wind, "wind")]:
           dims = [f"{name}_{axis}" for axis in ("t", "z", "y", "x")]
           for dim, size in zip(dims, data.shape):
               nc.createDimension(dim, size)
           var = nc.createVariable(name, data.dtype, dims)
           var.type = kind
           var[:] = data

Write the domain numbers as floats (``0.0``, not ``0``). Python integers are stored as 64-bit integers, which ForeFire does not read: it prints ``SWx attribute of unhandled type int64`` and the domain comes out wrong.

Then, with a wind from the west:

.. code-block:: none

   setParameters[ForeFireDataDirectory=.;propagationModel=Rothermel;fuelsTable=STDlandIndexedFuelsTable]
   loadData[landscape.nc;2026-07-15T12:00:00Z]
   startFire[lonlat=(8.712,41.959,0);t=0]
   trigger[wind;vel=(3,0,0)]
   step[dt=1200]
   print[]

The fire starts in the middle of the square, runs east to the break at ``x = 1400`` and stops there.

``tools/mesonh/preprocessing/genForeFireCase.py`` in the source tree writes the same kind of file from 2-D arrays, with the wind as ``windU`` and ``windV``. Give it a finite ``Lt`` such as ``0.0``: an infinite one stops the domain from loading, with ``NetCDF: Numeric conversion not representable``.

Loading it
----------

``loadData[file;date]`` creates the domain and loads the layers. The date is that of ``t0``, written exactly as ``2026-07-15T12:00:00Z``.

A relative path goes under :ref:`ForeFireDataDirectory <param-ForeFireDataDirectory>`, the ``ForeFire`` folder of the working directory unless you change it, hence ``ForeFireDataDirectory=.`` in the scripts above. A path starting with ``/`` is used as it is.

``loadData[file]``, without a date, loads the layers onto a domain already made with :ref:`FireDomain <cmd-FireDomain>`. Without a domain it does nothing.

Longitude and latitude
~~~~~~~~~~~~~~~~~~~~~~

``BBoxWSEN`` puts the domain on the globe: the domain's south-west corner is at ``(west, south)`` and its north-east corner at ``(east, north)``, with longitude and latitude in between scaled linearly. :ref:`startFire <cmd-startFire>` then takes ``lonlat=(lon,lat,z)``, and :ref:`print <cmd-print>` writes GeoJSON and KML in degrees. Write ``BBoxWSEN`` as text, not as four numbers. No projection is read from the file.

Saving a landscape
------------------

:ref:`save <cmd-save>` writes the current layers to a file in the same layout, which ``loadData`` reads back:

.. code-block:: none

   trigger[fuelIndice;loc=(500,500,0);fuelType=0]
   save[filename=edited.nc;fields=altitude,fuel,wind]

The saved fuel includes changes made with ``trigger[fuelIndice]``. The file goes to the working directory, not to ``ForeFireDataDirectory``. Elevation is saved in whole metres.

When it goes wrong
------------------

- ``File ... doesn't exist or no longer available``: the path, usually a missing ``ForeFireDataDirectory=.``.
- ``Error: Domain variable not found``: the file has no ``domain`` variable. The run goes on with a meaningless domain.
- ``... attribute of unhandled type int64``: domain numbers written as integers.
- ``WARNING, NC variable ... is not in domain``: the layer does not overlap the domain, often because the domain attributes were not read.
- ``layer for slope doesn't rely on existing altitude layer``: no ``altitude`` layer was found, so the ground is flat.
- ``Array ... out of bounds``: a 2-D variable; make it ``(1, 1, ny, nx)``.
- A fuel ID that the table lacks gives no message: the fire stops wherever that fuel is.
