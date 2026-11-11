.. _userguide-models:

Models and References
=====================

These are the propagation and flux models registered in ForeFire, along with
their published scientific references, required fuel table columns or dynamic
layers, and simulation parameters. The fuel file format is described in
:doc:`fuels_and_models`.

Propagation models
------------------

A propagation model gives the rate of spread of each point of the front. You
select it by its registered name with the
:ref:`propagationModel <param-propagationModel>` parameter.

Rothermel
^^^^^^^^^

*   **What it is:** the Rothermel semi-empirical surface spread model, with
    the wind speed limit.
*   **References:**

    *   Rothermel, R.C. (1972). *A mathematical model for predicting fire
        spread in wildland fuels*. USDA Forest Service Research Paper
        INT-115. https://doi.org/10.2737/INT-RP-115
    *   For the wind limit: Andrews, P.L., Cruz, M.G., Rothermel, R.C. (2013).
        *Examination of the wind speed limit function in the Rothermel
        surface fire spread model*. International Journal of Wildland Fire
        22, 959–969. https://doi.org/10.1071/WF12122

*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=Rothermel]

    It reads the standard fuel table described in
    :ref:`The Standard Rothermel / Balbi Fuel Table <rothermel-fuel-file>`,
    plus ``slope`` and ``normalWind``. ``windReductionFactor`` (default
    ``0.4``) scales the wind it uses; when ``windReductionFactor < 1.0``, the
    Andrews et al. (2013) wind speed limit is applied. Setting
    ``RothermelLoggerCSVPath`` logs inputs and rates of spread to a CSV file.

RothermelAndrews2018
^^^^^^^^^^^^^^^^^^^^

*   **What it is:** the Rothermel model as written in Tables 5 and 6 of
    Andrews (2018).
*   **Reference:** Andrews, P.L. (2018). *The Rothermel surface fire spread
    model and associated developments: A comprehensive explanation*. USDA
    Forest Service General Technical Report RMRS-GTR-371.
    https://doi.org/10.2737/RMRS-GTR-371
*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=RothermelAndrews2018]

    Its fuel table uses imperial columns: ``fl1h_tac``, ``fd_ft``,
    ``SAVcar_ftinv``, ``mdOnDry1h_r``, ``fuelDens_lbft3``, ``H_BTUlb``,
    ``Dme_pc``, ``totMineral_r`` and ``effectMineral_r``, along with
    ``slope`` and ``normalWind``.

Balbi2020
^^^^^^^^^

*   **What it is:** the convective–radiative Balbi model, solved
    iteratively (:math:`R = R_b + R_c + R_r`).
*   **Reference:** Balbi, J.H., Chatelon, F.J., Morvan, D., Rossi, J.L.,
    Marcelli, T., Morandini, F. (2020). *A convective–radiative propagation
    model for wildland fires*. International Journal of Wildland Fire 29(8),
    723–738. https://doi.org/10.1071/WF19103 (open access:
    https://hal.science/hal-03251626)
*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=Balbi2020]

    It registers ``slope``, ``normalWind``, ``moisture``, ``temperature`` and
    the standard 20 ``fuel.*`` columns in
    :ref:`The Standard Rothermel / Balbi Fuel Table <rothermel-fuel-file>`
    (from ``fuel.Rhod`` to ``fuel.Blai``). The air heat capacity is fixed at
    ``1150`` J/kg/K in the spread computation.

BalbiNov2011, BalbiNov2011Curv and BalbiNov2011TMdMl
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*   **What they are:** the quasi-steady radiative Balbi physical spread model
    with the live/dead fuel moisture damping factor :math:`\xi`, plus two
    variants that include front curvature or dynamic temperature and moisture
    layers.
*   **References:**

    *   Base radiative formulation: Balbi, J.H., Morandini, F., Silvani, X.,
        Filippi, J.B., Rinieri, F. (2009). *A physical model for wildland
        fires*. Combustion and Flame 156(12), 2217–2230.
        https://doi.org/10.1016/j.combustflame.2009.07.010
    *   Formulation with live/dead moisture factor :math:`\xi`: Santoni, P.A.,
        Filippi, J.B., Balbi, J.H., Bosseur, F. (2011). *Wildland fire
        behaviour case studies and fuel models for landscape-scale fire
        modeling*. Journal of Combustion 2011, 613424.
        https://doi.org/10.1155/2011/613424 (open access:
        https://hal.science/hal-00647897)

