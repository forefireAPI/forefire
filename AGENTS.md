# ForeFire workspace

ForeFire is a wildfire spread simulator. The C++ core is in `src/` (fronts,
domain, data layers, `propagation/` and `flux/` models), the interactive shell
in `app/forefire`, the Python bindings in `bindings/python`, the Emscripten
build in `bindings/wasm` and the browser GUI served by `listenHTTP[]` in
`tools/htdocs`. Do not commit, push or open a pull request until asked. Never
merge.

Read README.md, CONTRIBUTING.md and TESTING.md, check `git status` and keep
uncommitted edits you did not make. Take behaviour from the current source
(shell commands are in `src/Command.cpp`), not from old scripts in `tools/` or
values quoted in issues. Long simulations, large downloads and Meso-NH runs are
not part of routine development.

- Native build: CMake 3.15+, C++17. NetCDF, MPI, CLI, Python, WASM and tests
  are `FOREFIRE_*` options in CMakeLists.txt. The version lives only in
  `src/include/Version.h` and changes in the release commit, never on `dev`.
- Python: the root pyproject.toml builds `pyforefire`. The core keeps global
  state, so a second `ForeFire()` in one process inherits the first one's
  parameters. Use `ForeFireRunner` for sweeps and parallel runs.
- Browser: the WASM build has no NetCDF. Do not assume a native feature works
  there without running `bindings/wasm/smoke.mjs`.
- Meso-NH coupling (`MPI_COUPLING`, `docs/mnh`, `tests/mnh_*`) is not built in
  CI. Report coupled behaviour as untested unless you ran it.
- Tests: CTest unit tests, `tests/runff` and `tests/python`, see TESTING.md.
  `*.ref` files are in Git LFS; configure with `-DFOREFIRE_CHECK_LFS=OFF` when
  only `tests/runff` was pulled. Report a test you could not run as skipped.

Propagation and flux models, their parameters and the core data structures
(singletons, `FireDomain`, `DataBroker`) are the author's scientific and
architectural decisions. Agents implement; they do not decide physics or
architecture. Propose such changes in an issue and wait for a maintainer.
Do not change simulation results while doing docs, packaging, CI or test work,
and never regenerate a `.ref` file to make a test pass. A change that moves
results goes in its own pull request and at the top of the CHANGELOG entry.

Pull requests target `dev`. Add user-visible changes under `## [Unreleased]`
in CHANGELOG.md. Keep issue numbers and history out of code comments; they
belong in the CHANGELOG and the pull request. Report security problems through
GitHub private vulnerability reporting, never in a public issue. Do not leave
build directories, virtual environments, test outputs, workstation paths or
credentials in the tree.
