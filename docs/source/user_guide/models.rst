.. _userguide-models:

Models and References
=====================

These are the propagation and flux models in ForeFire that rest on a
published reference. If you need to choose a model or cite one, start here.
The fuel file format is described in :doc:`fuels_and_models`.

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

    It reads the fuel table described in
    :ref:`The Rothermel / Balbi Fuel File <rothermel-fuel-file>`.
    ``windReductionFactor`` (default ``0.4``) scales the wind it uses.

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

    Its fuel table uses its own columns: ``fl1h_tac``, ``fd_ft``,
    ``SAVcar_ftinv``, ``mdOnDry1h_r``, ``fuelDens_lbft3``, ``H_BTUlb``,
    ``Dme_pc``, ``totMineral_r`` and ``effectMineral_r``.
    ``windReductionFactor`` scales the wind, as for ``Rothermel``.

Balbi2020
^^^^^^^^^

*   **What it is:** the convective-radiative Balbi model, solved
    iteratively.
*   **Reference:** Balbi, J.H., Chatelon, F.J., Morvan, D., Rossi, J.L.,
    Marcelli, T., Morandini, F. (2020). *A convective–radiative propagation
    model for wildland fires*. International Journal of Wildland Fire 29(8),
    723–738. https://doi.org/10.1071/WF19103 (open access:
    https://hal.science/hal-03251626)
*   **How to select it:**

    .. code-block:: none

       setParameter[propagationModel=Balbi2020]

    It reads the same fuel table as ``Rothermel``. Optional parameters:
    ``Balbi2020.cooling`` (default ``0``) and ``Balbi2020.Cpa`` (default
    ``1004``).

Farsite
^^^^^^^

*   **What it is:** the multi-class Rothermel model of FARSITE (1 h, 10 h,
    100 h, live herbaceous and live woody fuels, with dynamic load transfer),
    with elliptical growth of each point along the wind.
*   **References:**

    *   Finney, M.A. (1998, revised 2004). *FARSITE: Fire Area Simulator —
        model development and evaluation*. USDA Forest Service Research Paper
        RMRS-RP-4. https://doi.org/10.2737/RMRS-RP-4
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
    ``heatd`` and ``heatl``. Fuel moistures are set with
    ``moistures.ones``, ``moistures.tens``, ``moistures.hundreds``,
    ``moistures.liveh`` and ``moistures.livew``.
    ``Farsite.adjustementWind`` (default ``1``) scales the wind.

Flux models
-----------

A flux model gives what the burning area releases, such as heat, for
coupling with an atmospheric model. You attach it to a flux layer with
:ref:`addLayer <cmd-addLayer>`, or make it the default for the ``heatFlux``
layer with ``heatFluxDefaultModel``.

BurnUpHeatFlux
^^^^^^^^^^^^^^

*   **What it is:** a heat flux where the remaining fuel decays
    exponentially after ignition, giving sensible heat from the dry fuel and
    latent heat from its water.
*   **Reference:** Mandel, J., Beezley, J.D., Kochanski, A.K. (2011).
    *Coupled atmosphere-wildland fire modeling with WRF 3.3 and SFIRE 2011*.
    Geoscientific Model Development 4, 591–610, equations 3–5.
    https://doi.org/10.5194/gmd-4-591-2011
*   **How to select it:**

    .. code-block:: none

       addLayer[name=heatFlux;type=flux;modelName=BurnUpHeatFlux]

    or

    .. code-block:: none

       setParameter[heatFluxDefaultModel=BurnUpHeatFlux]

    It reads ``w`` (burn time), ``Sigmad`` and ``DeltaH`` from the fuel
    table. Optional parameters: ``burnup.timeCoeff`` (default ``0.8514``)
    and ``burnup.waterLatentHeat`` (default ``2.5e6`` J/kg).

----

Other models in ``src/propagation`` and ``src/flux`` are not listed here yet.