*   **How to select them:**

    .. code-block:: none

       setParameter[propagationModel=BalbiNov2011]

    All three variants read ``slope``, ``normalWind`` and the 20 ``fuel.*``
    columns (``Rhod``, ``Rhol``, ``Md``, ``Ml``, ``sd``, ``sl``, ``e``,
    ``Sigmad``, ``Sigmal``, ``stoch``, ``RhoA``, ``Ta``, ``Tau0``, ``Deltah``,
    ``DeltaH``, ``Cp``, ``Ti``, ``X0``, ``r00``, ``Blai``) from
    :ref:`The Standard Rothermel / Balbi Fuel Table <rothermel-fuel-file>`:

    *   ``BalbiNov2011`` takes ``Md``, ``Ml`` and ``Ta`` directly from the
        fuel table. Optional parameters: ``BalbiNov2011.Cpa`` (default ``1004``),
        ``BalbiNov2011.adjustementSlope`` (default ``1``) and
        ``BalbiNov2011.adjustementWind`` (default ``1``).
    *   ``BalbiNov2011Curv`` additionally registers ``frontCurvature`` and
        weights the radiative geometric factor by a dimensionless curvature
        term using ``BalbiNov2011Curv.gravity`` (default ``9.81``) and
        ``BalbiNov2011Curv.Cpa`` (default ``1004``).
    *   ``BalbiNov2011TMdMl`` additionally registers the dynamic layers
        ``temperature`` and ``moisture``, using ``temperature`` for ambient
        air temperature :math:`T_a` and ``moisture`` for live fuel moisture
        :math:`M_l` (while dead fuel moisture :math:`M_d` is read from
        ``fuel.Md``). Optional parameters: ``BalbiNov2011TMdMl.Cpa`` (default ``1004``),
        ``BalbiNov2011TMdMl.adjustementSlope`` (default ``1``) and
        ``BalbiNov2011TMdMl.adjustementWind`` (default ``1``).

Balbi2015 and BalbiUnsteady
^^^^^^^^^^^^^^^^^^^^^^^^^^^

*   **What they are:** non-stationary formulations of the Balbi model that
    account for local ``frontDepth`` (via flame optical depth
    :math:`1 - \exp(-\text{frontDepth}/\delta)`) and ``frontCurvature``.
*   **Reference:** Filippi, J.B., Balbi, J.H. (2010). *A reduced model of
    non-stationary wildland fire spread*. In *Proceedings of the VI
    International Conference on Forest Fire Research*, Coimbra, Portugal.
    https://hal.science/hal-00593614
*   **How to select them:**

    .. code-block:: none

       setParameter[propagationModel=Balbi2015]

    Both models register ``slope``, ``frontDepth``, ``frontCurvature``,
    ``normalWind`` and the 20 standard Balbi ``fuel.*`` columns:

    *   ``Balbi2015`` also registers ``moisture`` and ``temperature``, and
        accepts ``Balbi2015.Cpa`` (default ``1004``).
    *   ``BalbiUnsteady`` accepts ``BalbiUnsteady.Cpa`` (default ``1004``). In the current source
        code, when the flame tilt angle :math:`\gamma > 0`, ``BalbiUnsteady``
        returns early with the fourth power of the curvature correction factor
        divided by 10 (``curvCor / 10``) before evaluating the full radiative
        polynomial.

Farsite
^^^^^^^

*   **What it is:** the multi-class Rothermel model of FARSITE (1 h, 10 h,
    100 h, live herbaceous and live woody fuels, with dynamic load transfer),
    with elliptical growth of each point along the wind.
*   **References:**

    *   Finney, M.A. (1998, revised 2004). *FARSITE: Fire Area Simulator —
        model development and evaluation*. USDA Forest Service Research Paper
        RMRS-RP-4. https://doi.org/10.2737/RMRS-RP-4
    *   For the standard dynamic fuel models: Scott, J.H., Burgan, R.E.
        (2005). *Standard fire behavior fuel models: a comprehensive set for
        use with Rothermel's surface fire spread model*. USDA Forest Service
        General Technical Report RMRS-GTR-153.
        https://doi.org/10.2737/RMRS-GTR-153
    *   For the elliptical spread: Richards, G.D. (1990). *An elliptical
        growth model of forest fire fronts and its numerical solution*.
        International Journal for Numerical Methods in Engineering 30(6),
        1163–1179. https://doi.org/10.1002/nme.1620300606

