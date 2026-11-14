.. _userguide-scenario-studio:

Browser Scenario Studio
=======================

Scenario Studio runs ForeFire in a WebAssembly worker, using FFTile terrain and
fuel data. The GitHub Pages build bundles a Corsica subset for manual-wind
experiments; the full hosted application can also read an ERA archive.

Workflow
--------

Choose a fixed domain (up to 2° × 2°), start time, model and ignition point or
GeoJSON perimeter. Tiles load as the front approaches them; missing coverage
has zero fuel. Loading tiles does not expand the domain. An imported perimeter
is already burned at startup and is shared by all ensemble members.

The computed progress bar tracks solver time. The movable timeline selects the
view and the scheduled action time. Actions include ignitions, manual wind,
fuel breaks, fuel replacement and persistent nonburnable water-drop footprints.
Past edits replay from the initial state. RESET retains the initial case and
scenario; the new-case icon clears them.

FFState downloads preserve live nodes, arrival history and supported forcing;
base tiles remain external. In native ForeFire, resume with
``loadState[file=case.ffstate;provider=/path/to/tiles]``. Coupled attachment and
its checks are documented in ``tests/ffstate/README.md`` in the source tree.

Ensembles
---------

Three to fifteen members run sequentially in fresh workers. The seeded wind and
dead-moisture biases follow the subset of Table 2 in Allaire, Filippi & Mallet
(2020), DOI 10.1071/WF19073. These editable settings are not a fitted local
calibration. Reference numerics are used by default; Ultrafast is optional and
can cross narrow roads. Danger-class moisture and fuel mappings are experimental.

Maps show the fraction of members burned by the selected time. P10/P50/P90
arrival zones use all members, treating unburned cells as infinite arrival.
Initial burn is grey. The 384 × 384 display and retained perimeter snapshots
approximate the spread; they are not the native arrival matrix. The displayed
wind is timestamped reference 10 m wind, without member biases or model wind
reduction. Boundary-reaching members are flagged.

Build and hosting
-----------------

See ``bindings/wasm/scenario-studio/README.md`` for the source layout, staging
commands, vendored licenses and test commands. Git contains no generated WASM,
weather archive or tile payload. The optional PHP POST launcher needs PHP 8.1+
and does not run on GitHub Pages. ERA requires a range-capable archive with
appropriate CORS. Manual-wind operation needs neither PHP nor ERA.
