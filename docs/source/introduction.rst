.. _introduction:

Introduction
============

Welcome to the official documentation for ForeFire.

What is ForeFire?
-----------------

ForeFire is a powerful, open-source **wildfire simulation engine** written in C++. Its core purpose is to simulate the spread of wildfires over complex terrain using various physical models and geospatial data inputs.

The engine's logic is compiled into a **reusable shared library (`libforefireL.so`)**. While this library can be directly linked against by other C/C++ or Fortran programs (the primary mechanism for coupling with atmospheric models like MesoNH), the most common way to use ForeFire is through the **`forefire` executable program**.

This `forefire` program acts as a **command-line interpreter**. It reads and executes commands from:

*   Script files (typically `.ff` files)
*   An interactive console session
*   An HTTP server interface (activated by the ``listenHTTP[]`` command, powering the web UI)

These commands instruct the underlying library engine to perform actions like loading data, setting parameters, defining ignitions, and advancing the simulation state over time.

This structure allows ForeFire to be both a versatile standalone simulation tool and a computational engine integrable into larger modeling systems.

Key Capabilities
----------------

*   **Physics-Based Simulation:** Implements established Rate of Spread (ROS) models (e.g., based on Rothermel equations) and provides a framework for adding custom models.
*   **Geospatial Data Handling:** Natively uses NetCDF files for input data layers such as elevation, fuel type, and weather variables.
*   **Fire-Atmosphere Coupling:** Explicitly designed for two-way coupling with atmospheric models (e.g., MesoNH) to capture crucial wind-fire interactions.
*   **High-Performance Computing:** The C++ core is optimized for performance, and MPI support enables efficient parallel execution on multi-core systems and clusters, allowing for large-scale and faster-than-real-time simulations.
*   **Multiple Interaction Modes:** Accessible via command-line scripts, an interactive console, a web-based visualization interface, direct C++ library linking, and potentially Python bindings.
*   **Extensibility:** Users can implement and integrate their own C++ modules for specific fire behavior (ROS) or flux calculations.
*   **Open Source:** Developed under the GPLv3 license, fostering transparency and community contributions.

Intended Use Cases
------------------

ForeFire is suitable for a range of applications, including:

*   Wildfire behavior research
*   Detailed reanalysis of past fire events
*   Operational or near-real-time fire spread forecasting (standalone or ensemble)
*   Investigating fire-atmosphere interactions through coupled simulations
*   Testing and comparing different fire spread models

Supported Platforms
-------------------

ForeFire is primarily developed and tested on **Unix-like systems (Linux and macOS)**.

About This Documentation
------------------------

This documentation aims to provide comprehensive guidance for users and developers. It is organized into the following main sections:

*   **Getting Started:** Installation, quick start guides, and running basic examples.
*   **User Guide:** Explanations of core concepts, configuration files, and common workflows.
*   **Reference:** Detailed reference material for commands, parameters, and file formats.
*   **API Reference:** Documentation for the C++ library classes and functions (generated from source code).
*   **About:** License, citation information, and acknowledgements.

We recommend starting with the :doc:`quickstart` or :doc:`installation` guides.