.. _userguide-core-concepts:

Core Concepts
=============

Understanding the fundamental concepts behind ForeFire can help you use the simulator more effectively and interpret its behavior and outputs. This page provides a high-level overview of the key components and the simulation process.

Simulation Engine & Interpreter
-------------------------------

As discussed in the :doc:`/introduction`, ForeFire consists of:

1.  **Simulation Engine** (Library ``libforefireL``): The core C++ code containing the physics, data structures, and algorithms for simulating fire spread.
2.  **Interpreter** (Executable ``forefire``): The program you typically interact with. It reads commands (from scripts, console, or web UI) and instructs the engine library what to do.

This separation allows ForeFire to be used both as a standalone tool (via the interpreter) and as a component linked into other software (like atmospheric models).

Key Simulation Components
-------------------------

The simulation engine relies on several key C++ components (classes) to manage the fire spread process:

*   **`Simulator` & `TimeTable` (Event-Driven Core)**

    *   ForeFire uses an **event-driven** approach. Instead of fixed time steps for the entire simulation, it maintains a `TimeTable` (a prioritized queue) of future events.
    *   Each event typically corresponds to a `FireNode` needing an update at a specific future time.
    *   The `Simulator` processes events from the `TimeTable` in chronological order. It retrieves the next event, tells the associated object (like a `FireNode`) to update its state and calculate its next move, and then the object schedules its *next* update event back into the `TimeTable`.
    *   **This event-driven approach means the simulation doesn't use fixed global time steps; different parts of the front can advance according to their own calculated timings.**

*   **`FireDomain` (The Simulation World)**

    *   Represents the overall simulation area and context.
    *   Manages the simulation's current time.
    *   Defines the spatial boundaries (often derived from the :doc:`/user_guide/landscape_file`).
    *   Contains the `DataBroker` to access environmental data.
    *   Owns the main `FireFront` (s).
    *   Manages the `TimeTable` of simulation events.
    *   May handle discretization of the domain into cells (e.g., for flux calculations or burning map outputs).

*   **`FireFront` & `FireNode` (Representing the Fire)**

    *   ForeFire uses a **Lagrangian** approach to track the fire's edge.
    *   A `FireFront` represents a continuous segment of the fire perimeter. It is essentially a linked list of `FireNode` objects.
    *   A `FireNode` is a point (marker) on the fire front. It has properties like:

        *   Location (x, y, z coordinates)
        *   Velocity (calculated based on local conditions)
        *   Normal vector (direction perpendicular to the front)
        *   Curvature and potentially front depth
    *   When a `FireNode`'s event occurs, it calculates its propagation speed based on local environmental data and the chosen :ref:`propagation model <param-propagationmodel>`, determines its next location, and schedules its next update.
    *   The `FireDomain` monitors the `FireNode` s to handle topological changes:

        *   **Adding nodes:** When nodes spread too far apart (controlled by :ref:`perimeterResolution <param-perimeterresolution>`), new nodes are inserted.
        *   **Merging:** When different parts of a `FireFront` touch, they merge.
        *   **Splitting:** Not explicitly mentioned in summary, but might occur if front self-intersects or hits complex boundaries.
        *   **Inner Fronts:** `FireFront` s can be nested to represent unburned islands within the main fire perimeter.

*   **`DataBroker` & `DataLayer` (Accessing Environmental Data)**

    *   The `DataBroker` is the central hub for all environmental data needed by the simulation (terrain, fuel, weather).
    *   It manages a collection of `DataLayer` objects. Each `DataLayer` provides access to a specific type of data (e.g., elevation, fuel index, wind speed).
    *   **Data layers can also manage which specific physical model (e.g., which heat flux calculation) applies at different locations, often based on index maps read from the input files.**
    *   Different `DataLayer` subclasses handle various data sources and formats:

        *   Loading grids from the NetCDF :doc:`/user_guide/landscape_file`.
        *   Handling constant values across the domain (e.g., constant wind).
        *   Calculating gradients (like slope/aspect from elevation).
        *   Accessing the computed arrival time map (Burning Map).
        *   Interpolating data in space and potentially time (e.g., for time-varying wind fields).
    *   When a `FireNode` needs environmental data to calculate its speed, it asks the `DataBroker`, which efficiently retrieves the necessary values from the relevant `DataLayer` (s) at the node's location.

*   **`PropagationModel` & `FluxModel` (The Physics)**

    *   ForeFire uses a **Strategy pattern** for physical calculations, allowing different models to be plugged in.
    *   `PropagationModel` subclasses (e.g., `Rothermel`, `Balbi`, `Iso`) implement specific algorithms to calculate the Rate of Spread (ROS) based on properties fetched via the `DataBroker`. The active model is chosen using the :ref:`propagationModel <param-propagationmodel>` parameter.
    *   `FluxModel` subclasses (e.g., `HeatFluxBasicModel`) calculate fluxes (like heat, water vapor) from the burning area, often needed for coupled simulations or specific research outputs.

*   **`SimulationParameters` (Configuration)**

    *   A central place (likely a Singleton class internally) holding the global configuration values set by the user via :ref:`setParameter <cmd-setparameter>` or :ref:`setParameters <cmd-setparameters>` commands. This includes things like model choices, resolution parameters, output settings, etc.

Simulation Workflow Summary
---------------------------

1.  **Initialization:**

    *   The `forefire` interpreter starts.
    *   Commands from a script file (or interactive input) are processed.
    *   :doc:`Parameters </reference/parameters>` are set.
    *   The :ref:`FireDomain <cmd-firedomain>` is created.
    *   :ref:`loadData <cmd-loaddata>` populates the `DataBroker` with `DataLayer`s from the :doc:`/user_guide/landscape_file`. The :doc:`/user_guide/fuels_file` information is associated.
    *   The initial fire state is defined using :ref:`startFire <cmd-startfire>` or custom `FireFront`/`FireNode` commands, scheduling the first update events for the initial nodes into the `TimeTable`.

2.  **Simulation Loop (driven by `step` or `goTo`):**

    *   The `Simulator` gets the chronologically next event (usually a `FireNode` update) from the `TimeTable`.
    *   The `Simulator` advances the global simulation time to the event time.
    *   The `Simulator` calls the update method on the `FireNode`.
    *   The `FireNode` calculates its new position based on its previously calculated velocity.
    *   The `FireDomain` checks for topological changes (node spacing, potential merges).
    *   The `Simulator` calls the time-advance method on the `FireNode`.
    *   The `FireNode`:

        *   Determines its local normal vector and curvature.
        *   Requests necessary environmental properties (fuel type, slope, wind, etc.) from the `DataBroker` for its current location.
        *   Calls the active `PropagationModel`'s `getSpeed()` method with these properties.
        *   Calculates its new velocity and intended next location.
        *   Determines its next update time based on speed and resolution parameters.
        *   Schedules its next update event back into the `TimeTable`.
    *   This loop continues until the target time specified by :ref:`goTo <cmd-goto>` is reached, the duration specified by :ref:`step <cmd-step>` elapses, or the `TimeTable` becomes empty.

3.  **Output:**

    *   Commands like :ref:`print <cmd-print>`, :ref:`save <cmd-save>`, or :ref:`plot <cmd-plot>` are executed (either when encountered in the script or scheduled via :ref:`@t=... <cmd-@>`) to query the state of the `FireFront` (s) or `DataLayer` (s) (like the burning map) and write them to files.

Understanding these core concepts provides context for the various commands, parameters, and input files required to run a ForeFire simulation.