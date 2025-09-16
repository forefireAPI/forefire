.. _userguide-fuels-and-models:

Fuel & Model Parameterization
=============================

A core feature of ForeFire is its flexibility as a **general Rate of Spread (ROS) model solver**. Rather than being tied to a single fire spread equation, it can use different physical or empirical models depending on the user's needs. This is controlled via the :ref:`propagationModel <param-propagationModel>` parameter.

.. note::
   **For New Users:** The most common use case for ForeFire is simulating surface wildfires using the standard **Rothermel** model. This model **requires a ``fuels.csv`` file**. If this is your goal, you can skip directly to the section :ref:`The Rothermel / Balbi Fuel File <rothermel-fuel-file>` to learn how to prepare this critical input.

The choice of propagation model is critical because it dictates what physical parameters the simulation requires. This guide explains the available models and how to provide the necessary parameterization for each.

Available Propagation Models
----------------------------

ForeFire includes several built-in propagation models. Below are the primary models intended for general use. For a complete list of all implemented models, including experimental ones, users can refer to the C++ source code.

**Iso**
^^^^^^^
*   **Description:** A simple isotropic model where the fire spreads at a constant rate in all directions. It ignores all environmental factors except for blocking terrain.
*   **Use Case:** Ideal for simple tests, debugging setups, or scenarios where a constant spread rate is desired.
*   **Parameterization:** This model **does not** use an external fuels file. The spread speed is set directly in the simulation script via a model-specific parameter.
*   **Example:**

  .. code-block:: bash

    setParameter[propagationModel=Iso]
    setParameter[Iso.speed=0.5]  # Sets spread to 0.5 meters/second

**Rothermel**
^^^^^^^^^^^^^
*   **Description:** Implements the Rothermel (1972) surface fire spread model, a widely used semi-empirical model in wildfire simulation.
*   **Use Case:** The standard choice for simulating surface fire spread in various fuel types (grass, shrub, timber litter).
*   **Parameterization:** This model requires a detailed set of fuel bed parameters. These are provided via a **CSV file**, the format of which is detailed in the section below.

**Balbi** variants (e.g., `BalbiNov2011`, `Balbi2015`)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
*   **Description:** Implements physical models based on the work of Balbi et al. These models take a different approach from Rothermel, often considering factors like flame geometry and radiative heat transfer more explicitly.
*   **Use Case:** Primarily for research applications, model comparison studies, or scenarios where a more physics-based approach is preferred.
*   **Parameterization:** These models also source their parameters from the same CSV fuel file format used by Rothermel, though they may use a different subset of the columns for their calculations.

.. note::
  The relationship between a model and its parameters is key: when a model like `Rothermel` is selected, it informs the simulation engine which properties it needs (e.g., `slope`, `normalWind`, `fuel.Rhod`, etc.). The engine then provides these values, retrieving the `fuel.*` properties from the loaded CSV table.

----

.. _rothermel-fuel-file:

The Rothermel / Balbi Fuel File (`fuels.csv`)
-----------------------------------------------

When a `propagationModel` like `Rothermel` or a `Balbi` variant is selected, ForeFire needs a file that defines the physical characteristics of the fuel types present in the landscape.

**File Format**

*   **Format:** Comma Separated Value (CSV).
*   **Delimiter:** The parser expects a **semicolon (`;`)** as the delimiter.
*   **Header:** The first line *must* be a header row containing the exact column names specified below. The file parser requires all columns to be present, even if a specific model does not use all of them.

**Required Columns**

The ``Index`` column links these properties to the integer values in the landscape file's fuel layer.

*   ``Index``: **Critical.** Unique non-negative integer for the fuel type (e.g., `0` for non-burnable).
*   ``Rhod``: Oven-dry particle density (kg/m³).
*   ``Rhol``: (Model Specific)
*   ``Md``: Oven-dry fuel moisture content (dimensionless ratio, e.g., 0.1 for 10%).
*   ``Ml``: (Model Specific)
*   ``sd``: Surface-area-to-volume ratio of dry fuel (m²/m³).
*   ``sl``: (Model Specific)
*   ``e``: Fuel bed depth (meters).
*   ``Sigmad``: Oven-dry fuel load (kg/m²).
*   ``Sigmal``: (Model Specific)
*   ``stoch``: (Model Specific)
*   ``RhoA``: (Model Specific)
*   ``Ta``: (Model Specific)
*   ``Tau0``: (Model Specific)
*   ``Deltah``: (Model Specific)
*   ``DeltaH``: Fuel particle heat content (J/kg).
*   ``Cp``: (Model Specific)
*   ``Cpa``: (Model Specific)
*   ``Ti``: (Model Specific)
*   ``X0``: (Model Specific)
*   ``r00``: (Model Specific)
*   ``Blai``: (Model Specific)
*   ``me``: Moisture content of extinction (dimensionless ratio).

.. important::
   While all columns are required by the file parser, the parameters most fundamentally driving the *Rothermel* calculation are typically: ``Index``, ``Rhod``, ``Md``, ``sd``, ``e``, ``Sigmad``, ``DeltaH``, and ``me``. Different models may utilize different columns.

----

Finding and Creating Fuel Parameter Sets
----------------------------------------

Finding appropriate fuel parameter values is a scientific task in itself. Users have two primary resources:

1.  **Reference Examples:** The test cases within the ForeFire repository (e.g., `tests/runff/fuels.csv`) provide the best and intended reference for a correctly formatted file.
2.  **External Parameter Libraries:** For users looking for standard or pre-published fuel parameter sets, the following external repository, maintained by the ForeFire team, is the recommended resource:

    *   **wildfire_ROS_models:** `https://github.com/forefireAPI/wildfire_ROS_models`
    *   This repository contains parameterizations for various fuel models and is a valuable companion to ForeFire for preparing simulation inputs.