.. _userguide-landscape-file:

Landscape File (landscape.nc)
=============================

.. warning::

    This documentation is still based on forefire v1.0. The landscape file format and requirements may change in future versions (e.g., v2). Always refer to the latest documentation or release notes for updates.

The Landscape File, typically ending with the ``.nc`` extension, is the primary geospatial input file for ForeFire. It provides the environmental context over which the fire simulation will run.

File Format
-----------

The Landscape File **must** be in the **NetCDF (Network Common Data Form)** format (`.nc`). This is a standard format for storing array-oriented scientific data. ForeFire uses libraries to read the necessary variables directly from this file.

Required Data Variables
-----------------------

At a minimum, a functional ForeFire simulation generally requires the following variables to be present within the NetCDF landscape file.

ForeFire reads every variable in the file and dispatches on its ``type`` attribute. A variable without one is skipped, with a message on the console, so a misspelled name is not an error: the layer is simply absent from the simulation.

*   **Fuel Index Map**

    *   **Variable name:** ``fuel``, matched **exactly**. ``fuel_index`` and ``land_cover`` are not recognised.
    *   **Attribute:** ``type = "fuel"``.
    *   **Description:** A 2D array (grid) containing **integer values** representing the fuel type at each location in the simulation domain.
    *   **Crucial Link:** Each unique integer value in this array **must** correspond directly to an ``Index`` value present in the model parameter file (see :doc:`/user_guide/fuels_and_models`) used for the simulation. ForeFire uses this to look up the fuel properties needed for the fire spread calculations. Index `0` is often used for non-burnable areas.
    *   **Dimensions:** read from the variable itself; the axes are ordered outermost first.

*   **Elevation (Digital Elevation Model - DEM)**

    *   **Variable name:** ``altitude``, matched **exactly**. ``elevation``, ``dem`` and ``hgt`` are not recognised, and the simulation then runs without slope.
    *   **Attribute:** ``type = "data"``.
    *   **Description:** A 2D array containing the elevation (height above sea level) for each grid cell in the domain. This is essential for calculating slope, which significantly impacts fire spread.
    *   **Units:** Meters (m).
    *   **Dimensions:** read from the variable itself. It does **not** have to match the fuel grid: the shipped test case has fuel at 10 m and elevation at 100 m.

Optional Data Variables
-----------------------

While fuel and elevation are fundamental, Landscape files often contain additional variables to support more complex simulations or specific model requirements:

*   **Wind**

    Two forms are read, both in metres per second (m/s):

    *   a single ``wind`` variable holding both components, with the two leading axes selecting component and level, as in the shipped test case;
    *   or separate ``windU`` (West-East) and ``windV`` (South-North) variables, matched **exactly**. ``wind_u`` and ``U`` are not recognised.

    Time may be one of the variable's axes, allowing for dynamic wind conditions.

    **Note:** If wind data is not provided in the landscape file, wind conditions must be specified via other means, such as constant values set via :ref:`parameters <reference-parameters>` or dynamic conditions applied using the :ref:`trigger <cmd-trigger>` command.

*   **Other Potential Variables:** Depending on the specific propagation models or simulation setup, other variables might be included or derived:
    *   ``slope``: slope steepness, derived internally from ``altitude``. There is no ``aspect`` layer.
    *   Canopy characteristics (if using specific canopy models).

Coordinate System & Domain Definition
-------------------------------------

