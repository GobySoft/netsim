# Building NETSIM

## Install from deb package on Debian / Ubuntu

The easiest way to install *netsim* is using Debian (stable and oldstable) and Ubuntu (currently supported LTS releases). Packages for these releases are built for the amd64, arm64, and armhf architectures and uploaded to http://packages.gobysoft.org

To install release packages on Ubuntu, run:

```
echo "deb http://packages.gobysoft.org/ubuntu/release/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
```

or for Debian:

```
echo "deb http://packages.gobysoft.org/debian/release/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
```

In both cases, then run:

```
sudo apt-key adv --recv-key --keyserver keyserver.ubuntu.com 19478082E2F8D3FE
sudo apt update
sudo apt install libnetsim-dev netsim-apps
```

## Build from source / Other distributions & releases

For other Linux distributions or releases, you will be required to build from source.

The list of build dependencies:

 - Jack2 library (`libjack-jackd2-dev` on Debian/Ubuntu): <https://github.com/jackaudio/jack2>
 - Goby3 core and moos libraries (and optionally gui library) (`libgoby3-dev, libgoby3-moos-dev, libgoby3-gui-dev` on Debian/Ubuntu): <http://gobysoft.org/doc/3.0/html/>
 - CMake and a C++ compiler (GCC or Clang)

Once you have the dependencies installed:

```
cd netsim
./build.sh
```

You can pass command line parameters to CMake by setting the environmental variable NETSIM_CMAKE_FLAGS and to make as arguments to ./build.sh or in the environmental variable NETSIM_MAKE_FLAGS.

For example:

 - To build with multiple cores:

```
./build.sh -j4
# or equivalently
NETSIM_MAKE_FLAGS=-j4 ./build.sh
```
 - To enable building Doxygen documentation:
```
NETSIM_CMAKE_FLAGS="-Dbuild_doc=ON" ./build.sh
```