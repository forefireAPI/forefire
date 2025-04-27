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

At a minimum, a functional ForeFire simulation generally requires the following variables to be present within the NetCDF landscape file:

*   **Fuel Index Map**

    *   **Typical Variable Name(s):** ``fuel``, ``fuel_index``, ``land_cover`` (The exact name might be configurable or expected by default - check standard examples or parameters).
    *   **Description:** A 2D array (grid) containing **integer values** representing the fuel type at each location in the simulation domain.
    *   **Crucial Link:** Each unique integer value in this array **must** correspond directly to an ``Index`` value present in the :doc:`fuels_file` used for the simulation. ForeFire uses this to look up the fuel properties needed for the fire spread calculations. Index `0` is often used for non-burnable areas.
    *   **Dimensions:** Typically (Y, X) or (latitude, longitude) corresponding to the spatial grid.

*   **Elevation (Digital Elevation Model - DEM)**

    *   **Typical Variable Name(s):** ``altitude``, ``elevation``, ``dem``, ``hgt``.
    *   **Description:** A 2D array containing the elevation (height above sea level) for each grid cell in the domain. This is essential for calculating slope, which significantly impacts fire spread.
    *   **Units:** Meters (m).
    *   **Dimensions:** Must match the dimensions and grid of the Fuel Index Map.

Optional Data Variables
-----------------------

While fuel and elevation are fundamental, Landscape files often contain additional variables to support more complex simulations or specific model requirements:

*   **Wind Components (U and V)**

    *   **Typical Variable Name(s):** ``windU`` / ``wind_u`` / ``U`` (for West-East component), ``windV`` / ``wind_v`` / ``V`` (for South-North component).
    *   **Description:** 2D or 3D arrays representing the wind speed components across the domain. If 3D, the third dimension is typically time, allowing for dynamic wind conditions.
    *   **Units:** Meters per second (m/s).
    *   **Dimensions:** Must match the spatial dimensions (Y, X) of Fuel/Elevation. Time dimension added if applicable.
    *   **Note:** If wind data is not provided in the landscape file, wind conditions must be specified via other means, such as constant values set via :ref:`parameters <reference-parameters>` or dynamic conditions applied using the :ref:`trigger <cmd-trigger>` command.

*   **Other Potential Variables:** Depending on the specific propagation models or simulation setup, other variables might be included or derived:
    *   ``slope``: Slope steepness (often calculated internally from DEM).
    *   ``aspect``: Slope direction (often calculated internally from DEM).
    *   Canopy characteristics (if using specific canopy models).

Coordinate System & Domain Definition
-------------------------------------

*   **Projection:** While ForeFire's internal simulation often works with Cartesian coordinates (meters relative to an origin), the input NetCDF file should ideally contain **projected geospatial data**. Common choices include UTM (Universal Transverse Mercator) zones appropriate for the region of interest. The projection information (e.g., projection string, EPSG code) should ideally be included as global attributes or variable attributes within the NetCDF file if possible, although ForeFire might rely on user-defined domain bounds.
*   **Grid:** All 2D spatial variables (Fuel, Elevation, Wind) within the file must share the **same grid resolution, spatial extent, and coordinate system**.
*   **Domain Bounds:** The simulation domain is defined when creating the :ref:`FireDomain <cmd-FireDomain>`. You specify the South-West (SW) and North-East (NE) corners in the *projected Cartesian coordinates* (meters). Optionally, you can also provide the WGS84 geographic bounding box (`opt:BBoxWSEN` in `FireDomain`), which helps ForeFire align the simulation coordinate system with real-world locations, especially for outputs like GeoJSON or KML.

Relationship to Fuels File
--------------------------

As mentioned under Required Variables, the link between the integer values in the ``fuel`` layer of the NetCDF file and the ``Index`` column in the :doc:`fuels_file` is **absolutely critical**. If the landscape file contains a fuel index (e.g., `5`) that is not defined in the ``fuels.csv``, the simulation will likely fail or produce incorrect results.

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

3.  **ForeFire Helpers (Potentially):** Previous versions of ForeFire included Python helper scripts (e.g., `genForeFireCase.py`). While the status of V2 helpers is pending, tools might exist or be developed to assist in this NetCDF creation process. Consult the documentation sections on available tools once updated.

Loading in ForeFire
-------------------

The Landscape File is loaded into the simulation using the :ref:`loadData <cmd-loadData>` command, specifying the path to the ``.nc`` file and the corresponding UTC timestamp for the data:

.. code-block:: none

   loadData[path/to/your_landscape.nc;YYYY-MM-DDTHH:MM:SSZ]

Example:

.. code-block:: none

   loadData[aullene_data.nc;2023-08-10T14:00:00Z]

This command populates ForeFire's internal ``DataBroker`` with the necessary data layers for the simulation.