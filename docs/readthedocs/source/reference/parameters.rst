.. _reference-parameters:

Parameter Reference
===================

This page documents the configuration parameters managed by ``SimulationParameters`` (defaults verified against ``src/SimulationParameters.cpp`` or, where it sets none, the fallback in the code that reads the parameter). Most are read when the ``FireDomain`` is created, so set them before ``FireDomain`` or ``loadData``. Parameters of individual propagation and flux models are listed with each model in :doc:`/user_guide/models`. Parameters are **case-sensitive** and are set with :ref:`setParameter <cmd-setParameter>`, :ref:`setParameters <cmd-setParameters>`, or ``ff["key"] = value`` in Python.

Physics, Spread & Fuels
-----------------------

.. _param-propagationModel:

propagationModel
~~~~~~~~~~~~~~~~
* **Description:** Registered name of the Rate of Spread (ROS) model used when no spatial propagation model map is loaded. See :doc:`/user_guide/models` and :doc:`/user_guide/fuels_and_models`.
* **Default:** ``Iso``

Iso.speed
~~~~~~~~~
* **Description:** Constant rate of spread (in m/s) returned by the ``Iso`` propagation model.
* **Default:** ``1.0`` (read when ``Iso`` is instantiated)

windReductionFactor
~~~~~~~~~~~~~~~~~~~
* **Description:** Multiplier applied to the wind speed normal to the front by ``Rothermel`` and ``FrontDepthDriven``. In ``Rothermel``, a value below ``1`` also switches the wind-speed cap to the Andrews, Cruz and Rothermel (2013) limit.
* **Default:** ``0.4``

propagationSpeedAdjustmentFactor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Global multiplier applied to the rate of spread returned by the propagation model.
* **Default:** ``1``

minSpeed
~~~~~~~~
* **Description:** A ``FireNode`` whose speed (in m/s) is at or below this value stops advancing. It is a stopping threshold, not a floor on the speed.
* **Default:** ``0.005``

maxSpeed
~~~~~~~~
* **Description:** Speed cap (in m/s) used only by the ``BMapLoggerForANNTraining`` model. Other models are not capped.
* **Default:** ``10``

fuelsTableFile
~~~~~~~~~~~~~~
* **Description:** Path to the semicolon-delimited fuel table file (e.g., ``fuels.csv``), resolved relative to ``caseDirectory/ForeFireDataDirectory`` (set ``ForeFireDataDirectory=.`` when files sit directly in ``caseDirectory``). Ignored if ``fuelsTable`` is set inline.
* **Default:** ``fuels.ff``

fuelsTable
~~~~~~~~~~
* **Description:** Inline semicolon-delimited fuel table string (or a built-in table name such as ``STDfarsiteFuelsTable`` or ``STDlandIndexedFuelsTable``). When set, ForeFire parses this string directly instead of opening ``fuelsTableFile``. Both are read when the ``FireDomain`` is created.
* **Default:** *(unset)*

fuel / windU / windV / altitude
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Uniform values used when the model needs a layer that does not exist: integer fuel index for ``fuel``, wind components in m/s for ``windU``/``windV``, elevation in meters for ``altitude`` (needed for ``slope``). ForeFire then creates a constant layer with that value.
* **Default:** ``0`` (when unset)

heatFluxDefaultModel / vaporFluxDefaultModel / SPOTF1DefaultModel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Registered C++ flux model used for the ``heatFlux``, ``vaporFlux``, and ``SPOTF1`` (spotting) flux layers. Any flux layer ``<name>`` looks up ``<name>DefaultModel`` the same way.
* **Default:** ``heatFluxBasic``, ``vaporFluxBasic``, ``SpottingFluxBasic``

FRPToWatts
~~~~~~~~~~
* **Description:** Conversion factor from Fire Radiative Power (``frp`` in megawatts) to total power in watts used by :ref:`emit <cmd-emit>`. A value at or below ``0`` falls back to ``1e7``.
* **Default:** ``10000000`` (``1e7``)


Domain, Georeferencing & Front Discretization
---------------------------------------------

.. _param-perimeterResolution:

perimeterResolution
~~~~~~~~~~~~~~~~~~~
* **Description:** Target maximum spacing (in meters) between adjacent ``FireNode`` markers along a ``FireFront``. A new marker is inserted when two neighbors are more than twice this distance apart, and neighbors closer than this distance are merged.
* **Default:** ``40``

spatialIncrement
~~~~~~~~~~~~~~~~
* **Description:** Distance (in meters) each ``FireNode`` advances on every update event (:math:`\Delta t_i = \text{spatialIncrement} / R_i`).
* **Default:** ``2``

.. note::
   When creating a ``FireDomain``, ForeFire checks that ``spatialIncrement / perimeterResolution <= 0.3`` (hard-coded in ``FireDomain.cpp``) and prints a warning if exceeded. The legacy ``spatialCFLmax`` parameter is not used by the solver.

