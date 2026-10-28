Installation
============

There are three ways to install ForeFire. Which one you want depends on what
you intend to do with it:

.. list-table::
  :header-rows: 1
  :widths: 20 45 35

  * - Method
    - Use it when
    - Section
  * - **pip**
    - You want to run simulations or drive ForeFire from Python.
    - :ref:`install-pip`
  * - **Docker**
    - You are on Windows, or you want the web console with no setup.
    - :doc:`quickstart`
  * - **From source**
    - You need MPI coupling with Meso-NH, a CPU-tuned build, or you are
      working on ForeFire itself.
    - :ref:`install-source`

.. _install-pip:

Install with pip
----------------

This is the fastest route, and it needs no compiler, no CMake and no NetCDF
installation of your own — NetCDF is bundled inside the wheel.

.. code-block:: bash

  pip install forefire

Requirements
~~~~~~~~~~~~

- **CPython 3.9 to 3.14.** The free-threaded builds (``cp314t``) are
  deliberately not published: the C++ core still keeps mutable global state, so
  a wheel advertising free-threading would silently re-enable the GIL on
  import.
- **Linux** on ``x86_64`` or ``aarch64`` (``manylinux_2_28`` or newer), or
  **macOS 14+ on Apple Silicon**.

There is no wheel for Intel macOS, musl-based Linux (Alpine), 32-bit targets,
Windows or PyPy. On those platforms pip falls back to the source distribution,
which needs the build prerequisites in :ref:`install-source`. Windows users are
better served by Docker — see :doc:`quickstart`.

What you get
~~~~~~~~~~~~

Both interfaces, from the one package:

.. code-block:: bash

  forefire -v

.. code-block:: python

  import pyforefire as forefire

  ff = forefire.ForeFire()
  ff.execute("FireDomain[sw=(0,0,0);ne=(10000,10000,0);t=0]")
  ff.addLayer("propagation", "Iso", "propagationModel")
  ff.execute("startFire[loc=(5000,5000,0.0)]")
  ff.execute("step[dt=1000]")
  print(ff.execute("print[]"))

.. note::

  The distribution on PyPI is called ``forefire``; the importable module keeps
  its historical name, ``pyforefire``.

What you do not get
~~~~~~~~~~~~~~~~~~~

Published wheels are built with three options turned off, so a few things are
only available in a source build:

- **MPI coupling.** Fire-atmosphere runs with Meso-NH need
  ``FOREFIRE_ENABLE_MPI``, so they need a source build.
- **CPU tuning.** Wheels are built without ``-march=native``, so that they run
  on any machine of the right architecture. A source build with
  ``FOREFIRE_NATIVE_ARCH=ON`` will be faster on the machine that built it.
- **The** ``ANN_test`` **helper**, built by ``FOREFIRE_BUILD_TOOLS``.

.. _install-source:

Build from source
-----------------

A native build, in exchange for managing the dependencies yourself. Two routes:
the install script (Debian/Ubuntu only), or manual steps (any Unix-like
system).

Prerequisites
~~~~~~~~~~~~~

- **A C++ compiler**, such as ``g++``. On Debian/Ubuntu this comes with
  ``build-essential``.
- **CMake** 3.15 or newer, and **Make**.
- **NetCDF — both the C library and the legacy C++4 API.** The C++4 API is a
  separate package from the C library on every distribution, and it is the one
  people usually miss.

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

  The library is named ``netcdf_c++4`` everywhere except Homebrew, which calls
  it ``netcdf-cxx4`` (``netcdf-cxx`` in older bottles); CMake looks for all
  three. The older ``libnetcdf-cxx-legacy-dev`` package is a *different*,
  pre-C++4 API and will not work.

  If NetCDF is installed somewhere CMake does not search, point at it with
  ``-DNETCDF_HOME=/path/to/netcdf`` (and ``-DNETCDF_CXX_HOME=...`` if the C++
  API lives elsewhere).

Option 1: the install script (Debian/Ubuntu)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``install-forefire.sh`` automates the process on Debian-based systems.

1.  **Clone the repository:**

  .. code-block:: bash

    git clone https://github.com/forefireAPI/forefire.git
    cd forefire

2.  **Run the install script:**

  .. warning::

    This script requires ``sudo`` privileges to install system packages using
    ``apt``. Review the script if you have concerns.

  .. code-block:: bash

    sudo bash install-forefire.sh

