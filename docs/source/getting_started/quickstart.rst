.. _quickstart:
.. _execution_modes:

Quick Start & Execution Modes
=============================

ForeFire can be run in four ways: directly in your browser (via WebAssembly), from Python, from the command-line interpreter, or inside Docker with the interactive web console.

1. Try it in the browser (no install)
-------------------------------------

The `in-browser WebAssembly demo <https://forefireapi.github.io/forefire/>`_ runs the C++ engine directly in your browser tab over elevation and fuel layers decoded from map tiles:

- **Explore mode:** click anywhere on the map to ignite a fire, adjust wind speed and direction on the dial, and scrub the timeline in 2D or 3D terrain view.
- **Research mode:** switch propagation models (``Rothermel``, ``BalbiNov2011Curv``, ``BalbiUnsteady``, ``TroisPourcent``, ``Iso``), tune fuel moistures (``Md``, ``Ml``), bed depth (``e``), ``windReductionFactor``, ``perimeterResolution``, and ``spatialIncrement``, and inspect arrival-time isochrones or front spread speeds.

.. _quickstart-pip:

2. Run from Python (1 minute)
-----------------------------

On Linux or macOS (Apple Silicon), install the pre-built wheel:

.. code-block:: bash

  pip install forefire

Run an idealized simulation in memory and plot the expanding perimeters:

.. code-block:: python

  import numpy as np
  import pyforefire as forefire

  ff = forefire.ForeFire()
  ff.execute("FireDomain[sw=(0,0,0);ne=(10000,10000,0);t=0]")
  ff.addLayer("propagation", "Iso", "propagationModel")
  ff.execute("startFire[loc=(5000,5000,0.0)]")
  ff.execute("step[dt=1000]")
  print(ff.execute("print[]"))

``print[]`` returns the fire front hierarchy as text — one ``FireNode`` entry per marker along the perimeter, with its coordinates, velocity vector, and timestamp. To feed custom fuel maps and wind fields from NumPy arrays, continue to :doc:`/user_guide/python_arrays`.

3. Command-line interpreter (Batch, Interactive & Local Web UI)
---------------------------------------------------------------

Whether installed via ``pip`` or built from source, the ``forefire`` executable runs ``.ff`` scripts in three modes. The repository includes a reference case in Corsica under ``tests/runff/`` (note: pull Git LFS objects first if working from a git clone):

- ``real_case.ff`` — main simulation script
- ``params.ff`` — solver and model parameters included by ``real_case.ff``
- ``data.nc`` — NetCDF landscape file (elevation and fuel indices)
- ``fuels.csv`` — fuel property table

Mode A: Batch execution (``-i``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Feed a script file non-interactively from the directory containing its data:

.. code-block:: bash

  cd tests/runff
  forefire -i real_case.ff

This runs the script from start to finish and writes ``to_reload.ff`` (front state) and ``ForeFire.0.nc`` (arrival-time matrix).

Mode B: Interactive shell
~~~~~~~~~~~~~~~~~~~~~~~~~

Start the interpreter with no arguments to get an interactive ``forefire>`` prompt with command history and tab completion:

.. code-block:: text

  $ cd tests/runff
  $ forefire
  forefire> include[real_case.ff]
  forefire> getParameter[propagationModel]
  forefire> step[dt=600]
  forefire> quit[]

Mode C: Local Web UI (``listenHTTP[]`` or ``-l``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The interpreter embeds a local HTTP command and visualization server.

.. note::
   Outside Docker, ``FOREFIREHOME`` **must** point to a ForeFire source checkout so the HTTP server can serve ``$FOREFIREHOME/tools/htdocs``. Without ``FOREFIREHOME``, ``http://localhost:8000/`` returns ``404``.

.. code-block:: bash

  export FOREFIREHOME=/path/to/forefire
  cd "$FOREFIREHOME/tests/runff"
  forefire -l

Or from inside an interactive session:

.. code-block:: text

  forefire> listenHTTP[]
  ForeFire HTTP command server listening at http://localhost:8000

Open ``http://localhost:8000/`` in your browser.

.. _quickstart-docker:

4. Full Web UI walkthrough with Docker
--------------------------------------

The official container image ``ghcr.io/forefireapi/forefire:latest`` bundles the compiled engine, ``FOREFIREHOME``, the web interface, and the ``tests/runff`` dataset. It works on Linux, macOS, and Windows without cloning the repository or installing Git LFS.

1. **Start the container:**

   .. code-block:: bash

     docker run -it --rm -p 127.0.0.1:8000:8000 ghcr.io/forefireapi/forefire:latest

   Binding ``127.0.0.1:8000:8000`` restricts port ``8000`` to your local machine. You land in a shell as user ``forefire`` inside ``/forefire``.

2. **Start ForeFire in the test directory and bind the HTTP server:**

   .. code-block:: bash

     cd tests/runff
     forefire

   At the prompt, bind to ``0.0.0.0:8000`` so the container port forwarding can reach the server (``listenHTTP[]`` with no argument binds only to the container's internal ``localhost``):

   .. code-block:: text

     forefire> listenHTTP[0.0.0.0:8000]

3. **Run the simulation from your browser:**

   Open ``http://localhost:8000/`` on your host machine:

   - Type ``include[real_case.ff]`` in the command box (or click the **`include`** button) and click **`Send`**.
   - Click **`Refresh Map`** to display the simulated fire perimeters in the Aullène region of Corsica.

   .. note::
      When running ``include[real_case.ff]`` through the Web UI, the console prints ``systemExec is not available over HTTP``. This is expected: for security, shell commands (the ``systemExec[bash check_data_nc.bash]`` check on line 1 of ``real_case.ff``) are refused over HTTP, while the rest of the simulation runs normally.

   .. image:: /_static/images/gui_real_case_ff.jpg
     :alt: ForeFire Web UI showing the Aullène simulation in Corsica
     :align: center
     :width: 90%

4. **Exit:** type ``quit[]`` at the ``forefire>`` prompt, then ``exit`` in the container shell.

Where to Go Next
----------------

- :doc:`/user_guide/forefire_script` — syntax and structure of ``.ff`` scripts.
- :doc:`/user_guide/models` & :doc:`/user_guide/fuels_and_models` — choosing a propagation model and preparing ``fuels.csv``.
- :doc:`/user_guide/python_arrays` & :doc:`/user_guide/ensembles` — driving ForeFire and ensembles from Python.