*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=Farsite]
       setParameter[fuelsTable=STDfarsiteFuelsTable]

    ``STDfarsiteFuelsTable`` is a fuel table built into ForeFire; your own
    table needs the columns ``h1``, ``h10``, ``h100``, ``lh``, ``lw``,
    ``dynamic``, ``sav1``, ``savlh``, ``savlw``, ``depth``, ``xmext``,
    ``heatd`` and ``heatl``. It also registers ``slope``, ``normalWind``,
    ``windU``, ``windV`` and the five moisture properties ``moist.ones``,
    ``moist.tens``, ``moist.hundreds``, ``moist.liveh`` and ``moist.livew``
    (configured via ``moistures.ones``, etc.). ``Farsite.adjustementWind``
    (default ``1``) scales the wind, and ``FarsiteLoggerCSVPath`` optionally
    logs inputs and spread rates to a CSV file.

ANNPropagationModel
^^^^^^^^^^^^^^^^^^^

*   **What it is:** a propagation model that evaluates a pre-trained
    feed-forward artificial neural network (ANN) to predict the rate of spread
    (clamped to :math:`[0, 2]\;\text{m/s}`).
*   **Reference:** Filippi, J.-B., Baggio, R., Paugam, R., Bosseur, F.,
    Leblanc, A., Alonso-Pinar, A. (2025). *A Modular, Scriptable C++
    Simulation Engine and Library for Wildland-Fire Spread*. Journal of Open
    Source Software 10(116), 8680.
    https://doi.org/10.21105/joss.08680
*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=ANNPropagationModel]
       setParameter[FFANNPropagationModelPath=network.ffann]

    It loads the network architecture, weights, and input property names from
    the file specified by ``FFANNPropagationModelPath``, and dynamically
    registers each input name listed in that file with the ``DataBroker``.

LavaPropagationModel
^^^^^^^^^^^^^^^^^^^^

*   **What it is:** a viscous Bingham-fluid propagation model adapting
    ForeFire front tracking to effusive volcanic lava flows on a slope.
*   **Reference:** Durand, J., Tulet, P., Leriche, M., Bielli, S.,
    Villeneuve, N., Di Muro, A., Filippi, J.-B. (2014). *Modeling the lava
    heat flux during severe effusive volcanic eruption: An important impact on
    surface air quality*. Journal of Geophysical Research: Atmospheres 119(19),
    11729–11742. https://doi.org/10.1002/2014JD022034 (open access:
    https://hal.science/hal-01164000)
*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=LavaPropagationModel]

    It registers ``slope``, ``frontCurvature``, ``frontFastestInSection`` and
    the fuel table columns ``fuel.viscosity``, ``fuel.flowSpeed`` and
    ``fuel.rugosity``, and reads the optional parameter
    ``LavaModel.slowFactor`` (default ``1``).

Analytical, idealized and utility propagation models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The remaining propagation models in ``src/propagation/`` provide analytical
spread laws, idealized test cases, or training data loggers:

.. list-table::
   :header-rows: 1
   :widths: 22 42 36

   * - Registered name
     - What it computes
     - Registered properties & parameters
   * - ``Iso``
     - Constant isotropic rate of spread :math:`R = \text{Iso.speed}` in all directions.
     - No fuel or layer properties; parameter ``Iso.speed`` (default ``1.0`` m/s).
   * - ``IsotropicFuel``
     - Isotropic fuel-dependent rate of spread :math:`R = \text{speed\_module} \times \text{vv\_coeff}`.
     - ``fuel.vv_coeff``; parameter ``speed_module``.
   * - ``WindDriven``
     - Wind-driven rate of spread :math:`R = \text{vv\_coeff} \times u_n`.
     - ``normalWind``, ``fuel.vv_coeff``.
   * - ``CurvatureDriven``
     - Curvature-dependent rate of spread :math:`R = \max\!\left(0,\; \text{speed\_module} \cdot \text{vv\_coeff} \cdot \left(1 - K_{\text{curv}} \kappa / \sqrt{1 + \beta \kappa^2}\right)\right)`.
     - ``frontCurvature``, ``fuel.vv_coeff``, ``fuel.Kcurv``, ``fuel.beta``; parameter ``speed_module``.
   * - ``FrontDepthDriven``
     - Front-depth and wind-driven rate of spread :math:`R = \text{windReductionFactor} \cdot u_n \cdot \text{speed\_module} \cdot \text{vv\_coeff} \cdot (1 + K_{\text{depth}} \cdot d)`.
     - ``frontDepth``, ``normalWind``, ``fuel.vv_coeff``, ``fuel.Kdepth``; parameters ``speed_module``, ``windReductionFactor``.
   * - ``TroisPourcent``
     - Empirical rule-of-thumb spread rate :math:`R = R_0 + \max\!\left(0,\; f_w u_n + f_s (1 + \text{slope})\right)`.
     - ``slope``, ``normalWind``; parameters ``TroisPourcent.R0`` (default ``0.01``), ``TroisPourcent.windFactor`` (default ``0.03``), ``TroisPourcent.slopeFactor`` (default ``0.0``).
   * - ``SamplePropagationModel``
     - Template model computing :math:`R = \max(0,\; \text{windReductionFactor} \cdot \text{slope} \cdot u_n) \cdot \Sigma_d`.
     - ``slope``, ``normalWind``, ``fuel.Sigmad``; parameter ``windReductionFactor``.
   * - ``BMapLoggerForANNTraining``
     - Propagates at the inverse of ``arrival_time_gradient`` (clamped to ``maxSpeed``) while logging ``ROS`` and all ANN inputs to a CSV file to build training datasets.
     - ``arrival_time_gradient`` plus the input properties listed in ``FFANNPropagationModelPath``; parameters ``FFANNPropagationModelPath``, ``FFBMapLoggerCSVPath``, ``maxSpeed``.

