.. _userguide-tiles:

Real terrain from tiles
=======================

``.fftile`` stores the landscape of one half-degree cell of the globe, and ``.ffstate`` stores one fire at one instant so a run can resume. Neither needs NetCDF, HDF5 or PROJ: a build with ``-DFOREFIRE_ENABLE_NETCDF=OFF`` reads both, and so do the Python wheels and the WebAssembly build. NetCDF landscapes (:doc:`landscape_file`) keep working.

.. code-block:: bash

   cmake -S . -B build -DFOREFIRE_ENABLE_NETCDF=OFF -DFOREFIRE_ENABLE_MPI=OFF
   cmake --build build -j

Landscape Tiles (.fftile)
-------------------------

A tile is named after its south-west corner, ``{lat}_{lon}.fftile`` (e.g. ``41.5_8.5.fftile``), and holds a fixed set of fields:

* **Fuel:** model IDs 0–255 on a global grid of about 10 m (lossless PNG). ``0`` is non-burnable or no data.
* **Altitude** (JPEG, or lossless 16-bit PNG when JPEG is not accurate enough).
* **Slope** east and north, ``dz/dx`` and ``dz/dy`` (JPEG).
* **Wind response** ``muu``, ``muv``, ``mvu``, ``mvv``: how the terrain turns and scales a uniform wind (JPEG).

The fuel properties, the propagation model and the weather are not in the tile; they stay in the case. Each tile has a halo around its core so interpolation across tile borders is continuous.

Write and inspect tiles with ``tools/tiles/fftile.py``; the binary layout is in ``tools/tiles/README.md`` in the source tree.

.. code-block:: bash

   python tools/tiles/fftile.py tiles/42_8.5.fftile --inspect

Running on tiles
~~~~~~~~~~~~~~~~

**From a tile directory or URL, centered on a point** (longitude, latitude). This creates the domain; no ``FireDomain`` command is needed:

.. code-block:: none

   loadData[provider=/path/to/tiles;center=(8.5,42);size=(0.5,0.5)]
   loadData[provider=https://example.org/tiles;center=(8.5,42);date=2026-07-15T12:00:00Z]

See :ref:`loadData <cmd-loadData>` for the arguments. HTTP tiles are cached in ``ForeFireDataDirectory/tiles-cache/`` (see :ref:`fftileCacheDirectory <param-fftileCacheDirectory>`) and reused offline. A missing tile (or HTTP 404/410) gives zero fuel and no spread; a corrupt tile or any other network error stops the run instead.

**With an explicit domain**, set :ref:`fftilesPath <param-fftilesPath>` and ``SWLngLat`` before ``FireDomain``. The domain can then be any size and may extend beyond the available tiles:

.. code-block:: none

   setParameter[fftilesPath=/path/to/tiles]
   setParameter[SWLngLat=8.49,41.99]
   setParameter[propagationModel=Rothermel]
   FireDomain[sw=(0,0,0);ne=(20000,20000,0);t=0]
   startFire[lonlat=(8.495,41.995,0);t=0]

Tiles found in ``caseDirectory/ForeFireDataDirectory/tiles/`` take precedence over ``fftilesPath``.

Changing the fuel
~~~~~~~~~~~~~~~~~

Tiles are read-only, so ``trigger[fuelIndice]`` does not apply. Two alternatives:

* :ref:`trigger[tileFuel] <cmd-trigger>` sets a fuel ID inside a longitude/latitude polygon.
* A case-local ``tiles/{lat}_{lon}_fuel.png`` (8-bit grayscale, south row first, same size as the tile's fuel image) replaces the fuel of that tile.

Checkpoints (.ffstate)
----------------------

.. code-block:: none

   saveState[file=current.ffstate]

In a new session:

.. code-block:: none

   loadData[current.ffstate]
   step[dt=3600]

The file holds the fronts and nodes with their pending motion, the arrival times, the effective parameters and fuel table, the wind and the fuel changes. The tiles stay outside and are identified by hash, so a changed tile is rejected. Loading restores the saved time and does not advance it. To branch a scenario from a saved fire:

.. code-block:: none

   loadState[file=current.ffstate]
   include[scenarios/fuelbreak.ff]
   step[dt=3600]
   saveState[file=fuelbreak-result.ffstate]

Current limits:

* Models: ``Rothermel``, ``Balbi2015``, ``Farsite``, ``Iso`` and ``WindDriven``, on tile domains.
* Custom raster layers, spatial flux-model maps, ANN models and pending script commands are not saved; ``saveState`` fails rather than writing a partial file.
* Initialize before saving: ``step[dt=0]`` or a real step.

A standalone state can be attached to a running Meso-NH simulation with ``loadState[mode=attach]``; see ``tests/ffstate/README.md`` and ``tests/mnh/fftiles-coupling.md`` in the source tree.
