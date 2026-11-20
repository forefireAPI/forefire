.. _about-statement-of-need:

Is ForeFire the right tool?
===========================

Wildfire tools tend to fall on one of two sides, and each side has its limits
(`Filippi et al., 2025 <https://doi.org/10.21105/joss.08680>`_):

- **Combustion and CFD codes** such as FIRETEC or WFDS resolve the flame in 3D,
  but cost too much to forecast a landscape-scale fire faster than real time.
- **Atmosphere-coupled codes** such as WRF-SFire run inside an atmospheric
  model and need the computing power and data that come with it.
- **Operational simulators** such as Farsite (now FlamMap) or Prometheus spread
  fronts over tens of kilometers in seconds, but with fixed modeling choices,
  as compiled programs with a graphical interface and little scripting.
  Open-source libraries such as ELMFIRE or Cell2Fire are tied to one
  rate-of-spread model and have no scripting language.

ForeFire sits between the two:

- **Fast where it matters.** The front is a set of markers moved by discrete
  events, so the computation stays on the active part of the front, at meter
  resolution. The JOSS paper reports more than 100 hectares per second on a
  single CPU.
- **Models you can swap or write.** Rothermel, Balbi, a Farsite emulation and a
  neural-network model ship with the engine, and a new one is a single
  ``.cpp`` file (:doc:`../user_guide/writing_models`).
- **One script language everywhere.** The same ``.ff`` commands run from the
  command line, from Python, in the browser (WebAssembly), and coupled both
  ways with the `Meso-NH <https://mesonh.cnrs.fr/>`_ atmospheric model.

Who uses it
-----------

- **Researchers** developing and comparing rate-of-spread and flux models, or
  running coupled fire–atmosphere simulations.
- **Forecasting services**: ForeFire runs in the French national decision
  support system OPEN DFCI, on the `FireCaster <https://forefire.univ-corse.fr/>`_
  demonstration platform, and in commercial services (AriaFire Firecaster,
  umgrauemeio Pantera, OroraTech FireSpread).
- **Teachers and students**, who can run it in Python or in a browser tab.
