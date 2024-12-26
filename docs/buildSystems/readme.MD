### Scons and other build systems


Forefire can also be built with [scons](https://www.scons.org/). Install it with
```
apt install scons -y
```

A sample `SConstruct` file is included with the distribution.
Run it with
```
scons
```

To build with all warnings enabled
```
 scons -Q w=1 
```

Troubleshooting: If it does not work, try replacing the `Sconstruct` file with `./tools/Sconstruct`. Set the environment variables, and insert the path to the Netcdf (and Java headers for JNI bindings if required).

Make: A simple `makefile` is also available in the `tools` directory

More info on build systems can be found on [this issue](https://github.com/forefireAPI/firefront/issues/9)