.. _running-the-example:

Running an Example
==================

After successfully installing ForeFire (either :doc:`from source <installation>` or via :doc:`Docker <quickstart>`), you can run the example simulation located in the ``tests/runff/`` directory. This helps verify your installation and introduces you to the basic command execution methods.

Simulation Files Used
---------------------

The directory ``tests/runff/`` contains the necessary files for the ``real_case.ff`` simulation:

- ``real_case.ff``: The main **script file** defining the simulation steps and commands. This is the file we will execute.
- ``params.ff``: Contains simulation parameters (like model choices, resolutions) included by ``real_case.ff``.
- ``data.nc``: The NetCDF **landscape file** containing geospatial data (elevation, fuel types) for the simulation domain.
- ``fuels.csv``: The **fuels definition file**, specifying properties for different fuel types referenced in ``data.nc``.

The ``real_case.ff`` file itself contains a sequence of ForeFire commands that set up parameters, load data, define an ignition, run the simulation, and save results. To learn about the syntax, structure, and typical commands used within a ``.ff`` script file, please refer to the :doc:`ForeFire Script File guide </user_guide/forefire_script>`.

Executing the Example Script
----------------------------

The following sections demonstrate three different ways to execute the commands contained within the ``real_case.ff`` script using the ``forefire`` interpreter.

1: Direct Execution (Command Line / Batch Mode)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the simplest way to run a simulation non-interactively by feeding the entire script file to the interpreter. It's useful for automated runs.

1.  **Navigate to the test directory:**
  
  Ensure your terminal's current directory is the root of the cloned `forefire` repository. Then navigate:

  .. code-block:: bash

    cd tests/runff

2.  **Execute using the** ``-i`` **flag:**
  
  This tells ForeFire to read and execute all commands sequentially from the specified file (``real_case.ff``).

  If ``forefire`` is in your PATH:

  .. code-block:: bash

    forefire -i real_case.ff

  If ``forefire`` is NOT in your PATH (run from the root of the repo):

  .. code-block:: bash

    ../../bin/forefire -i real_case.ff

3.  **Observe:** ForeFire will print status messages to the console as it executes the commands within ``real_case.ff``. It will likely create output files (as specified by ``print``/``save`` commands in the script) in the current directory (``tests/runff``).

2: Interactive Console
~~~~~~~~~~~~~~~~~~~~~~

This method starts the ForeFire interpreter first, allowing you to execute the script file using the ``include`` command, and potentially interact further before or after.

1.  **Navigate to the test directory** (if not already there):

  .. code-block:: bash

    cd tests/runff

2.  **Start the ForeFire interpreter:**

  If ``forefire`` is in your PATH:

  .. code-block:: bash

    forefire

  If ``forefire`` is NOT in your PATH (run from the root of the repo):

  .. code-block:: bash

    ../../bin/forefire

3.  **Run the script using the** ``include`` **command:**

  Once you see the ``forefire>`` prompt, type the command to execute the script file.

  .. code-block:: none

    forefire> include[input=real_case.ff]


4.  **Observe:** The simulation will run similarly to Method 1, executing the commands from ``real_case.ff`` and printing output to the console. Afterwards, you remain in the interactive console (``forefire>`` prompt) and can inspect parameters (e.g., ``getParameter[propagationModel]``), run further steps manually (e.g., ``step[dt=600]``), or exit using ``quit[]``.

3: Web Interface
~~~~~~~~~~~~~~~~

This method uses the built-in HTTP server to provide a web-based console and map visualization. It executes commands in the same way as the interactive console but through your browser.

1.  **Navigate to the test directory** (if not already there):

  .. code-block:: bash

    cd tests/runff

2.  **Start the ForeFire interpreter** (as shown in Method 2, ensuring you start it from *within* the `tests/runff` directory for simplicity with file paths in the next steps).

3.  **Launch the HTTP server:**
  
  At the ``forefire>`` prompt, type:

  .. code-block:: none

    forefire> listenHTTP[]

4.  **Use the Web Interface:**

  - Open your browser to ``http://localhost:8000/`` (or the specified port).
  - In the command input box in the web UI, type ``include[input=real_case.ff]`` and press Enter or click Send. This executes the script file relative to where the interpreter was started (which we ensured was `tests/runff`).
  - Click "Refresh Map" periodically to see the simulation progress visually. You can also type other commands directly into the web console.

Choosing a Method
-----------------

- Use **Method 1 (Direct Execution)** for standard, non-interactive runs or scripting.
- Use **Method 2 (Interactive Console)** when you want to experiment with commands step-by-step or inspect the state directly via text after running a script.
- Use **Method 3 (Web UI)** for visual feedback and interactive demonstrations.

Next Steps
----------

Now that you've seen the different ways to *execute* a ForeFire script, you can learn more about:

- How to **write and structure** your own scripts in the :doc:`ForeFire Script File guide </user_guide/forefire_script>`.
- The specific :doc:`Input Files </user_guide/basic_configuration>` required (Fuels, Landscape).
- The detailed :doc:`Command </reference/commands>` and :doc:`Parameter </reference/parameters>` references.