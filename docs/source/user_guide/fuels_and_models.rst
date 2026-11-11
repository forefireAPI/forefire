.. _userguide-fuels-and-models:

Fuel Tables & Model Parameterization
====================================

ForeFire is a general Rate of Spread (ROS) and flux solver. When a model is instantiated, it informs the ``DataBroker`` which fuel properties it needs via ``registerProperty("fuel.<column>")`` (see :doc:`core_concepts`). Consequently, **the required columns in your fuel table depend on the model you select**.

.. seealso::
   For the list of propagation and flux models with published scientific references and how to select each one, see :doc:`models`.

How Fuel Tables Are Loaded
--------------------------

You can supply a fuel table in two ways:

1. **External CSV file** (``fuelsTableFile``):
   Set ``setParameter[fuelsTableFile=fuels.csv]`` (default ``fuels.ff``, resolved relative to ``caseDirectory/ForeFireDataDirectory``; set ``ForeFireDataDirectory=.`` when the file sits directly in ``caseDirectory``).
2. **Inline string parameter** (``fuelsTable``):
   Pass the CSV content directly into the ``fuelsTable`` parameter (for example from Python via ``ff["fuelsTable"] = ...``, or using one of the built-in standard tables such as ``setParameter[fuelsTable=STDfarsiteFuelsTable]``).

In both cases:

* The delimiter is a **semicolon** (``;``).
* The first line must be a header row starting with ``Index``.
* Each integer value in the landscape's ``fuel`` index layer is looked up against the ``Index`` column. Index ``0`` is conventionally reserved for non-burnable ground (water, bare rock, roads, or buildings, with bed depth ``e = 0``).
* Only the ``fuel.*`` columns registered by the active propagation and flux models are read from the table.

Model Families and Their Fuel Columns
-------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Model(s)
     - Required Fuel Columns
   * - ``Iso``
     - **None.** Spreads at the constant speed given by ``setParameter[Iso.speed=0.5]`` (m/s) in all directions.
   * - ``WindDriven``
     - ``Index;vv_coeff``: the rate of spread is ``vv_coeff × normal wind`` (idealized wind-driven model used in :doc:`python_arrays` and :doc:`ensembles`).
   * - ``Rothermel``, ``BalbiNov2011``, ``Balbi2020``, ``Balbi2015``
     - The standard 23-column Rothermel/Balbi fuel table detailed in :ref:`rothermel-fuel-file` below.
   * - ``RothermelAndrews2018``
     - Imperial Rothermel columns from Andrews (2018): ``Index;fl1h_tac;fd_ft;SAVcar_ftinv;mdOnDry1h_r;fuelDens_lbft3;H_BTUlb;Dme_pc;totMineral_r;effectMineral_r``.
   * - ``Farsite``
     - Multi-class Scott & Burgan (2005) columns (built into ForeFire via ``setParameter[fuelsTable=STDfarsiteFuelsTable]``): ``Index;code;h1;h10;h100;lh;lw;dynamic;sav1;savlh;savlw;depth;xmext;heatd;heatl;desc``.

----

.. _rothermel-fuel-file:

The Standard Rothermel / Balbi Fuel Table (fuels.csv)
-----------------------------------------------------

The standard ``fuels.csv`` format (used in ``tests/runff/fuels.csv`` and ``STDlandIndexedFuelsTable``) provides 23 semicolon-separated columns covering both Rothermel (1972) and the Balbi physical propagation and flux models:

.. code-block:: text

   Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me
   1;500.0;500.0;0.1;0.7;5000.0;5000.0;0.5;0.8;0.2;8.3;1.0;300.0;70000.0;2300000.0;1.8e7;1800.0;1000.0;600.0;0.3;2.5e-5;3.0;0.3

