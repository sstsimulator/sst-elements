FROM ubuntu:20.04

RUN apt-get update \
 && apt-get install -y \
              build-essential \
              curl \
              gfortran \
              git \
              python3 \
              vim \
 && apt-get clean all

RUN git clone -b package/sst-core-updates https://github.com/sknigh/spack.git

RUN /spack/bin/spack -k install sst-elements@11.0.0 \
 && spack clean -a
