.. _reference-commands:

Command Reference
=================

This page lists all commands recognized by the ForeFire interpreter (``.ff`` scripts, interactive ``forefire>`` shell, HTTP server, and ``ff.execute("...")`` in Python).

.. note::
   - ``FireFront`` and ``FireNode`` use **indentation** (one tab ``\t`` or ``4`` spaces per level relative to the first command) to define their parent–child hierarchy.
   - In the syntax blocks below, ``opt:`` marks an optional argument—do not type ``opt:`` in your command (write ``cmap=turbo``, not ``opt:cmap=turbo``).
   - A ``#`` starts a comment **only at the beginning of a line**. Anything after the closing ``]`` on a command line is parsed as a schedule operator (:ref:`@ <cmd-@>`); trailing comments or two or more trailing spaces after ``]`` cause ``unknown post operator`` and prevent the command from executing.

List of Commands
----------------

.. _cmd-FireDomain:

FireDomain
~~~~~~~~~~

.. code-block:: none

   FireDomain[sw=(x,y,z);ne=(x,y,z);t=seconds]

Creates the simulation domain. Fronts, nodes, and data layers live inside this ``FireDomain``.

**Arguments:**

* ``sw=(x,y,z)``: Southwest corner coordinates in Cartesian meters.
* ``ne=(x,y,z)``: Northeast corner coordinates in Cartesian meters.
* ``t=seconds``: Initial simulation time in seconds (``FireDomain`` does not accept ``date=``).

**Georeferencing:** To place a standalone ``FireDomain`` on the globe without ``loadData``, set the ``BBoxWSEN`` parameter (``westLon,southLat,eastLon,northLat`` in WGS84, without parentheses) **before** calling ``FireDomain``. Otherwise the domain defaults to ``DefaultSWLngLat`` (in Corsica).

**Example:**

.. code-block:: none

   setParameter[BBoxWSEN=8.36215875,41.711125,9.1366311,42.28667]
   FireDomain[sw=(0,0,0);ne=(64000,64000,0);t=2400]


.. _cmd-FireFront:

FireFront (Indented)
~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

       FireFront[t=<seconds>;opt:id=<front_id>;opt:domain=<domain_id>]

Creates a ``FireFront`` contour inside the active ``FireDomain`` (1 indent level: 4 spaces or 1 tab) or an inner island front nested inside an enclosing ``FireFront`` (2 indent levels: 8 spaces or 2 tabs).

**Arguments:**

* ``t=<seconds>``: Simulation time in seconds.
* ``opt:id=<front_id>``: Optional integer identifier.
* ``opt:domain=<domain_id>``: Optional domain ID (defaults to ``0``).

**Example:**

.. code-block:: none

       FireFront[id=26;domain=0;t=0]


.. _cmd-FireNode:

FireNode (Indented)
~~~~~~~~~~~~~~~~~~~

.. code-block:: none

           FireNode[loc=(x,y,z);vel=(vx,vy,vz);t=seconds;opt:domain=<domain_id>;opt:id=<node_id>;opt:fdepth=<meters>;opt:kappa=<factor>;opt:state=<state_name>]

Appends a Lagrangian marker node to the current ``FireFront`` (indented one level deeper than its parent ``FireFront``, e.g., 8 spaces for an outer front or 12 spaces for an inner front).

.. important::
   At least three arguments are required (typically ``loc``, ``vel``, and ``t``); passing fewer than three arguments throws ``MissingOption``. Ordering the nodes of a front **clockwise** makes the front expand outward; ordering them **counter-clockwise** makes the front contract inward (for unburned islands).

**Arguments:**

* ``loc=(x,y,z)``: Spatial coordinates in meters. **Required.**
* ``vel=(vx,vy,vz)``: Initial velocity vector in m/s (use ``(0,0,0)`` at initialization). **Required.**
* ``t=seconds``: Timestamp in seconds (defaults to current domain time if omitted when other optional arguments are present). **Required** for a 3-argument call.
* ``opt:domain=<domain_id>``: Domain ID (default ``0``).
* ``opt:id=<node_id>``: Node identifier (default ``0``).
* ``opt:fdepth=<meters>``: Initial front depth in meters (defaults to ``initialFrontDepth``).
* ``opt:kappa=<factor>``: Initial local curvature (default ``0.``).
* ``opt:state=<state_name>``: Node state (``init``, ``moving``, ``link``, ``final``; defaults to ``init``).

**Example:**

.. code-block:: none

           FireNode[loc=(100,200,0);vel=(0,0,0);t=0]
           FireNode[domain=0;id=1;fdepth=2;kappa=0.1;loc=(3.5,2.6,1.1);vel=(-0.1,-0.03,0.01);t=2.1;state=moving]


