.. _reference-parameters:

Parameter Reference
===================

This page documents the configuration parameters managed by ``SimulationParameters`` (defaults verified against ``src/SimulationParameters.cpp``). Parameters are **case-sensitive** and are set with :ref:`setParameter <cmd-setParameter>`, :ref:`setParameters <cmd-setParameters>`, or ``ff["key"] = value`` in Python.

Physics, Spread & Fuels
-----------------------

.. _param-propagationModel:

``propagationModel``
~~~~~~~~~~~~~~~~~~~~
* **Description:** Registered name of the Rate of Spread (ROS) model used when no spatial propagation model map is loaded. See :doc:`/user_guide/models` and :doc:`/user_guide/fuels_and_models`.
* **Default:** ``Iso``

``Iso.speed``
~~~~~~~~~~~~~
* **Description:** Constant rate of spread (in m/s) returned by the ``Iso`` propagation model.
* **Default:** ``1.0`` (read when ``Iso`` is instantiated)

``windReductionFactor``
~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Multiplier scaling ambient/surface wind speed to mid-flame height (used by ``Rothermel`` and ``RothermelAndrews2018``).
* **Default:** ``0.4``

``propagationSpeedAdjustmentFactor``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Global multiplier applied to the rate of spread returned by the propagation model.
* **Default:** ``1``

``minSpeed``
~~~~~~~~~~~~
* **Description:** Minimum propagation speed floor (in m/s) enforced when advancing a ``FireNode``.
* **Default:** ``0.005``

``maxSpeed``
~~~~~~~~~~~~
* **Description:** Maximum propagation speed cap (in m/s).
* **Default:** ``10``

``fuelsTableFile``
~~~~~~~~~~~~~~~~~~
* **Description:** Path to the semicolon-delimited fuel table file (e.g., ``fuels.csv``), resolved relative to ``caseDirectory``. Ignored if ``fuelsTable`` is set inline.
* **Default:** ``fuels.ff``

``fuelsTable``
~~~~~~~~~~~~~~
* **Description:** Inline semicolon-delimited fuel table string (or a built-in table name such as ``STDfarsiteFuelsTable`` or ``STDlandIndexedFuelsTable``). When set, ForeFire parses this string directly instead of opening ``fuelsTableFile``.
* **Default:** *(unset)*

``defaultFuelType``
~~~~~~~~~~~~~~~~~~~
* **Description:** Fallback integer fuel index used when no ``fuel`` layer is registered.
* **Default:** ``1`` (when read)

``heatFluxDefaultModel`` / ``vaporFluxDefaultModel`` / ``SPOTF1DefaultModel``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Default registered C++ flux models assigned to the ``heatFlux``, ``vaporFlux``, and ``SPOTF1`` (spotting) flux layers.
* **Default:** ``heatFluxBasic``, ``vaporFluxBasic``, ``SpottingFluxBasic``

``FRPToWatts``
~~~~~~~~~~~~~~
* **Description:** Conversion factor from Fire Radiative Power (``frp`` in megawatts) to total power in watts used by :ref:`emit <cmd-emit>`.
* **Default:** ``10000000`` (``1e7``)


Domain, Georeferencing & Front Discretization
---------------------------------------------

.. _param-perimeterResolution:

``perimeterResolution``
~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Target maximum spacing (in meters) between adjacent ``FireNode`` markers along a ``FireFront``. When two neighbors separate beyond this distance, new markers are inserted.
* **Default:** ``40``

``spatialIncrement``
~~~~~~~~~~~~~~~~~~~~
* **Description:** Distance (in meters) each ``FireNode`` advances on every update event (:math:`\Delta t_i = \text{spatialIncrement} / R_i`).
* **Default:** ``2``

.. note::
   When creating a ``FireDomain``, ForeFire checks that ``spatialIncrement / perimeterResolution <= 0.3`` (hard-coded in ``FireDomain.cpp``) and prints a warning if exceeded. The legacy ``spatialCFLmax`` parameter is not used by the solver.