BBoxWSEN
~~~~~~~~
* **Description:** Geographic WGS84 bounding box ``westLon,southLat,eastLon,northLat`` (comma-separated, no parentheses). Set this **before** calling ``FireDomain[...]`` in standalone runs without a NetCDF file so ``lonlat`` ignitions and GeoJSON/KML exports are georeferenced. Only the west and south values are used, as the southwest corner of the domain.
* **Default:** *(unset; populated automatically from the NetCDF* ``domain`` *variable when using* ``loadData`` *)*

SWLngLat / DefaultSWLngLat
~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** WGS84 southwest corner ``lon,lat``. In a standalone run the domain takes the first one set of ``SWLngLat``, ``BBoxWSEN``, ``WSENLBRT`` (same layout as ``BBoxWSEN``), then ``DefaultSWLngLat``. Creating the domain writes the corner it used into ``SWLngLat``, so a later domain in the same session keeps that corner unless you set ``SWLngLat`` again.
* **Default:** ``SWLngLat`` unset; ``DefaultSWLngLat = 8.6192,41.765`` (Corsica)

metersPerDegreeLat / metersPerDegreesLon
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Scale (m per degree) used to convert between domain meters and longitude/latitude. Both must be set together (note the ``s`` in ``Degrees`` for longitude). Set automatically from the ``BBoxWSEN`` attribute of a NetCDF landscape.
* **Default:** unset: computed from a fixed Earth radius at the latitude of the southwest corner

year / month / day
~~~~~~~~~~~~~~~~~~
* **Description:** Reference date (at 00:00 UTC) of a standalone ``FireDomain`` created without one, i.e. when no ``loadData`` date has set it. ``startFire[date=...]`` and ``ISOdate`` count from this date.
* **Default:** ``2012``, ``1``, ``1``

normalScheme
~~~~~~~~~~~~
* **Description:** Scheme used to compute the outward unit normal vector at each ``FireNode``. Valid values: ``medians``, ``weightedMedians``, ``splines``, ``slopeCorrectedWeightedMedians``.
* **Default:** ``medians``

curvatureComputation
~~~~~~~~~~~~~~~~~~~~
* **Description:** Boolean (``0`` or ``1``) enabling local front curvature calculation at each ``FireNode``.
* **Default:** ``1``

curvatureScheme
~~~~~~~~~~~~~~~
* **Description:** Scheme used to compute front curvature when ``curvatureComputation=1``. Valid values: ``circumradius``, ``angle``.
* **Default:** ``circumradius``

smoothing
~~~~~~~~~
* **Description:** Read but not used by the current node update.
* **Default:** ``1``

relax
~~~~~
* **Description:** Relaxation factor (:math:`0\text{–}1`) that blends a node's previous speed and front depth with the newly computed ones.
* **Default:** ``0.5``

frontDepthComputation
~~~~~~~~~~~~~~~~~~~~~
* **Description:** Boolean (``0`` or ``1``) enabling local burning front depth computation along the normal direction.
* **Default:** ``0``

frontDepthScheme
~~~~~~~~~~~~~~~~
* **Description:** Scheme used to compute front depth when ``frontDepthComputation=1``. Valid values: ``normalDir`` (or ``normalDirection``), ``closest``. Other values are ignored.
* **Default:** ``normalDir``

initialFrontDepth
~~~~~~~~~~~~~~~~~
* **Description:** Initial front depth (in meters) assigned to newly created ``FireNode`` markers.
* **Default:** ``20``

minimalPropagativeFrontDepth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Front depth (in meters) with two uses. The arrival-time map (``BMap``) resolution is the larger of this value and ``spatialIncrement / √2``, unless ``arrivalMapResolution`` is set. With ``frontDepthComputation=1``, a ``FireNode`` whose front depth is at or below this value stops advancing.
* **Default:** ``10.``

arrivalMapResolution
~~~~~~~~~~~~~~~~~~~~
* **Description:** Cell size (in meters) of the arrival-time map, replacing the value derived from ``minimalPropagativeFrontDepth``. Must be positive.
* **Default:** unset

maxFrontDepth
~~~~~~~~~~~~~
* **Description:** Upper cap on computed front depth (in meters).
* **Default:** ``200.``

burningTresholdFlux
~~~~~~~~~~~~~~~~~~~
* **Description:** Heat flux threshold (in :math:`\text{W/m}^2`) above which a cell is considered actively burning when computing front depth.
* **Default:** ``10``

initialBurningDuration
~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Gives arrival times (in seconds) to the area inside an initial fire front: a point at distance ``d`` inside a front ignited at ``t`` gets ``t - d × initialBurningDuration / initialFrontDepth``. If ``initialFrontDepth`` is ``0`` or less, the whole inside gets ``t``.
* **Default:** ``30``

