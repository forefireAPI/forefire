.. _reference-commands:

Command Reference
=================

This page lists the commands available in the ForeFire interpreter. Commands are typically followed by arguments enclosed in square brackets `[...]`, often using a `key=value` format separated by semicolons `;`. Some commands may accept positional arguments.

.. note::
   Some commands like ``FireFront`` and ``FireNode`` rely on **indentation** (spaces) when used in a script file to define their hierarchical relationship (e.g., a ``FireNode`` belongs to a ``FireFront``, which belongs to a ``FireDomain``). This is indicated in their descriptions.

List of Commands
----------------

.. _cmd-FireDomain:

``FireDomain``
~~~~~~~~~~~~~~

.. code-block:: none

   FireDomain[sw=(x,y,z);ne=(x,y,z);t=seconds|date=YYYY-MM-DDTHH:MM:SSZ;opt:BBoxWSEN=(lonWest,latSouth,lonEast,latNorth)]

Create the main FireDomain. Fronts and nodes will be created within this FireDomain.

**Arguments:**

*   ``sw=(x,y,z)``: Southwest corner coordinates (x,y,z) in meters.
*   ``ne=(x,y,z)``: Northeast corner coordinates (x,y,z) in meters.
*   ``t=seconds|date=YYYY-MM-DDTHH:MM:SSZ``: Time associated with the fire domain, either in seconds since simulation start or as an absolute ISO 8601 GMT date/time.
*   ``opt:BBoxWSEN=(lonWest,latSouth,lonEast,latNorth)``: (Optional) WGS84 coordinates for the north-oriented bounding box (west longitude, south latitude, east longitude, north latitude).

**Example:**

.. code-block:: none

   FireDomain[sw=(0,0,0);ne=(64000,64000,0);BBoxWSEN=(8.36215875,41.711125,9.1366311,42.28667);t=2400]


.. _cmd-FireFront:

``FireFront`` (Indented)
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

       FireFront[id=<front_id>;domain=<domain_id>;t=<time>]  # Example indentation

Creates a FireFront within a FireDomain or another FireFront.

.. important::
    The prefixed spacing (indentation) defines its hierarchical level.
    A FireFront directly within a FireDomain typically has **4 spaces** indentation in a script file. An inner FireFront (representing an unburned island) nested inside another FireFront would have **8 spaces**.

**Arguments:**

*   ``id=<front_id>``: Identifier for the fire front.
*   ``domain=<domain_id>``: Domain ID where the front is created.
*   ``t=<time>``: Time (in seconds) at which the fire front is created.

**Example:**

.. code-block:: none

       FireFront[id=26;domain=0;t=0]


.. _cmd-FireNode:

``FireNode`` (Indented)
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

           FireNode[loc=(x,y,z);vel=(x,y,z);t=seconds;opt:domain=<domain_id>;opt:id=<node_id>;opt:fdepth=<meters>;opt:kappa=<factor>;opt:state=<state_name>;opt:frontId=<front_id>] # Example indentation

Creates a FireNode (a point marker on the fire line) within a FireFront.

.. important::
    The prefixed spacing (indentation) defines its hierarchical level.
    A FireNode inside a FireFront typically has **4 additional spaces** relative to its parent FireFront in a script file (e.g., 8 spaces if the parent FireFront has 4).

**Arguments:**

*   ``loc=(x,y,z)``: Spatial coordinates (x,y,z) in meters where the node is created.
*   ``vel=(x,y,z)``: Initial velocity vector (x,y,z) of the node.
*   ``t=seconds``: Time (in seconds) associated with the fire node.
*   ``opt:domain=<domain_id>``: (Optional) Domain ID where the node is created.
*   ``opt:id=<node_id>``: (Optional) Identifier for the fire node.
*   ``opt:fdepth=<meters>``: (Optional) Initial fire depth in meters.
*   ``opt:kappa=<factor>``: (Optional) Initial curvature factor (tan value).
*   ``opt:state=<state_name>``: (Optional) State of the node (e.g., init, moving, splitting, merging, final).
*   ``opt:frontId=<front_id>``: (Optional) ID of the front this node belongs to.


**Example:**

.. code-block:: none

           FireNode[domain=0;id=1;fdepth=2;kappa=0.1;loc=(3.5,2.6,1.1);vel=(-0.1,-0.03,0.01);t=2.1;state=moving;frontId=26]


.. _cmd-@:

``@`` (Schedule Operator)
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

   command[arguments]@t=seconds
   command[arguments]@nowplus=seconds

