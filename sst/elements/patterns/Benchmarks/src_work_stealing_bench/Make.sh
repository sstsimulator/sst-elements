#!/bin/bash
if [[ $1 == "clean" ]] ; then
    rm -f ws
    exit
fi

if [[ $1 == "extra" ]] ; then
    extra="-Wextra -Wunused-macros -pedantic"
else
    extra=""
fi

mpic++ -Wall $extra -I../.. -I.. -I../Collectives \
	driver.cc \
	ws.cc \
	../util.c \
	../stat_p.c \
	../../stats.cc \
	../Collectives/allreduce.cc \
	../../collective_topology.cc \
	../../msg_counter.cc \
	-o ws -lm
