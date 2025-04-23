Installation
============

ForeFire is primarily designed for Linux environments. Here are the recommended ways to get it running.

Method 1: Using Docker (Recommended Easiest)
---------------------------------------------

This is often the simplest way to get a working ForeFire environment, as it bundles all dependencies. It requires Docker to be installed on your system.

1.  **Clone the repository:**
    .. code-block:: bash

       git clone https://github.com/forefireAPI/firefront.git
       cd firefront

2.  **Build the Docker image:**
    The `Dockerfile` in the repository defines the environment. Build the image using:
    .. code-block:: bash

       docker build . -t forefire:latest

3.  **Running the Container:**
    Instructions for running the container and using ForeFire within it are covered in the :doc:`quickstart` guide.

Method 2: Building from Source (Linux/Unix-like)
-------------------------------------------------

This method involves compiling ForeFire directly on your system.

**Prerequisites:**

Before building, ensure you have the necessary tools and libraries:

*   **C++ Compiler:** A modern C++ compiler (like `g++`). Package typically called `build-essential` or similar.
*   **CMake:** Build system generator (`cmake`).
*   **Make:** Build tool (`make`).
*   **NetCDF Libraries:** ForeFire requires NetCDF support. The specific package needed is the C++ interface.
    *   On **Debian/Ubuntu**, the install script uses `libnetcdf-c++4-dev`.
    *   *(Note: Ensure this is the correct/intended library. Older docs might mention `libnetcdf-cxx-legacy-dev`. Verify which one is actually required by the current CMake setup).*
    *   On other systems, find the equivalent package (e.g., `netcdf-cxx-devel`, `netcdf-cxx4`).

**Option 2a: Using the Install Script (Recommended for Debian/Ubuntu)**

The repository provides a convenience script (`install-forefire.sh`) that automates the process on Debian-based systems like Ubuntu.

1.  **Clone the repository:**
    .. code-block:: bash

       git clone https://github.com/forefireAPI/firefront.git
       cd firefront

2.  **Run the install script:**
    .. warning::
       This script requires `sudo` privileges to install system packages using `apt`. Review the script if you have concerns.

    .. code-block:: bash

       sudo bash install-forefire.sh

**What the Install Script Does:**

*   **Updates Package Lists:** Runs `apt-get update`.
*   **Installs Dependencies:** Installs `build-essential`, `libnetcdf-c++4-dev`, and `cmake` using `apt-get install -y`.
*   **Builds ForeFire:**
    *   Creates a `build` directory.
    *   Navigates into `build`.
    *   Runs `cmake ..` to configure the build.
    *   Runs `make` to compile the code.
*   **Reports Install Location:** Prints the location of the built binaries (usually `$PROJECT_ROOT/bin`).
*   **(Optional) Updates PATH:**
    *   Prompts the user if they want to add the ForeFire `bin` directory to their PATH permanently.
    *   If 'yes', it appends `export PATH=` and `export FOREFIREHOME=` lines to the user's `~/.bashrc` file.
    *   It tries to detect the correct user's home directory even when run with `sudo` (using `$SUDO_USER`).
    *   **Note:** This only modifies `.bashrc`. If you use a different shell (like `zsh` or `fish`), you will need to configure the PATH manually (see below).

**Option 2b: Manual Build Steps (All Linux/Unix-like Systems)**

Use this method if you are not on Debian/Ubuntu, prefer manual control, or don't want to use the install script.

1.  **Clone the repository:**
    .. code-block:: bash

       git clone https://github.com/forefireAPI/firefront.git
       cd firefront

2.  **Install Prerequisites Manually:**
    Use your system's package manager to install `cmake`, `make`, a C++ compiler (`build-essential` or equivalent), and the required NetCDF C++ development library (e.g., `libnetcdf-c++4-dev`, `netcdf-cxx-devel`, etc.).
    Example for Debian/Ubuntu (if not using the script):
    .. code-block:: bash

       sudo apt update
       sudo apt install build-essential cmake libnetcdf-c++4-dev # Verify package name!

3.  **Create a build directory and run CMake & Make:**
    .. code-block:: bash

       mkdir build
       cd build
       cmake ..
       make

4.  **Executable Location:** The main executable `forefire` will be located at `../bin/forefire` (relative to the `build` directory).

Making ForeFire Executable System-Wide (Manual PATH setup)
------------------------------------------------------------

If you built from source (manually or via the script but declined the automatic PATH update, or use a shell other than bash), the `forefire` executable is in the `bin` directory within the repository. To run it easily from any location, add this directory to your system's PATH environment variable.

**For the current terminal session:**

.. code-block:: bash

   # Execute this from the root of the firefront repository
   export PATH=$PATH:`pwd`/bin

**Permanently:**

Add the following line to your shell's configuration file (e.g., `~/.bashrc`, `~/.zshrc`, `~/.profile`, or `~/.config/fish/config.fish`). Replace `/path/to/firefront` with the actual absolute path to the cloned repository.

.. code-block:: bash

   export PATH="/path/to/firefront/bin:$PATH"

*Optional:* The install script also sets `export FOREFIREHOME="/path/to/firefront"`. You may want to add this line as well, as some scripts or components might potentially use it.

.. code-block:: bash

   export FOREFIREHOME="/path/to/firefront"

After editing your configuration file, either restart your terminal or reload the configuration (e.g., `source ~/.bashrc`).