noInitialScan
~~~~~~~~~~~~~
* **Description:** When set (to any value), skips scanning the interior of initial fire fronts into the arrival-time burning map at startup. The scan is also skipped when ``BMapFiles`` is set.
* **Default:** *(unset)*

BMapFiles
~~~~~~~~~
* **Description:** NetCDF arrival-time map (as written by ``save[]``) loaded when the domain is created, from ``caseDirectory/ForeFireDataDirectory``. With a non-zero ``parallelInit``, ``.<domainID>.nc`` is appended to the name.
* **Default:** *(unset)*

bmapLayer
~~~~~~~~~
* **Description:** Boolean (``0`` or ``1``). Set to ``1`` to register the arrival-time burning map (``BMap``) as a queryable ``DataLayer``, read from Python with ``ff["BMap"]``. Not needed for ``plot[parameter=arrival_time]`` or :func:`pyforefire.run_ensemble`.
* **Default:** ``0``

max_inner_front_nodes_filter
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Threshold node count used when filtering small inner fronts.
* **Default:** ``50``


Files, Directories & Output Formatting
--------------------------------------

dumpMode
~~~~~~~~
* **Description:** Output format used by :ref:`print <cmd-print>`: ``ff`` (native ForeFire script format), ``json`` (Cartesian JSON), ``geojson`` (projected WGS84 GeoJSON), or ``kml`` (projected WGS84 KML).
* **Default:** ``ff``

defaultColormap
~~~~~~~~~~~~~~~
* **Description:** Default colormap used by :ref:`plot <cmd-plot>` when ``cmap=`` is omitted.
* **Default:** ``turbo``

.. _param-ISOdate:

ISOdate
~~~~~~~
* **Description:** ISO 8601 UTC date of the simulation. Set to the reference date by ``loadData`` with a date, then updated to the current simulation date after every ``step`` and ``goTo``, so ``*ISOdate*`` in a ``print`` filename gives the date of the front.
* **Default:** ``2012-01-01T00:00:00Z``

count
~~~~~
* **Description:** Integer counter incremented automatically every time ``print[filename]`` writes a file. Can be referenced in filenames via ``*count*``.
* **Default:** ``0``

caseDirectory
~~~~~~~~~~~~~
* **Description:** Base directory for resolving relative input paths (such as ``fuelsTableFile``).
* **Default:** Current working directory (``getenv("PWD")``, or ``.``)

.. _param-ForeFireDataDirectory:

ForeFireDataDirectory
~~~~~~~~~~~~~~~~~~~~~
* **Description:** Subdirectory inside ``caseDirectory`` searched for auxiliary ForeFire data files.
* **Default:** ``ForeFire``

experiment
~~~~~~~~~~
* **Description:** Experiment name prefix used when saving the arrival-time map via ``save[]`` (writes ``<fireOutputDirectory>/<experiment>.<domainID>.nc``).
* **Default:** ``ForeFire``

fireOutputDirectory
~~~~~~~~~~~~~~~~~~~
* **Description:** Directory where ``save[]`` and periodic automatic outputs write files. Note that ``print[filename]`` writes relative to the current working directory, not ``fireOutputDirectory``.
* **Default:** ``.``

outputFiles / outputsUpdate / debugFronts / bmapOutputUpdate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Automatic outputs. With ``outputsUpdate`` above ``0``, the front is written every ``outputsUpdate`` seconds to ``<caseDirectory>/<fireOutputDirectory>/<outputFiles>.<domainID>.<time>`` in the ``dumpMode`` format (set both before ``FireDomain``). With ``debugFronts`` not ``0``, the front is written to ``<caseDirectory>/<fireOutputDirectory>/<outputFiles>.<domainID>`` after every ``step``/``goTo``. With ``bmapOutputUpdate`` above ``0``, the arrival-time map is saved as by ``save[]`` (same file, overwritten) each time a ``step``/``goTo`` crosses a multiple of ``bmapOutputUpdate`` seconds.
* **Default:** ``output``, ``0``, ``0``, ``0``

.. _param-NetCDFfile:

NetCDFfile
~~~~~~~~~~
* **Description:** NetCDF landscape whose layers are loaded when a ``FireDomain`` is created, unless ``fftilesPath`` is set. Relative paths resolve under ``caseDirectory/ForeFireDataDirectory``. Set automatically by ``loadData[landscape.nc;date]``.
* **Default:** ``""`` (nothing loaded)


Atmospheric Coupling (Meso-NH) & MPI
------------------------------------

runmode
~~~~~~~
* **Description:** Execution mode, set by ForeFire when the domain is created: ``standalone``, ``coupled`` (Meso-NH sub-domain) or ``masterMNH``. In a script there is nothing to set: a standalone ``FireDomain`` writes ``standalone`` over any value other than ``masterMNH``.
* **Default:** ``standalone``

