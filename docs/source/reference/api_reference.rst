API Reference (Python & C++)
============================

This page covers the Python bindings (``pyforefire``, installed via ``pip install forefire``) and the primary C++ classes in ``libforefire``.

Python API (pyforefire)
-----------------------

In-Process Engine (ForeFire)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. py:class:: pyforefire.ForeFire()

   In-process wrapper around the C++ ``PLibForeFire`` engine. Because the C++ core holds process-wide state, use :class:`ForeFireRunner` below when running multiple independent simulations in parallel.

   .. py:method:: execute(command: str) -> str

      Executes a ForeFire ``.ff`` command string (e.g., ``"step[dt=60]"``, ``"print[]"``) and returns the command's output as a string.

   .. py:method:: __getitem__(key: str) -> Union[float, str, numpy.ndarray]
   .. py:method:: __setitem__(key: str, value: Union[str, int, float])

      Gets or sets a simulation parameter in ``SimulationParameters`` (e.g., ``ff["propagationModel"] = "Rothermel"``; numeric parameters return a ``float``). If ``key`` is not a parameter, ``ff[key]`` queries ``FireDomain::getDataMatrix(key)`` and returns a 2D ``float64`` NumPy array (e.g., ``ff["arrival_time"]``, ``ff["BMap"]``). Also accessible via ``getString(key)``, ``setString(key, val)``, ``getDouble(key)``, ``setDouble(key, val)``, ``getInt(key)``, and ``setInt(key, val)``.

   .. py:method:: addLayer(type: str, layer_name: str, key_name: str)

      Registers a propagation, flux, or data layer across the domain (for example, ``ff.addLayer("propagation", "Rothermel", "propagationModel")``, where ``key_name`` must already be set in ``SimulationParameters``).

   .. py:method:: addIndexLayer(type: str, name: str, x0: float, y0: float, t0: float, width: float, height: float, timespan: float, values: numpy.ndarray)

      Adds a 4D integer NumPy array shaped ``(nt, nz, ny, nx)`` as an index layer (e.g., ``type="table"``, ``name="fuel"``). See :doc:`/user_guide/python_arrays`.

   .. py:method:: addScalarLayer(type: str, name: str, x0: float, y0: float, t0: float, width: float, height: float, timespan: float, values: numpy.ndarray)

      Adds a 4D ``float64`` NumPy array shaped ``(nt, nz, ny, nx)`` as a continuous scalar layer (e.g., ``altitude``, ``windU``, ``windV``).

Isolated Worker Process (ForeFireRunner)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. py:class:: pyforefire.ForeFireRunner(stdout=None, stderr=None)

   Runs a ``ForeFire`` instance inside a dedicated child process so multiple simulations can run concurrently without sharing global C++ state. Supports the ``with`` context-manager protocol and provides the same methods as :class:`ForeFire` (``execute``, ``ff[key] = val``, ``addLayer``, ``addIndexLayer``, ``addScalarLayer``), plus:

   .. py:method:: fork() -> ForeFireRunner

      Forks the worker process (using OS ``fork`` where available), returning a new :class:`ForeFireRunner` that shares the exact spun-up state and streams of the parent simulation at minimal cost.

   .. py:method:: close()

      Terminates the worker process.

.. py:exception:: pyforefire.WorkerDied

   Raised when a :class:`ForeFireRunner` child process exits unexpectedly (e.g., due to a fatal signal in the C++ kernel).

Ensembles, Burn Probability & Verification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :doc:`/user_guide/ensembles` for worked examples of these functions:

* ``pyforefire.run_ensemble(setup, members, until, perturb=..., fork=True, bbox=None, max_workers=None, stdout=subprocess.DEVNULL, stderr=None)`` — runs an ensemble of ``members`` up to simulation time ``until`` (by default ``perturb`` applies each ``member`` dict via ``ff[k] = v``) and returns a ``(n_members, ny, nx)`` array of fire arrival times (``inf`` where unburned).
* ``pyforefire.burn_probability(arrivals, t=None)`` — computes the fraction of ensemble members that burned each cell (by time ``t``, or by the end of the run if ``t is None``).
* ``pyforefire.arrival_time(ff, path, bbox=None)`` — extracts the 2D arrival-time matrix from an active simulation via ``plot[parameter=arrival_time;filename=<path>]``.
* ``pyforefire.dice(burnt, observed)`` — Sørensen–Dice similarity coefficient :math:`2|A \cap B| / (|A| + |B|)`.
* ``pyforefire.brier_score(probability, observed)`` — mean squared error of a burn-probability map against a boolean observed burn scar.
* ``pyforefire.reliability(probability, observed, bins=(0, 0.1, 0.3, 0.5, 0.7, 0.9, 1))`` — calibration curve ``(observed_frequency, cell_count)`` per probability bin edge interval.
* ``pyforefire.helpers.printToPathe(print_output)`` — parses ``ff.execute("print[]")`` output into a list of Matplotlib ``Path`` objects whose ``.vertices`` are ``(N, 2)`` NumPy arrays.

----

C++ API Reference
-----------------

This section provides an overview of the primary C++ classes in ``libforefire``.

Core Simulation Objects
~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: libforefire::FireDomain
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FireFront
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FireNode
   :project: ForeFire
   :members:

Data Management
~~~~~~~~~~~~~~~

.. doxygenclass:: libforefire::DataBroker
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::DataLayer
   :project: ForeFire
   :members:

.. note::
   ForeFire provides specializations of ``DataLayer`` for in-memory arrays, NetCDF variables, fuel tables, and atmospheric fields, including ``ArrayDataLayer``, ``NCXYZTDataLayer``, ``FuelDataLayer``, and ``FluxLayer``.

Scientific Model Interfaces
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: libforefire::PropagationModel
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FluxModel
   :project: ForeFire
   :members:

User Interaction & Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: libforefire::Command
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::SimulationParameters
   :project: ForeFire
   :members:

Fundamental Utilities
~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: libforefire::FFPoint
   :project: ForeFire
   :members:

.. doxygenclass:: libforefire::FFVector
   :project: ForeFire
   :members:
