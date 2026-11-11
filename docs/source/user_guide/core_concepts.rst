.. _userguide-core-concepts:

Core Concepts & Architecture
============================

ForeFire is designed to propagate a reactive interface (a fire front) over a 2D surface while computing the physical and chemical fluxes released into the atmosphere behind the front. Understanding its four core abstractions—**Lagrangian Discrete-Event Front Tracking**, **Data Layers & Maps of Models**, **Arrival-Time Flux Integration**, and **Pluggable C++ Models**—makes it straightforward to configure simulations and extend the solver.

1. Discrete-Event Lagrangian Front Tracking
-------------------------------------------

Unlike Eulerian grid solvers that advance every cell of a domain at a fixed global time step :math:`\Delta t`, ForeFire tracks the fire edge using **Lagrangian markers** driven by a **Discrete-Event Simulation (DEVS)** scheduler:

* **FireFront & FireNode:**
  A ``FireFront`` is a closed contour represented as a doubly linked list of ``FireNode`` markers. Each ``FireNode`` stores its position :math:`\vec{x}_i = (x, y, z)`, velocity vector :math:`\vec{v}_i`, outward unit normal :math:`\vec{n}_i`, local curvature :math:`\kappa_i`, front depth, and timestamp :math:`t_i`.

  * **Front orientation (winding):** The order of nodes along a ``FireFront`` defines whether it expands or shrinks. Nodes ordered **clockwise** produce an outward normal and an **expanding** fire perimeter; nodes ordered **counter-clockwise** produce an inward normal and a **contracting** front (used for unburned islands nested inside an outer ``FireFront``).

* **Event-Driven TimeTable (`Simulator` & `TimeTable`):**
  ForeFire maintains a priority queue (``TimeTable``) of future events ordered chronologically. When a ``FireNode`` is processed at time :math:`t_i`:

  1. It moves to its previously scheduled position by a fixed spatial step ``spatialIncrement`` (:math:`\Delta x`).
  2. ``FireDomain`` enforces topological consistency: if adjacent nodes separate beyond ``perimeterResolution``, intermediate nodes are inserted; if two front segments collide, they merge.
  3. The node computes its local normal :math:`\vec{n}_i` (using ``normalScheme``: ``medians``, ``weightedMedians``, or ``splines``) and curvature :math:`\kappa_i` (using ``curvatureScheme``: ``circumradius`` or ``angle``).
  4. It queries the active ``PropagationModel`` for its rate of spread :math:`R_i` along :math:`\vec{n}_i`, computes its next update time :math:`t_i + \Delta x / R_i`, and schedules itself back into the ``TimeTable``.

  Because each marker advances at its own pace :math:`\Delta t_i = \Delta x / R_i`, fast-moving head fire nodes update frequently while slow flank or backing nodes update rarely.

2. DataBroker, DataLayers, and the Map of Models
------------------------------------------------

All environmental and model data inside a ``FireDomain`` are managed by the ``DataBroker`` as named ``DataLayer`` objects:

* **Environmental DataLayers:** provide elevation (``altitude``), derived terrain slope along the front normal (``slope``), fuel type indices (``fuel``), wind components (``windU``, ``windV``, and projected ``normalWind``), or dynamic fields from NumPy arrays, NetCDF files, or a coupled atmospheric model.
* **Propagative and Flux Layers (Map of Models):**
  Every computed property—propagation velocity or a surface flux such as ``heatFlux`` or ``vaporFlux``—is backed by a dedicated layer. When a single model applies across the entire domain, setting ``propagationModel`` (or calling ``addLayer``) creates a uniform model layer automatically.

  For heterogeneous domains (for example, wildland vegetation adjacent to an industrial zone, or a volcano with distinct crater and lava emission models), a layer can store a spatial **map of model indices**, routing each location to a different model implementation:

.. figure:: /_static/images/mapOfModels.svg
   :alt: Different models applied in different regions of the domain
   :align: center
   :width: 65%

   Different models (A, B, C) can be assigned to distinct areas of the simulation domain.

.. figure:: /_static/images/mapOfModelsMap.svg
   :alt: Discretized index map selecting which model applies in each cell
   :align: center
   :width: 65%

   Each model is registered with an integer index, and the layer stores the spatial grid of model indices.

3. Arrival-Time Map (``BMap``) and Flux Computation
---------------------------------------------------

While propagation models act on 1D Lagrangian markers along the fire edge, atmospheric coupling and emission inventories require 2D surface fluxes (such as heat in :math:`\text{W/m}^2` or chemical species in :math:`\text{kg/m}^2\text{/s}`) integrated over atmospheric grid cells.

ForeFire bridges these scales through the **Burning Map (``BMap``)**:

