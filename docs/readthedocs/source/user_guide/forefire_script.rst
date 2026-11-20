.. _userguide-forefire-script:

Script files (.ff)
==================

A ``.ff`` script is a plain-text list of commands. It sets parameters, loads data, starts fires, schedules events and writes results. The same commands work at the ``forefire>`` prompt, over HTTP, and from Python with ``ff.execute("...")``. Every command is listed in :doc:`/reference/commands`.

A typical script
----------------

.. code-block:: none

   # 1. Model and solver parameters (ForeFireDataDirectory=. looks in caseDirectory)
   setParameters[ForeFireDataDirectory=.;propagationModel=Rothermel;fuelsTableFile=fuels.csv;perimeterResolution=30;spatialIncrement=2;dumpMode=geojson]

   # 2. Load the NetCDF landscape (creates the FireDomain from the 'domain' variable)
   loadData[data.nc;2024-07-15T12:00:00Z]

   # 3. Ignite (WGS84 lon,lat,z, or Cartesian loc=(x,y,z))
   startFire[lonlat=(8.85,41.78,0);t=0]

   # 4. Initial wind, and a wind shift at t = 1 h
   trigger[wind;vel=(5.0,2.0,0.0)]
   trigger[wind;vel=(2.0,6.0,0.0)]@t=3600

   # 5. Run to t = 2 h
   goTo[t=7200]

   # 6. Write the final front and the arrival-time matrix
   print[final_front.geojson]
   save[]

At the end you have ``final_front.geojson`` with the perimeter at two hours, and the arrival-time matrix written by ``save[]``. ``tests/runff/real_case.ff`` and ``tests/runff/params.ff`` in the repository are a complete real case.

Syntax
------

A command is ``CommandName[arg1=val1;arg2=val2]``. Some take positional arguments, such as ``loadData[data.nc;2024-01-01T12:00:00Z]`` or ``include[params.ff]``. Command and parameter names are case-sensitive.

A line that starts with ``#`` or ``*`` is a comment, except inside a ``"""`` block (see below). Comments only work on their own line.

.. warning::
   Put nothing after the closing ``]``, not even a comment or trailing spaces. Everything after ``]`` is read as a schedule operator (:ref:`@t=... <cmd-@>`). A ``# comment`` or two or more spaces there give ``unknown post operator``, and the command does not run.

Scheduling
~~~~~~~~~~

``@t=<seconds>`` (absolute simulation time) or ``@nowplus=<seconds>`` (relative to now), right after ``]``, puts the command in the ``TimeTable`` instead of running it now:

.. code-block:: none

   trigger[wind;vel=(6.0,2.0,0.0)]@t=1800
   print[front_t1800.geojson]@t=1800
   save[]@nowplus=600

Multiline values
~~~~~~~~~~~~~~~~

Triple quotes ``"""`` let a value span several lines, for example an inline fuel table:

.. code-block:: none

   setParameter[fuelsTable="""Index;vv_coeff
   1;1.0
   2;0.3"""]

Substitution and protected values
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In ``print[...]`` and ``systemExec[...]``, ``*paramName*`` is replaced with the current value of ``paramName``. Any ``:`` in the result becomes ``-``, so ISO dates are safe in file names:

.. code-block:: none

   print[front_*ISOdate*_step_*count*.geojson]

``setParameter[key=!value]`` protects ``key``: later ``setParameter`` calls for it are ignored for the rest of the session.

Drawing a front by hand
-----------------------

For a point ignition, :ref:`startFire <cmd-startFire>` creates a small clockwise triangle of 3 nodes. For any other shape, write the front with :ref:`FireFront <cmd-FireFront>` and :ref:`FireNode <cmd-FireNode>`.

Indentation sets the hierarchy, one tab or four spaces per level, relative to the first command of the script:

- Level 0: ``FireDomain[...]`` and other top-level commands.
- Level 1: the outer ``FireFront[...]``.
- Level 2: the ``FireNode[...]`` of the outer front, or an inner ``FireFront[...]`` (an unburned island).
- Level 3: the ``FireNode[...]`` of the inner front.

Each ``FireNode`` needs at least ``loc=(x,y,z)``, ``vel=(vx,vy,vz)`` and ``t=<seconds>``. With fewer than 3 arguments it raises ``MissingOption``.

Node order sets the direction (see :ref:`front winding <front-winding>`): clockwise expands, counter-clockwise contracts. Here the outer front is clockwise and the island inside it is counter-clockwise:

.. code-block:: none
   :caption: Expanding outer perimeter with a contracting inner island

   FireDomain[sw=(-1000,-1000,0);ne=(1000,1000,0);t=0]
       FireFront[t=0]
           FireNode[loc=(0,100,0);vel=(0,0,0);t=0]
           FireNode[loc=(100,0,0);vel=(0,0,0);t=0]
           FireNode[loc=(0,-100,0);vel=(0,0,0);t=0]
           FireNode[loc=(-100,0,0);vel=(0,0,0);t=0]
           FireFront[t=0]
               FireNode[loc=(0,50,0);vel=(0,0,0);t=0]
               FireNode[loc=(-50,0,0);vel=(0,0,0);t=0]
               FireNode[loc=(0,-50,0);vel=(0,0,0);t=0]
               FireNode[loc=(50,0,0);vel=(0,0,0);t=0]
