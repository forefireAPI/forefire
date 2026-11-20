.. _introduction:

Introduction
============

ForeFire simulates how a wildland fire spreads over a landscape. It is a C++ engine with Python bindings, developed at CNRS / Université de Corse Pascal Paoli (`Filippi et al., 2009 <https://doi.org/10.3894/JAMES.2009.1.11>`_; `Filippi et al., 2025 <https://doi.org/10.21105/joss.08680>`_).

ForeFire follows the fire line, not a grid. The perimeter is a set of markers that each move at their own pace, so the work goes where the fire is. This gives meter-scale resolution over large landscapes at more than 100 ha/s on one CPU core. :doc:`/user_guide/core_concepts` explains how.

.. figure:: /_static/forefire-demo.gif
   :alt: A fire spreading east with the wind in the ForeFire browser demo
   :target: https://forefireapi.github.io/forefire/classic/

   The front moving with the wind in the browser demo. Click the map there to light a fire of your own.

Ways to run it
--------------

The same engine runs in four places:

- ``forefire``, the command-line interpreter. It runs ``.ff`` scripts (:doc:`/user_guide/forefire_script`), opens an interactive shell, and serves a browser interface with ``listenHTTP[]``.
- ``pyforefire``, the Python package (``pip install forefire``). It drives the engine from Python, takes NumPy arrays as input (:doc:`/user_guide/python_arrays`), and runs ensembles in separate processes (:doc:`/user_guide/ensembles`).
- ``libforefireL``, the C++17 library. Atmospheric models link to it over MPI and C/Fortran bindings, as Meso-NH does (:doc:`/user_guide/mesonh`).
- ``forefire.wasm``, a WebAssembly build that runs the full solver in a browser or in Node.js. Try the `in-browser demo <https://forefireapi.github.io/forefire/>`_.

What it does
------------

- Propagation models are plug-ins: Rothermel, Balbi, Farsite, a neural-network emulator and more (:doc:`/user_guide/models`). A new model is one ``.cpp`` file.
- Flux models (``BurnUpHeatFlux``, ``ForeFireV1HeatFlux`` and others) compute the heat and chemical species released behind the front.
- Input comes from NumPy arrays, NetCDF landscape files, ``.fftile`` tiles (:doc:`/user_guide/tiles`) or web map tiles in the browser. Output goes to GeoJSON, KML, NetCDF, PNG, JPG or ASCII grids.
- Coupled with `Meso-NH <https://mesonh.cnrs.fr/>`_, the wind drives the fire and the fire's heat and vapor feed back into the atmosphere.
- A spun-up simulation can be forked into many runs to estimate burn probability under uncertain wind or parameters, and scored against an observed perimeter.

Platforms
---------

- Linux (``x86_64``, ``aarch64``) and macOS (Apple Silicon): wheels with ``pip install forefire``, or a CMake build.
- Windows: use WSL, Docker (``ghcr.io/forefireapi/forefire:latest``) or the browser demo.

Next: :doc:`quickstart`.