*   **Projection:** While ForeFire's internal simulation often works with Cartesian coordinates (meters relative to an origin), the input NetCDF file should ideally contain **projected geospatial data**. Common choices include UTM (Universal Transverse Mercator) zones appropriate for the region of interest. The projection information (e.g., projection string, EPSG code) should ideally be included as global attributes or variable attributes within the NetCDF file if possible, although ForeFire might rely on user-defined domain bounds.
*   **Grid:** Each layer is built from the dimensions of its own variable, so the file does not have to share a single grid: the shipped test case has fuel at 10 m and elevation at 100 m.
*   **Domain:** The file **must** contain a ``domain`` variable. Without it the load aborts with ``Error: Domain variable not found``. Its attributes carry the extent, ``SWx``, ``SWy``, ``SWz``, ``Lx``, ``Ly``, ``Lz``, ``t0`` and ``Lt``, and :ref:`loadData <cmd-loadData>` builds the :ref:`FireDomain <cmd-FireDomain>` from them, so a landscape file needs no explicit ``FireDomain`` command.
*   **Geographic alignment:** an optional ``BBoxWSEN`` attribute on the ``domain`` variable gives the WGS84 bounding box of the grid as ``lonWest,latSouth,lonEast,latNorth``. ForeFire reads it to place ``lonlat`` ignitions and to write geo-referenced output. ``WSENLBRT`` carries the same box followed by the vertical extent and the domain size.

Relationship to the Fuels & Models File
---------------------------------------

As mentioned under Required Variables, the link between the integer values in the ``fuel`` layer of the NetCDF file and the ``Index`` column in the model parameter file (see :doc:`/user_guide/fuels_and_models`) is **absolutely critical**. If the landscape file contains a fuel index (e.g., `5`) that is not defined in the ``fuels.csv``, the simulation will likely fail or produce incorrect results.

Creating Landscape Files
------------------------

Generating a suitable ``landscape.nc`` file typically involves standard Geographic Information System (GIS) workflows:

1.  **Obtain Source Data:**

    *   **DEM:** Download elevation data for your region (e.g., from SRTM, ASTER GDEM, national datasets, LiDAR).
    *   **Fuel Map:** Obtain a land cover or fuel type raster map (e.g., CORINE, LANDFIRE, national datasets, or custom classifications). Remember this might need reclassification to match the indices in your ``fuels.csv``.
    *   **Wind Data:** Obtain wind fields from meteorological models (e.g., WRF, GFS, ECMWF) or reanalysis datasets if dynamic wind is needed.

2.  **GIS Processing:** Use GIS software (like QGIS, ArcGIS, GRASS GIS) or geospatial libraries (like GDAL, Rasterio in Python):

    *   Ensure all layers are **reprojected** to the *same* target projected coordinate system (e.g., UTM).
    *   Ensure all layers are **resampled** or **aligned** to the *same* grid resolution and spatial extent.
    *   **Clip** the layers to your desired simulation domain boundaries.
    *   **Convert** the final processed layers into a single NetCDF file with appropriate variable names.

3.  **ForeFire Helpers:** ``tools/preprocessing/genForeFireCase.py`` writes the file for you from numpy arrays, which is usually easier than assembling the NetCDF by hand:

    .. code-block:: python

       import numpy as np
       from genForeFireCase import FiretoNC

       FiretoNC("landscape.nc",
                domainProperties={'SWx': 0., 'SWy': 0., 'SWz': 0.,
                                  'Lx': 1000., 'Ly': 1000., 'Lz': 0.,
                                  't0': 0., 'Lt': np.inf},
                parametersProperties={'date': "2026-08-12T12:00:00Z",
                                      'duration': 3600,
                                      'refYear': 2026, 'refDay': 224,
                                      'year': 2026, 'month': 8, 'day': 12},
                fuelModelMap=fuel,        # (NY, NX) integer fuel indices
                elevation=elevation,      # (NY, NX) metres
                wind={"zonal": windU, "meridian": windV})

    Field arrays are indexed outermost axis first: ``(NY, NX)``, or ``(NZ, NY, NX)`` and ``(NT, NZ, NY, NX)`` when they vary with height or time. Every key shown under ``parametersProperties`` is required.

Loading in ForeFire
-------------------

The Landscape File is loaded into the simulation using the :ref:`loadData <cmd-loadData>` command, specifying the path to the ``.nc`` file and the corresponding UTC timestamp for the data:

.. code-block:: none

   loadData[path/to/your_landscape.nc;YYYY-MM-DDTHH:MM:SSZ]

Example:

.. code-block:: none

   loadData[aullene_data.nc;2023-08-10T14:00:00Z]

This command populates ForeFire's internal ``DataBroker`` with the necessary data layers for the simulation.