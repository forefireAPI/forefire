.. _userguide-simulation-inputs:

What a simulation needs
=======================

ForeFire moves a line of points across a map. Each point keeps asking one question: how fast does the fire go here, in the direction I am facing? The propagation model answers it from what lies under the point (fuel, slope) and what blows over it (wind). A simulation is everything that question needs, plus a place to start and a way to see the result.

This page builds one up, a piece at a time. Every script below runs as-is with ``forefire -i``, or line by line with ``ff.execute(...)`` in Python.

The smallest fire
-----------------

.. code-block:: none

   setParameters[propagationModel=Iso;Iso.speed=0.5]
   FireDomain[sw=(0,0,0);ne=(2000,2000,0);t=0]
   startFire[loc=(1000,1000,0);t=0]
   step[dt=600]
   print[]

- The **domain** is the rectangle the fire lives in, in metres (:ref:`FireDomain <cmd-FireDomain>`).
- The **model** is the rule that gives the speed. ``Iso`` ignores everything and spreads at ``Iso.speed`` m/s.
- The **ignition** is where the fire starts (:ref:`startFire <cmd-startFire>`).
- :ref:`step <cmd-step>` advances time and :ref:`print <cmd-print>` writes the front.

The fire starts as a small triangle and, after ten minutes, is a circle about 700 m across. Nothing in the domain tells the points apart, so nothing gives the fire any other shape. (The warning about ``fuels.ff`` is harmless: ``Iso`` reads no fuel.)

Something to burn
-----------------

A physical model such as ``Rothermel`` reads the fuel under each point. Fuel comes in two parts that are easy to confuse:

- The **fuel map** says *which* fuel is where: one integer per cell. It is part of the landscape.
- The **fuel table** says what each integer *means*: bed depth, load, moisture and whatever else the model reads (:doc:`fuels_and_models`). It belongs to the case, so the same map can run with drier or wetter fuel.

Here the table is one built into ForeFire, and ``fuel=2`` puts fuel ``2`` everywhere, a map with a single value:

.. code-block:: none
   :emphasize-lines: 1

   setParameters[propagationModel=Rothermel;fuelsTable=STDlandIndexedFuelsTable;fuel=2]
   FireDomain[sw=(0,0,0);ne=(2000,2000,0);t=0]
   startFire[loc=(1000,1000,0);t=0]
   step[dt=600]
   print[]

Still a circle, and a slow one: it grows about 20 m in ten minutes. The fuel is the same everywhere and nothing pushes the fire, but the speed now comes from the fuel. Change a value in the table and the circle grows faster or slower. Fuel ``0`` means nothing burns there (water, rock, roads), and it is what you get if you give no fuel at all.

Wind
----

.. code-block:: none
   :emphasize-lines: 2

   setParameters[propagationModel=Rothermel;fuelsTable=STDlandIndexedFuelsTable;fuel=2]
   setParameters[windU=5;windV=0]
   FireDomain[sw=(0,0,0);ne=(2000,2000,0);t=0]
   startFire[loc=(1000,1000,0);t=0]
   step[dt=600]
   print[]

``windU`` blows west to east and ``windV`` south to north, in m/s. In ten minutes the head of the fire runs about 270 m east, more than ten times further than without wind, while the back does not move. Each point feels only the wind along the direction it faces.

This wind is the same everywhere. Over real terrain it is not: valleys channel it, ridges speed it up. ForeFire handles that with a **wind response**, computed in advance for the terrain. It stores how the ground turns and scales a wind, so you give one vector and get a field:

.. code-block:: none

   trigger[wind;vel=(5,0,0)]
   trigger[wind;vel=(0,8,0)]@t=3600

Each ``trigger[wind]`` sets the vector, and the response spreads it over the map. That is how a forecast or a wind shift drives a real case. The response comes with the landscape (below). Without one, ``trigger[wind]`` has nothing to act on.

Coupled with Meso-NH (:doc:`mesonh`), there is no vector to set: the atmospheric model computes the wind and the fire feeds its heat back.

Terrain
-------

Fire runs faster uphill. Give ForeFire the elevation as an ``altitude`` layer and it works out the slope under each point, along the direction the point is facing. Without it, the ground is flat.

Elevation cannot be typed in one line the way fuel and wind were above. It needs a map, and so does a fuel map that is not uniform. That map is the landscape.

The landscape
-------------

The fuel map, the elevation and the wind response come together, from one of three sources:

- **Made-up landscapes in Python** (:doc:`python_arrays`): NumPy arrays you build yourself. For idealized cases, for learning, and for generating data in bulk.
- **Your own rasters** (:doc:`landscape_file`): one NetCDF file holding the domain and its layers, built from your data.
- **Real terrain from tiles** (:doc:`tiles`): ready-made fuel, elevation and wind response over the globe, downloaded as the domain needs them.

Loading a landscape file or tiles with :ref:`loadData <cmd-loadData>` also creates the domain, so you skip ``FireDomain``. And once the domain sits on the globe, ``startFire`` can take ``lonlat=(lon,lat,z)`` instead of metres.

Seeing the result
-----------------

- :ref:`print <cmd-print>` writes the front: text, GeoJSON or KML.
- :ref:`save <cmd-save>` writes the arrival-time map, when the fire reached each cell, to NetCDF.
- :ref:`saveState <cmd-saveState>` writes a ``.ffstate`` checkpoint to resume from (:doc:`tiles`).

Add ``@t=...`` after a command to run it later instead of now, for example a ``print`` every hour (:doc:`forefire_script`).

At a glance
-----------

.. list-table::
   :header-rows: 1
   :widths: 16 18 66

   * - Piece
     - Needed?
     - Comes from
   * - Domain
     - Always
     - ``FireDomain``, or ``loadData`` on a landscape file, tiles or a ``.ffstate``.
   * - Model
     - Always
     - ``setParameter[propagationModel=...]``. All models and their papers: :doc:`models`.
   * - Ignition
     - Always
     - ``startFire`` at ``loc=`` (metres) or ``lonlat=``. Other shapes: draw the front by hand (:doc:`forefire_script`).
   * - Fuel map
     - Most models
     - The landscape, or ``setParameter[fuel=...]`` for one fuel everywhere.
   * - Fuel table
     - Most models
     - ``fuelsTableFile`` (CSV), ``fuelsTable`` (string or built-in). Columns per model: :ref:`fuel-columns-per-model`. ``Iso`` needs no fuel.
   * - Elevation
     - Optional
     - The landscape. Flat without it.
   * - Wind
     - Wind-driven models
     - ``trigger[wind]`` over the landscape's wind response, ``windU``/``windV`` parameters for a uniform field, or Meso-NH.
   * - Output
     - To see results
     - ``print``, ``save``, ``saveState``.