.. _cmd-@:

@ (Schedule Operator)
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

   command[arguments]@t=seconds
   command[arguments]@nowplus=seconds

Queues the preceding command in the event ``TimeTable`` to execute when simulation time reaches ``t`` (or ``current_time + nowplus``).

**Example:**

.. code-block:: none

   print[front_t1200.geojson]@t=1200
   save[]@nowplus=600


.. _cmd-startFire:

startFire
~~~~~~~~~

.. code-block:: none

   startFire[loc=(x,y,z)|lonlat=(lon,lat,z);t=seconds|date=YYYY-MM-DDTHH:MM:SSZ]

Ignites a minimal clockwise triangular fire front (3 ``FireNode`` markers) at the given location and time.

**Arguments:**

* ``loc=(x,y,z)``: Ignition point in Cartesian domain coordinates (meters).
* ``lonlat=(lon,lat,z)``: Ignition point in WGS84 coordinates (longitude, latitude, elevation). All three components are required (``lonlat=(lon,lat)`` is ignored).
* ``t=seconds`` or ``date=YYYY-MM-DDTHH:MM:SSZ``: Ignition time in seconds or ISO 8601 UTC format.

**Example:**

.. code-block:: none

   startFire[loc=(5000,5000,0);t=0]
   startFire[lonlat=(9.0,42.0,0);date=2024-07-15T10:00:00Z]


.. _cmd-step:

step
~~~~

.. code-block:: none

   step[dt=seconds]

Advances the simulation clock by the relative duration ``dt`` (in seconds).

**Example:**

.. code-block:: none

   step[dt=600]


.. _cmd-goTo:

goTo
~~~~

.. code-block:: none

   goTo[t=seconds]

Advances the simulation clock until absolute simulation time ``t`` (in seconds).

**Example:**

.. code-block:: none

   goTo[t=3600]


.. _cmd-setParameter:

setParameter
~~~~~~~~~~~~

.. code-block:: none

   setParameter[paramName=value]
   setParameter[paramName=!value]

Sets a single simulation parameter. Prefixing the value with ``!`` protects the parameter from being overwritten by later ``setParameter`` calls during the session.

**Example:**

.. code-block:: none

   setParameter[propagationModel=Rothermel]
   setParameter[perimeterResolution=!20]


.. _cmd-setParameters:

setParameters
~~~~~~~~~~~~~

.. code-block:: none

   setParameters[param1=val1;param2=val2;...;paramN=valN]

Sets multiple semicolon-separated simulation parameters in one call.

**Example:**

.. code-block:: none

   setParameters[perimeterResolution=30;spatialIncrement=1.5;dumpMode=geojson]


.. _cmd-getParameter:

getParameter
~~~~~~~~~~~~

.. code-block:: none

   getParameter[key]

Returns (and prints) the current string value of parameter ``key``.

**Example:**

.. code-block:: none

   getParameter[propagationModel]


.. _cmd-loadData:

loadData
~~~~~~~~

.. code-block:: none

   loadData[landscape_file.nc;YYYY-MM-DDTHH:MM:SSZ]
   loadData[domain_file.ff]

Loads environmental data or a domain file. Arguments are positional and separated by ``;``:

* **Two arguments** (``loadData[landscape.nc;YYYY-MM-DDTHH:MM:SSZ]``): Reads a NetCDF landscape file (requires a build with ``FOREFIRE_ENABLE_NETCDF=ON``), sets the ``NetCDFfile`` parameter to the given path, builds the ``FireDomain`` from the ``domain`` variable attributes, and initializes the simulation reference date from the ISO 8601 UTC timestamp.
* **One argument** (``loadData[file.ff]``): Reads a saved domain state file without NetCDF.

**Example:**

.. code-block:: none

   loadData[data.nc;2024-07-15T08:00:00Z]


.. _cmd-addLayer:

addLayer
~~~~~~~~

.. code-block:: none

   addLayer[name=<layerName>;opt:type=<layerType>;opt:modelName=<model>;opt:value=<V>]

Adds a uniform ``DataLayer`` (or model layer) spanning the active ``FireDomain`` to the ``DataBroker``.

**Arguments:**

* ``name=<layerName>``: Layer name (e.g., ``propagation``, ``heatFlux``, ``windU``).
* ``opt:type=<layerType>``: Layer category (``data`` by default, ``propagation``, ``flux``).
* ``opt:modelName=<model>``: Registered C++ model name when ``type`` is ``propagation`` or ``flux`` (e.g., ``Rothermel``, ``BurnUpHeatFlux``).
* ``opt:value=<V>``: Constant numerical value for ``data`` layers (defaults to the parameter of the same name if set, else ``0``).

**Example:**