paramsFile / InitFile / InitFiles / InitTime / parallelInit
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Filenames and restart controls read by the Meso-NH coupling library (``CLibForeFire.cpp``) during coupled initialization—not used by standalone ``include[]``.
* **Default:** ``paramsFile = Params.ff``, ``InitFile = Init.ff``, ``InitFiles = output``, ``InitTime = 99999999999999``, ``parallelInit = 0``

atmoNX / atmoNY / atmoNZ
~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Grid dimensions of the atmospheric mesh. In standalone runs ``atmoNX`` × ``atmoNY`` sets the cells used to integrate fluxes over the domain. Set from the ``nx``/``ny`` dimensions by ``loadData[file.nc;date]`` and imposed by Meso-NH in coupled runs.
* **Default:** ``100``, ``100``, ``20``

MNHExchangeScalarLayersNames
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Comma-separated names of Meso-NH scalar fields; in coupled runs ForeFire creates one layer per name to receive them (``tools/htdocs/mnh3d.html`` displays them live).
* **Default:** ``plumeTopHeight,plumeBottomHeight,smokeAtGround,tke``

watchedProc / CommandOutputs / FireDomainOutputs / FireFrontOutputs / FireNodeOutputs / FDCellsOutputs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* **Description:** Verbose diagnostic flags and MPI rank filter, read when the domain is created (``watchedProc = -2`` silences all ranks, ``-1`` prints all ranks, ``n >= 0`` prints only rank ``n``; a standalone domain is rank ``0``).
* **Default:** ``watchedProc = -2``, ``CommandOutputs = 0``, ``FireDomainOutputs = 0``, ``FireFrontOutputs = 1``, ``FireNodeOutputs = 1``, ``FDCellsOutputs = 1``

Tiles & Checkpoints
-------------------

See :doc:`/user_guide/tiles`.

.. _param-fftilesPath:

fftilesPath
~~~~~~~~~~~
* **Description:** Directory or HTTP(S) URL with ``{lat}_{lon}.fftile`` files. Set it **before** ``FireDomain`` to read the landscape from tiles. Relative paths resolve under ``caseDirectory/ForeFireDataDirectory``. Tiles in ``ForeFireDataDirectory/tiles/`` take precedence. If it is not set, the domain loads NetCDF data instead. Set automatically by ``loadData[provider=...]``.
* **Default:** not set

fftileCacheSize
~~~~~~~~~~~~~~~
* **Description:** Number of decoded tiles kept in memory per domain, including tiles known to be missing.
* **Default:** ``8``


.. _param-fftileCacheDirectory:

fftileCacheDirectory
~~~~~~~~~~~~~~~~~~~~
* **Description:** Where tiles downloaded over HTTP(S) are cached, relative to ``caseDirectory/ForeFireDataDirectory`` unless absolute.
* **Default:** ``tiles-cache``

arrivalMapSubdivisions
~~~~~~~~~~~~~~~~~~~~~~
* **Description:** On a standalone tile domain, stores arrival times in sparse 0.1-degree blocks of N×N cells (``64``, ``128``, ``256`` or ``512``; 256 is about 43 m). Changes only the arrival map, not the fuel resolution or the front. Rejected on coupled or parallel domains. Set before ``FireDomain``.
* **Default:** ``0`` (regular arrival map)

.. _param-ffstateFile:

ffstateFile
~~~~~~~~~~~
* **Description:** ``.ffstate`` checkpoint used by ``loadState`` without ``file=``. In Meso-NH coupling, set it in ``Params.ff`` so the fire settings are prepared before the domain is built, then call ``loadState[mode=attach]``.
* **Default:** ``""``

Terrain-corrected normal scheme
-------------------------------

``normalScheme=slopeCorrectedWeightedMedians`` is an opt-in alternative to the
legacy defaults. It forms the horizontal weighted-median front normal, estimates
two altitude derivatives using centred differences at ``spatialIncrement`` (a
one-sided difference when one neighbour is unavailable), then projects onto the
estimated terrain tangent plane and normalizes. An axis without usable samples
uses zero derivative. Altitude sampling follows the domain coordinate frame,
including tile-backed terrain; no aspect layer is needed. This first version
samples altitude rather than using stored FFTile gradient components.

The model ``slope`` property is divided by the horizontal normal length only
for this scheme, preserving the convention of altitude change per horizontal
distance. Wind projection and node motion use the same stored normal. The new
scheme adds four altitude samples per normal update away from coverage edges,
and a square root/division per model slope lookup. Existing schemes and the
``medians`` default are unchanged. DEM resolution and derivative sampling scale
still affect results; spline normals are not modified by this option.