Schedule operator to trigger the preceding command at a specific time `t` or after a duration `nowplus`.

**Arguments:**

*   ``t=seconds``: Time in seconds when the scheduled command should execute.
*   ``nowplus=seconds``: Delta duration in seconds from the current simulation time when the scheduled command should execute.

**Example:**

.. code-block:: none

   print[]@t=1200          # Schedule a print command to run at sim time t=1200 seconds
   save[]@nowplus=600     # Schedule a save command to run at current sim time + 600 seconds


.. _cmd-startFire:

``startFire``
~~~~~~~~~~~~~

.. code-block:: none

   startFire[loc=(x,y,z)|lonlat=(lon,lat);t=seconds|date=YYYY-MM-DDTHH:MM:SSZ]

Creates the smallest possible triangular fire front (an ignition point) at the specified location and time.

**Arguments:**

*   ``loc=(x,y,z)``: Starting location using Cartesian coordinates (x,y,z) in meters.
*   ``lonlat=(lon,lat)``: Starting location using WGS84 coordinates (longitude, latitude). *Use either loc or lonlat.*
*   ``t=seconds|date=YYYY-MM-DDTHH:MM:SSZ``: Time when the fire is started, either in seconds since simulation start or as an absolute ISO 8601 GMT date/time.

**Example:**

.. code-block:: none

   startFire[loc=(0.0,0.0,0.0);t=0.]
   startFire[lonlat=(9.0,42.0);date=2024-07-15T10:00:00Z]


.. _cmd-step:

``step``
~~~~~~~~

.. code-block:: none

   step[dt=seconds]

Advances the simulation forward by the specified time duration `dt`.

**Arguments:**

*   ``dt=seconds``: Duration (in seconds) for which the simulation will run.

**Example:**

.. code-block:: none

   step[dt=600]  # Run simulation for 600 seconds (10 minutes)


.. _cmd-goTo:

``goTo``
~~~~~~~~

.. code-block:: none

   goTo[t=seconds]

Advances the simulation until it reaches the specified absolute simulation time `t`.

**Arguments:**

*   ``t=seconds``: The target simulation time (in seconds since simulation start) to advance to.

**Example:**

.. code-block:: none

   goTo[t=3600]  # Run simulation until t = 3600 seconds


.. _cmd-setParameter:

``setParameter``
~~~~~~~~~~~~~~~~

.. code-block:: none

   setParameter[paramName=value]

Sets a single simulation parameter to a given value.

**Arguments:**

*   ``paramName=value``: The name of the parameter to set and its new value.

**Example:**

.. code-block:: none

   setParameter[perimeterResolution=0.5]
   setParameter[propagationModel=Rothermel]


.. _cmd-setParameters:

``setParameters``
~~~~~~~~~~~~~~~~~

.. code-block:: none

   setParameters[param1=val1;param2=val2;...;paramN=valN]

Sets multiple simulation parameters at once, separated by semicolons.

**Arguments:**

*   ``param=value`` pairs separated by `;`.

**Example:**

.. code-block:: none

   setParameters[perimeterResolution=0.5;spatialIncrement=0.1;outputFrequency=300]


.. _cmd-getParameter:

``getParameter``
~~~~~~~~~~~~~~~~

.. code-block:: none

   getParameter[key]

Retrieves and prints the current value of a specified simulation parameter.

**Arguments:**

*   ``key``: The name of the parameter whose value you want to retrieve.

**Example:**

.. code-block:: none

   getParameter[propagationModel]
   getParameter[dumpMode]


.. _cmd-loadData:

``loadData``
~~~~~~~~~~~~

.. code-block:: none

   loadData[landscape_file.nc;YYYY-MM-DDTHH:MM:SSZ]

Loads environmental data from a NetCDF landscape file into the simulation, associating it with a specific UTC date/time. Arguments are positional.

**Arguments:**

*   Positional 1: Path to the NetCDF landscape file (e.g., `data.nc`).
*   Positional 2: The UTC date and time (ISO 8601 format) corresponding to the data (e.g., `2020-02-10T17:35:54Z`).

**Example:**

.. code-block:: none

   loadData[my_landscape.nc;2024-07-15T08:00:00Z]


.. _cmd-addLayer:

``addLayer``
~~~~~~~~~~~~

.. code-block:: none

   addLayer[name=<layerName>;opt:type=<layerType>;opt:modelName=<model>;opt:value=<V>]