``BBoxWSEN``
~~~~~~~~~~~~
* **Description:** Geographic WGS84 bounding box ``westLon,southLat,eastLon,northLat`` (comma-separated, no parentheses). Set this **before** calling ``FireDomain[...]`` in standalone runs without a NetCDF file so ``lonlat`` ignitions and GeoJSON/KML exports are georeferenced.
* **Default:** *(unset; populated automatically from the NetCDF ``domain`` variable when using ``loadData``)*

``SWLngLat`` / ``DefaultSWLngLat``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Fallback WGS84 southwest corner ``lon,lat`` used when ``BBoxWSEN`` is not set.
* **Default:** ``DefaultSWLngLat = 8.6192,41.765`` (Corsica)

``normalScheme``
~~~~~~~~~~~~~~~~
* **Description:** Scheme used to compute the outward unit normal vector at each ``FireNode``. Valid values: ``medians``, ``weightedMedians``, ``splines``.
* **Default:** ``medians``

``curvatureComputation``
~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Boolean (``0`` or ``1``) enabling local front curvature calculation at each ``FireNode``.
* **Default:** ``1``

``curvatureScheme``
~~~~~~~~~~~~~~~~~~~
* **Description:** Scheme used to compute front curvature when ``curvatureComputation=1``. Valid values: ``circumradius``, ``angle``.
* **Default:** ``circumradius``

``smoothing``
~~~~~~~~~~~~~
* **Description:** Smoothing weight applied during normal vector computation.
* **Default:** ``1``

``relax``
~~~~~~~~~
* **Description:** Relaxation factor (:math:`0\text{–}1`) used when updating node normal vectors and front depth.
* **Default:** ``0.5``

``frontDepthComputation``
~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Boolean (``0`` or ``1``) enabling local burning front depth computation along the normal direction.
* **Default:** ``0``

``frontDepthScheme``
~~~~~~~~~~~~~~~~~~~~
* **Description:** Scheme used to compute front depth when ``frontDepthComputation=1``. Valid value: ``normalDir``.
* **Default:** ``normalDir``

``initialFrontDepth``
~~~~~~~~~~~~~~~~~~~~~
* **Description:** Initial front depth (in meters) assigned to newly created ``FireNode`` markers.
* **Default:** ``20``

``minimalPropagativeFrontDepth``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Minimum front depth (in meters) used to size the arrival-time burning matrix (``BMap``) resolution alongside ``spatialIncrement``.
* **Default:** ``10.``

``maxFrontDepth``
~~~~~~~~~~~~~~~~~
* **Description:** Upper cap on computed front depth (in meters).
* **Default:** ``200.``

``burningTresholdFlux``
~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Heat flux threshold (in :math:`\text{W/m}^2`) above which a cell is considered actively burning when computing front depth.
* **Default:** ``10``

``initialBurningDuration``
~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Duration (in seconds) for which the area inside an initial fire front is marked as burning at initialization.
* **Default:** ``30``

``noInitialScan``
~~~~~~~~~~~~~~~~~
* **Description:** When set (to any value), skips scanning the interior of initial fire fronts into the arrival-time burning map at startup.
* **Default:** *(unset)*

``bmapLayer``
~~~~~~~~~~~~~
* **Description:** Boolean (``0`` or ``1``). Set to ``1`` to register the arrival-time burning map (`BMap`) as a queryable ``DataLayer`` (required by :func:`pyforefire.run_ensemble`).
* **Default:** ``0``

``max_inner_front_nodes_filter``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Threshold node count used when filtering small inner fronts.
* **Default:** ``50``


Files, Directories & Output Formatting
--------------------------------------

``dumpMode``
~~~~~~~~~~~~
* **Description:** Output format used by :ref:`print <cmd-print>`: ``ff`` (native ForeFire script format), ``json`` (Cartesian JSON), ``geojson`` (projected WGS84 GeoJSON), or ``kml`` (projected WGS84 KML).
* **Default:** ``ff``

``defaultColormap``
~~~~~~~~~~~~~~~~~~~
* **Description:** Default colormap used by :ref:`plot <cmd-plot>` when ``cmap=`` is omitted.
* **Default:** ``turbo``

