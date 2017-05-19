#!/bin/bash

#/** apply a patch to memory hierarchy for using gem5 **/
cd ../../memHierarchy
patch -p0 < ../CramSim/tests/patches/memH.patch


#/** get the gem5 **/
cd ../
git clone https://gem5.googlesource.com/public/gem5 ./gem5
cd ./gem5
git checkout 9ff8a2be92084e6be11b9ebb8cda9a20c4256398
patch -p0 < ../CramSim/tests/patches/gem5.patch

export PKG_CONFIG_PATH=$SST_CORE_HOME/lib/pkgconfig
scons ./build/ARM/gem5.opt -j 8
scons ./build/ARM/libgem5_opt.so -j 8

make -C ext/sst

cp ./build/ARM/libgem5_opt.so $SST_ELEMENT_HOME/lib/sst-elements-library
cp ./ext/sst/libgem5.so $SST_ELEMENT_HOME/lib/sst-elements-library

#/** compile dtb **/
make -C system/arm/dt


#/** get the linux kernel **/
cd ../CramSim/tests
git clone https://github.com/gem5/linux-arm-gem5.git
cd linux-arm-gem5
git checkout gem5/v4.3

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- gem5_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j4

cp -rf ../../../gem5/system/arm/dt ./
