# ForeFire

![logo](./docs/images/forefire.jpg)

_Refer to the [Wiki](https://github.com/forefireAPI/firefront/wiki) for a more detailed guide on using ForeFire._

ForeFire is an [open-source code for wildland fire spread models](https://www.researchgate.net/publication/278769168_ForeFire_open-source_code_for_wildland_fire_spread_models), developed and maintained by Université de Corse Pascal Paoli.

Access the [demo simulator here](http://forefire.univ-corse.fr/sim/dev/).

![demo](./docs/images/sim-forefire.jpg)


It has been designed and runs on Unix systems. Three modules can be built with the source code.

The main binaries are  
  - An interpreter (executable)
  - A shared library (with C/C++/Java Python and Fortran bindings)

## 1. Requirements

The requirements and ForeFire can be installed by running `install-forefire.sh` (Ubuntu or Debian distributions)

```
cd forefire

sudo sh install-forefire.sh
```

The program will be built in: `./bin/forefire`

OR run the commands:

```
apt-get update

apt install build-essential -y

apt install libnetcdf-c++4-dev -y

apt install cmake -y
```

To install
- The C++ compiler
- [NetCDF Library](https://www.unidata.ucar.edu/software/netcdf/) and [NetCDF-C++ ](https://www.unidata.ucar.edu/downloads/netcdf/netcdf-cxx/index.jsp)
- [Cmake](https://cmake.org/) build tool

## 2. Build

### 2.1 Cmake

To build with cmake run the script
```
sh cmake-build.sh
```

To make the program [executable from eveywhere](https://unix.stackexchange.com/questions/3809/how-can-i-make-a-program-executable-from-everywhere) (during the session) Add the bin folder to path
```
export PATH=$PATH:`pwd`/bin
```
If you want to change it permanently, paste
```
export PATH="</path/to/file>:$PATH"

```
at the end of your `~/.bashrc` file. The file can be edited with
```
nano ~/.bashrc
```
for example
```
export PATH="/mnt/c/gitrepos/forefire/bin:$PATH"
```


### 2.2 Scons and Other build systems
More information on other build systems are available [here](./docs/buildSystems/readme.MD)

## 3. Running an example

An example for the region of aullene in south France is provided. The example contains 3 files
- fuels.ff
- aullene.ff
- landscape.nc:

Run the example with

```
cd firefront/examples/aullene/

../../bin/forefire -i aullene.ff
```
The simulation result will be outputed in JSON format


### 4. Running with python
Go and check [pyForeFire](https://github.com/forefireAPI/pyForeFire)
It may be included directly in this repo in futire releases

## 5. Building with Docker
A sample Dockerfile can allow to build a Docker image with
```
docker build . -t forefire
```

To run this image and interactively acces the continer use
```
docker run -it forefire bash
```
