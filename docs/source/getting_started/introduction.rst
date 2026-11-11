.. _introduction:

Introduction
============

**ForeFire** is an open-source, discrete-event-driven simulation engine for wildland fire spread written in C++ and developed at CNRS / Université de Corse Pascal Paoli (`Filippi et al., 2009 <https://doi.org/10.3894/JAMES.2009.1.11>`_; `Filippi et al., 2025 <https://doi.org/10.21105/joss.08680>`_).

Instead of advancing a fixed spatial grid over global time steps, ForeFire tracks the fire perimeter as a dynamic Lagrangian mesh of markers (``FireNode`` objects grouped into ``FireFront`` contours) and schedules updates in an event queue (``TimeTable``). Computational effort is concentrated exclusively on the active fire line, enabling meter-scale resolution over large landscapes at speeds exceeding 100 ha/s on a single CPU core.

Architecture
------------

ForeFire is built around a core C++ library with multiple entry points:

1. **Simulation Engine (``libforefireL``):**
   The C++17 core implementing Lagrangian front tracking, discrete-event scheduling, the ``DataBroker`` layer system, and pluggable Rate of Spread (ROS) and flux models. It can be compiled with or without NetCDF (``-DFOREFIRE_ENABLE_NETCDF=OFF``) and linked into atmospheric models via MPI and ISO C/Fortran bindings.

2. **Python Package (``pyforefire`` via ``pip install forefire``):**
   Self-contained Python bindings that expose the simulation engine, direct NumPy array ingestion (``addIndexLayer``, ``addScalarLayer``), isolated multi-process execution (``ForeFireRunner``), ensemble forecasting (``run_ensemble``, ``burn_probability``), and input validation (``forefire-validate``).

3. **Command-Line Interpreter & HTTP Server (``forefire``):**
   Executes ``.ff`` simulation scripts, provides an interactive shell with tab completion, and serves a browser-based graphical interface via ``listenHTTP[]``.

4. **WebAssembly Module (``forefire.wasm`` / ``forefire.mjs``):**
   An Emscripten build (compiled with ``FOREFIRE_ENABLE_NETCDF=OFF``) that runs the complete solver and all propagation models directly inside a browser Web Worker or Node.js. Try it live in the `in-browser demo <https://forefireapi.github.io/forefire/>`_.

Key Capabilities
----------------

- **Pluggable Physics (Strategy Pattern):** Includes standard surface spread models (:doc:`/user_guide/models` — ``Rothermel``, ``RothermelAndrews2018``, ``Balbi2020``, ``BalbiNov2011``, ``Farsite``), heat and chemical flux models (``BurnUpHeatFlux``, ``ForeFireV1HeatFlux``), and a neural-network emulator (``ANNPropagationModel``). Adding a new model requires a single ``.cpp`` file.
- **Flexible Data Ingestion:** Feed environmental layers (elevation, fuel index maps, wind fields) from **NumPy arrays** in memory, **NetCDF** landscape files, or **web map tiles** (in the WebAssembly build). Export perimeters and rasters to GeoJSON, KML, NetCDF, PNG, JPG, or ASCII grids.
- **Two-Way Fire–Atmosphere Coupling:** Couple directly with the open-source `Meso-NH <https://mesonh.cnrs.fr/>`_ atmospheric model over MPI so surface winds drive fire spread while fire heat and vapor fluxes force atmospheric convection (see :doc:`/user_guide/mesonh`).
- **Ensembles & Burn Probability:** Fork a spun-up simulation across worker processes to evaluate wind or parameter uncertainty and score probabilistic forecasts against observed perimeters (see :doc:`/user_guide/ensembles`).

Supported Platforms
-------------------

- **Linux (`x86_64`, `aarch64`) and macOS (Apple Silicon):** pre-built wheels via ``pip install forefire``, or native CMake builds.
- **Windows:** run via **WSL** (Linux ``pip`` wheel or source build), **Docker** (``ghcr.io/forefireapi/forefire:latest``), or the **in-browser WebAssembly demo**.

Where to Go Next
----------------

- :doc:`installation` — install with ``pip``, Docker, or CMake from source.
- :doc:`quickstart` — run your first simulation in the browser, Python, the CLI, or Docker.
- :doc:`/user_guide/core_concepts` — understand Lagrangian markers, the ``DataBroker``, arrival-time maps (``BMap``), and how to write a new C++ model.
