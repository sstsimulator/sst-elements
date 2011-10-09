#!/bin/bash
if [[ $1 == "clean" ]] ; then
    rm ghost
    exit
fi

mpic++ -Wall -I../.. -I.. \
	ghost_driver.cc \
	ghost.c \
	memory.c \
	neighbors.c \
	ranks.c \
	work.c \
	../util.c \
	../stat_p.c \
	../../stats.cc \
	../Collectives/allreduce.cc \
	../../collective_topology.cc \
	-o ghost -lm
