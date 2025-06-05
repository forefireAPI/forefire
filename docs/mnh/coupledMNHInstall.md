# How to Install Coupled ForeFire with Meso NH; tested on OSX Ventura with Apple Silicon

This guide details the steps to install coupled ForeFire with Meso NH on Apple Silicon Macs running OSX Ventura.

## Requirements

Before proceeding, ensure the following packages are installed via Homebrew:

- GCC (should be pre-installed)
- GFortran
- OpenMPI

Acquire MesoNH from the official depot at [MesoNH Depot](http://mesonh.aero.obs-mip.fr). If encountering Git issues, download the tarball from [MNH-V5-7-0.tar.gz](http://mesonh.aero.obs-mip.fr/mesonh/dir_open/dir_MESONH/MNH-V5-7-0.tar.gz).

**Important Note:** Utilize Bash instead of Ksh to avoid potential script compatibility issues.

## Configuration Steps

1. **Export Environment Variables**

   Begin by setting the necessary environment variables in your terminal:

   ```bash
   export ARCH=LXgfortran
   export VER_MPI=MPIAUTO
   export OPTLEVEL=O2
   export MNH_FOREFIRE=1.0
   ```

2. **Run Configuration Script**

   Navigate to the `src` directory within the MesoNH directory and execute the configuration script:

   ```bash
   ./configure
   ```

   This should generate a configuration similar to:

   ```
   $SRC_MESONH/conf/profile_mesonh-LXgfortran-R8I4-MNH-V5-7-0-FF-MPIAUTO-O2
   ```

3. **Load the Configuration Profile**

   Load the generated configuration profile by running:

   ```bash
   . $SRC_MESONH/conf/profile_mesonh-LXgfortran-R8I4-MNH-V5-7-0-FF-MPIAUTO-O2
   ```


## Installation and Configuration

After setting up your environment variables, proceed with the compilation:

```bash
make -j 4
```

Then installation :

```bash
make installmaster
```

This command initiates the installation process, and upon completion, you should have a working setup of Meso NH ready for further configuration and compilation of additional components, such as ForeFire.

### Compiling ForeFire and Linking with Meso NH

ForeFire must be compiled separately before being linked. Follow the cmake instruction for forefire, to link, follow the steps below after compiling ForeFire:

1. Ensure ForeFire is compiled with a shared library, on osX it should be located at `firefront/lib/libforefireL.dylib`.
2. Navigate to the Meso NH `exe/` directory and create a symbolic link to the ForeFire library:

   ```bash
   ln -s PATHTOCOMPILEDFF/lib/libforefireL.dylib libForeFire.so
   ```

Replace `PATHTOCOMPILEDFF` with the actual path to your compiled ForeFire library. This step integrates ForeFire with Meso NH, enabling them to work together seamlessly.

In the `exe/` directory, you should now have manies entries but most importantly :

- `MESONH-LXgfortran-R8I4-MNH-V5-7-0-FF-MPIAUTO-O2-> $SRC_MESONH/src/dir_obj-LXgfortran-R8I4-MNH-V5-7-0-FF-MPIAUTO-O2/MASTER/MESONH`

And one more, in the same directory for ForeFire: `libForeFire.so -> PATHTOCOMPILEDFF/lib/libforefireL.dylib`

### Running a Simulation

To run a simulation with ForeFire:

1. Navigate to the case directory, for example, the one for case 016_FOREFIRE.
2. Set the `MPIRUN` environment variable to configure the number of CPUs to use. For instance, to use 2 CPUs, execute:

   ```bash
   export MPIRUN="mpirun -np 2"
   ```
3. Run the simulation as other KTESTS

## Fixups for OSX

During the installation and configuration of Meso NH and its components on OSX, you may encounter specific issues that require workarounds. Below are some solutions to common problems:

### Fixing libxml2 Configuration Issue

When configuring NetCDF 4.9.2, if you encounter an issue related to libxml2, follow these steps to resolve it:

1. Install autotools if you haven't already:

   ```bash
   brew install autoconf automake libtool
   ```

2. Navigate to the NetCDF C library source directory:

   ```bash
   cd $SRC_MESONH/src/LIB/netcdf-c-4.9.2/
   ```

3. Open `configure.ac` and locate the following line around line 1222:

   ```plaintext
   if test "x$ISOSX" = xyes && test "x$have_libxml2" = xno; then
   ```

4. Modify the line by removing the second `test` keyword, so it reads:

   ```plaintext
   if test "x$ISOSX" = xyes && "x$have_libxml2" = xno; then
   ```

5. Run `autoreconf` to regenerate the `configure` script from the modified `configure.ac`. This change should allow the configuration process to proceed without the libxml2 error.

### Fixing NetCDF C++ Configuration Hanging Issue

If the configuration of NetCDF C++ hangs due to a version file issue, perform the following steps:

1. In the `$SRC_MESONH/src/LIB/netcdf-cxx4-4.3.1` directory, you will find a `VERSION` file that incorrectly contains a version number (e.g., "-n 4.4.3"). This file needs to be empty.

2. You will need to use two terminals for this fix. In one terminal, navigate to the `$SRC_MESONH/src/LIB/netcdf-cxx4-4.3.1` directory. In the other terminal, run the Meso NH make process.

3. While the make process is running, frequently clear the `VERSION` file in the first terminal by running:

   ```bash
   cat VERSION; echo "" > VERSION
   ```

   This needs to be done quickly between the creation and usage of the `VERSION` file during the make process. Pay attention to the compilation stages: first HDF5 (which takes a few minutes), then a quick NetCDF 9 configuration, followed by the NetCDF C++ configuration where you need to apply the fix.

### Fixing MPI Run Infinite Loop Issue

On OSX, you might encounter an infinite loop issue with `mpirun` due to case sensitivity in paths. To resolve this:

1. Navigate to the Meso NH bin directory:

   ```bash
   cd $SRC_MESONH/bin
   ```

2. Rename `Mpirun` to avoid conflicts with the case-insensitive filesystem of OSX:

   ```bash
   mv Mpirun NoMpirun
   ```
