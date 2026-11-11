.. _userguide-landscape-file:

Landscape File (landscape.nc)
=============================

The NetCDF landscape file (``.nc``) packages the domain geometry and spatial data layers (fuel index map, elevation, and optional wind fields) for geospatial simulations.

.. tip::
   If you are working in Python with idealized grids or custom rasters in memory, you do not need to create a NetCDF file—pass NumPy arrays directly with ``addIndexLayer`` and ``addScalarLayer`` (see :doc:`python_arrays`).

How ForeFire Reads a Landscape File
-----------------------------------

When :ref:`loadData <cmd-loadData>` opens a NetCDF file, it inspects the ``domain`` variable first and then iterates over all remaining variables in the file, dispatching each one according to its ``type`` attribute. Any variable without a ``type`` attribute is skipped with a console message.

1. The domain Variable (Required)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The file **must** contain a variable named ``domain`` with ``domain:type = "domain"``. Without it, loading aborts with ``Error: Domain variable not found``.

Its attributes define the Cartesian simulation box (in meters and seconds) and optional WGS84 georeferencing:

* **Cartesian extent:** ``SWx``, ``SWy``, ``SWz`` (southwest corner in meters), ``Lx``, ``Ly``, ``Lz`` (domain lengths in meters), ``t0`` and ``Lt`` (initial time and duration in seconds). ``loadData`` constructs the :ref:`FireDomain <cmd-FireDomain>` directly from these attributes, so no separate ``FireDomain[...]`` command is needed.
* **Geographic alignment** (``BBoxWSEN``): an optional string attribute ``"lonWest,latSouth,lonEast,latNorth"`` in WGS84 degrees. ForeFire uses ``BBoxWSEN`` (or ``WSENLBRT``, which appends vertical and Cartesian bounds) to project ``lonlat=(lon,lat,z)`` ignitions and to export georeferenced GeoJSON and KML perimeters. ForeFire does not read EPSG codes or WKT projection strings from NetCDF attributes.

2. Required Data Layers
~~~~~~~~~~~~~~~~~~~~~~~

* **Fuel Index Map** (``fuel``)

  * **Variable name:** ``fuel`` (matched **exactly**; ``fuel_index`` and ``land_cover`` are not recognized by the C++ loader).
  * **Attribute:** ``fuel:type = "fuel"``.
  * **Values:** integer indices corresponding to the ``Index`` column of your fuel table (:doc:`fuels_and_models`). Index ``0`` typically denotes non-burnable cells.
  * **Dimensions:** read from the variable itself, ordered outermost axis first: ``(NY, NX)`` or ``(NT, NZ, NY, NX)``.

* **Elevation** (``altitude``)

  * **Variable name:** ``altitude`` (matched **exactly**; if named ``elevation``, ``dem``, or ``hgt``, it is ignored and the simulation runs on flat ground).
  * **Attribute:** ``altitude:type = "data"``.
  * **Values:** height above sea level in meters (m). ForeFire automatically derives the ``slope`` layer from ``altitude``.
  * **Resolution:** each layer uses its own grid dimensions, so ``altitude`` does not need to share the resolution of ``fuel`` (for example, ``tests/runff/data.nc`` pairs a 10 m fuel raster with a 100 m DEM).

3. Optional Wind Layers
~~~~~~~~~~~~~~~~~~~~~~~

Wind fields in meters per second (m/s) can be stored in either of two forms:

* **Combined** ``wind`` **variable** with ``wind:type = "wind"`` and four dimensions ``(2, wind_directions, wind_rows, wind_columns)``, where the leading axis selects the zonal (``U``) and meridional (``V``) response fields (as in ``tests/runff/data.nc``).
* **Separate** ``windU`` **(West–East) and** ``windV`` **(South–North) variables** (matched **exactly**).

If no wind variable is present in the NetCDF file, supply wind via ``addScalarLayer`` or the :ref:`trigger <cmd-trigger>` command.

Creating a Landscape File from Python
-------------------------------------

The helper ``tools/preprocessing/genForeFireCase.py`` builds a compliant NetCDF file directly from NumPy arrays:

.. code-block:: python

   import numpy as np
   from genForeFireCase import FiretoNC

   FiretoNC(
       "landscape.nc",
       domainProperties={
           "SWx": 0.0, "SWy": 0.0, "SWz": 0.0,
           "Lx": 1000.0, "Ly": 1000.0, "Lz": 0.0,
           "t0": 0.0, "Lt": np.inf,
       },
       parametersProperties={
           "date": "2026-08-12T12:00:00Z",
           "duration": 3600,
           "refYear": 2026, "refDay": 224,
           "year": 2026, "month": 8, "day": 12,
       },
       fuelModelMap=fuel,          # (NY, NX) integer fuel indices
       elevation=elevation,        # (NY, NX) meters
       wind={"zonal": windU, "meridian": windV},
   )

You can also export the active ``altitude``, ``fuel``, and ``wind`` layers of a running simulation to a NetCDF landscape file with :ref:`save <cmd-save>`:

.. code-block:: none

   save[filename=landscape_snapshot.nc;fields=altitude,fuel,wind;compression=6]

Validating a Landscape File (forefire-validate)
-----------------------------------------------

If a cell in the ``fuel`` raster contains an integer index that is missing from ``fuels.csv``, the simulation can fail or produce invalid spread rates without an obvious error message. Run ``forefire-validate`` (included with ``pip install forefire``; requires ``pip install netCDF4``) to check both files before starting a run:

.. code-block:: bash

   forefire-validate landscape.nc fuels.csv

It checks that:

1. A fuel variable exists and **every unique fuel index in the raster is defined in the fuel table**.
2. An elevation variable is present and reports any shape mismatch.
3. Wind layers are present (note: ``forefire-validate`` checks for 2D wind variables and prints a warning on files that store a combined 4D ``wind(t,z,y,x)`` variable such as ``tests/runff/data.nc``; also make sure the variable names in the file match the exact C++ names ``fuel``, ``altitude``, and ``wind``).

Loading the File in a Script
----------------------------

Use :ref:`loadData <cmd-loadData>` with the path to the ``.nc`` file (resolved relative to ``caseDirectory/ForeFireDataDirectory``; set ``ForeFireDataDirectory=.`` when the file is in the current directory) and an optional ISO 8601 UTC start date:

.. code-block:: none

   setParameter[ForeFireDataDirectory=.]
   loadData[data.nc;2023-08-10T14:00:00Z]