1. As ``FireNode`` markers sweep across the landscape, ``FireDomain`` records the fire arrival time :math:`t^i(x, y)` on a high-resolution 2D matrix (typically at least 10 times finer than the atmospheric grid resolution).
2. Each ``FluxLayer`` maps the coarse atmospheric cells onto the underlying ``BMap`` sub-cells:

.. figure:: /_static/images/fluxComputation.svg
   :alt: Coarse atmospheric cells overlaying the fine arrival-time burning map
   :align: center
   :width: 65%

   Four coarse atmospheric cells (thick lines) over the fire front contour (dark grey) and the finely resolved arrival-time matrix ``BMap`` (thin lines).

3. To compute the total flux for an atmospheric cell at time :math:`t`, ForeFire sums the contributions of all ``BMap`` sub-cells inside it. For each sub-cell where the fire has arrived (:math:`t \ge t^i`), the active ``FluxModel`` evaluates the flux as a function of the elapsed time since ignition :math:`t - t^i` (for example, constant release over the flame residence time :math:`\tau = \tau_0 / s_d` in ``heatFluxNominal``, or exponential fuel mass decay in ``BurnUpHeatFlux``).

.. _custom-cpp-models:

4. Implementing a New Model in C++
----------------------------------

Propagation models (in ``src/propagation/``) and flux models (in ``src/flux/``) inherit from ``ForeFireModel`` via ``PropagationModel`` or ``FluxModel``. Adding a new model requires only a single ``.cpp`` file.

How Property Registration Works
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In its constructor, a model declares which environmental or fuel properties it needs by calling ``registerProperty("<name>")``. This returns an integer index into the ``double* valueOf`` array that the ``DataBroker`` populates before calling ``getSpeed(valueOf)`` or ``getValue(valueOf, bt, et, at)``:

.. list-table:: Built-in properties available to ``registerProperty``
   :header-rows: 1
   :widths: 28 72

   * - Property Name
     - Description
   * - ``fuel.<Column>``
     - Any column from the fuel table (e.g., ``fuel.Rhod``, ``fuel.sd``, ``fuel.Md``, ``fuel.DeltaH``).
   * - ``altitude``
     - Terrain elevation in meters (`m`).
   * - ``slope``
     - Terrain slope :math:`\tan\alpha` in the outward normal direction of the ``FireNode`` (propagation models).
   * - ``windU``, ``windV``
     - Zonal (West–East) and meridional (South–North) wind components (`m/s`).
   * - ``normalWind``
     - Wind projected onto the outward normal vector :math:`\vec{u} \cdot \vec{n}` (`m/s`, propagation models).
   * - ``frontDepth``
     - Local depth of the burning front normal to the perimeter (`m`, requires ``frontDepthComputation=1``).
   * - ``curvature``
     - Local front curvature :math:`\kappa` (requires ``curvatureComputation=1``).

Minimal Propagation Model Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For example, to implement a propagation model :math:`R = A \frac{\max(0, \vec{u}\cdot\vec{n})}{1 + M_d}`, copy ``src/propagation/SamplePropagationModel.cpp``:

.. code-block:: cpp

   #include "../PropagationModel.h"
   #include "../FireDomain.h"

   namespace libforefire {

   class MyWindMoistureModel : public PropagationModel {
       static const std::string name;
       static int isInitialized;

       size_t normalWind;
       size_t Md;
       double coeffA;

   public:
       MyWindMoistureModel(const int& mindex = 0, DataBroker* db = 0)
           : PropagationModel(mindex, db) {
           // 1. Register environmental and fuel properties
           normalWind = registerProperty("normalWind");
           Md = registerProperty("fuel.Md");

           if (numProperties > 0) properties = new double[numProperties];
           dataBroker->registerPropagationModel(this);

           // 2. Read optional parameter from SimulationParameters
           coeffA = 0.1;
           if (params->isValued("MyWindMoisture.A"))
               coeffA = params->getDouble("MyWindMoisture.A");
       }

       std::string getName() { return name; }

       // 3. Compute Rate of Spread (m/s) from the populated valueOf array
       double getSpeed(double* valueOf) {
           double u = valueOf[normalWind] > 0. ? valueOf[normalWind] : 0.;
           return coeffA * u / (1. + valueOf[Md]);
       }
   };

   const std::string MyWindMoistureModel::name = "MyWindMoisture";
   PropagationModel* getMyWindMoistureModel(const int& idx, DataBroker* db) {
       return new MyWindMoistureModel(idx, db);
   }
   int MyWindMoistureModel::isInitialized =
       FireDomain::registerPropagationModelInstantiator(name, getMyWindMoistureModel);

   } // namespace libforefire

Once placed in ``src/propagation/`` and recompiled with CMake, the model is immediately selectable via ``setParameter[propagationModel=MyWindMoisture]``. Flux models follow the exact same pattern in ``src/flux/``, inheriting from ``FluxModel`` and implementing ``getValue(double* valueOf, const double& bt, const double& et, const double& at)``.
