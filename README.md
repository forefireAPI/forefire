# ForeFire

![logo](./docs_legacy/images/forefire.jpg)

<!-- _Refer to the [Wiki](https://github.com/forefireAPI/firefront/wiki) for a more detailed guide on using ForeFire._ -->

ForeFire is an open-source code for wildland fire spread models, developed and maintained by Université de Corse Pascal Paoli.
- [Website](https://forefire.univ-corse.fr/)
- [Demo simulator](http://forefire.univ-corse.fr/sim)
- [Publication](https://www.researchgate.net/publication/278769168_ForeFire_open-source_code_for_wildland_fire_spread_models)

![demo](./docs_legacy/images/sim-forefire.jpg)


It has been designed and runs on Unix systems.
The main binaries that can be built from source (CPP) are :
- An interpreter (executable)
- A shared library (with Python and Java bindings)

## 1. Building from source

ForeFire can be built from source by running `install-forefire.sh`

```
cd forefire

sudo bash install-forefire.sh
```

You can inspect this installer script to understand the linux dependencies and build commands

The program will be built in: `./bin/forefire`

The installer will ask if you wish to make `forefire` command available on the shell, by adding `export PATH="</path/to/bin>:$PATH"` at the end of your `~/.bashrc` file. Alternatively the file can also be manually edited with  `nano ~/.bashrc`

## 2. Running an example

Examples are provided on the `tests` folder. You can inspect `tests/runff/run.bash` to check usage

## 3. Running with python
Go and check [./bindings/python/README.md](./bindings/python/README.md)

## 4. Building with Docker
A sample Dockerfile can allow to build a Docker image with
```
docker build . -t forefire
```

To run this image and interactively acces the continer use
```
docker run -it forefire bash
```
