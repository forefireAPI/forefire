C++ API Reference
=================

This section provides a high-level overview of the most fundamental C++ classes and interfaces that constitute the ForeFire simulation engine. For a comprehensive, detailed API reference for all classes and functions, please consult the full Doxygen-generated documentation.

Core Simulation Objects
-----------------------

These classes represent the primary entities in a wildfire simulation.

.. doxygenclass:: libforefire::FireDomain
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FireFront
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FireNode
   :project: ForeFire
   :members:

Data Management
---------------

These classes are central to how ForeFire handles environmental and simulation data.

.. doxygenclass:: libforefire::DataBroker
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::DataLayer
   :project: ForeFire
   :members:

.. rubric:: Note
   ForeFire provides various specializations of ``DataLayer`` (e.g., for 2D/3D arrays, NetCDF files, fuel properties, atmospheric data) to handle different data sources and types. Refer to the full Doxygen documentation for details on specific implementations like ``Array2DdataLayer``, ``NCXYZTDataLayer``, ``FuelDataLayer``, etc.

Scientific Model Interfaces
---------------------------

These abstract base classes define how new scientific models for fire spread and flux calculations can be integrated.

.. doxygenclass:: libforefire::PropagationModel
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FluxModel
   :project: ForeFire
   :members:

User Interaction & Configuration
--------------------------------

These components manage user commands and simulation parameters.

.. doxygenclass:: libforefire::Command
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::SimulationParameters
   :project: ForeFire
   :members:

Fundamental Utilities
---------------------

Basic geometric data types used throughout the simulation.

.. doxygenclass:: libforefire::FFPoint
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FFVector
   :project: ForeFire
   :members: