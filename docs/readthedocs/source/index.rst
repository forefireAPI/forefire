ForeFire
========

**ForeFire** is a modular, scriptable, discrete-event-driven C++ simulation engine and Python library for wildland fire spread, developed by CNRS at the `Université de Corse Pascal Paoli <https://www.univ-corse.fr/>`_.

.. code-block:: bash

   pip install forefire

`Try it in the browser <https://forefireapi.github.io/forefire/>`_ · `Server-side demo <https://forefire.univ-corse.fr/sim>`_ · `GitHub <https://github.com/forefireAPI/forefire>`_ · `JOSS paper <https://doi.org/10.21105/joss.08680>`_

Where to start
--------------

.. grid:: 1 2 2 2
   :gutter: 3

   .. grid-item-card:: Your first fire
      :link: getting_started/quickstart
      :link-type: doc

      In the browser, from Python, or from the command line.

   .. grid-item-card:: What a simulation needs
      :link: user_guide/simulation_inputs
      :link-type: doc

      Domain, landscape, fuel, wind, model and ignition, and where each one comes from.

   .. grid-item-card:: Script files (.ff)
      :link: user_guide/forefire_script
      :link-type: doc

      The command language that drives every simulation.

   .. grid-item-card:: Real terrain from tiles
      :link: user_guide/tiles
      :link-type: doc

      Fuel, elevation and wind over the globe, ready to run.

The science
-----------

The physical models in ForeFire come from published papers: Rothermel (1972) and its revision by Andrews (2018), the Balbi models, Farsite after Scott & Burgan (2005), and the heat and vapour fluxes used for coupling with the atmosphere. :doc:`user_guide/models` lists every propagation and flux model with its reference, the fuel columns it reads and how to select it.

Going further
-------------

.. grid:: 1 2 2 2
   :gutter: 3

   .. grid-item-card:: Made-up landscapes in Python
      :link: user_guide/python_arrays
      :link-type: doc

      Build fuel, terrain and wind as NumPy arrays.

   .. grid-item-card:: Ensembles
      :link: user_guide/ensembles
      :link-type: doc

      Many runs at once and burn probability maps.


   .. grid-item-card:: Coupling with Meso-NH
      :link: user_guide/mesonh
      :link-type: doc

      Fire-atmosphere simulations with the Meso-NH model.

   .. grid-item-card:: How it works
      :link: user_guide/core_concepts
      :link-type: doc

      Markers on the front, data layers and the arrival-time map.

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Getting Started

   getting_started/introduction
   getting_started/quickstart

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Setting Up a Simulation

   user_guide/simulation_inputs
   user_guide/forefire_script
   user_guide/fuels_and_models
   user_guide/models

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Landscapes

   user_guide/python_arrays
   user_guide/landscape_file
   user_guide/tiles

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Going Further

   user_guide/ensembles
   user_guide/checkpoints
   user_guide/scenario_studio
   user_guide/mesonh
   user_guide/core_concepts

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Reference

   reference/commands
   reference/parameters

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: For Developers

   getting_started/installation
   user_guide/writing_models
   reference/api_reference

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: About

   about/statement_of_need
   about/citation
   about/license
