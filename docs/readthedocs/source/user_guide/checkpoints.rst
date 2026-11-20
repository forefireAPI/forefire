.. _userguide-checkpoints:

Pausing and branching a fire
============================

A ``.ffstate`` file holds one fire at one instant. Save it to resume the run later, or to start several scenarios from the same moment: a wind shift, a fuel break, a different moisture. Checkpoints work on tile landscapes (:doc:`tiles`).

Save and resume
---------------

.. code-block:: none

   setParameters[fuelsTable=STDlandIndexedFuelsTable;propagationModel=Rothermel]
   loadData[provider=/path/to/tiles;center=(9.2,42.2);size=(0.1,0.1);date=2026-07-15T12:00:00Z]
   startFire[lonlat=(9.2,42.2,0);t=0]
   trigger[wind;vel=(5,2,0)]
   step[dt=3600]
   saveState[file=current.ffstate]

Later, in a new session:

.. code-block:: none

   loadData[current.ffstate]
   step[dt=3600]

Loading creates the domain, so there is no ``FireDomain`` and no ``loadData`` on the tiles. The fire comes back at the time it was saved, one hour here, and the next step takes it to two.

A relative path goes under :ref:`ForeFireDataDirectory <param-ForeFireDataDirectory>`, the ``ForeFire`` folder of the working directory unless you change it. That folder has to exist: without it, ``saveState`` stops with ``cannot create``.

Branch a scenario
-----------------

Load the same checkpoint once per scenario and change one thing before stepping:

.. code-block:: none

   loadState[file=current.ffstate]
   include[scenarios/windshift.ff]
   step[dt=3600]
   saveState[file=windshift-result.ffstate]

where ``scenarios/windshift.ff`` holds the change, here ``trigger[wind;vel=(0,8,0)]``. To run many branches from Python without reloading, see :doc:`ensembles`.

What the file holds
-------------------

The fronts and their points, with the motion they had pending; the arrival times; the parameters in effect and the fuel table, including changes made during the run; the wind; and the fuel changes, from ``trigger[tileFuel]`` or a fuel image (:doc:`tiles`).

The tiles themselves stay outside. The file records a hash of each one, and loading rejects a tile whose content changed. ``loadState[file=...;provider=...]`` reads the tiles from another directory or URL; the hashes still have to match.

When a save is refused
----------------------

``saveState`` writes the whole fire or nothing. It refuses, with a message, when the run holds something the format cannot carry:

- A propagation model other than ``Rothermel``, ``Balbi2015``, ``Farsite``, ``Iso`` or ``WindDriven``.
- A landscape that is not tiles, or a fuel, wind or terrain layer added on top of the tiles.
- Flux models with a spatial map.
- A command still waiting to run, such as ``print[]@t=9000``. Save before scheduling it, or after it has run.

If it asks to complete initialization, run ``step[dt=0]`` before saving.

Coupled with Meso-NH, ``loadState[mode=attach]`` attaches a standalone checkpoint to the running atmospheric simulation. The checks are described in ``tests/ffstate/README.md`` and ``tests/mnh/fftiles-coupling.md`` in the source tree.

All arguments: :ref:`saveState <cmd-saveState>`, :ref:`loadState <cmd-loadState>`.
