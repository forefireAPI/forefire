.. _userguide-simulation-inputs:

What a simulation needs
=======================

A ForeFire run is a domain with a landscape on it, a propagation model, a fire to start, and usually wind and a fuel table. You then step the simulation forward and write out the front or the arrival times. This page lists each piece, says when you need it, and points to the page that explains it.

The pieces
----------

.. list-table::
   :header-rows: 1
   :widths: 16 14 70

   * - Piece
     - Needed?
     - How to provide it
   * - Domain
     - Always
     - The rectangle the fire lives in, in metres. Created by :ref:`FireDomain <cmd-FireDomain>`, or for you by :ref:`loadData <cmd-loadData>` on a NetCDF file with a date, on tiles with ``center=``, or on a ``.ffstate`` checkpoint (:ref:`loadState <cmd-loadState>` too). ``loadData`` on a NetCDF file *without* a date only adds layers to a domain that already exists.
   * - Model
     - Always
     - ``setParameter[propagationModel=...]``. The models and their papers are in :doc:`models`.
   * - Ignition
     - Always
     - :ref:`startFire <cmd-startFire>` at ``loc=(x,y,z)`` in metres, or at ``lonlat=(lon,lat,z)``. ``lonlat`` needs the domain placed on the globe: a NetCDF file with ``BBoxWSEN``, tiles, or ``BBoxWSEN``/``SWLngLat`` set before ``FireDomain`` (otherwise it defaults to a point in Corsica). For any other shape, draw the front by hand (:doc:`forefire_script`).
   * - Fuel map
     - Most models
     - Integer fuel indices over the domain: a NumPy array with ``addIndexLayer`` (:doc:`python_arrays`), the ``fuel`` variable of a NetCDF file (:doc:`landscape_file`), or ``.fftile`` tiles (:doc:`tiles`).
   * - Fuel table
     - Most models
     - What each fuel index means: a CSV file (``fuelsTableFile``), a string (``fuelsTable``), or a built-in table. The columns depend on the model (:ref:`fuel-columns-per-model`). ``Iso`` needs neither a fuel map nor a table.
   * - Elevation
     - Optional
     - Flat ground without it. A NumPy array named ``altitude`` with ``addScalarLayer``, the ``altitude`` variable of a NetCDF file (ForeFire derives the slope from it), or tiles (they store altitude and slope).
   * - Wind
     - Wind-driven models
     - A fixed field: separate ``windU``/``windV`` in NetCDF, :ref:`addLayer[name=windU;value=...] <cmd-addLayer>`, or Meso-NH in a coupled run (:doc:`mesonh`). Or a uniform vector set with :ref:`trigger[wind;vel=(u,v,0)] <cmd-trigger>`, which ForeFire spreads over the terrain through a wind response field: ``windU``/``windV`` arrays (``windScalDir``), the NetCDF ``wind`` variable, or the response stored in tiles. Without one of these, ``trigger[wind]`` has nothing to act on.
   * - Output
     - To see results
     - :ref:`print <cmd-print>` writes the front (text, GeoJSON, KML). :ref:`save <cmd-save>` writes the arrival-time matrix to NetCDF (needs a NetCDF build). :ref:`saveState <cmd-saveState>` writes a ``.ffstate`` checkpoint to resume from (some models only, see :doc:`tiles`).

On tiles, the fuel can also be changed during a run with :ref:`trigger[tileFuel] <cmd-trigger>` or a case-local fuel image (:doc:`tiles`).

Which landscape source?
-----------------------

Fuel, elevation and wind response come together from one of three sources:

- **NumPy arrays** (:doc:`python_arrays`): no files, no GIS. Good for idealized cases, for learning the API, and for generating data in bulk.
- **NetCDF file** (:doc:`landscape_file`): one ``.nc`` file with the domain and its layers. The reference case in ``tests/runff`` uses it.
- **Tiles** (:doc:`tiles`): real terrain from ``.fftile`` files, with no NetCDF dependency. Also what the browser builds use, and the base for ``.ffstate`` checkpoints.

Where to go next
----------------

- :doc:`forefire_script`: the command language that puts these pieces together, with a complete example.
- :doc:`python_arrays`: the same thing from Python.
- :doc:`landscape_file` and :doc:`tiles`: the two file-based landscapes.
- :doc:`fuels_and_models`: writing a fuel table.
