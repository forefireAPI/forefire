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

   .. grid-item-card:: Install
      :link: getting_started/installation
      :link-type: doc

      With pip, or build from source with CMake.

   .. grid-item-card:: Run your first fire
      :link: getting_started/quickstart
      :link-type: doc

      In the browser, from Python, or from the command line.

   .. grid-item-card:: Core concepts
      :link: user_guide/core_concepts
      :link-type: doc

      Front tracking, data layers, arrival-time maps, and adding a model.

   .. grid-item-card:: Script files (.ff)
      :link: user_guide/forefire_script
      :link-type: doc

      The command language that drives every simulation.

Going further
-------------

.. grid:: 1 2 2 2
   :gutter: 3

   .. grid-item-card:: Propagation models
      :link: user_guide/models
      :link-type: doc

      Every rate-of-spread model in the engine, with its paper.

   .. grid-item-card:: Python and NumPy
      :link: user_guide/python_arrays
      :link-type: doc

      Run from arrays, without NetCDF or GIS files.

   .. grid-item-card:: Ensembles
      :link: user_guide/ensembles
      :link-type: doc

      Many runs at once and burn probability maps.

   .. grid-item-card:: Coupling with Meso-NH
      :link: user_guide/mesonh
      :link-type: doc

      Fire-atmosphere simulations with the Meso-NH model.

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Getting Started

   getting_started/introduction
   getting_started/installation
   getting_started/quickstart

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: User Guide

   user_guide/core_concepts
   user_guide/models
   user_guide/fuels_and_models
   user_guide/landscape_file
   user_guide/forefire_script
   user_guide/python_arrays
   user_guide/ensembles
   user_guide/scenario_studio
   user_guide/mesonh

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Reference

   reference/commands
   reference/parameters
   reference/api_reference

.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: About

   about/statement_of_need
   about/citation
   about/license