.. code-block:: none

   addLayer[name=windU;value=5.0]
   addLayer[name=heatFlux;type=flux;modelName=BurnUpHeatFlux]


.. _cmd-trigger:

trigger
~~~~~~~

.. code-block:: none

   trigger[wind;vel=(vx,vy,vz)]
   trigger[fuelIndice;loc=(x,y,z);fuelType=<int>]

Updates dynamic environmental conditions at the moment the command executes. The first token (without ``=``) selects the target layer. To apply the change at a future time, schedule the command with :ref:`@t=... <cmd-@>` (a ``t=`` inside the brackets is ignored by ``wind`` and ``fuelIndice``).

**Example:**

.. code-block:: none

   trigger[wind;vel=(5.0,2.0,0.0)]@t=1800
   trigger[fuelIndice;loc=(1000,1500,0);fuelType=0]


.. _cmd-emit:

emit
~~~~

.. code-block:: none

   emit[layer=<flux_layer>;loc=(x,y,z);radius=<metres>;duration=<seconds>;flux=<per_m2_per_s>]

Injects an external emission source into an existing flux layer (created beforehand with :ref:`addLayer <cmd-addLayer>`) over a specified area and ``duration`` starting at the domain's current time.

**Arguments:**

* ``layer=<flux_layer>`` (or ``name=``): Target flux layer name. **Required.**
* ``duration=<seconds>``: Emission duration in seconds. **Required.**
* **Location** (give one): ``loc=(x,y,z)``, ``lonlat=(lon,lat,z)``, ``sw=(x,y,z);ne=(x,y,z)``, or ``swlonlat=(lon,lat,z);nelonlat=(lon,lat,z)``.
* **Area** (if not using a ``sw``/``ne`` box): ``radius=<metres>`` (disc of area :math:`\pi r^2`) or ``surface=<m2>`` (alias ``area=<m2>``).
* **Magnitude** (give one): ``flux=<per_m2_per_s>``, ``frp=<MW>`` (converted to watts via ``FRPToWatts``, default ``1e7``, and divided by area), ``value=<power>`` (alias ``val=``), or ``total=<energy>``.

**Example:**

.. code-block:: none

   addLayer[name=heatFlux;type=flux;modelName=heatFluxBasic]
   emit[layer=heatFlux;lonlat=(9.05,41.95,0);radius=50;duration=600;frp=120]


.. _cmd-print:

print
~~~~~

.. code-block:: none

   print[opt:filename]

Formats the current fire front state according to ``dumpMode`` (``ff``, ``json``, ``geojson``, or ``kml``).

* Without arguments (``print[]``), writes to the standard output stream.
* With ``filename``, writes to ``filename`` **relative to the current working directory** (not ``fireOutputDirectory``), increments the ``count`` parameter, expands any ``*paramName*`` tokens in ``filename``, and replaces ``:`` with ``-``.

**Example:**

.. code-block:: none

   setParameter[dumpMode=geojson]
   print[front_state_t1200.geojson]
   print[front_*ISOdate*_*count*.geojson]


.. _cmd-save:

save
~~~~

.. code-block:: none

   save[]
   save[filename=<fname.nc>;opt:fields=altitude,fuel,wind;opt:compression=<0-10>]

Saves simulation outputs to NetCDF (requires ``FOREFIRE_ENABLE_NETCDF=ON``). ``save`` has two modes:

1. **Arrival-time Burning Map** (``save[]`` with no arguments):
   Writes the 2D ``arrival_time_of_front`` matrix (with ``-9999`` for unburned cells, deflate level 6) to ``<fireOutputDirectory>/<experiment>.<domainID>.nc`` (by default ``./ForeFire.0.nc``).
2. **Landscape Snapshot** (``save[filename=...]``):
   Writes the ``domain`` variable and selected environmental layers from the ``DataBroker`` to ``filename``.

   * ``filename=<fname.nc>``: Output NetCDF path. **Required** for landscape mode.
   * ``opt:fields=altitude,fuel,wind``: Comma-separated list of layers to write, **without parentheses**. Only ``altitude``, ``fuel``, and ``wind`` (which combines ``windU`` and ``windV``) are recognized. Defaults to ``altitude,fuel`` when omitted.
   * ``opt:compression=<0-10>``: NetCDF-4 deflate compression level (default ``0``, no compression).

**Example:**

.. code-block:: none

   # Save arrival-time matrix to ForeFire.0.nc
   save[]
   # Save landscape layers with deflate compression
   save[filename=landscape_snapshot.nc;fields=altitude,fuel,wind;compression=6]


.. _cmd-plot:

plot
~~~~

