---
title: 'ForeFire: A Modular, Scriptable C++ Simulation Engine and Library for Wildland-Fire Spread' 
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
  - MPI
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
  - smoke modeling

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
    affiliation: [1, 3]
    corresponding: false

affiliations:
 - name: SPE, UMR 6134, CNRS, University of Corsica Pascal Paoli, Corte, France
   index: 1
   ror: 016nwev19
 - name: Centre for Technological Risk Studies, Department of Chemical Engineering, Universitat Politécnica de Catalunya 
   index: 2
   ror: 03mb6wj31
 - name: Faculty of Science, The University of Melbourne, 4 Water St., Creswick, Australia 
   index: 3
   ror: 01ej9dk98
 - name: umgrauemeio 1.5°C, Brazil
   index: 4   

date: 25 May 2025 
bibliography: paper.bib

---

# Summary

**ForeFire** is a modular, high-performance wildland fire simulation engine implemented in C++. It is designed to model the spread of wildfire perimeters over large landscapes at meter scale resolution, serving both as a research platform and an operational forecasting tool. 
ForeFire can be used to forecast wildfires spanning thousands of hectares within seconds, supporting wildfire management operations, while also providing an open testbed for experimenting with new fire behavior, fire intensity, and fire flux models in a scientific context.

ForeFire offers multiple interfaces and utilities to maximize usability and integration. 
The core C++ library has Fortran and Python bindings and is accompanied by a lightweight scriptable interpreter (in custom FF language), a local HTTP service that enables a customizable graphical user interface and ability to load, save and export data in NetCDF, GeoJson, KML, png and jpg. 
ForeFire is discrete‑event‑driven [@filippi2009], focusing computational effort on the active region of the fire front defined as a dynamic mesh or multipolygons of fire markers. 

Eventually ForeFire is also capable of two-way coupling with the MesoNH [@lac2018] atmospheric model to account for fire-atmosphere interaction [@filippi2013].

# Statement of need

Wildfire modeling tools have historically been split between **complex combustion research models** and **streamlined operational tools**, each with distinct limitations. 
On one end of the spectrum, computational combustion and fluid dynamics (CFD) based models (e.g., FIRETEC [@linn2005] or WFDS [@mell2007]) provide detailed physics with complete control of combustion models at flame scale but are highly computationally intensive and yet unable to provide faster than real time large wildfire forecasting. 
On the other end, operational wildfire simulators built for practitioners, such as widely used Farsite (now flammap) [@finney1998] or Canadian Prometheus [@garcia2008], while not resolving the combustion at flame scale, are able to simulate tens of kilometers long fire front in matter of seconds, prioritizing speed and simplicity at the expense of generality. These codes have definite built-in modeling assumptions and are distributed (freely) as compiled software with graphical interfaces and limited access to source code. 
While being well-packaged is advantageous for direct use, it makes it difficult for researchers to implement new modeling approaches or to streamline and script integration into different experimental frameworks. This gap between highly complex customizable models and more rigid operational tools creates a need for an intermediate solution: a wildfire simulator that is both **adaptable** and **high-performing**. 
ForeFire was developed to fill this need by bridging the flexibility of research-oriented code with the efficiency and user-friendliness of operational systems [@filippi2018].

Other open source libraries for wildland fire spread do exist as for example ElmFire [@lautenberger2013] or Cell2Fire [@pais2021], but with respect to ForeFire, these packages use a different numerical scheme, and are tied to a single spread models and do not provide a scriptable scenario interface or atmospheric coupling. 
Another family of wildland fire spread solvers such as WRF/SFire [@mandel2011] can resolve coupled front propagation and local meteorology but must be run within an atmospheric model that requires a large amount of processing power and data.

Each of the above systems addresses certain needs (e.g., operational use, physical accuracy, or atmospheric coupling) but in order not to multiply development efforts on multiple code and validation bases, there was a need for a **unified solution** that could be open, modular, and performant. 
ForeFire’s modular architecture and scripting interface allow users to easily swap or implement new fire models without changing the core code and test multiple simulation scenarions with same input files. It can also run fast in a standalone mode (for purely surface fire simulations) or be coupled with atmospheric models, all under a consistent interface and parameterization.

ForeFire’s support for **multiple language bindings** (HTTP, C++ API, Python/NumPy, Fortran) is also unique and further expands its usability across different domains. These capabilities enable rapid prototyping and experimentation on large datesets, and also support operational needs by allowing integration into decision support pipelines including coupled with atmospheric models.

# Typical Use Cases

## Rapid prototyping of new spread or flux models
The software also implements several standard fire fluxes and spread rate models, including different variants of the widely used semi-empirical Rothermel model [@andrews2018] as well as Balbi semi‑physical formulation [@balbi2009], with an architecture that makes it trivial to switch, extend or add to this model base.  
Researchers who wish to experiment with alternative formulations can add a new model by adding a single `.cpp` file from the `src/propagation` or `src/flux` using any existing model file as template and implement the `getSpeed()` or `getValue()` function. Input variables—including fuel descriptors, wind components, or slope—are declared at the top of the file. 
Internally these quantities are handled as *layers* that can come from Python NumPy array, supplied from input NetCDF files or generated on the fly by ForeFire (e.g. slope derived from the elevation layer). 
Once compiled, the new model is available in the scripting interface and in the C++/Python APIs without additional changes.