Adds a data layer to the DataBroker. Useful for adding constant value layers or potentially layers associated with specific flux/propagation models. Bounds match the current FireDomain.

**Arguments:**

*   ``name=<layerName>``: Name for the new data layer (e.g., `windU`, `heatFlux`).
*   ``opt:type=<layerType>``: (Optional) Type of layer (e.g., `flux`, `propagation`). Defaults to `data`.
*   ``opt:modelName=<model>``: (Optional) Associated model name if applicable (e.g., `heatFluxBasic`).
*   ``opt:value=<V>``: (Optional) Constant value for the layer. If not given, ForeFire may search for a parameter of the same name, otherwise defaults to 0.

**Example:**

.. code-block:: none

   addLayer[name=windU;value=5.0]  # Add constant Eastward wind component
   addLayer[name=heatFlux;type=flux;modelName=heatFluxBasic;value=3]


.. _cmd-trigger:

``trigger``
~~~~~~~~~~~

.. code-block:: none

   trigger[fuelIndice=<value>;loc=(x,y,z);fuelType=<int|wind>;vel=(vx,vy,vz);t=<time>]

Triggers a change in simulation data at a specific time and location. Can be used for dynamic fuel changes or injecting wind conditions.

**Arguments:**

*   ``fuelIndice=<value>``: Fuel index value.
*   ``loc=(x,y,z)``: Location (x,y,z) where the trigger is applied.
*   ``fuelType=<int|wind>``: Can be a fuel type integer or the keyword `wind` for dynamic wind trigger.
*   ``vel=(vx,vy,vz)``: Velocity vector (vx,vy,vz) associated with the trigger (used for wind).
*   ``t=<time>``: Time (in seconds) at which the trigger is activated.

**Example:**

.. code-block:: none

   trigger[fuelType=wind;vel=(5.0,2.0,0.0);t=1800] # Trigger new wind at t=1800s


.. _cmd-print:

``print``
~~~~~~~~~

.. code-block:: none

   print[opt:filename]

Prints a representation of the current simulation state (primarily the fire front location) to the console or to a specified file. The output format is determined by the `dumpMode` parameter (set via `setParameter`).

**Output Formats (dumpMode):**

*   `ff`: Native format, potentially re-parsable by ForeFire.
*   `json`: Compact Cartesian JSON format.
*   `geojson`: GeoJSON format (requires projection).
*   `kml`: KML format (requires projection).

**Arguments:**

*   ``opt:filename``: (Optional) If provided, output is written to this file instead of the console.

**Example:**

.. code-block:: none

   setParameter[dumpMode=geojson]
   print[front_state_t1200.geojson]  # Save front state at current time as GeoJSON
   print[]                           # Print state to console in current dumpMode


.. _cmd-save:

``save``
~~~~~~~~

.. code-block:: none

   save[opt:filename=<fname.nc>;opt:fields=(field1,field2,...)]
   save[]

Saves simulation state or landscape data to a NetCDF file.

**Modes:**

1.  **Save Arrival Time Map (Default):** If no arguments are given (`save[]`), saves the computed fire arrival time map for the current domain to a standard filename like `ForeFire.<domainID>.nc`.
2.  **Save Landscape Data:** If `filename` and `fields` are provided, saves specified data layers (e.g., altitude, fuel, wind) from the DataBroker to the given NetCDF filename.

**Arguments:**

*   ``opt:filename=<fname.nc>``: (Optional) Specify the output NetCDF filename for saving landscape data.
*   ``opt:fields=(field1,field2,...)``: (Optional) Comma-separated list of data layer names to save (e.g., `altitude`, `windU`, `windV`, `fuel`). Used only when `filename` is also provided.

**Example:**

.. code-block:: none

   save[]  # Save arrival time map to ForeFire.<domainID>.nc
   save[filename=landscape_snapshot.nc;fields=(altitude,fuel,windU)] # Save specific layers


.. _cmd-plot:

``plot``
~~~~~~~~

.. code-block:: none

   plot[parameter=(param_name);filename=<fname.png/jpg/nc/asc>;opt:range=(min,max);opt:area=(area_spec);opt:size=(eni,enj);opt:cmap=<map_name>;opt:histbins=<N>;opt:projectionOut=(json|<fname.kml>)]

Generates a plot or data export of a specified simulation parameter. Output format depends on the `filename` extension.

**Arguments:**

