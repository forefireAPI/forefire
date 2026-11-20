.. _userguide-python-arrays:

Made-up landscapes in Python
============================

When the landscape is an idea rather than a place, such as a slope with a fuel break, a valley that turns the wind, or a thousand random maps to train a model on, build it as NumPy arrays and hand it to ForeFire. No file and no GIS step: arrays in, the fire front back as an array.

It needs the Python package (``pip install forefire``). The pieces are the ones from :doc:`simulation_inputs`; only the landscape changes source.

A whole fire from arrays
------------------------

A 4 km square of uniform fuel, a wind blowing east, a fire lit in the middle and run for four minutes:

.. code-block:: python

   import numpy as np
   import pyforefire as forefire
   from pyforefire import helpers

   ff = forefire.ForeFire()
   ff["fuelsTable"] = "Index;vv_coeff\n1;1.0\n2;0.3"
   ff["propagationModel"] = "WindDriven"

   L = 4000.0
   ff.execute(f"FireDomain[sw=(0,0,0);ne=({L},{L},0);t=0]")

   ny = nx = 200
   fuel = np.full((1, 1, ny, nx), 1)   # fuel 1 everywhere

   wind = np.zeros((2, 2, ny, nx))     # the wind response
   wind[0, 0] = 1.0                    # windU = 1 × u + 0 × v
   wind[1, 1] = 1.0                    # windV = 0 × u + 1 × v

   ff.addLayer("propagation", "WindDriven", "propagationModel")
   ff.addIndexLayer("table", "fuel", 0., 0., 0., L, L, 0., fuel)
   ff.addScalarLayer("windScalDir", "windU", 0., 0., 0., L, L, 0., wind[0:1])
   ff.addScalarLayer("windScalDir", "windV", 0., 0., 0., L, L, 0., wind[1:2])

   ff.execute("startFire[loc=(2000,2000,0);t=0]")
   ff.execute("trigger[wind;vel=(8,0,0)]")
   ff.execute("step[dt=240]")

``WindDriven`` is a toy model, fast and easy to read: the fire runs at ``vv_coeff`` times the wind it faces, and its table has that one column. For a physical model, put its columns in the table instead (:ref:`fuel-columns-per-model`); the arrays stay the same.

How the arrays are laid out
---------------------------

Every layer is a 4-D array shaped ``(time, component, y, x)``. The first row is the south edge and the first column the west edge, and the ``L, L`` arguments stretch the array over the domain whatever its number of cells. Fuel has one time and one component, so it is ``(1, 1, ny, nx)`` of integers, each an ``Index`` of the fuel table. Paint a slower fuel over the east half:

.. code-block:: python

   fuel[0, 0, :, nx // 2:] = 2

Fuel ``0`` does not burn and needs no row in the table.

The wind response
-----------------

``trigger[wind;vel=(u,v,0)]`` gives one wind for the whole domain. The **wind response** (:doc:`simulation_inputs`) says what that wind becomes in each cell, as two linear maps, one per component:

.. code-block:: none

   windU = wind[0, 0] × u + wind[0, 1] × v
   windV = wind[1, 0] × u + wind[1, 1] × v

So ``np.zeros((2, 2, ny, nx))`` with ``wind[0, 0]`` and ``wind[1, 1]`` set to 1 is the identity: every cell feels exactly the wind of the trigger, as on flat open ground. With all zeros, no cell feels any wind. These are the four fields a tile stores as ``muu``, ``muv``, ``mvu`` and ``mvv`` (:doc:`tiles`), computed there from the real terrain.

Any other values bend the wind. Halve it where a ridge shelters the ground, or turn it 90° to the left over the whole domain:

.. code-block:: python

   wind[:, :, :, :nx // 2] *= 0.5          # west half: half the wind

   wind[0, 0], wind[0, 1] = 0.0, -1.0       # windU = -v
   wind[1, 0], wind[1, 1] = 1.0, 0.0        # windV =  u

With the turn, the same eastward trigger drives the fire north. A new ``trigger[wind]`` during the run changes ``u`` and ``v`` and keeps the response.

Getting the front back
----------------------

:ref:`print <cmd-print>` returns the front as text, and ``helpers.printToPathe`` turns it into Matplotlib paths whose ``vertices`` are ``(N, 2)`` arrays of coordinates in metres:

.. code-block:: python

   pathes = helpers.printToPathe(ff.execute("print[]"))
   front = np.vstack([p.vertices for p in pathes])
   print(front[:, 0].min(), front[:, 0].max())

After the four minutes above, the head has run from ``x = 2000`` to about ``x = 3900``.

Where to go next
----------------

- ``tests/python/idealizedwind.py`` in the source tree runs the same setup with a wind that turns at each step and plots the front.
- To run on a real place instead, use :doc:`tiles` or :doc:`landscape_file`.