Developping a Rate Of Spread wildfire model was the original purpose of this simulation code and helped to iterate versions the Balbi Rate Of Spread formulation on real-cases studies in [@balbi2009] and [@santoni2011]. It also served to implement various heat and chemical species flux models used for volcanic eruption in [@filippi2021], wildfire plume chemical compounds in [@strada2012] or industrial fires in [@baggio2022]. 
There is also a generic `ANNPropagationModel` that expects a trained graph input file that will be used in place of these functions. 

## Batch simulations with the ForeFire scripting
Custom FF language allows to easily generate multiple scenarios, including fire-fighting strategies or ensemble forecasts, with light scripting efforts. The interpreter run these scripts either from a file or from another process (piped trough the shell). A FF script is a set of instructions that are interpreted at a specific date (starting at a run date, with 0 defined as `00:00` and updated with a `step`or a `goTo` commands) or if the command is post-fixed by a `@t=` operator that schedules the execution of a command. 

Each of these commands (such as, `goTo[t=42]`, `print[state.ff]`, `include[state.ff]`, or `plot[parameter=speed;filename=ROS.png]`) of ForeFire is bound or can directly be called from HTTP, C++, Fortran or Python, and are the core logic of the library. Help messages and autocompletion are directly available from the shell interpreter that can be run interactively, with several examples provided in the repository.

A typical script that loads data at a date and time, starts a fire, schedules a wind shift and prints outputs is written :
```text
setParameter[propagationModel=Rothermel]
loadData[data.nc;2025-02-10T17:35:54Z]
startFire[lonlat=(-8.1, 39.9,0)]
trigger[wind;vel=(10.0,0.0,0.)]@t=75000
step[dt=36000]
print[]
```
But the interface can also be run interactively using a highly customizable web‑based graphical interface (command `listenHTTP[host:port]`), that starts a local HTTP service with ForeFire bindings and serves standard or user‑defined web pages as shown in \autoref{fig:gui}.

![Default web interface accessed trough ForeFire internal HTTP service and exposing interactive commands. The simulation here is a wildfire in Portugal with ForeFire coupled with an atmospheric model that provides winds\label{fig:gui}](gui.jpg)

By utilizing pre-compiled datasets over extensive regions, this approach supports continent-wide operational forecasting services. 
It is applied in identifying optimal escape routes [@kamilaris2023], integrated into the French national WildFire Decision Support System [OPEN DFCI](https://opendfci.fr/), showcased on the [FireCaster demonstration platform](https://forefire.univ-corse.fr/), and adopted by various commercial companies.

But the ability to run as batch also means it can be used to perform systematic model evaluation [@filippi2014] generate simulation ensembles [@allaire2020] or large simulation databases for deep learning [@allaire2021],[@allaire2022]

### 3. Two-way coupling with the MesoNH atmospheric model
The same scripts and input data can be executed in coupled mode with Open-Source atmospheric model [MesoNH](https://mesonh.cnrs.fr/) [@lac2018] with sample coupled cases available in the main [MesoNH Repository](https://src.koda.cnrs.fr/mesonh/). 
In this configuration MesoNH launches ForeFire (FORTRAN Bindings) as an MPI enabled sub-process, a master process integrates the fire perimeter using surface fields from the atmospheric model and distributes fluxes fields to the atmosphic model. Coupled simulation can be run on a laptop at field scale to evaluate model ability on prescribed well instrumented experiments [@filippi2013], but also on more than 400 processors to perform faster than real-time high resolution simulation of fire induced winds on a large wildfire [@filippi2018], fire induces convection during an extreme wildfire event [@couto2024],[@campos2023] or even to estimate wildfire spotting (firebrands) with a resolved fire plume [@alonsopinar2025].

While simulations in coupled mode can take significantly more time and generate much larger output files, they are designed to run efficiently on a supercomputing cluster with some specific routines to generate 3D output files through ForeFire. 
Python helper scripts available in the ForeFire repository convert these simulation outputs to VTK/VTU files, allowing three-dimensional rendering of fire-atmosphere variables in Open-Source [ParaView](https://www.paraview.org/) as shown in \autoref{fig:coupled}. 

![Coupled simulation of the Pedrogao Grande wildfire [@couto2024], rendered in Paraview. On the ground, the burned area is in orange, while among atmospheric variables, downbursts are highlighted in red and pyrocu clouds in blue\label{fig:coupled}](coupled.jpg)

# Acknowledgements
This work has been supported by the Centre National de la Recherche Scientifique and French National Research Agency under grants **ANR-09-COSI-006-01 (IDEA)** and **ANR-16-CE04-0006 (FIRECASTER)**. The authors thank all contributors and collaborators who have assisted in the development and testing of the ForeFire software.

# References