Column Reference
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 12 18 48 22

   * - Column
     - Units
     - Physical Meaning
     - Primary Models
   * - ``Index``
     - integer
     - Unique non-negative fuel type identifier matching the ``fuel`` index raster (``0`` = non-burnable).
     - All
   * - ``Rhod``
     - :math:`\text{kg/m}^3`
     - Oven-dry dead fuel particle density (typically ``400–720``).
     - ``Rothermel``, ``Balbi*``
   * - ``Rhol``
     - :math:`\text{kg/m}^3`
     - Live fuel particle density (typically ``500–720``).
     - ``BalbiNov2011``
   * - ``Md``
     - ratio (0–1)
     - Dead fuel moisture content on a dry-mass basis (e.g., ``0.10`` for 10%).
     - ``Rothermel``, ``Balbi*``, ``ForeFireV1*``
   * - ``Ml``
     - ratio
     - Live fuel moisture content on a dry-mass basis (e.g., ``0.5–2.0``).
     - ``BalbiNov2011``, ``ForeFireV1*``
   * - ``sd``
     - :math:`\text{m}^{-1}`
     - Surface-area-to-volume ratio of dead fuel particles (e.g., ``2000–6000``).
     - ``Rothermel``, ``Balbi*``, ``ForeFireV1*``
   * - ``sl``
     - :math:`\text{m}^{-1}`
     - Surface-area-to-volume ratio of live fuel particles.
     - ``BalbiNov2011``
   * - ``e``
     - :math:`\text{m}`
     - Fuel bed depth (height of the surface fuel layer; ``0`` stops propagation).
     - ``Rothermel``, ``Balbi*``
   * - ``Sigmad``
     - :math:`\text{kg/m}^2`
     - Oven-dry dead fuel load per unit ground area.
     - ``Rothermel``, ``Balbi*``, ``BurnUpHeatFlux``, ``ForeFireV1*``
   * - ``Sigmal``
     - :math:`\text{kg/m}^2`
     - Live fuel load per unit ground area.
     - ``BalbiNov2011``, ``ForeFireV1*``
   * - ``stoch``
     - ratio
     - Stoichiometric air-to-fuel mass ratio :math:`s_t` (typically ``8.3–17``).
     - ``BalbiNov2011``
   * - ``RhoA``
     - :math:`\text{kg/m}^3`
     - Ambient air density :math:`\rho_a` (typically ``1.0–1.225``).
     - ``Balbi*``
   * - ``Ta``
     - :math:`\text{K}`
     - Ambient air temperature :math:`T_a` (typically ``300``).
     - ``Balbi*``
   * - ``Tau0``
     - :math:`\text{s/m}`
     - Anderson flame residence-time constant :math:`\tau_0` (residence time :math:`\tau = \tau_0 / s_d`, typically ``70000–75590``).
     - ``Balbi*``, ``heatFluxNominal``
   * - ``Deltah``
     - :math:`\text{J/kg}`
     - Latent heat of water vaporization / fuel drying :math:`\Delta h` (typically ``2.3e6``).
     - ``Balbi*``
   * - ``DeltaH``
     - :math:`\text{J/kg}`
     - Low heat of combustion of dry fuel :math:`\Delta H` (typically ``1.5e7–1.86e7``).
     - ``Rothermel``, ``Balbi*``, ``BurnUpHeatFlux``, ``ForeFireV1*``
   * - ``Cp``
     - :math:`\text{J/(kg}\cdot\text{K)}`
     - Specific heat capacity of fuel particles (typically ``1800–2030``).
     - ``Balbi*``
   * - ``Cpa``
     - :math:`\text{J/(kg}\cdot\text{K)}`
     - Specific heat capacity of air (typically ``1000–1150``).
     - ``Rothermel`` (registered)
   * - ``Ti``
     - :math:`\text{K}`
     - Fuel ignition temperature :math:`T_i` (typically ``600``).
     - ``Balbi*``
   * - ``X0``
     - ratio (0–1)
     - Radiant heat fraction factor :math:`\chi_0` (typically ``0.3``).
     - ``Balbi*``
   * - ``r00``
     - :math:`\text{m}`
     - Balbi radiative length-scale parameter :math:`r_{00}` (typically ``2.5e-5``).
     - ``Balbi*``
   * - ``Blai``
     - ratio
     - Leaf Area Index (LAI, typically ``2.0–4.0``).
     - ``BalbiNov2011``, ``ForeFireV1*``
   * - ``me``
     - ratio (0–1)
     - Dead fuel moisture content of extinction (typically ``0.25–0.30``).
     - ``Rothermel``, ``ForeFireV1*``

.. note::
   ``BurnUpHeatFlux`` additionally registers ``fuel.w`` (characteristic burn time in seconds), ``fuel.Sigmad``, and ``fuel.DeltaH`` (see :doc:`models`).

Validating Your Fuel Table Against a Landscape
----------------------------------------------

Before running a simulation with a NetCDF landscape file, run ``forefire-validate`` (installed with ``pip install forefire``) to verify that every integer fuel index present in the raster is defined in your CSV table:

.. code-block:: bash

   forefire-validate data.nc fuels.csv

Reference Fuel Libraries
------------------------

- **In-tree test cases:** ``tests/runff/fuels.csv`` and the built-in ``STDlandIndexedFuelsTable`` / ``STDfarsiteFuelsTable`` tables in ``src/SimulationParameters.cpp``.
- **Companion ROS toolbox:** ``tools/wildfire_ROS_models/`` in the repository includes Python implementations, polar plots, and Sobol sensitivity analyses for ``Rothermel1972``, ``RothermelAndrews2018``, ``Balbi2020``, and ``Cruz`` crown fire models, along with standard fuel databases in ``fuels_database.py``.
