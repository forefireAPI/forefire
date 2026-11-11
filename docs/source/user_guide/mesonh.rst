Fire-atmosphere coupling with Meso-NH
============================================================

ForeFire can run coupled to `Meso-NH <https://mesonh.cnrs.fr/>`_, an
atmospheric research model developed by CNRS and Meteo-France. In a coupled
run, the fire changes the atmosphere, and the atmosphere feeds back on the
fire's spread.

What each model does
--------------------

ForeFire tracks the fire perimeter and computes its propagation from the
fuel, terrain and local conditions. Its flux models describe the heat and
water vapour released by burning fuel. Meso-NH computes the evolution of
the atmosphere, including the wind that drives the fire.

The exchange works in both directions:

.. code-block:: text

   Meso-NH  -- wind ---------------------->  ForeFire
   Meso-NH  <-- heat and water vapour -----  ForeFire

Heating from the fire can change the atmospheric flow. The updated wind
then affects the next advance of the fire perimeter. This feedback is what
makes a coupled run useful for studying fire-atmosphere interactions.

In a standalone ForeFire run, wind can also vary in space and time, but
ForeFire does not itself solve the atmospheric response to the fire.

How a coupled run starts
------------------------

Meso-NH drives the simulation and loads ForeFire as a shared library.
You launch the Meso-NH executable with MPI rather than starting a separate
ForeFire command-line session. The atmospheric and fire models exchange
data during the run.

This requires a native ForeFire build with MPI coupling enabled and a
compatible Meso-NH build. The published Python wheels do not include MPI
coupling; the browser WebAssembly demo does not run Meso-NH either.

Try the supplied cases
----------------------

The repository includes two examples:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Case
     - Purpose
   * - ``tests/mnh_ideal``
     - A short run with idealised relief and one atmospheric domain.
   * - ``tests/mnh_real_nested``
     - The Pedrogao Grande 2017 case, with two nested atmospheric domains.
       It runs 20 minutes of simulated time using four MPI processes.

Follow the `coupled installation and execution guide
<https://github.com/forefireAPI/forefire/blob/dev/docs/mnh/coupledMNHInstall.md>`_
for prerequisites, compilation, linking the library and running either
case. The guide was tested with Meso-NH 6.0.1 on Ubuntu 24.04 and also
explains how to compare coupled results between ForeFire versions.

Watch the simulation in 3D
------------------------------------------------------------

The Pedrogao case starts a local HTTP server on port 8080. After completing
the setup in the guide, set ``FOREFIREHOME`` to the root of your ForeFire
checkout in the same shell, before launching the case:

.. code-block:: bash

   export FOREFIREHOME=/path/to/forefire
   cd "$FOREFIREHOME/tests/mnh_real_nested"
   bash run.bash

While it runs, open ``http://localhost:8080/mnh3d.html``. The page displays
the terrain, fuel, fire perimeter, plume top and surface wind. The browser
needs internet access to load its JavaScript dependencies.

The page follows the run live. Once it has steps to show, a Timeline slider
lets you scrub or play them back. For this bundled case, opening the page
after the run replays the steps saved in ``ForeFire/Outputs`` and ``MODEL2``. Use one viewer tab at a time. For comparisons of
simulation outputs, leave the viewer closed, as explained in the guide.
