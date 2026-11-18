.. _userguide-fuels-and-models:

Fuel tables
===========

Each model asks for the fuel properties it needs, with ``registerProperty("fuel.<column>")`` (see :doc:`core_concepts`). So the columns your fuel table needs depend on the model you pick. The models themselves, with their papers, are in :doc:`models`.

Loading a table
---------------

There are two ways:

- From a CSV file: ``setParameter[fuelsTableFile=fuels.csv]``. The default is ``fuels.ff``, resolved relative to ``caseDirectory/ForeFireDataDirectory``. Set ``ForeFireDataDirectory=.`` when the file sits directly in ``caseDirectory``.
- As a string in the ``fuelsTable`` parameter: from Python with ``ff["fuelsTable"] = ...``, inline in a script (:doc:`forefire_script`), or with a built-in table such as ``setParameter[fuelsTable=STDfarsiteFuelsTable]``.

Either way:

- Columns are separated by ``;``.
- The first line is a header that starts with ``Index``.
- Each integer in the landscape's ``fuel`` layer is looked up in the ``Index`` column. Index ``0`` is by convention non-burnable ground (water, rock, roads, buildings), with bed depth ``e = 0``.
- Only the ``fuel.*`` columns that the active models registered are read.

Columns per model
-----------------

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Model
     - Fuel columns
   * - ``Iso``
     - None. Spreads at ``Iso.speed`` (m/s) in every direction, e.g. ``setParameter[Iso.speed=0.5]``.
   * - ``WindDriven``
     - ``Index;vv_coeff``. The rate of spread is ``vv_coeff × normal wind``. Used in :doc:`python_arrays` and :doc:`ensembles`.
   * - ``Rothermel``, ``BalbiNov2011``, ``Balbi2020``, ``Balbi2015``
     - The 23-column Rothermel/Balbi table below (:ref:`rothermel-fuel-file`).
   * - ``RothermelAndrews2018``
     - Imperial columns from Andrews (2018): ``Index;fl1h_tac;fd_ft;SAVcar_ftinv;mdOnDry1h_r;fuelDens_lbft3;H_BTUlb;Dme_pc;totMineral_r;effectMineral_r``.
   * - ``Farsite``
     - Scott & Burgan (2005) columns, built in as ``setParameter[fuelsTable=STDfarsiteFuelsTable]``: ``Index;code;h1;h10;h100;lh;lw;dynamic;sav1;savlh;savlw;depth;xmext;heatd;heatl;desc``.

.. _rothermel-fuel-file:

The Rothermel/Balbi table (fuels.csv)
-------------------------------------

This is the format of ``tests/runff/fuels.csv`` and of the built-in ``STDlandIndexedFuelsTable``. Its 23 columns cover Rothermel (1972) and the Balbi propagation and flux models:

.. code-block:: text

   Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me
   1;500.0;500.0;0.1;0.7;5000.0;5000.0;0.5;0.8;0.2;8.3;1.0;300.0;70000.0;2300000.0;1.8e7;1800.0;1000.0;600.0;0.3;2.5e-5;3.0;0.3

.. list-table::
   :header-rows: 1
   :widths: 12 18 48 22

   * - Column
     - Units
     - Meaning
     - Used by
   * - ``Index``
     - integer
     - Fuel type ID, matching the ``fuel`` index raster (``0`` = non-burnable).
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
     - Dead fuel moisture on a dry-mass basis (``0.10`` for 10%).
     - ``Rothermel``, ``Balbi*``, ``ForeFireV1*``
   * - ``Ml``
     - ratio
     - Live fuel moisture on a dry-mass basis (e.g. ``0.5–2.0``).
     - ``BalbiNov2011``, ``ForeFireV1*``
   * - ``sd``
     - :math:`\text{m}^{-1}`
     - Surface-area-to-volume ratio of dead particles (e.g. ``2000–6000``).
     - ``Rothermel``, ``Balbi*``, ``ForeFireV1*``
   * - ``sl``
     - :math:`\text{m}^{-1}`
     - Surface-area-to-volume ratio of live particles.
     - ``BalbiNov2011``
   * - ``e``
     - :math:`\text{m}`
     - Fuel bed depth. ``0`` stops propagation.
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
     - Anderson residence-time constant :math:`\tau_0`; residence time is :math:`\tau = \tau_0 / s_d` (typically ``70000–75590``).
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
     - Specific heat of fuel particles (typically ``1800–2030``).
     - ``Balbi*``
   * - ``Cpa``
     - :math:`\text{J/(kg}\cdot\text{K)}`
     - Specific heat of air (typically ``1000–1150``).
     - ``Rothermel`` (registered)
   * - ``Ti``
     - :math:`\text{K}`
     - Ignition temperature :math:`T_i` (typically ``600``).
     - ``Balbi*``
   * - ``X0``
     - ratio (0–1)
     - Radiant heat fraction :math:`\chi_0` (typically ``0.3``).
     - ``Balbi*``
   * - ``r00``
     - :math:`\text{m}`
     - Balbi radiative length scale :math:`r_{00}` (typically ``2.5e-5``).
     - ``Balbi*``
   * - ``Blai``
     - ratio
     - Leaf area index (typically ``2.0–4.0``).
     - ``BalbiNov2011``, ``ForeFireV1*``
   * - ``me``
     - ratio (0–1)
     - Dead fuel moisture of extinction (typically ``0.25–0.30``).
     - ``Rothermel``, ``ForeFireV1*``

``BurnUpHeatFlux`` also registers ``fuel.w``, the characteristic burn time in seconds (see :doc:`models`).

Checking a table against a landscape
------------------------------------

``forefire-validate`` (installed with ``pip install forefire``) checks that every fuel index in a NetCDF landscape is defined in your table:

.. code-block:: bash

   forefire-validate data.nc fuels.csv

If an index is missing, it tells you which one, before you spend time on a run.

More fuel tables
----------------

- In the repository: ``tests/runff/fuels.csv``, and the built-in ``STDlandIndexedFuelsTable`` and ``STDfarsiteFuelsTable`` in ``src/SimulationParameters.cpp``.
- ``tools/wildfire_ROS_models/`` has Python versions of ``Rothermel1972``, ``RothermelAndrews2018``, ``Balbi2020`` and the ``Cruz`` crown fire models, with polar plots, Sobol sensitivity analyses and fuel databases in ``fuels_database.py``.
