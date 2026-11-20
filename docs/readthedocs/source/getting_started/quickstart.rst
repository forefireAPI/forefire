.. _quickstart:
.. _execution_modes:

Your first fire
===============

There are three ways in. Pick the one that fits you: each installs what it needs and ends with a fire running.

- :ref:`In the browser <quickstart-browser>`: nothing to install. To see what ForeFire does.
- :ref:`From Python <quickstart-pip>`: ``pip install forefire``. To drive simulations from code.
- :ref:`From the command line <quickstart-docker>`: Docker, or pip and a copy of the repository. To run a real case from a script.

You only need to compile ForeFire yourself to couple it with Meso-NH, to build the WebAssembly module or to change the C++ (:doc:`installation`).

.. _quickstart-browser:

In the browser
--------------

The `online demo <https://forefireapi.github.io/forefire/>`_ runs the full C++ engine in your browser tab, compiled to WebAssembly. It opens on Scenario Studio: pick an ignition point in Corsica, set the wind, and watch the fire spread over real fuel and terrain (:doc:`/user_guide/scenario_studio`).

The `lighter demo <https://forefireapi.github.io/forefire/classic/>`_ runs anywhere on the globe, over fuel and elevation decoded from map tiles. Its research mode lets you switch propagation models, change fuel moisture and solver settings, and look at the arrival-time isochrones.

.. _quickstart-pip:
.. _install-pip:

From Python
-----------

.. code-block:: bash

   pip install forefire

There are wheels for Linux (``x86_64``, ``aarch64``) and macOS 14+ on Apple Silicon, for CPython 3.9 to 3.14. On Windows, use WSL. Nothing else is needed: NetCDF comes inside the wheel. The package is called ``forefire``, the module ``pyforefire``.

The smallest fire possible: a square of 2 km, a fire lit in the middle, ten minutes of spread at 0.5 m/s.

.. code-block:: python

   import pyforefire as forefire

   ff = forefire.ForeFire()
   ff["propagationModel"] = "Iso"
   ff["Iso.speed"] = 0.5

   ff.execute("FireDomain[sw=(0,0,0);ne=(2000,2000,0);t=0]")
   ff.execute("startFire[loc=(1000,1000,0);t=0]")
   ff.execute("step[dt=600]")

   front = ff.execute("print[]")
   print(front)

Ignore the warning about ``fuels.ff``: ``Iso`` reads no fuel. Each ``execute`` runs one ForeFire command, the same commands a script uses. ``print[]`` returns the front, one ``FireNode`` per point along it:

.. code-block:: none

   FireDomain[sw=(0,0,0);ne=(2000,2000,0);t=600]
       FireFront[id=2;domain=0;t=0]
           FireNode[domain=0;id=4;fdepth=20;kappa=0.0110199;loc=(992.959,1375.89,0);vel=(-0.020101,0.499596,0);t=596.525;state=moving;frontId=2]
           ...

To see it, turn it into coordinates (``pip install matplotlib``):

.. code-block:: python

   import matplotlib.pyplot as plt
   from pyforefire.helpers import printToPathe

   for path in printToPathe(front):
       plt.plot(*path.vertices.T)
   plt.gca().set_aspect("equal")
   plt.show()

A circle about 700 m across. Every point moved at the same speed, because ``Iso`` ignores fuel, terrain and wind. :doc:`/user_guide/simulation_inputs` starts from this fire and adds them one by one.

.. _quickstart-docker:

From the command line
---------------------

The ``forefire`` program runs scripts of commands (``.ff`` files, :doc:`/user_guide/forefire_script`). The repository comes with a real case in ``tests/runff/``: a fire in the Aullène region of Corsica, on a landscape of fuel and elevation.

With Docker
~~~~~~~~~~~

The image has ForeFire, the example case and a web interface. It works on Linux, macOS and Windows.

.. code-block:: bash

   docker run -it --rm -p 127.0.0.1:8000:8000 ghcr.io/forefireapi/forefire:latest

You land in a shell inside the container. ``127.0.0.1:`` keeps the port on your machine: a bare ``-p 8000:8000`` would let anyone on the network send commands to ForeFire. Run the case:

.. code-block:: bash

   cd tests/runff
   forefire -i real_case.ff

It writes the arrival-time map to ``ForeFire.0.nc`` and the final front to ``to_reload.ff``.

To watch it on a map instead, start ``forefire`` with no argument and open the web interface:

.. code-block:: text

   forefire> listenHTTP[0.0.0.0:8000]

Open ``http://localhost:8000/``, type ``include[real_case.ff]`` in the command box, click **Send**, then **Refresh Map**. The console says ``systemExec is not available over HTTP``: that is expected, shell commands are refused over HTTP and the rest of the case runs.

.. image:: /_static/images/gui_real_case_ff.jpg
   :alt: ForeFire web interface showing the Aullène fire in Corsica
   :align: center
   :width: 90%

Type ``quit[]`` to leave ForeFire, then ``exit`` to leave the container.

Without Docker
~~~~~~~~~~~~~~

``pip install forefire`` also installs the ``forefire`` program. The example case is in the repository, and its landscape file is stored with Git LFS, so install `Git LFS <https://git-lfs.com/>`_ before cloning:

.. code-block:: bash

   pip install forefire
   git clone https://github.com/forefireAPI/forefire.git
   cd forefire/tests/runff
   forefire -i real_case.ff

The web interface needs ``FOREFIREHOME`` to point at the clone, where its files live:

.. code-block:: bash

   export FOREFIREHOME=/path/to/forefire
   forefire -l

``-l`` starts the web interface on ``http://localhost:8000/``. Run ``forefire`` with no argument for an interactive prompt, with history and tab completion.

Where to go next
----------------

:doc:`/user_guide/simulation_inputs` takes the smallest fire above and builds a real simulation from it: fuel, wind, terrain, and where the landscape comes from.
