.. _userguide-core-concepts:

Core concepts
=============

ForeFire moves a fire front over a 2D surface and computes what the fire releases into the atmosphere behind it. Three ideas cover how it works: markers on the front, data layers, and the arrival-time map. Adding a model of your own is in :ref:`custom-cpp-models`.

Markers on the front
--------------------

A grid solver updates every cell at a global time step :math:`\Delta t`. ForeFire instead follows the fire edge with markers and updates each one when it is due.

A ``FireFront`` is a closed contour, stored as a doubly linked list of ``FireNode`` markers. Each node holds its position :math:`\vec{x}_i = (x, y, z)`, velocity :math:`\vec{v}_i`, outward normal :math:`\vec{n}_i`, curvature :math:`\kappa_i`, front depth and time :math:`t_i`.

.. _front-winding:

The order of the nodes sets the direction of the normal. Clockwise nodes give an outward normal and a front that expands. Counter-clockwise nodes give an inward normal and a front that contracts, which is how an unburned island inside an outer front is drawn.

Updates live in a priority queue, the ``TimeTable``. When a node comes up at time :math:`t_i`:

1. It moves to its scheduled position, a fixed step ``spatialIncrement`` (:math:`\Delta x`) away.
2. ``FireDomain`` keeps the front consistent. If two neighbours are more than twice ``perimeterResolution`` apart, a node is inserted between them. Nodes closer than ``perimeterResolution`` are merged. Two colliding front segments merge.
3. The node computes its normal (``normalScheme``: ``medians``, ``weightedMedians`` or ``splines``) and curvature (``curvatureScheme``: ``circumradius`` or ``angle``).
4. It asks the ``PropagationModel`` for its rate of spread :math:`R_i` along :math:`\vec{n}_i` and schedules its next update at :math:`t_i + \Delta x / R_i`.

Each node advances at its own pace, :math:`\Delta t_i = \Delta x / R_i`. Head-fire nodes update often, flank and backing nodes rarely.

Data layers and the map of models
---------------------------------

All data in a ``FireDomain`` sits in named ``DataLayer`` objects, managed by the ``DataBroker``.

Environmental layers hold elevation (``altitude``), slope along the front normal (``slope``), fuel indices (``fuel``) and wind (``windU``, ``windV``, and ``normalWind`` projected on the normal). They come from NumPy arrays, NetCDF files, tiles or a coupled atmospheric model.

Each computed quantity, the propagation speed or a flux such as ``heatFlux`` or ``vaporFlux``, also has its own layer. With one model for the whole domain, setting ``propagationModel`` (or calling ``addLayer``) creates a uniform layer.

A layer can also store a map of model indices, so different places use different models. Think of vegetation next to an industrial zone, or a volcano with separate crater and lava models:

.. figure:: /_static/images/mapOfModels.svg
   :alt: Different models applied in different regions of the domain
   :align: center
   :width: 65%

   Models A, B and C each cover part of the domain.

.. figure:: /_static/images/mapOfModelsMap.svg
   :alt: Discretized index map selecting which model applies in each cell
   :align: center
   :width: 65%

   Each model has an integer index, and the layer stores the grid of indices.

Arrival-time map and fluxes
---------------------------

Propagation works on markers along a line. An atmospheric model needs fluxes per grid cell: heat in :math:`\text{W/m}^2`, chemical species in :math:`\text{kg/m}^2\text{/s}`.

The burning map (``BMap``) connects the two:

1. As markers sweep the landscape, ``FireDomain`` writes the arrival time :math:`t^i(x, y)` into a fine 2D matrix, usually at least 10 times finer than the atmospheric grid.
2. Each ``FluxLayer`` maps the coarse atmospheric cells onto the ``BMap`` sub-cells:

.. figure:: /_static/images/fluxComputation.svg
   :alt: Coarse atmospheric cells overlaying the fine arrival-time burning map
   :align: center
   :width: 65%

   Four atmospheric cells (thick lines) over the front (dark grey) and the arrival-time matrix ``BMap`` (thin lines).

3. The flux of a cell at time :math:`t` is the sum over its burned sub-cells (:math:`t \ge t^i`). The ``FluxModel`` computes each one from the time since ignition, :math:`t - t^i`. ``heatFluxNominal`` releases a constant flux over the residence time :math:`\tau = \tau_0 / s_d`; ``BurnUpHeatFlux`` decays the fuel mass exponentially.