**What the install script does:**

- **Installs dependencies:** runs ``apt-get update`` and installs the
  prerequisites listed above.
- **Builds ForeFire** using CMake and Make.
- **Reports the install location:** usually ``$PROJECT_ROOT/bin``.
- **(Optional) updates PATH:**

  - Prompts you before doing anything, and only if you agree appends
    ``export PATH=`` and ``export FOREFIREHOME=`` lines to ``~/.bashrc``.
  - It detects the invoking user's home directory even under ``sudo``, via
    ``$SUDO_USER``.
  - **Note:** this only modifies ``.bashrc``. For ``zsh`` or ``fish``,
    configure the PATH manually (see below).

Option 2: manual build (any Linux/Unix-like system)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use this if you are not on Debian/Ubuntu, prefer manual control, or do not
want to run the install script.

1.  **Clone the repository:**

  .. code-block:: bash

    git clone https://github.com/forefireAPI/forefire.git
    cd forefire

2.  **Install the prerequisites** for your system, from the table above.

3.  **Configure and build:**

  .. code-block:: bash

    cmake -S . -B build
    cmake --build build -j

  The executable lands in ``bin/forefire``. Check it with:

  .. code-block:: bash

    ./bin/forefire -v

Build options
~~~~~~~~~~~~~

The build is driven by ``FOREFIRE_*`` CMake options; pass them at configure
time, for example ``cmake -S . -B build -DFOREFIRE_ENABLE_MPI=OFF``.

.. list-table::
  :header-rows: 1
  :widths: 30 15 55

  * - Option
    - Default
    - Effect
  * - ``FOREFIRE_ENABLE_MPI``
    - ON
    - Fire-atmosphere coupling with Meso-NH, when MPI is found. Replaces the
      compiler with the MPI wrapper.
  * - ``FOREFIRE_NATIVE_ARCH``
    - ON
    - ``-march=native``. Turn it off for a binary you intend to move to
      another machine.
  * - ``FOREFIRE_BUILD_PYTHON``
    - OFF
    - Build the ``pyforefire`` extension module.
  * - ``FOREFIRE_STATIC_CORE``
    - OFF
    - Link the core statically.
  * - ``FOREFIRE_BUILD_TOOLS``
    - ON
    - Build the ``ANN_test`` helper, needed by ``tests/runANN``.
  * - ``FOREFIRE_CHECK_LFS``
    - ON
    - Fail early if the Git LFS test fixtures were not pulled.
  * - ``FOREFIRE_BUILD_TESTS``
    - ON
    - Build the C++ unit tests and register them with CTest.
  * - ``FOREFIRE_ENABLE_WARNINGS``
    - ON
    - Compile ForeFire's own sources with ``-Wall -Wextra``.
  * - ``FOREFIRE_WARNINGS_AS_ERRORS``
    - OFF
    - Fail the build on a compiler warning.
  * - ``FOREFIRE_SANITIZE``
    - *(empty)*
    - Sanitizers to build with, passed to ``-fsanitize=``, for example
      ``address,undefined``.

Wheel builds flip six of these: MPI, native-arch, tools, tests and the LFS
check off, Python and the static core on. That is what makes a wheel run on a
machine other than the one that built it.

``TESTING.md`` at the repository root covers the test suites and the sanitizer
build in detail.

Making ForeFire available system-wide
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After a source build, ``forefire`` lives in the repository's ``bin``
directory. To run it from anywhere, add that directory to your PATH.

**For the current terminal session:**

.. code-block:: bash

  # Execute this from the root of the forefire repository
  export PATH=$PATH:`pwd`/bin

**Permanently:**

Add the following to your shell's configuration file (``~/.bashrc``,
``~/.zshrc``, ``~/.profile``, or ``~/.config/fish/config.fish``), replacing
``/path/to/forefire`` with the absolute path to the cloned repository.

.. code-block:: bash

  export PATH="/path/to/forefire/bin:$PATH"

*Optional:* the install script also sets ``FOREFIREHOME``, which some scripts
and components use to locate the repository.

.. code-block:: bash

  export FOREFIREHOME="/path/to/forefire"

Then restart your terminal or reload the configuration, for example with
``source ~/.bashrc``.
