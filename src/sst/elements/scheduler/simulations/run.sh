#!/bin/bash


sst --model-options="--topo=dragonfly --shape=9:2:4:4 --routingAlg=minimal --numCores=2 --netFlitSize=8B --netPktSize=1024B --netBW=4GB/s --host_bw=1GB/s --group_bw=1GB/s --global_bw=4GB/s --embermotifLog=./motif --loadFile=loadfile --mapFile=mapFile.txt --networkStatOut=networkStats.csv --rankmapper=ember.CustomMap " emberLoad.py

#sst --model-options="--topo=torus --shape=3x4x6 --numCores=2 --netFlitSize=8B --netPktSize=1024B --netBW=1GB/s --embermotifLog=./motif --loadFile=loadfile " emberLoad.py

rm core.*

