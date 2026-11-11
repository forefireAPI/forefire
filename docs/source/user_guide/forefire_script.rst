.. _userguide-forefire-script:

ForeFire Script File (.ff)
==========================

A ForeFire script (conventionally ``.ff``) is a plain-text sequence of interpreter commands that configures parameters, loads data, initializes fire fronts, schedules events, and exports results. Every command available in a ``.ff`` file can also be executed interactively at the ``forefire>`` prompt, sent over HTTP, or passed to ``ff.execute("...")`` in Python.

Grammar & Syntax Rules
----------------------

1. **Command Format:**
   Each command takes the form ``CommandName[arg1=val1;arg2=val2]`` (or positional arguments such as ``loadData[data.nc;2024-01-01T12:00:00Z]`` and ``include[params.ff]``). Command and parameter names are **case-sensitive**.

2. **Comments (Whole Line Only):**
   Lines starting with ``#`` or ``*`` (outside multiline blocks) are ignored as comments.

   .. warning::
      **Never place a comment or trailing spaces after the closing bracket** ``]`` **on the same line.** Everything after ``]`` is parsed as a schedule post-operator (:ref:`@t=... <cmd-@>`). A trailing ``# comment`` or two or more trailing spaces after ``]`` triggers ``unknown post operator`` and **prevents the command from running**.

3. **Schedule Operator** (``@t=`` and ``@nowplus=``):
   Append ``@t=<seconds>`` (absolute simulation time) or ``@nowplus=<seconds>`` (relative to current domain time) immediately after ``]`` to queue a command in the event ``TimeTable`` instead of executing it immediately:

   .. code-block:: none

      trigger[wind;vel=(6.0,2.0,0.0)]@t=1800
      print[front_t1800.geojson]@t=1800
      save[]@nowplus=600

4. **Multiline Arguments** (``"""..."""``):
   Wrap an argument value in triple quotes ``"""`` to span multiple lines—for example, to embed an inline fuel table inside a script:

   .. code-block:: none

      setParameter[fuelsTable="""Index;vv_coeff
      1;1.0
      2;0.3"""]

5. **Parameter Substitution** (``*param*``) **and Protected Values** (``!``):

   * In ``print[...]`` and ``systemExec[...]``, any ``*paramName*`` token in the argument string is replaced with the current value of ``paramName`` (and any ``:`` in the resulting string is replaced with ``-`` so ISO timestamps are safe in filenames):

     .. code-block:: none

        print[front_*ISOdate*_step_*count*.geojson]

   * Prefixing a value with ``!`` in ``setParameter[key=!value]`` marks ``key`` as **protected**: subsequent ``setParameter`` calls for that key are ignored for the rest of the session.

Hierarchy & Front Orientation (FireFront / FireNode)
----------------------------------------------------

When constructing custom fire perimeters with :ref:`FireFront <cmd-FireFront>` and :ref:`FireNode <cmd-FireNode>`, ForeFire uses **indentation** and **node ordering (winding)**:

* **Indentation rule:** Each hierarchy level is indented by **one tab** (``\t``) **or four spaces** relative to the first command in the script:

  * Level 0 (no indent): ``FireDomain[...]`` and top-level commands.
  * Level 1 (4 spaces or 1 tab): outer ``FireFront[...]`` inside the ``FireDomain``.
  * Level 2 (8 spaces or 2 tabs): ``FireNode[...]`` belonging to the outer ``FireFront``, **or** an inner ``FireFront[...]`` (unburned island) nested inside the outer front.
  * Level 3 (12 spaces or 3 tabs): ``FireNode[...]`` belonging to the inner ``FireFront``.

* **Required** ``FireNode`` **arguments:** Every ``FireNode`` requires at least three arguments: ``loc=(x,y,z)``, ``vel=(vx,vy,vz)``, and ``t=<seconds>`` (fewer than 3 arguments raises ``MissingOption``).

* **Winding direction (Expanding vs. Contracting fronts):**
  The normal vector direction is determined by the winding order of the ``FireNode`` markers:

  * **Clockwise order** :math:`\rightarrow` outward normal (**expanding** fire perimeter).
  * **Counter-clockwise order** :math:`\rightarrow` inward normal (**contracting** perimeter, used for unburned islands inside an outer front).

.. code-block:: none
   :caption: Expanding outer perimeter with a contracting inner island

   FireDomain[sw=(-1000,-1000,0);ne=(1000,1000,0);t=0]
       FireFront[t=0]
           FireNode[loc=(0,100,0);vel=(0,0,0);t=0]
           FireNode[loc=(100,0,0);vel=(0,0,0);t=0]
           FireNode[loc=(0,-100,0);vel=(0,0,0);t=0]
           FireNode[loc=(-100,0,0);vel=(0,0,0);t=0]
           FireFront[t=0]
               FireNode[loc=(0,20,0);vel=(0,0,0);t=0]
               FireNode[loc=(-20,0,0);vel=(0,0,0);t=0]
               FireNode[loc=(0,-20,0);vel=(0,0,0);t=0]
               FireNode[loc=(20,0,0);vel=(0,0,0);t=0]

For simple point ignitions, :ref:`startFire <cmd-startFire>` automatically creates a small clockwise triangular front of 3 nodes.

Typical Script Workflow
-----------------------

.. code-block:: none

   # 1. Configure model and solver parameters (ForeFireDataDirectory=. looks in caseDirectory)
   setParameters[ForeFireDataDirectory=.;propagationModel=Rothermel;fuelsTableFile=fuels.csv;perimeterResolution=30;spatialIncrement=2;dumpMode=geojson]

   # 2. Load the NetCDF landscape (creates the FireDomain from the 'domain' variable)
   loadData[data.nc;2024-07-15T12:00:00Z]

   # 3. Ignite the fire (in WGS84 lon,lat,z or Cartesian loc=(x,y,z))
   startFire[lonlat=(8.85,41.78,0);t=0]

   # 4. Set initial wind and schedule a wind shift at t = 1 hour
   trigger[wind;vel=(5.0,2.0,0.0)]
   trigger[wind;vel=(2.0,6.0,0.0)]@t=3600

   # 5. Advance simulation clock by 2 hours
   goTo[t=7200]

   # 6. Export final front perimeter and arrival-time matrix
   print[final_front.geojson]
   save[]

See ``tests/runff/real_case.ff`` and ``tests/runff/params.ff`` in the repository for a complete real-world setup.