Flux models
-----------

A flux model gives what the burning area releases, such as heat, water vapor,
or chemical species, for coupling with an atmospheric model. You attach it to
a flux layer with :ref:`addLayer <cmd-addLayer>`, or make it the default for
the ``heatFlux`` layer with ``heatFluxDefaultModel``.

BurnUpHeatFlux
^^^^^^^^^^^^^^

*   **What it is:** a heat flux where the remaining fuel decays
    exponentially after ignition, giving sensible heat from the dry fuel and
    latent heat from its water content, following the WRF-SFIRE exponential
    approximation to the BURNUP fuel consumption model.
*   **References:**

    *   Mandel, J., Beezley, J.D., Kochanski, A.K. (2011). *Coupled
        atmosphere-wildland fire modeling with WRF 3.3 and SFIRE 2011*.
        Geoscientific Model Development 4, 591–610, equations 3–5.
        https://doi.org/10.5194/gmd-4-591-2011
    *   Original BURNUP model approximated by Mandel et al. (2011): Albini,
        F.A., Reinhardt, E.D. (1995). *Modeling ignition and burning rate of
        large woody natural fuels*. International Journal of Wildland Fire
        5(2), 81–91. https://doi.org/10.1071/WF9950081

*   **How to select it:**

    .. code-block:: none

       addLayer[name=heatFlux;type=flux;modelName=BurnUpHeatFlux]

    or

    .. code-block:: none

       setParameter[heatFluxDefaultModel=BurnUpHeatFlux]

    It registers ``fuel.w`` (characteristic burn time), ``fuel.Sigmad``,
    ``fuel.DeltaH`` and the ``moisture`` layer. Optional parameters:
    ``burnup.timeCoeff`` (default ``0.8514``) and ``burnup.waterLatentHeat``
    (default ``2.5e6`` J/kg).

ForeFireV1HeatFlux and ForeFireV1VaporFlux
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*   **What they are:** exponential-decay heat and water-vapor flux models for
    dead and live fuel layers, using a particle-size residence time scale
    (:math:`T_0 \propto s_d^{-1.236}`) corrected for Leaf Area Index (LAI) and
    fuel moisture.
*   **References:**

    *   Flame residence time scaling: Burrows, N.D. (2001). *Flame residence
        times and rates of weight loss of eucalypt forest fuel particles*.
        International Journal of Wildland Fire 10(2), 137–143.
        https://doi.org/10.1071/WF01005
    *   ForeFire–Meso-NH fire–atmosphere coupling: Filippi, J.B., Bosseur, F.,
        Mari, C., Lac, C., Le Moigne, P., Cuenot, B., Veynante, D., Cariolle,
        D., Balbi, J.-H. (2009). *Coupled atmosphere-wildland fire modelling*.
        Journal of Advances in Modeling Earth Systems 1, 11.
        https://doi.org/10.3894/JAMES.2009.1.11 (open access:
        https://hal.science/hal-00433103)
    *   Filippi, J.B., Bosseur, F., Pialat, X., Santoni, P.A., Strada, S.,
        Mari, C. (2011). *Simulation of coupled fire/atmosphere interaction
        with the MesoNH-ForeFire models*. Journal of Combustion 2011, 540390.
        https://doi.org/10.1155/2011/540390

