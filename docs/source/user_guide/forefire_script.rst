.. _userguide-forefire-script:

ForeFire Script File (.ff)
==========================

The ForeFire Script File, conventionally given a ``.ff`` extension, is a plain text file that contains a sequence of commands to control and execute a ForeFire simulation. It acts as the primary way to define a simulation scenario for non-interactive runs.

Purpose
-------

The script file allows you to automate the entire simulation process, including:

- Setting simulation parameters.
- Loading necessary input data (landscape, fuels).
- Defining the initial state of the fire (ignition points or lines).
- Specifying dynamic conditions (like changing winds).
- Controlling the simulation's time progression.
- Generating output files at desired intervals or at the end.

Basic Syntax
------------

- **One Command Per Line:** Each line in the script file generally represents a single ForeFire command.
- **Command Structure:** Commands follow the format ``CommandName[argument1=value1;argument2=value2;...]``.
  
  - The command name is case-sensitive (e.g., ``setParameter`` is different from ``setparameter``).
  - Arguments are enclosed in square brackets ``[...]``.
  - Arguments are typically key-value pairs separated by an equals sign ``=``.
  - Multiple arguments within the brackets are separated by semicolons ``;``.
  - Some commands might take positional arguments (like :ref:`loadData <cmd-loadData>`) or have optional arguments (prefixed with `opt:` in the :doc:`Command Reference </reference/commands>`).
- **Comments:** Lines beginning with a hash symbol ``#`` are treated as comments and are ignored by the interpreter.
- **Whitespace:** Leading and trailing whitespace on a line is generally ignored, except for indentation (see below).

Indentation for Hierarchy (FireFront / FireNode)
------------------------------------------------

Certain commands, specifically :ref:`FireFront <cmd-FireFront>` and :ref:`FireNode <cmd-FireNode>`, use **leading spaces (indentation)** to define their relationship within the simulation structure when creating custom initial fire perimeters:

- A ``FireFront`` defined within a :ref:`FireDomain <cmd-FireDomain>` is typically indented (e.g., 4 spaces).
- A ``FireNode`` belonging to that ``FireFront`` is indented further relative to the `FireFront` (e.g., another 4 spaces, totaling 8).
- An inner ``FireFront`` (representing an unburned island within another `FireFront`) would be indented relative to its parent `FireFront`.

.. code-block:: none
   :caption: Example Indentation for Custom Front

   # FireDomain defined earlier or implicitly...

       # Indented FireFront (e.g., 4 spaces)
       FireFront[t=0]
           # Further indented FireNode (e.g., 8 spaces total)
           FireNode[loc=(100,100,0);t=0]
           FireNode[loc=(200,100,0);t=0]
           FireNode[loc=(150,150,0);t=0]
           # ... potentially more nodes defining the outer perimeter


.. important::
   Consistent use of spaces (not tabs) is recommended for indentation. The exact number of spaces required might need experimentation or checking examples, but the relative indentation defines the hierarchy. Using :ref:`startFire <cmd-startFire>` is often simpler for basic point ignitions.

Typical Script Workflow
-----------------------

While the exact commands depend on the simulation, a common workflow within a ``.ff`` script looks like this:

1.  **Setup Parameters:** Define simulation controls, model choices, resolution, etc.

  .. code-block:: none

    # Include parameters from a separate file (optional)
    # include[input=params.ff]

    # Set specific parameters directly
    setParameters[propagationModel=Rothermel;perimeterResolution=30;dumpMode=geojson]

2.  **Load Input Data:** Load the geospatial context.

  .. code-block:: none

    # Load landscape and associate with a time
    loadData[my_landscape.nc;2024-01-01T12:00:00Z]
    # Note: The fuels file (e.g., fuels.csv) is usually implicitly loaded
    # based on the 'fuelsTableFile' parameter or defaults,
    # but its path might need to be relative to 'caseDirectory'.

3.  **Define Simulation Domain (if needed):** While ``loadData`` can implicitly define the domain extent based on the NetCDF file, you can also explicitly define it using ``FireDomain``.

  .. code-block:: none

    # Optional explicit domain definition (using projected coords)
    # FireDomain[sw=(0,0,0);ne=(50000,50000,0);t=0]

4.  **Define Initial Fire State:** Specify where and when the fire starts.

  .. code-block:: none

  # Simple point ignition
  startFire[lonlat=(9.1, 42.2); t=0] # Using geographic coordinates

  # --- OR ---

  # Custom initial front (using projected coords and indentation)
  # Ensure FireDomain covers these coordinates
  #    FireFront[t=0]
  #        FireNode[loc=(2500,3000,150);t=0]
  #        FireNode[loc=(2600,3000,150);t=0]
  #        FireNode[loc=(2550,3100,150);t=0]


5.  **Set Dynamic Conditions (Optional):** Introduce time-varying inputs, like wind changes.

  .. code-block:: none

    # Set initial wind (applied from t=0 if not in landscape file)
    trigger[fuelType=wind;vel=(3.0, -1.0, 0.0);t=0]

    # Change wind later in the simulation
    trigger[fuelType=wind;vel=(0.0, 5.0, 0.0);t=3600] # New wind at 1 hour

6.  **Advance Simulation Time:** Run the simulation forward.

  .. code-block:: none

    # Run until a specific absolute simulation time
    goTo[t=7200] # Run until t = 2 hours

    # --- OR ---

    # Run in discrete steps (useful for periodic output)
    # step[dt=600] # Run for 10 minutes
    # print[front_t600.geojson]@t=600 # Output using scheduler
    # step[dt=600] # Run for another 10 minutes
    # print[front_t1200.geojson]@t=1200
    # ...

7.  **Generate Outputs:** Save the simulation results.

  .. code-block:: none

    # Save final front geometry (format set by dumpMode parameter)
    print[final_front.geojson]

    # Save final arrival time map to NetCDF
    save[] # Uses default filename pattern ForeFire.<domainID>.nc

    # Plot a specific variable to an image
    # plot[parameter=arrival_time_of_front;filename=arrival_map.png]


Complete Example (`real_case.ff`)
---------------------------------

The file ``tests/runff/real_case.ff`` serves as a complete, working example demonstrating many of these concepts in practice. After understanding the basics outlined here and consulting the :doc:`Command </reference/commands>` and :doc:`Parameter </reference/parameters>` references, studying ``real_case.ff`` is highly recommended.