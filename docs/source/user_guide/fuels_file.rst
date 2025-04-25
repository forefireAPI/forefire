.. _userguide-fuels-file:

Fuels File (fuels.csv)
======================

The Fuels File, typically named ``fuels.csv``, is a crucial input file for ForeFire. Its primary purpose is to define the physical characteristics of the different fuel types present in your simulation landscape. These characteristics are used by the selected fire propagation model (e.g., Rothermel) to calculate the rate of spread.

File Format
-----------

The file must be in a standard **Comma Separated Value (CSV)** format.

*   **Delimiter:** Values within a row should be separated by a consistent delimiter, typically a **semicolon (`;`)**. Ensure you use the same delimiter throughout the file.
*   **Header Row:** The **first line** of the file *must* be a header row containing the exact names of the parameter columns as shown below.
*   **Data Rows:** Each subsequent line defines a single fuel type, corresponding to an integer index (including 0) used in the landscape file.

Column Headers & Parameters
---------------------------

The standard ``fuels.csv`` file requires the following columns. While the descriptions often relate to parameters used by the Rothermel model, the file format requires all listed columns.

*   ``Index``

    *   **Description:** **Critical.** A unique non-negative integer identifier (including 0) for the fuel type. Index `0` is often used to represent non-burnable areas. This number directly links this row to the corresponding fuel type values in the landscape file's fuel layer.
    *   **Units:** N/A (Integer ID)

*   ``Rhod``

    *   **Description:** Oven-dry particle density of the fuel material.
    *   **Units:** kg/m³ (May be internally converted for model calculations expecting different units).

*   ``Rhol``

    *   **Description:** Parameter related to fuel particle density.
    *   **Units:** kg/m³

*   ``Md``

    *   **Description:** Oven-dry fuel moisture content (dimensionless ratio, e.g., 0.10 for 10%).
    *   **Units:** Dimensionless (Value / 100)

*   ``Ml``

    *   **Description:** Parameter related to fuel moisture content.
    *   **Units:** Dimensionless

*   ``sd``

    *   **Description:** Surface-area-to-volume ratio of the oven-dry fuel particles.
    *   **Units:** m²/m³ (Verify consistency with data source; internal conversions may occur).

*   ``sl``

    *   **Description:** Parameter related to surface-area-to-volume ratio.
    *   **Units:** m²/m³

*   ``e``

    *   **Description:** Fuel bed depth (thickness of the fuel layer).
    *   **Units:** meters (m)

*   ``Sigmad``

    *   **Description:** Oven-dry fuel load (mass per unit area).
    *   **Units:** kg/m²

*   ``Sigmal``

    *   **Description:** Parameter related to fuel load.
    *   **Units:** kg/m²

*   ``stoch``

    *   **Description:** Parameter potentially related to stochastic fuel properties or other models.
    *   **Units:** Various

*   ``RhoA``

    *   **Description:** Parameter potentially related to fuel arrangement or density.
    *   **Units:** Various

*   ``Ta``

    *   **Description:** Parameter potentially related to ambient temperature or thermal properties.
    *   **Units:** Various (e.g., Kelvin)

*   ``Tau0``

    *   **Description:** Parameter potentially related to ignition time or thermal constants.
    *   **Units:** Various (e.g., seconds)

*   ``Deltah``

    *   **Description:** Parameter related to fuel heat content.
    *   **Units:** J/kg

*   ``DeltaH``

    *   **Description:** Fuel particle heat content (Heat of Combustion). Often a key parameter for Rothermel.
    *   **Units:** J/kg (May be internally converted for model calculations expecting different units).

*   ``Cp``

    *   **Description:** Parameter related to specific heat capacity.
    *   **Units:** Various (e.g., J/kg/K)

*   ``Cpa``

    *   **Description:** Parameter potentially related to air specific heat capacity.
    *   **Units:** Various (e.g., J/kg/K)

*   ``Ti``

    *   **Description:** Parameter potentially related to ignition temperature.
    *   **Units:** Various (e.g., Kelvin)

*   ``X0``

    *   **Description:** Parameter used in certain model formulations.
    *   **Units:** Various

*   ``r00``

    *   **Description:** Parameter used in certain model formulations.
    *   **Units:** Various

*   ``Blai``

    *   **Description:** Parameter potentially related to Leaf Area Index or biomass characteristics.
    *   **Units:** Various

*   ``me``

    *   **Description:** Moisture content of extinction (dimensionless ratio, e.g., 0.30 for 30%). The fuel moisture level above which the fuel will not burn.
    *   **Units:** Dimensionless (Value / 100)

.. important::
    
    While the file requires all columns, the parameters most fundamentally driving the *Rothermel* fire spread calculation are typically: ``Index``, ``Rhod``, ``Md``, ``sd``, ``e``, ``Sigmad``, ``DeltaH``, and ``me``. Ensure the values in these columns are accurate for your intended fuels when using Rothermel. Always ensure your header names exactly match those listed above.

The `Index` Column
------------------
The ``Index`` column is fundamental for linking the fuel properties defined in this file to the spatial distribution of fuels defined in the :doc:`landscape_file`. The integer value in each cell of the landscape file's "fuel" layer must correspond to one of the ``Index`` values present in this ``fuels.csv`` file. ForeFire uses this link to look up the physical properties for the fuel type at any given location during the simulation.

Using Custom Fuels
------------------

If your fuel data does not align with standard fuel models:

1.  **Mapping:** You can map your custom fuel categories to the `Index` values of the closest equivalent standard fuel types. You would then use your *mapped* fuel raster when creating the landscape file, ensuring it uses the standard `Index` values.
2.  **Creating Custom File:** Alternatively, you can create your own ``fuels.csv`` file. You *must* include the required header columns exactly as shown above and define the parameters accurately for each of your custom fuel types, assigning a unique `Index` to each (including 0 if needed for non-burnable areas).