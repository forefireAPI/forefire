---


title: 'ForeFire: High‑performance C++ simulator and solver for wildland‑fire spread' 
tags:
  - C++
  - Python
  - wildfire simulation
  - wildland fire
  - forest fire
  - bushfire
  - fire behaviour
  - fire spread
  - front‑tracking
  - discrete‑event simulation
  - DEVS
  - combustion
  - fire physics
  - numerical solver
  - numerical modeling
  - high‑performance computing
  - parallel computing
  - mpi
  - fire‑atmosphere coupling
  - atmospheric modeling
  - mesoscale meteorology
  - weather coupling
  - data assimilation
  - fuel models
  - hazard prediction
  - risk assessment
  - decision support
  - emergency management
  - operational forecasting
  - software library
  - python bindings
  - command‑line tool
  - simulation engine
  - model validation
  - benchmarking
  - machine learning
  - neural networks
  - smoke modelling




authors:
  - name: Jean-Baptiste Filippi
    orcid: 0000-0002-6244-0648 
    affiliation: 1
    corresponding: true
  - name: Roberta Baggio
    orcid: 0000-0003-1531-0530
    affiliation: 1
    corresponding: false
  - name: Ronan Paugam
    orcid: 0000-0001-6429-6910  
    affiliation: 2
    corresponding: false
  - name: Frédéric Bosseur
    orcid: 0000-0002-8108-0887
    affiliation: 1
    corresponding: false
  - name: Antonio Leblanc
    affiliation: 4
    corresponding: false
  - name: Alberto Alonso-Pinar
    orcid: 0009-0009-2051-9700
    affiliation: "1,2"
    corresponding: false

affiliations:
 - name: SPE, UMR 6134, CNRS, University of Corsica Pascal Paoli, Corte, France # TODO: Verify exact affiliation wording and add ROR if available
   index: 1
   ror: 016nwev19 # Example ROR for University of Corsica
 - name: SPE, UMR 6134, CNRS, University of Corsica Pascal Paoli, Corte, France # TODO: Verify exact affiliation wording and add ROR if available
   index: 1
   ror: 016nwev19 # Example ROR for University of Corsica
 - name: SPE, UMR 6134, CNRS, University of Corsica Pascal Paoli, Corte, France # TODO: Verify exact affiliation wording and add ROR if available
   index: 
   ror: 016nwev19 # Example ROR for University of Corsica
   

date: DD Month YYYY 
bibliography: paper.bib

# Summary

**ForeFire** is a modular, high-performance wildland fire simulation engine implemented in C++. It is designed to model the spread of wildfire perimeters over large landscapes at meter scale resolution, serving both as a research platform and an operational forecasting tool. ForeFire can be used to forecast wildfires spanning thousands of hectares within seconds, supporting wildfire management operations, while also providing an open testbed for experimenting with new fire behavior, power, and fluxes models in a scientific context.

ForeFire offers multiple interfaces and utilities to maximize usability and integration. The core C++ library has Fortran and Python bindings (exposing arrays via NumPy) and is accompanied by a lightweight scriptable interpreter of the custom FF language, a local HTTP service that enables a customizable graphical user interface and ability to load, save and export data in NetCDF, GeoJson, KML, png and jpg (thanks to stb_image_write [CITE STB]). The software implements several standard fire fluxes and spread rate models, including different variants of the widely used semi-empirical Rothermel model \[@rothermel\_1972]\[@rothermel and andrews\_2017] as well as Balbi **et al.** semi physical formulation \[@balbi\_2009] and several simpler models. 
Users may easily switch between or extend these models, each of them is defined in a single C++ file and adding a one is as simple as adding a file with a model template. In addition, data-driven models can be used, including the ability to activate feedforward graphs trained externally (e.g., with TensorFlow or PyTorch) and serialized in a compact binary format. 
Eventually ForeFire is also capable of two-way coupling with the MesoNH [Cite MESONH] atmospheric model to account for fire-atmosphere interaction studies [@filippi\_2013]. This combination of high-performance computation, modular physics, and multi-interface support makes ForeFire a unique open-source tool for wildfire simulation.

# Statement of need


