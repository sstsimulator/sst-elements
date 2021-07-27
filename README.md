![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Structural Simulation Toolkit (SST)

#### Copyright (c) 2009-2021, National Technology and Engineering Solutions of Sandia, LLC (NTESS)
Portions are copyright of other developers:
See the file CONTRIBUTORS.TXT in the top level directory
of this repository for more information.

---

The Structural Simulation Toolkit (SST) was developed to explore innovations in highly concurrent systems where the ISA, microarchitecture, and memory interact with the programming model and communications system. The package provides two novel capabilities. The first is a fully modular design that enables extensive exploration of an individual system parameter without the need for intrusive changes to the simulator. The second is a parallel simulation environment based on MPI. This provides a high level of performance and the ability to look at large systems. The framework has been successfully used to model concepts ranging from processing in memory to conventional processors connected by conventional network interfaces and running MPI.

---

## Install

### Install with Spack

```
spack install sst-elements
```

### (TODO) Pull the docker container

```
docker pull sst-docker-image-name:latest-tag
```

## Building

Build from source locally
```
./autogen.sh

# make a build directory
mkdir build && cd build

# optionally build other elements
OTHER_ELEMENTS=

# configure
../configure --with-sst-core=<Path to SST Core> --prefix=$PWD/../install $OTHER_ELEMENTS

# build and install
make -j && make install
```

Build the container
```
docker build -t sst-container:latest .
```

Visit the [site](http://sst-simulator.org/SSTPages/SSTTopDocBuildInfo/) for more detailed instructions, including steps for how to build with support for elements.

---

Visit [sst-simulator.org](http://sst-simulator.org) to learn more about SST.

See [Contributing](https://github.com/sstsimulator/sst-elements/blob/devel/CONTRIBUTING.md) to learn how to contribute to SST.

##### [LICENSE](https://github.com/sstsimulator/sst-elements/blob/devel/LICENSE)
