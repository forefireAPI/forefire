Running an Example
==================

After successfully installing ForeFire (either :doc:`from source <installation>` or via :doc:`Docker <quickstart>`), you can run the example simulation located in the ``tests/runff/`` directory. This helps verify your installation and introduces you to the basic workflow and command execution methods.

Simulation files
----------------

The directory ``tests/runff/`` contains the necessary files for the ``real_case.ff`` simulation:

*   ``real_case.ff``: The main **script file** defining the simulation steps. This is the file we will execute.
*   ``params.ff``: Contains simulation parameters (like model choices, resolutions) included by ``real_case.ff``.
*   ``data.nc``: The NetCDF **landscape file** containing geospatial data (elevation, fuel types) for the simulation domain.
*   ``fuels.csv``: The **fuels definition file**, specifying properties for different fuel types referenced in ``data.nc``.


Understanding the Example Script (`real_case.ff`)
-------------------------------------------------

Before running the example, let's briefly look at what the ``real_case.ff`` script instructs ForeFire to do. While the full script might be longer, it typically performs actions like these using specific ForeFire commands:

1.  **Include Parameters:** Often starts by including parameter settings from another file (like ``params.ff``) using the :ref:`include <cmd-include>` command.

  .. code-block:: none

    # Example snippet from a .ff file:
    include[input=params.ff]

2.  **Load Landscape Data:** Loads the terrain, fuel, and potentially weather data from the NetCDF file (``data.nc``) using the :ref:`loadData <cmd-loadData>` command, associating it with a start date/time.
  
  .. code-block:: none

    # Example snippet:
    loadData[data.nc;2020-02-10T17:35:54Z] # Date/time may vary

3.  **Define Ignition:** Starts the fire at a specific location and time using the :ref:`startFire <cmd-startFire>` command.
  
  .. code-block:: none
  
    # Example snippet (coordinates may vary):
    startFire[lonlat=(8.916,41.816);t=0]

4.  **Set Dynamic Conditions (Optional):** May use :ref:`trigger <cmd-trigger>` commands to introduce changes during the simulation, like setting wind conditions at specific times.
  
  .. code-block:: none
    
    # Example snippet (values/time may vary):
    trigger[fuelType=wind;vel=(5.0, 2.0, 0.0);t=1800] # Set wind at 1800s

5.  **Run the Simulation:** Advances the simulation time using :ref:`step <cmd-step>` (run for a duration) or :ref:`goTo <cmd-goTo>` (run until a specific time).
  
  .. code-block:: none
    
    # Example snippet (time may vary):
    goTo[t=7200] # Run simulation until t = 7200 seconds (2 hours)

6.  **Generate Output:** Saves the final state (or intermediate states) using commands like :ref:`print <cmd-print>` (saves front geometry) or :ref:`save <cmd-save>` (saves arrival time map).
    
  .. code-block:: none

    # Example snippet (filename may vary):
    setParameter[dumpMode=geojson] # Set output format
    print[output_front.geojson]
    save[] # Save arrival time map

7.  **Terminate:** The script execution finishes, or it might contain an explicit :ref:`quit <cmd-quit>` command.

Knowing these basic steps helps understand what happens when you execute the ``real_case.ff`` script using the methods below. For full details on each command, see the :doc:`Command Reference <reference/commands>`.

Executing the Example Script
----------------------------

You can run the ``real_case.ff`` simulation (which contains commands like those shown above) using the ``forefire`` executable in several ways:

1: Direct Execution (Command Line / Batch Mode)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the simplest way to run a simulation non-interactively by feeding the entire script file to the interpreter. It's useful for automated runs or when you don't need interaction.

1.  **Navigate to the test directory:**
    
  .. code-block:: bash

    cd tests/runff

2.  **Execute using the** ``-i`` **flag:**
    This tells ForeFire to read and execute all commands sequentially from the specified file (``real_case.ff``).

    If ``forefire`` is in your PATH:
    
    .. code-block:: bash

      forefire -i real_case.ff

    If ``forefire`` is NOT in your PATH (running from ``tests/runff``):
    
    .. code-block:: bash

      ../../bin/forefire -i real_case.ff

3.  **Observe:** ForeFire will print status messages to the console as it executes the commands within ``real_case.ff``. It will likely create output files (like ``to_reload.ff`` or others specified by ``print``/``save`` commands in the script) in the current directory (``tests/runff``).

2: Interactive Console
~~~~~~~~~~~~~~~~~~~~~~

This method starts the ForeFire interpreter first, allowing you to type all the commands manually. You can then use the ``include`` command to execute the script file, and potentially interact further before or after.

1.  **Navigate to the test directory:**
    
  .. code-block:: bash

    cd tests/runff

2.  **Start the ForeFire interpreter:**

    If ``forefire`` is in your PATH:
    
    .. code-block:: bash

      forefire

    If ``forefire`` is NOT in your PATH (running from ``tests/runff``):
    
    .. code-block:: bash

      ../../bin/forefire

3.  **Run the script using the** ``include`` **command:**
    
    Once you see the ``forefire>`` prompt, type the command to execute the script file:
    
    .. code-block:: none

       forefire> include[real_case.ff]

4.  **Observe:** The simulation will run similarly to Method 1, executing the commands from ``real_case.ff`` and printing output to the console. Afterwards, you remain in the interactive console (``forefire>`` prompt) and can inspect parameters (e.g., ``getParameter[propagationModel]``), run further steps manually (e.g., ``step[dt=600]``), or exit using ``quit[]``.

3: Web Interface
~~~~~~~~~~~~~~~~

This method uses the built-in HTTP server to provide a web-based console and map visualization. It executes commands in the same way as the interactive console but through your browser. It's great for demonstrations or visual exploration.

1.  **Navigate to the test directory:**
    
    .. code-block:: bash

      cd tests/runff

2.  **Start the ForeFire interpreter** (as shown in Method 2).

3.  **Launch the HTTP server:**
    At the ``forefire>`` prompt, type:
    
    .. code-block:: none

      forefire> listenHTTP[]

4.  **Use the Web Interface:**
    
    *   Open your browser to ``http://localhost:8000/`` (or the specified port).
    *   In the command input box in the web UI, type ``include[real_case.ff]`` and press Enter or click Send. This executes the script file.
    *   Click "Refresh Map" periodically to see the simulation progress visually. You can also type other commands directly into the web console.

Choosing a Method
-----------------

*   Use **Method 1 (Direct Execution)** for standard, non-interactive runs or scripting.
*   Use **Method 2 (Interactive Console)** when you want to experiment with commands step-by-step or inspect the state directly via text after running a script.
*   Use **Method 3 (Web UI)** for visual feedback and interactive demonstrations.

Next Steps
----------

Now that you've seen how to run an existing example script and understand the basic commands it contains, you can learn more about:

*   The :doc:`basic_configuration` files required for setting up your own simulations.
*   The detailed :doc:`reference/commands` and :doc:`reference/parameters`.
*   Exploring the core concepts in the **User Guide** (coming soon!).