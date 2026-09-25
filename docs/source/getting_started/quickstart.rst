Quick Start
===========

Your first simulation, two ways: from a pip install in about a minute, or in
Docker with the interactive web console.

.. _quickstart-pip:

The one-minute version, with pip
--------------------------------

On Linux or macOS on Apple Silicon, nothing needs to be compiled and no
dependencies need to be installed on your system:

.. code-block:: bash

  pip install forefire

Then run a simulation from Python:

.. code-block:: python

  import pyforefire as forefire

  ff = forefire.ForeFire()
  ff.execute("FireDomain[sw=(0,0,0);ne=(10000,10000,0);t=0]")
  ff.addLayer("propagation", "Iso", "propagationModel")
  ff.execute("startFire[loc=(5000,5000,0.0)]")
  ff.execute("step[dt=1000]")
  print(ff.execute("print[]"))

``print[]`` returns the state of the simulation as text — one ``FireNode``
entry per node of the front, each with its location, velocity and time. The
same commands work in the interactive interpreter, which the same install
provides:

.. code-block:: bash

  forefire

See :doc:`installation` for which platforms have wheels, and
:doc:`/user_guide/forefire_script` for what these commands mean.

.. _quickstart-docker:

The full example, with Docker
-----------------------------

This runs the standard **ForeFire example simulation** in its interactive web
console. It bundles all dependencies, so nothing is installed on your host
system, and it is the only way to run ForeFire on Windows.

Prerequisites
~~~~~~~~~~~~~
- Docker installed and running on your system.
- Git installed (for cloning the repository).

Steps
~~~~~

1.  **Clone the ForeFire repository:** Open your terminal and run:

  .. code-block:: bash

    git clone https://github.com/forefireAPI/forefire.git

2.  **Build the Docker image:**

  Navigate into the cloned repository directory. The `Dockerfile` defines the environment. Build the image with:
  
  .. code-block:: bash

    cd forefire
    docker build . -t forefire:latest

  This might take a few minutes the first time as it downloads base images and installs dependencies inside the Docker build environment.

3.  **Run the container interactively:**

  This command starts the container, maps port 8000 for the web interface, and gives you a bash shell inside the container:
  
  .. code-block:: bash

    docker run -it --rm -p 8000:8000 --name ff_interactive forefire

4.  **Inside the container, navigate to the test directory:**

  Your terminal prompt should now show you are inside the container (e.g., `root@<container_id>:/app#`). Navigate to the test directory:

  .. code-block:: bash

    cd tests/runff

5.  **Start the ForeFire interpreter:**

  .. code-block:: bash

    forefire

6.  **Launch the HTTP server from the ForeFire console:**

  Type the following command at the `forefire>` prompt:

  .. code-block:: none

    forefire> listenHTTP[]

  You should see the output : `>> ForeFire HTTP command server listening at http://localhost:8000`

7.  **Access the Web Console:**

  Open your local web browser (on your host machine, not inside the container) and navigate to `http://localhost:8000/`.

8.  **Run a Simulation:**

  In ForeFire, running a simulation and viewing the result are separate commands. The UI guides you through this.

    * **Step 1: Run the simulation script.** In the command input box, type `include[real_case.ff]` and click the **`Send`** button. The simulation will run on the server.
    * **Step 2: View the result.** After the command finishes, click the **`Refresh Map`** button to load the simulation results onto the map.
  
  You should see a simulation running in the Aullène region of Corsica.
  
  .. image:: /_static/images/gui_real_case_ff.jpg
    :alt: Screenshot of the ForeFire Web UI showing the Aullène example simulation
    :align: center
    :width: 90%
  
  **This confirms your Docker setup is working!**

9.  **Stop the Container:**

  When finished exploring:

  - In the ForeFire console (either web or terminal inside the container), type `quit`.
  - In the container's bash shell (terminal), type `exit`.
  - The `docker run` command used `--rm`, so the container will be automatically removed upon exit.

.. rubric:: Next Steps

Congratulations! You've successfully run your first ForeFire simulation and have a working environment. Here are some recommended next steps to deepen your understanding:

- **Explore Execution Modes:** Learn about the command-line (batch) and interactive console alternatives to the Web UI by reading the :doc:`execution_modes` guide.
- **Understand the Script:** To see what was inside the ``real_case.ff`` script you just ran, dive into the :doc:`/user_guide/forefire_script` guide.