*   ``parameter=(param_name)``: Parameter to plot/export. Examples: `speed`, `arrival_time_of_front`, `fuel`, `altitude`, `slope`, `windU`, `windV`, `Rothermel`.
*   ``filename=<fname>``: Output filename. Extension determines format:
    *   `.png`, `.jpg`: Image file.
    *   `.nc`: NetCDF file with data matrix and coordinates.
    *   `.asc`: ASCII grid file for GIS.
*   ``opt:range=(min,max)``: (Optional) Data range for colormap/histogram.
*   ``opt:area=(area_spec)``: (Optional) Area to plot/export. Options:
    *   `BBoxWSEN=(w_lon,s_lat,e_lon,n_lat)`: Geographic WGS84 bounding box.
    *   `BBoxLBRT=(leftX,bottomY,rightX,topY)`: Cartesian coordinate bounding box.
    *   `active`: Only the active (computationally relevant) part of the domain.
*   ``opt:size=(eni,enj)``: (Optional) Output matrix dimensions (pixels/grid cells), where `eni` is width and `enj` is height. Uses default resolution if omitted.
*   ``opt:cmap=<map_name>``: (Optional) Colormap name (e.g., `viridis`, `turbo`, `jet`, `fuel`).
*   ``opt:histbins=<N>``: (Optional) Number of bins for histogram (if applicable to output format).
*   ``opt:projectionOut=(json|<fname.kml>)``: (Optional) Output projection info:
    *   `json`: Output bounding box as JSON string to console.
    *   `<fname.kml>`: Save a KML file containing a GroundOverlay referencing the generated image (only useful for image outputs).

**Example:**

.. code-block:: none

   plot[parameter=arrival_time_of_front;filename=arrival.png;opt:area=active;opt:cmap=turbo]
   plot[parameter=fuel;filename=fuel_map.nc;opt:area=(BBoxWSEN=(8.5,41.8,9.0,42.2))]


.. _cmd-computeSpeed:

``computeSpeed``
~~~~~~~~~~~~~~~~

.. code-block:: none

   computeSpeed[]

Computes and returns an array of speed values using the first registered propagation model.
*Note: The exact format/use of the returned array within the interpreter might need further user clarification.*

**Example:**

.. code-block:: none

   computeSpeed[]


.. _cmd-include:

``include``
~~~~~~~~~~~

.. code-block:: none

   include[filename.ff]

Executes ForeFire commands contained within the specified script file. The filename is provided as a positional argument.

**Arguments:**

*   Positional 1: Path to the ForeFire script file (e.g., `.ff` or `.txt`) containing commands to execute.

**Example:**

.. code-block:: none

   include[real_case.ff]
   include[commands.txt]


.. _cmd-clear:

``clear``
~~~~~~~~~

.. code-block:: none

   clear[]

Clears all simulation data (domain, fronts, nodes, loaded data) and scheduled events, resetting the simulation state. Parameters are generally kept.

**Example:**

.. code-block:: none

   clear[]


.. _cmd-systemExec:

``systemExec``
~~~~~~~~~~~~~~

.. code-block:: none

   systemExec[command=<system_command_string>]

Executes a command in the underlying operating system shell.

**Arguments:**

*   ``command=<system_command_string>``: The command line string to execute.

**Example:**

.. code-block:: none

   systemExec[command=ls -l output_files/]
   systemExec[command=echo "Simulation step done" >> sim.log]


.. _cmd-listenHTTP:

``listenHTTP``
~~~~~~~~~~~~~~

.. code-block:: none

   listenHTTP[host:port]

Launches an HTTP server (on the machine running ForeFire) that listens for simulation commands via HTTP requests. The host and port are provided as positional arguments separated by a colon. Used by the web interface.

**Arguments:**

*   Positional 1: Hostname or IP address to bind the server to, followed by a colon, followed by the port number (e.g., `127.0.0.1:8000`, `0.0.0.0:8080`).

**Example:**

.. code-block:: none

   listenHTTP[0.0.0.0:8000]
   listenHTTP[localhost:8080]


.. _cmd-help:

``help``
~~~~~~~~

.. code-block:: none

   help[]

Displays a list of available commands and their brief descriptions.
*Note: For more detailed help on a specific command within the ForeFire interpreter, "Tab-Tab" completion or similar interactive features might be available if supported by the interpreter environment.*

**Example:**

.. code-block:: none

   help[]


.. _cmd-quit:

``quit``
~~~~~~~~

.. code-block:: none

   quit[]

Terminates the ForeFire interpreter session.

**Example:**

.. code-block:: none

   quit[]