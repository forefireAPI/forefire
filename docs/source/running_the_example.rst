Running an Example
====================================

After successfully building ForeFire, you can run the example simulation located in the `tests/runff/` directory. This helps verify your installation and demonstrates different ways to launch a simulation.

**The Target Example: `tests/runff/`**:

This directory contains the necessary files for the `real_case.ff` simulation:

- `real_case.ff`: The main script file defining the simulation steps (ignition, wind, duration, output).
- `params.ff`: Contains simulation parameters included by `real_case.ff`.
- `data.nc`: The NetCDF landscape file for the simulation domain.
- `fuels.csv`: The fuels definition file.

Ensure your terminal is in the `firefront` repository's root directory before running the commands below, or adjust paths accordingly.

You can run the `real_case.ff` simulation using the `forefire` executable in several ways:

1: Direct Execution (Command Line)
---------------------------------------------

This is the simplest way to run a simulation non-interactively, useful for scripting.

1.  **Navigate to the test directory:**

  .. code-block:: bash

    cd tests/runff

2.  **Execute using the `-i` flag:**

  This tells ForeFire to read and execute commands from the specified file.

  *If `forefire` is in your PATH:*
  
  .. code-block:: bash

    forefire -i real_case.ff

  *If `forefire` is NOT in your PATH (running from `tests/runff`):*
  
  .. code-block:: bash

    ../../bin/forefire -i real_case.ff

3.  **Observe:** ForeFire will print status messages to the console as it executes the commands in `real_case.ff` and will likely create output files (like `to_reload.ff`) in the current directory (`tests/runff`).

2: Interactive Console
-----------------------

This method allows you to interact with the ForeFire interpreter before, during, or after running the script.

1.  **Navigate to the test directory:**
    
  .. code-block:: bash

    cd tests/runff

2.  **Start the ForeFire interpreter:**

  *If `forefire` is in your PATH:*

  .. code-block:: bash

    forefire

  *If `forefire` is NOT in your PATH (running from `tests/runff`):*

  .. code-block:: bash

    ../../bin/forefire

3.  **Run the script using the `include` command:**

  Once you see the `forefire>` prompt, type:

  .. code-block:: none

    forefire> include[real_case.ff]

4.  **Observe:** The simulation will run similarly to Method 1, printing output to the console. Afterwards, you remain in the interactive console and can inspect parameters (`getParameter[...]`), run further steps (`step[...]`), or `quit[]`.

3: Web Interface
--------------------------------------

This method uses the same web interface shown in the Docker Quickstart, providing a visual map alongside the console. It's great for demonstrations or interactive exploration.

1.  **Navigate to the test directory:**
  
  .. code-block:: bash

    cd tests/runff

2.  **Start the ForeFire interpreter** (as shown in Method 2).

3.  **Launch the HTTP server:**

  At the `forefire>` prompt, type:
  
  .. code-block:: none

    forefire> listenHTTP[]

4.  **Use the Web Interface:**

  - Open your browser to `http://localhost:8000/`.
  - Use the command input in the web UI to type `include[real_case.ff]` and press Enter or Send.
  - Click "Refresh Map" to see the simulation progress visually.

Choosing a Method
-----------------
*   Use **Method 1 (Direct Execution)** for automated runs or when you don't need interaction.
*   Use **Method 2 (Interactive Console)** when you want to experiment with commands or inspect the simulation state directly via text.
*   Use **Method 3 (Web UI)** for visual feedback and interactive demonstrations.

Next Steps
----------
Now that you've seen how to run an existing example, you can learn more about the :doc:`basic_configuration` files to start setting up your own simulations.