``ISOdate``
~~~~~~~~~~~
* **Description:** Reference ISO 8601 UTC timestamp of the simulation (updated automatically when a timestamp is passed to ``loadData``).
* **Default:** ``2012-01-01T00:00:00Z``

``count``
~~~~~~~~~
* **Description:** Integer counter incremented automatically every time ``print[filename]`` writes a file. Can be referenced in filenames via ``*count*``.
* **Default:** ``0``

``caseDirectory``
~~~~~~~~~~~~~~~~~
* **Description:** Base directory for resolving relative input paths (such as ``fuelsTableFile``).
* **Default:** Current working directory (``getenv("PWD")``, or ``.``)

``ForeFireDataDirectory``
~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Subdirectory inside ``caseDirectory`` searched for auxiliary ForeFire data files.
* **Default:** ``ForeFire``

``experiment``
~~~~~~~~~~~~~~
* **Description:** Experiment name prefix used when saving the arrival-time map via ``save[]`` (writes ``<fireOutputDirectory>/<experiment>.<domainID>.nc``).
* **Default:** ``ForeFire``

``fireOutputDirectory``
~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Directory where ``save[]`` and periodic automatic outputs write files. Note that ``print[filename]`` writes relative to the current working directory, not ``fireOutputDirectory``.
* **Default:** ``.``

``outputFiles`` / ``outputsUpdate`` / ``bmapOutputUpdate``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Base filename pattern (``outputFiles``) and period in seconds for automatic front dumps (``outputsUpdate``) and automatic burning-map dumps (``bmapOutputUpdate``). ``0`` disables periodic dumps.
* **Default:** ``output``, ``0``, ``0``

``NetCDFfile``
~~~~~~~~~~~~~~
* **Description:** Recorded automatically by ``loadData[landscape.nc;date]``; also read in coupled Meso-NH initialization. ``loadData`` in scripts always requires its filename argument explicitly.
* **Default:** ``""``


Atmospheric Coupling (Meso-NH) & MPI
------------------------------------

``runmode``
~~~~~~~~~~~
* **Description:** Execution mode (``standalone`` or ``coupled``).
* **Default:** ``standalone``

``paramsFile`` / ``InitFile`` / ``InitFiles`` / ``InitTime`` / ``parallelInit``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Filenames and restart controls read by the Meso-NH coupling library (``CLibForeFire.cpp``) during coupled initialization—not used by standalone ``include[]``.
* **Default:** ``paramsFile = Params.ff``, ``InitFile = Init.ff``, ``InitFiles = output``, ``InitTime = 99999999999999``, ``parallelInit = 0``

``atmoNX`` / ``atmoNY`` / ``atmoNZ``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Grid dimensions of the atmospheric mesh (used to size flux integration cells; imposed automatically by Meso-NH in coupled runs).
* **Default:** ``100``, ``100``, ``20``

``refLongitude`` / ``refLatitude`` / ``year`` / ``month`` / ``day``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Reference origin and date imposed by Meso-NH in coupled runs (unused in standalone runs).
* **Default:** ``0``, ``0``, ``2012``, ``1``, ``1``

``MNHExchangeScalarLayersNames``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Comma-separated atmospheric scalar layers exchanged for live 3D coupled visualization (``mnh3d.html``).
* **Default:** ``plumeTopHeight,plumeBottomHeight,smokeAtGround,tke``

``watchedProc`` / ``CommandOutputs`` / ``FireDomainOutputs`` / ``FireFrontOutputs`` / ``FireNodeOutputs`` / ``FDCellsOutputs`` / ``HaloOutputs``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Verbose diagnostic flags and MPI rank filter (``watchedProc = -2`` silences all ranks, ``-1`` prints all ranks, ``n >= 0`` prints only rank ``n``).
* **Default:** ``watchedProc = -2``, ``CommandOutputs = 0``, ``FireDomainOutputs = 0``, ``FireFrontOutputs = 1``, ``FireNodeOutputs = 1``, ``FDCellsOutputs = 1``, ``HaloOutputs = 1``