Wildfire modeling tools have historically been split between **complex combustion research models** and **streamlined operational tools**, each with distinct limitations. On one end of the spectrum, computational combustion and fluid dynamics (CFD) based models (e.g., **FIRETEC** \[@linn\_2002] or WFDS [CITE WFDS]) provide detailed physics at flame scale but are highly computationally intensive and yet unable to provide faster than real time large wildfire forecasting. On the other end, operational wildfire simulators prioritize speed and simplicity at the expense of flexibility – they often use fixed empirical formulas and are distributed as closed-source software, making it difficult for researchers to try new modeling approaches and streamline integration in experimental frameworks. This gap between highly complex models and rigid operational tools creates a need for an intermediate solution: a wildfire simulator that is both **adaptable** and **high-performance**. **ForeFire** was developed to fill this need by bridging the flexibility of research-oriented code with the efficiency and user-friendliness of operational systems \[@filippi\_2018].

Many existing fire spread simulation systems illustrate the aforementioned trade-offs:

* **FARSITE, now FLAMMAP** – A widely used fire area simulator \[@finney\_1994], freely available that runs on Windows and uses Rothermel’s empirical spread model. FARSITE (now included in FLAMMAP) has proven valuable for operational planning, but it is closed-source and limited to its built-in modeling assumptions, which hinders extensions or integration with other scientific tools.
* **ELMFire** – An Eulerian level-set based fire spread model (recently developed as an open-source tool in Fortran) that employs a single spread formula (also derived from Rothermel-like semi-empirical algorithms). Its design is less extensible, making it difficult to incorporate alternative fire spread physics or custom behaviors beyond the provided formulation.
* **GridFire** – A raster (grid-based open source code written in Clojure) fire behavior model that computes fire spread and intensity across gridded landscape data. While it supports standard fire behavior metrics on a landscape \[@mdpi\_fire\_forecasts], its structured-grid approach and fixed spread rate calculations make it less flexible for irregular geometries or new model parameters compared to a front-tracking framework.
* **WRF-Fire** – The fire module of the Weather Research and Forecasting model (also known as WRF-SFIRE) couples a cellular fire spread model with a weather simulation \[@mandel\_2011; @coen\_2013]. This system enables fully coupled fire-atmosphere simulations, but the fire solver itself is tightly integrated into the WRF codebase. Using WRF-Fire as a standalone fire model or modifying its fire spread mechanics requires working within a large atmospheric model, making it far less modular and accessible for rapid prototyping of fire models.

Each of the above systems addresses certain needs (e.g., operational use, physical fidelity, or coupling) but none provides a **unified solution** that is open, modular, and performant. ForeFire distinguishes itself by combining the strengths of these approaches without their drawbacks. It uses an **event-driven, front-tracking simulation kernel** \[@filippi\_2009] that represents the fire perimeter as a set of moving markers, focusing computational effort on the active fire front. This asynchronous time-stepping method (constant-CFL discrete event simulation) yields efficient scaling to large fires while maintaining high resolution where the fire is fastest. At the same time, ForeFire’s **modular physics engine** and scripting interface allow users to *easily swap or implement new fire spread models* (from simple empirical rules to advanced semi-physical models) without changing the core code. The software can run in a standalone mode (for purely surface fire simulations) or be **coupled** with atmospheric models under a consistent interface, using similar initialization and data inputs for either mode. ForeFire’s support for **multiple language bindings** (C++ API, Python/NumPy, Fortran interface for coupling, and even a Java wrapper for GUI integration) further expands its usability across different domains. These capabilities enable **rapid prototyping** and experimentation with fire behavior algorithms – researchers can test novel modeling ideas or incorporate machine-learning-based fire spread predictions within the ForeFire framework – and also support operational needs by allowing integration into decision support pipelines and coupling with weather forecasts. In summary, ForeFire provides a unique platform that merges the flexibility of research models with the practicality of operational tools, addressing a critical need in the wildland fire modeling community \[@filippi\_2018].

# Acknowledgements
This work has been supported by the CNRS and French National Research Agency under grants **ANR-09-COSI-006-01 (IDEA)** and **ANR-16-CE04-0006 (FIRECASTER)**. The authors thank all contributors and collaborators who have assisted in the development and testing of the ForeFire software.

# References