.. code-block:: none

   plot[parameter=<param_name>;filename=<fname.png/jpg/nc/asc>;opt:range=(min,max);opt:BBoxWSEN=(w_lon,s_lat,e_lon,n_lat)|opt:BBoxLBRT=(leftX,bottomY,rightX,topY)|opt:BBoxLBRT=active;opt:size=eni,enj;opt:cmap=<map_name>;opt:histbins=<N>;opt:projectionOut=(json|<fname.kml>)]

Exports or renders a 2D matrix of ``parameter`` over the domain.

**Arguments:**

* ``parameter=<param_name>``: Layer or property name (e.g., ``speed``, ``arrival_time``, ``fuel``, ``altitude``, ``slope``, ``windU``, ``windV``, or a model name such as ``Rothermel``).
* ``filename=<fname>``: Output file path; the extension selects the format (``.png``, ``.jpg``, ``.nc``, or ``.asc``).
* ``opt:range=(min,max)``: Value range for colormapping.
* ``opt:BBoxWSEN=(w_lon,s_lat,e_lon,n_lat)`` / ``opt:BBoxLBRT=(leftX,bottomY,rightX,topY)`` / ``opt:BBoxLBRT=active``: Sub-region to export (defaults to the full domain).
* ``opt:size=eni,enj``: Output grid width and height in pixels/cells, written without parentheses.
* ``opt:cmap=<map_name>``: Colormap name (e.g., ``turbo``, ``viridis``, ``jet``, ``fuel``).
* ``opt:histbins=<N>``: Number of histogram bins.
* ``opt:projectionOut=(json|<fname.kml>)``: Prints bounding box JSON to the console or writes a KML ``GroundOverlay`` file alongside image outputs.

**Example:**

.. code-block:: none

   plot[parameter=arrival_time;filename=arrival.png;BBoxLBRT=active;cmap=turbo]


.. _cmd-computeSpeed:

computeSpeed
~~~~~~~~~~~~

.. code-block:: none

   computeSpeed[val1;val2;...;valN]

Evaluates ``getSpeed(values)`` directly on the active propagation model (selected by the ``propagationModel`` parameter) using the semicolon-separated numerical property values ``val1;val2;...`` supplied in the exact order registered by that model, and prints the resulting spread rate (in m/s). Calling ``computeSpeed[]`` with no arguments returns an error.

**Example:**

.. code-block:: none

   # For WindDriven (registers normalWind, curvature, fuel.vv_coeff, fuel.Kcurv, fuel.beta):
   setParameter[propagationModel=WindDriven]
   computeSpeed[5.0;0.0;0.1;1.0;1.0]


.. _cmd-include:

include
~~~~~~~

.. code-block:: none

   include[filename.ff]

Reads and executes commands sequentially from ``filename.ff`` (resolved relative to the current working directory).

**Example:**

.. code-block:: none

   include[real_case.ff]


.. _cmd-clear:

clear
~~~~~

.. code-block:: none

   clear[]

Frees the active ``FireDomain`` (including all fronts, nodes, and data layers) and cancels all scheduled events in the ``TimeTable``. Simulation parameters are preserved.


.. _cmd-systemExec:

systemExec
~~~~~~~~~~

.. code-block:: none

   systemExec[<shell_command_string>]

Executes a shell command via ``popen`` and prints its output.

.. important::
   - Pass the raw shell command directly inside the brackets—do **not** write ``command=...`` (writing ``systemExec[command=ls -l]`` passes the literal string ``command=ls -l`` to the shell).
   - Any ``*paramName*`` placeholder in the string is replaced with the value of parameter ``paramName``, and every ``:`` is replaced with ``-``.
   - **Refused over HTTP:** for security, ``systemExec`` always fails with ``systemExec is not available over HTTP`` when triggered from a remote HTTP session, including indirectly via ``include[]``.

**Example:**

.. code-block:: none

   systemExec[ls -l output_files/]
   systemExec[mkdir -p run_*experiment*]


.. _cmd-listenHTTP:

listenHTTP
~~~~~~~~~~

.. code-block:: none

   listenHTTP[opt:host:port]

Starts the embedded HTTP command and visualization server (intended for trusted local use).

* With no argument (``listenHTTP[]``), binds to ``localhost:8000`` and prints ``ForeFire HTTP command server listening at http://localhost:8000``.
* Inside a Docker container, pass ``listenHTTP[0.0.0.0:8000]`` so ports published on the host can reach the server.
* Static files are served only from the current working directory and ``$FOREFIREHOME/tools/htdocs`` (paths containing ``..`` or leading ``/`` are rejected, and ``systemExec`` commands are refused over HTTP).

**Example:**

.. code-block:: none

   listenHTTP[]
   listenHTTP[0.0.0.0:8000]


.. _cmd-quit:

quit
~~~~

.. code-block:: none

   quit[]

Stops execution of the current script or terminates the interactive interpreter session.
