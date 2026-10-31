.. _userguide-ensembles:

Ensembles and burn probability
==============================

A single simulation gives one perimeter for one wind. Wind forecasts are
uncertain, so the more useful answer is often a **burn-probability map**: run
the same fire under many plausible winds and count, for each point, the
fraction of runs that burnt it.

``pyforefire.run_ensemble`` runs the members and returns their arrival times;
``pyforefire.burn_probability`` turns them into that map. Each member runs in
its own :class:`ForeFireRunner` worker process, so the members run in parallel
and cannot disturb each other.

A complete example
------------------

The script below ignites a fire in a 4 km square domain, runs a two-minute
spin-up once, then forks 32 members that each continue under a different
easterly wind. It uses the minimal ``WindDriven`` model of
:ref:`Running from NumPy arrays <userguide-python-arrays>` and runs as-is
against a ``pip``-installed ForeFire.

.. code-block:: python

   import numpy as np
   import pyforefire

   L = 4000.0

   def setup(ff):
       ff["fuelsTable"] = "Index;vv_coeff;Kcurv;beta\n1;1.0;1.0;1.0"
       ff["defaultFuelType"] = 1.
       ff["windReductionFactor"] = 1.0
       ff["propagationModel"] = "WindDriven"
       ff.execute(f"FireDomain[sw=(0,0,0);ne=({L},{L},0);t=0]")
       ff.addLayer("propagation", "WindDriven", "propagationModel")
       ff.addIndexLayer("table", "fuel", 0., 0., 0., L, L, 0., np.ones((1, 1, 10, 10)))
       wind = np.zeros((2, 2, 10, 10))
       wind[0, 0] = 1.0  # windU along x
       wind[1, 1] = 1.0  # windV along y
       ff.addScalarLayer("windScalDir", "windU", 0., 0., 0., L, L, 0., wind[0:1])
       ff.addScalarLayer("windScalDir", "windV", 0., 0., 0., L, L, 0., wind[1:2])
       ff.execute("startFire[loc=(1000,2000,0);t=0]")
       ff.execute("trigger[wind;loc=(0,0,0);vel=(3,0,0)]")
       ff.execute("goTo[t=120]")  # spin-up shared by every member

   def perturb(ff, wind):
       u, v = wind
       ff.execute(f"trigger[wind;loc=(0,0,0);vel=({u},{v},0)]")

   rng = np.random.default_rng(1)
   angle = rng.normal(0, np.radians(20), 32)  # easterly, give or take 20 degrees
   speed = rng.uniform(2, 4, 32)
   winds = list(zip(speed * np.cos(angle), speed * np.sin(angle)))

   arrivals = pyforefire.run_ensemble(setup, winds, until=600, perturb=perturb)
   p = pyforefire.burn_probability(arrivals)

``arrivals`` is indexed ``[member, y, x]``, with row 0 at the south edge, and
holds ``inf`` where a member's front never arrived. ``p`` is indexed ``[y, x]``.
``burn_probability(arrivals, t=300)`` gives the probability of having burnt by
``t = 300`` instead of by the end of the run, so one ensemble answers the
question for any time up to ``until``.

To plot it:

.. code-block:: python

   import matplotlib.pyplot as plt

   fig, ax = plt.subplots()
   im = ax.imshow(p, origin="lower", extent=(0, L, 0, L), cmap="inferno", vmin=0, vmax=1)
   ax.contour(pyforefire.burn_probability(arrivals, t=300), levels=[0.5],
              origin="lower", extent=(0, L, 0, L), colors="cyan")
   fig.colorbar(im, label="burn probability")

Scoring against an observed fire
--------------------------------

Given the observed burnt area at time ``t`` as a boolean map ``observed`` on
the same grid (``[y, x]``, row 0 at the south edge), three helpers score the
ensemble:

.. code-block:: python

   p = pyforefire.burn_probability(arrivals, t)
   pyforefire.dice(arrivals <= t, observed)    # overlap of each member, 0 to 1
   pyforefire.brier_score(p, observed)         # error of the probability map, 0 is perfect
   frequency, count = pyforefire.reliability(p, observed)

* ``dice`` is 2|A ∩ B| / (|A| + |B|), one value per member.
* ``brier_score`` is the mean of ``(p - observed)²``. Applied to
  ``arrivals <= t`` it scores each member alone, so the ensemble can be
  compared with its members.
* ``reliability`` gives, for each probability bin (by default 0–0.1, 0.1–0.3,
  …, 0.9–1), the fraction of its cells that actually burnt. For a reliable
  ensemble, that fraction lies within the bin: about half of the cells given
  0.3–0.7 burnt. Stack the maps of several observation times to pool them.

An observed perimeter can be turned into such a map with
``rasterio.features.rasterize``; flip it with ``np.flipud`` if it comes north
row first.

What a member may change
------------------------

``setup(ff)`` builds the simulation; ``perturb(ff, member)`` applies one
member's changes. By default each member is a dict of parameters, set with
``ff[key] = value``.

With the default ``fork=True``, ``setup`` runs once and each member is a
:meth:`ForeFireRunner.fork` of it, so ``perturb`` runs **after** the spin-up.
That is the fast path, but it only reaches what the kernel reads while
stepping: the wind through ``trigger[wind;...]``, the fuels through
``trigger[fuel;...]``, new ignitions, and runtime parameters.

Some parameters are read once, when the domain and the models are built.
``windReductionFactor``, ``propagationSpeedAdjustmentFactor`` and
``Iso.speed`` are among them, so changing them on a fork has no effect. To
vary those, pass ``fork=False``: each member then gets a fresh runner and
``perturb`` runs **before** ``setup``.

.. code-block:: python

   members = [{"propagationSpeedAdjustmentFactor": f} for f in (0.4, 0.6, 0.8)]
   arrivals = pyforefire.run_ensemble(setup, members, until=600, fork=False)

Size and output
---------------

* The result holds one ``float64`` map per member at the domain's burning-map
  resolution. On a large domain, pass ``bbox=(left, bottom, right, top)`` in
  domain coordinates to keep only the area the fire can reach.
* Members run one per CPU by default; ``max_workers`` changes that.
* The kernel's output is discarded. Pass ``stdout=None`` to see it, or an open
  file to keep it.
* An exception in ``setup``, ``perturb`` or a member's run is raised again by
  ``run_ensemble``; a crash of a member's kernel raises
  ``pyforefire.WorkerDied``.
