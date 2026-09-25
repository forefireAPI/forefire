.. _userguide-python-arrays:

Running from NumPy arrays (no NetCDF, no GIS)
=============================================

ForeFire can run a complete simulation from plain NumPy arrays. You do **not**
need a :ref:`NetCDF landscape file <userguide-landscape-file>`, and you do not
need any GIS software: build a fuel map and a wind field as arrays in memory,
hand them to the engine, and step the simulation.

This is the lowest-friction way to use ForeFire, and it is the right entry
point if you install with ``pip install forefire`` and want *NumPy in, NumPy
out* — for example to generate training data for a surrogate model, to sweep
idealized scenarios, or simply to learn the API before wiring up real
geospatial data.

The two array layers
--------------------

Two binding methods take NumPy arrays directly:

* ``addIndexLayer(name, "fuel", SWx, SWy, SWz, Lx, Ly, Lz, array)`` — the
  **fuel index map**. Each cell holds an integer index into the fuel table.
* ``addScalarLayer(name, variable, SWx, SWy, SWz, Lx, Ly, Lz, array)`` — a
  continuous field such as a **wind component**.

The ``SW*`` / ``L*`` arguments give the south-west corner and the extent of the
layer in the simulation's Cartesian metres, so the array is stretched to cover
the domain regardless of its pixel resolution.

A complete example
------------------

The script below ignites a fire at the centre of a 4 km square domain and
drives it with a uniform easterly wind, entirely from arrays. It runs as-is
against a ``pip``-installed ForeFire.

.. code-block:: python

   import numpy as np
   import pyforefire as forefire
   from pyforefire import helpers

   ff = forefire.ForeFire()

   # Fuel table, inline — no CSV file needed. The columns depend on the
   # propagation model: the WindDriven model used here only needs vv_coeff
   # (see "Choosing a fuel table" below).
   ff["fuelsTable"] = "Index;vv_coeff;Kcurv;beta\n1;1.0;1.0;1.0\n2;0.3;1.0;1.0"
   ff["defaultFuelType"] = 1.

   # Solver parameters.
   for key, value in {
       "spatialIncrement": 0.5,
       "minimalPropagativeFrontDepth": 10,
       "perimeterResolution": 10,
       "initialFrontDepth": 0.1,
       "relax": 1.0,
       "smoothing": 0,
       "minSpeed": 0.0,
       "bmapLayer": 1,
       "windReductionFactor": 1.0,
   }.items():
       ff[key] = value
   ff["propagationModel"] = "WindDriven"

   # Define the domain in Cartesian metres — no NetCDF involved.
   Lx = Ly = 4000.0
   ff.execute(f"FireDomain[sw=(0,0,0);ne=({Lx},{Ly},0);t=0]")

   # Build the environment as arrays. The fuel map is a 4-D array shaped
   # (1, 1, width, height); the wind components are shaped (1, 2, width, height).
   W = H = 200
   fuel_map = np.full((1, 1, W, H), 1.0)      # fuel index 1 everywhere

   wind = np.zeros((2, 2, W, H))
   windU = wind[0:1]; windU[0, 0].fill(1.0); windU[0, 1].fill(0.0)
   windV = wind[1:2]; windV[0, 0].fill(0.0); windV[0, 1].fill(1.0)

   # Register the propagation model and the array layers.
   ff.addLayer("propagation", "WindDriven", "propagationModel")
   ff.addIndexLayer("table", "fuel", 0., 0., 0., Lx, Ly, 0., fuel_map)
   ff.addScalarLayer("windScalDir", "windU", 0., 0., 0., Lx, Ly, 0., windU)
   ff.addScalarLayer("windScalDir", "windV", 0., 0., 0., Lx, Ly, 0., windV)

   # Ignite at the centre and step the simulation, updating the wind each step.
   ff.execute("startFire[loc=(2000,2000,0.);t=0]")

   t = 0.0
   for _ in range(8):
       ff.execute("trigger[wind;loc=(0.,0.,0.);vel=(8,0,0);t=%f]" % t)
       ff.execute("step[dt=30]")
       t += 30

Getting the result back as an array
-----------------------------------

The :ref:`print <cmd-print>` command returns the fire front as text, and the
``helpers.printToPathe`` helper turns that into Matplotlib paths whose
``vertices`` are ordinary ``(N, 2)`` NumPy arrays of front coordinates — the
*NumPy out* half of the loop:

.. code-block:: python

   out = ff.execute("print[]")
   pathes = helpers.printToPathe(out)
   verts = np.vstack([p.vertices for p in pathes])

   print("front vertices:", verts.shape)
   print("x range: %.0f..%.0f   y range: %.0f..%.0f" % (
       verts[:, 0].min(), verts[:, 0].max(),
       verts[:, 1].min(), verts[:, 1].max()))

With the easterly wind above, the front that started as a point at
``(2000, 2000)`` has stretched downwind to roughly ``x = 3900`` after four
minutes of simulated time. You now have the perimeter as an array to plot,
post-process, or feed into whatever comes next.

Choosing a fuel table
---------------------

The columns of ``fuelsTable`` are read by the propagation model, so they change
with the model you select:

* ``WindDriven`` — a minimal table with ``vv_coeff`` (the rate of spread is
  ``vv_coeff × normal wind``), plus ``Kcurv`` and ``beta``. Convenient for
  idealized and teaching cases.
* ``Rothermel``, ``BalbiNov2011``, ``Balbi2015`` and the other physical models
  — the full fuel description in ``fuels.csv``. See
  :ref:`Fuels and Models <userguide-fuels-and-models>` for the columns.

To make propagation vary across the domain, add rows to the fuel table and
paint their indices into ``fuel_map``. For instance, filling the right half of
the map with a slower fuel:

.. code-block:: python

   fuel_map[0, 0, :, H // 2:] = 2.0   # index 2 = slower fuel

Where to go next
---------------

* ``tests/python/idealizedwind.py`` in the repository is a fuller worked
  version of this example: it drives the same array-based setup with a *turning*
  wind and plots each step.
* To run on real terrain instead of arrays, build a
  :ref:`NetCDF landscape file <userguide-landscape-file>` and load it with
  :ref:`loadData <cmd-loadData>`.
* :ref:`Fuels and Models <userguide-fuels-and-models>` explains the fuel table
  used by the physical propagation models.