*   **How to select them:**

    .. code-block:: none

       addLayer[name=heatFlux;type=flux;modelName=ForeFireV1HeatFlux]
       addLayer[name=vaporFlux;type=flux;modelName=ForeFireV1VaporFlux]

    Both models register ``fuel.sd``, ``fuel.Md``, ``fuel.Ml``,
    ``fuel.Sigmad``, ``fuel.Sigmal``, ``fuel.Blai`` and ``fuel.me`` from
    :ref:`The Standard Rothermel / Balbi Fuel Table <rothermel-fuel-file>`,
    and ``ForeFireV1HeatFlux`` additionally registers ``fuel.DeltaH``.

    Required vector parameters: ``FFfluxes.LAICoeffs`` (2 values) and
    ``FFfluxes.mCoeffs`` (5 values). Optional scalar parameters:
    ``FFfluxes.BurrowsCst`` (default ``208487``),
    ``FFfluxes.liveTimeScaleRatio`` (default ``5.0``), ``FFfluxes.chi_b``
    (default ``1.0``), ``radiativeFraction`` (default ``0.3``, used by
    ``ForeFireV1HeatFlux``), and ``FFfluxes.timeScalesRatio`` (default
    ``0.5``, used by ``ForeFireV1VaporFlux``).

CraterHeatFluxModel and CraterVaporFluxModel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*   **What they are:** heat and water-vapor flux models for an active volcanic
    crater, interpolating time series of surface crust fraction, wind-dependent
    convective exchange, and gas flow rates after eruption onset.
*   **Reference:** Durand, J. (2016). *Modélisation de la dynamique et de
    l'évolution physico-chimique des gaz volcaniques lors de l'éruption
    d'avril 2007 du Piton de la Fournaise*. PhD thesis, Université de La
    Réunion. https://theses.hal.science/tel-01391461
*   **How to select them:**

    .. code-block:: none

       addLayer[name=heatFlux;type=flux;modelName=CraterHeatFluxModel]
       addLayer[name=vaporFlux;type=flux;modelName=CraterVaporFluxModel]

    *   ``CraterHeatFluxModel`` registers ``windU`` and ``windV``, requires
        the array parameters ``lava.hours``, ``lava.crustFractions``,
        ``lava.windTresholds``, ``lava.A``, ``lava.B`` and
        ``crater.heatCorrections``, and accepts ``lava.eruptionTime`` (default
        ``0``), ``lava.crustTemperature`` (default ``400``) and
        ``lava.temperature`` (default ``800``).
    *   ``CraterVaporFluxModel`` registers no spatial properties, requires the
        array parameters ``crater.hours`` and ``crater.flows``, and accepts
        ``lava.eruptionTime`` (default ``0``), ``crater.area`` (default
        ``900`` :math:`\text{m}^2`) and ``crater.vaporEmissionRatio`` (default
        ``0.008``).

LavaSO2Flux and factorChemFlux
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*   **What they are:** chemical species flux models that interpolate an
    emission time series (``SO2.hours``, ``SO2.flows``) scaled by burning or
    lava area for coupled plume simulations.
*   **References:**

    *   Volcanic :math:`\text{SO}_2` and heat flux modeling: Durand, J.,
        Tulet, P., Leriche, M., Bielli, S., Villeneuve, N., Di Muro, A.,
        Filippi, J.-B. (2014). *Modeling the lava heat flux during severe
        effusive volcanic eruption: An important impact on surface air
        quality*. Journal of Geophysical Research: Atmospheres 119(19),
        11729–11742. https://doi.org/10.1002/2014JD022034
    *   Filippi, J.-B., Durand, J., Tulet, P., Bielli, S. (2021). *Multiscale
        modeling of convection and pollutant transport associated with
        volcanic eruption and lava flow: Application to the April 2007
        eruption of Piton de la Fournaise*. Atmosphere 12(4), 507.
        https://doi.org/10.3390/atmos12040507
    *   Industrial fire plume coupling (``factorChemFlux`` header reference):
        Baggio, R., Filippi, J.B., Truchot, B., Couto, F.T. (2022). *Local to
        continental scale coupled fire-atmosphere simulation of large
        industrial fire plume*. Fire Safety Journal 134, 103699.
        https://doi.org/10.1016/j.firesaf.2022.103699

