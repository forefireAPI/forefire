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
    orcid: 0009-0001-9043-2039
    corresponding: false
  - name: Alberto Alonso-Pinar
    orcid: 0009-0009-2051-9700
    affiliation: "1, 3"
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
Wildfire forecasting is both an active research area and an important need for decision support systems. **ForeFire** is a modular, high-performance, scriptable, discrete‑event‑driven simulation engine [@filippi2009] focusing computational effort on the active region of a fire front defined as a dynamic mesh (or multipolygons) of fire markers. It is designed to model the spread of wildfire perimeters over large landscapes at a meter scale resolution in seconds, serving both as a research platform and a tool for operational forecasting. The core C++ library has Fortran and Python bindings and is accompanied by a lightweight, scriptable interpreter (a custom FF language) that can load, save, and export data in NetCDF, GeoJSON, KML, PNG, and JPG, and includes a local HTTP service with a customizable graphical user interface. ForeFire can also account for fire-atmosphere interaction by two-way coupling with the MesoNH [@lac2018] atmospheric model [@filippi2013].

# Statement of need

Wildfire modeling tools have historically been split between **complex combustion research models** and **streamlined operational tools**, each with distinct limitations. 
Computational combustion and fluid dynamics (CFD)- based models (e.g., FIRETEC [@linn2005] or WFDS [@mell2007]) are highly computationally intensive yet unable to provide large-scale wildfire forecasts faster than real time. 
Atmospheric-coupled codes, such as WRF/SFire [@mandel2011], must be run within an atmospheric model and require substantial processing power and data.
Operational wildfire simulators, such as widely used Farsite [@finney1998] (now Flammap [@Finney2023FlamMap]), or Canadian Prometheus [@garcia2008], can simulate fire fronts spanning tens of kilometers in a matter of seconds, but have definite built-in modeling assumptions and are distributed as compiled software with graphical interfaces with limited scriptability. Other open-source libraries include ElmFire [@lautenberger2013] and Cell2Fire [@pais2021], which are tied to a single spread model and do not include a scripting language. Deep-learning approaches also exist, such as PyTorchFire [@XIA2025106401].

ForeFire was developed as a community tool to fill the gap between highly complex customizable models and more rigid operational tools: a **unified** wildfire simulator that is both **adaptable** (highly scriptable with multiple bindings) and **high-performing** (discrete‑event‑driven simulation with dynamic mesh allows to concentrate computation at meter scale resolution only on the active part of the front to perform speed over 100 Ha per second on a single CPU). It is intended to serve both as a research platform and a tool for operational forecasting.

# Typical Use Cases

## Rapid prototyping of new models
ForeFire implements several standard fire flux and spread rate models, such as Rothermel [@andrews2018] and Balbi [@balbi2009], and makes it trivial to switch, extend, or add to this base with a single `.cpp` file using any existing model file as a template.
Internally, data is handled as *layers* that can come from a NumPy array, be read from NetCDF, or be generated on the fly by ForeFire (e.g., slope derived from the elevation layer, fuel loaded as an index map with tabulated fuel — with part of [@Scott2005] fuel table already available). 
Developing a Rate Of Spread wildfire model was the original purpose of this simulation code and helped to iterate versions of the Balbi Rate Of Spread formulation on case studies in [@balbi2009] and [@santoni2011]. It also served to implement various heat and chemical species flux models used for volcanic eruption in [@filippi2021], plume chemistry [@strada2012], or industrial fires in [@baggio2022]. In addition, the code includes a generic `ANNPropagationModel` that implements a feedforward artificial neural network (ANN) and expects a pre-trained graph file.

## Batch simulations with the ForeFire scripting
Custom FF language allows users to easily generate multiple scenarios, including fire-fighting strategies, model evaluation [@filippi2014], ensemble forecasts [@allaire2020], or generate a deep learning database [@allaire2021]. A FF script is a set of scheduled instructions that are interpreted in real-time, advancing the simulation clock with a `step[dt=]` or a `goTo[t=]` command. 
Each of these commands (such as `goTo[t=42]`, `include[state.ff]`, `startFire[lonlat=(-8.1, 39.9,0)]@t=42`, `setParameter[propagationModel=Rothermel]` or `plot[parameter=speed;filename=ROS.png]`) can also be called from HTTP, C++, Fortran or Python, and constitutes the core logic of the library. Help and autocompletion are directly available in the interactive shell interpreter that also includes a batch mode. The graphical user interface is web‑based through an embedded HTTP service (command `listenHTTP[host:port]`) with user‑defined or default pages as shown in \autoref{fig:gui}.

![Default web interface with data layers on the left pane, commands displayed as buttons and displaying an atmospheric coupled simulation of a wildfire in Portugal.\label{fig:gui}](gui.jpg)

By utilizing pre-compiled datasets over extensive regions, this approach supports continent-wide operational forecasting services. It has been deployed to identify optimal escape routes [@kamilaris2023], integrated into the French National WildFire Decision Support System [OPEN DFCI](https://opendfci.fr/), showcased on the [FireCaster demonstration platform](https://forefire.univ-corse.fr/), and also currently used in commercial simulation services [AriaFire Firecaster](https://firecaster.ariafire.com), [umgrauemeio Pantera](https://www.umgrauemeio.com/en), and [Ororatech FireSpread](https://ororatech.com/all-products/fire-spread).


### Two-way coupling with the MesoNH atmospheric model
The same scripts can be executed in coupled mode with the Open-Source atmospheric model [MesoNH](https://mesonh.cnrs.fr/) [@lac2018] with fire propagating using surface fields (wind) from MesoNH and forcing heat and other flux fields into the atmosphere. An idealized coupled simulation can be run on a laptop at field scale [@filippi2013], but also on a supercomputer to forecast fire-induced winds of large wildfires [@filippi2018], fire-induced convection [@couto2024], [@campos2023], or even to estimate wildfire spotting [@alonsopinar2025].

Coupled simulations generate gigabytes of 3D data that can be converted to VTK/VTU files using Python helper scripts to visualize in the open-source tool ParaView, as shown in \autoref{fig:coupled}. 

![Coupled simulation of the Pedrogao Grande wildfire [@couto2024] (Paraview). On the ground, the burned area is in orange, while among atmospheric variables, downbursts are highlighted in red and pyro-cumulonimbus clouds in blue.\label{fig:coupled}](coupled.jpg)

# Acknowledgements
This work has been supported by the Centre National de la Recherche Scientifique and French National Research Agency under grants **ANR-09-COSI-006-01 (IDEA)** and **ANR-16-CE04-0006 (FIRECASTER)**. The authors thank all contributors and collaborators who have assisted in the development and testing of the ForeFire software.

# References
