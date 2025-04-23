Introduction
============

ForeFire is an open-source wildfire spread simulator designed for research and operational forecasting.

Key Features
------------

*   **Advanced Simulation:** Core C++ engine for fire propagation using geospatial data (terrain, fuel, weather). Implements various fire behavior models (e.g., based on Rothermel equations).
*   **Fire-Atmosphere Coupling:** Integrates with atmospheric models like `MesoNH <https://mesonh.aero.obs-mip.fr/mesonh/>`_ for realistic wind-fire interaction modeling.
*   **High Performance:** Optimized for speed, capable of faster-than-real-time simulations on laptops, with MPI support for parallel computing.
*   **Multiple Interfaces:** Usable via:
  *   C++ library
  *   Python bindings
  *   Interactive console (ForeFire interpreter)
  *   Web interface (via ``listenHTTP[]`` command)
  *   Shell scripts
*   **Extensible:** Designed to allow adding custom Rate of Spread (ROS) models in C++.
*   **Applications:** Suitable for research, detailed case reanalysis, and ensemble forecasting.

Supported Platforms
-------------------

ForeFire is primarily developed and tested on **Unix-like systems (Linux and macOS)**