Basic Configuration
===================

ForeFire simulations typically require three main input files to run:

1.  **Model Parameter File (e.g., fuels.csv)**:
    
    A file that defines physical parameters required by the selected propagation model. For example, the `Rothermel` and `Balbi` models use a ``fuels.csv`` file to define fuel bed properties. Other models, like `Iso`, do not require an external file. See the :doc:`Fuels & Models guide <fuels_and_models>` for details.
2.  **Landscape File (.nc)**:

A NetCDF file containing geospatial data layers for the simulation domain, typically including:

    - Elevation (Digital Elevation Model - DEM)
    - Fuel types map (matching the indices in the Fuels File)
    - (Optionally) Pre-defined wind fields or other environmental data.

3.  **ForeFire Script File (.ff)**:

A text file containing commands and parameters that control the simulation execution. This includes:

    - Setting simulation parameters (e.g., propagation model, output format).
    - Loading the landscape and fuels data.
    - Defining the ignition point(s) and time.
    - Setting wind conditions (e.g., via triggers).
    - Defining the simulation duration and time steps.
    - Specifying output file names and formats.