*   **How to select them:**

    .. code-block:: none

       addLayer[name=chemFlux;type=flux;modelName=factorChemFlux]

    Both ``factorChemFlux`` and ``LavaSO2Flux`` read the required array
    parameters ``SO2.hours`` and ``SO2.flows`` along with
    ``lava.eruptionTime`` (default ``0``), ``lava.area`` (default ``5.5e6``
    :math:`\text{m}^2`) and ``SO2.craterRatio`` (default ``0``; emission ratio
    is :math:`1 - \text{SO2.craterRatio}`). In the current source code,
    ``factorChemFlux`` returns the interpolated flux while ``LavaSO2Flux``
    returns ``0``.

Nominal and observation-driven flux models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The remaining flux models in ``src/flux/`` release a constant flux over a
fixed or fuel-derived residence time, or read flaming and smoldering flux
parameters from observation layers:

.. list-table::
   :header-rows: 1
   :widths: 22 42 36

   * - Registered name
     - What it computes
     - Registered properties & parameters
   * - ``heatFluxBasic``
     - Constant heat flux ``nominalHeatFlux`` during ``burningDuration`` seconds after arrival time.
     - No fuel or layer properties; parameters ``burningDuration`` (default ``300`` s), ``nominalHeatFlux`` (default ``1e6`` :math:`\text{W/m}^2`).
   * - ``heatFluxNominal``
     - Constant heat flux ``nominalHeatFlux`` over the fuel residence time :math:`\tau = \text{Tau0} / s_d`.
     - ``fuel.Tau0``, ``fuel.sd``; parameter ``nominalHeatFlux`` (default ``150000`` :math:`\text{W/m}^2`).
   * - ``heatFluxFromObs``
     - Two-stage (flaming + smoldering) sensible heat flux starting after ``FromObs_evaporationTime``.
     - ``FromObs_evaporationTime``, ``FromObs_residenceTime``, ``FromObs_burningTime``, ``FromObs_NominalHeatFlux_flaming``, ``FromObs_NominalHeatFlux_smoldering``.
   * - ``vaporFluxBasic``
     - Constant water-vapor flux ``nominalVaporFlux`` during ``burningDuration`` seconds after arrival time.
     - No fuel or layer properties; parameters ``burningDuration`` (default ``30`` s), ``nominalVaporFlux`` (default ``1.0`` :math:`\text{kg/(m}^2\cdot\text{s)}`).
   * - ``vaporFluxNominal``
     - Constant water-vapor flux ``nominalVaporFlux`` over the fuel residence time :math:`\tau = \text{Tau0} / s_d`.
     - ``fuel.Tau0``, ``fuel.sd``; parameter ``nominalVaporFlux`` (default ``1.0`` :math:`\text{kg/(m}^2\cdot\text{s)}`).
   * - ``vaporFluxFromObs``
     - Constant water-vapor flux ``FromObs_VaporFlux_evaporation`` during ``FromObs_evaporationTime`` after arrival time.
     - ``FromObs_evaporationTime``, ``FromObs_VaporFlux_evaporation``.
   * - ``SFMod``
     - Constant scalar flux ``nominalScalarFlux`` over the fuel residence time :math:`\tau = \text{Tau0} / s_d`.
     - ``fuel.Tau0``, ``fuel.sd``; parameter ``nominalScalarFlux`` (default ``1.0``).
   * - ``SFObs``
     - Two-stage (flaming + smoldering) scalar emission flux derived from observed heat fluxes, emission factors, and radiation fractions.
     - ``FromObs_NominalHeatFlux_flaming``, ``FromObs_NominalHeatFlux_smoldering``, ``FromObs_evaporationTime``, ``FromObs_residenceTime``, ``FromObs_burningTime``, ``FromObs_EFScalar_flaming``, ``FromObs_EFScalar_smoldering``, ``FromObs_radiationFraction_flaming``, ``FromObs_radiationFraction_smoldering``, ``FromObs_conversionFactor``.
   * - ``SpottingFluxBasic``
     - Spotting brand flux :math:`\text{nominalSpottingFlux} \times \Sigma_d` over the interval :math:`[t_a + \text{lagSpotting},\; t_a + \text{lagSpotting} + \text{spottingDuration}]`.
     - ``fuel.Sigmad``; parameters ``spottingDuration`` (default ``100`` s), ``nominalSpottingFlux`` (default ``1.0``), ``lagSpotting`` (default ``0.0`` s).
