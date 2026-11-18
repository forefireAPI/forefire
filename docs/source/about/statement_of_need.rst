.. _about-statement-of-need:

Statement of Need
=================

Wildfire modeling tools have historically been split between **complex combustion research models** and **streamlined operational tools**, each with distinct limitations:

- **Computational Fluid Dynamics (CFD) and combustion codes** (such as FIRETEC or WFDS) resolve detailed flame physics in 3D, but are too computationally expensive to provide landscape-scale wildfire forecasts faster than real time.
- **Atmosphere-coupled codes** (such as WRF-SFire) must run inside a full mesoscale atmospheric model, requiring substantial computing resources and meteorological preprocessing even when uncoupled surface runs would suffice.
- **Operational wildfire simulators** (such as FARSITE / FlamMap or Prometheus) simulate fire fronts spanning tens of kilometers in seconds, but bake in fixed modeling assumptions and are primarily distributed as compiled desktop applications with limited scriptability. Other open-source spread libraries (such as ELMFIRE or Cell2Fire) are tied to a single rate-of-spread formulation and lack a domain-specific scripting language.

ForeFire's Contribution
-----------------------

Developed by CNRS at the Université de Corse Pascal Paoli (`Filippi et al., 2025 <https://doi.org/10.21105/joss.08680>`_), **ForeFire** bridges the gap between customizable research codes and fast operational solvers by providing a **unified, modular, scriptable wildfire simulation engine**:

1. **Discrete-Event Lagrangian Front Tracking:**
   By representing the fire perimeter as a dynamic contour of Lagrangian markers updated through a discrete-event ``TimeTable``, ForeFire concentrates computation at meter-scale resolution strictly along the active fire front, achieving simulation speeds above 100 ha/s on a single CPU.

2. **Rapid Prototyping of Spread and Flux Models:**
   Propagation and flux models plug into the engine through a clean C++ Strategy interface (``registerProperty`` + ``getSpeed`` / ``getValue``). Researchers can compare Rothermel, Balbi, FARSITE, or neural-network formulations—or add a new physical model in a single ``.cpp`` file—without touching the front-tracking solver or I/O layers.

3. **Scriptable Across Python, CLI, WebAssembly, and HPC:**
   The same ``.ff`` command language drives batch experiments, interactive debugging, HTTP web visualization, Python/NumPy workflows (``pyforefire``), browser-based WebAssembly simulations, and two-way MPI coupling with the `Meso-NH <https://mesonh.cnrs.fr/>`_ atmospheric model.

Target Audience
---------------

- **Wildfire & Atmospheric Researchers:** developing and benchmarking rate-of-spread formulations, heat/chemical flux parameterizations, surrogate ML models, and coupled fire–atmosphere simulations.
- **Operational Forecasting & Decision Support Teams:** running fast deterministic or ensemble burn-probability forecasts (used in platforms such as OPEN DFCI, FireCaster, AriaFire, umgrauemeio Pantera, and OroraTech FireSpread).
- **Educators & Students:** exploring wildland fire dynamics interactively in Python or directly in a browser tab.
