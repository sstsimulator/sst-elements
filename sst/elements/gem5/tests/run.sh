#!/bin/sh

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${HOME}/SST/gem5-dev/build/X86
export GEM5=${HOME}/SST/gem5-dev
export M5_PATH=${HOME}/SST/gem5-data
export PYTHONPATH=${GEM5}/configs/common
exec sst test4_ncore.py 
#exec sst test4.py
