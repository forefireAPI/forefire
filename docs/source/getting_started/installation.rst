.. _install-source:

Building from source
====================

``pip install forefire`` covers running simulations from Python and the command line (:doc:`quickstart`). The wheels are built to run on any machine, so a few things need a build of your own:

- **Coupling with Meso-NH**, which needs MPI (``FOREFIRE_ENABLE_MPI=ON``, see :doc:`/user_guide/mesonh`).
- **Full speed on your CPU**: the wheels leave out ``-march=native``.
- **The WebAssembly module** for the browser.
- **Changing the C++**, for example to add a propagation model (:ref:`custom-cpp-models`).
- **The** ``ANN_test`` **helper**, built by ``FOREFIRE_BUILD_TOOLS=ON``.

Prerequisites
-------------

- **A C++17 compiler** (``g++`` or ``clang++``).
- **CMake** 3.15 or newer, and **Make**.
- **NetCDF — both the C library and the C++4 API** (unless building with ``-DFOREFIRE_ENABLE_NETCDF=OFF``):

  .. list-table::
    :header-rows: 1
    :widths: 25 75

    * - System
      - Packages
    * - Debian/Ubuntu
      - ``apt install libnetcdf-dev libnetcdf-c++4-dev``
    * - Fedora/RHEL
      - ``dnf install netcdf-devel netcdf-cxx4-devel``
    * - macOS (Homebrew)
      - ``brew install netcdf netcdf-cxx``

  The C++4 library is named ``netcdf_c++4`` on Linux and ``netcdf-cxx4`` (or ``netcdf-cxx``) on Homebrew; CMake checks all three. The older ``libnetcdf-cxx-legacy-dev`` package is a pre-C++4 API and will not work.

  If NetCDF lives in a custom prefix, pass ``-DNETCDF_HOME=/path/to/netcdf`` (and ``-DNETCDF_CXX_HOME=...`` if separate).

Option 1: install scripts (Debian/Ubuntu and macOS)
---------------------------------------------------

The repository includes helper scripts that install dependencies via ``apt`` or Homebrew and run the CMake build:

.. code-block:: bash

  git clone https://github.com/forefireAPI/forefire.git
  cd forefire

  # On Debian/Ubuntu:
  sudo bash install-forefire.sh

  # On macOS (with Homebrew):
  bash install-forefire-osx.sh

Option 2: manual CMake build
----------------------------

.. code-block:: bash

  git clone https://github.com/forefireAPI/forefire.git
  cd forefire
  cmake -S . -B build
  cmake --build build -j

The executable lands in ``bin/forefire`` and the shared library in ``lib/libforefireL.so`` (or ``.dylib``):

.. code-block:: bash

  ./bin/forefire -v

CMake build options
-------------------

Pass ``-DFOREFIRE_<OPTION>=...`` at configure time (for example, ``cmake -S . -B build -DFOREFIRE_ENABLE_MPI=OFF``). Below are the defaults for a standard native build:

.. list-table::
  :header-rows: 1
  :widths: 32 13 55

  * - Option
    - Default
    - Effect
  * - ``FOREFIRE_ENABLE_MPI``
    - ``ON``
    - Enable MPI coupling (for Meso-NH) when MPI is found on the system.
  * - ``FOREFIRE_ENABLE_NETCDF``
    - ``ON``
    - Build NetCDF-backed I/O paths. When ``OFF``, ForeFire compiles without NetCDF headers or libraries; array layers and in-memory runs work normally, while ``loadData`` (with a date) and ``save`` report ``no NetCDF support`` and ``FireDomain`` does not accept ``pgdNcFile``.
  * - ``FOREFIRE_BUILD_CLI``
    - ``ON``
    - Build the interactive ``forefire`` command-line interpreter.
  * - ``FOREFIRE_BUILD_WASM``
    - ``OFF``
    - Build the Emscripten/embind module (``forefire.wasm`` + ``forefire.mjs``). Automatically enabled under ``emcmake``.
  * - ``FOREFIRE_NATIVE_ARCH``
    - ``ON``
    - Compile with ``-march=native``. Turn ``OFF`` for portable binaries.
  * - ``FOREFIRE_BUILD_PYTHON``
    - ``OFF``
    - Build the ``pyforefire`` Python extension module.
  * - ``FOREFIRE_STATIC_CORE``
    - ``OFF``
    - Build the ForeFire core as a static library instead of ``libforefireL``.
  * - ``FOREFIRE_BUILD_TOOLS``
    - ``ON``
    - Build the ``ANN_test`` helper executable (used by ``tests/runANN``).
  * - ``FOREFIRE_CHECK_LFS``
    - ``ON``
    - Verify Git LFS test data integrity at configure time.
  * - ``FOREFIRE_BUILD_TESTS``
    - ``ON``
    - Build C++ unit tests and register them with CTest.
  * - ``FOREFIRE_ENABLE_WARNINGS``
    - ``ON``
    - Compile ForeFire sources with ``-Wall -Wextra``.
  * - ``FOREFIRE_WARNINGS_AS_ERRORS``
    - ``OFF``
    - Treat compiler warnings as errors.
  * - ``FOREFIRE_SANITIZE``
    - *(empty)*
    - Sanitizers passed to ``-fsanitize=`` (e.g. ``address,undefined``).

When driven by ``pip`` / ``scikit-build-core``, seven options flip automatically for portability: ``MPI=OFF``, ``NATIVE_ARCH=OFF``, ``TOOLS=OFF``, ``CHECK_LFS=OFF``, ``TESTS=OFF``, ``PYTHON=ON``, and ``STATIC_CORE=ON``.

When driven by ``emcmake`` (Emscripten), CMake automatically sets ``NETCDF=OFF``, ``MPI=OFF``, ``CLI=OFF``, ``WASM=ON``, and ``STATIC_CORE=ON`` to produce ``bin/forefire.wasm`` and ``bin/forefire.mjs``. See ``bindings/wasm/README.md`` in the repository for details on building and serving the WebAssembly target.

Environment variables (PATH and FOREFIREHOME)
---------------------------------------------

After a source build, add ``bin/`` to ``PATH`` and set ``FOREFIREHOME`` to the repository root:

.. code-block:: bash

  export FOREFIREHOME="/path/to/forefire"
  export PATH="$FOREFIREHOME/bin:$PATH"

``FOREFIREHOME`` is required when using the built-in web interface (``listenHTTP[]``) so the HTTP server can find the static UI files under ``$FOREFIREHOME/tools/htdocs`` (including the 2D console and the 3D Meso-NH viewer ``mnh3d.html``).
