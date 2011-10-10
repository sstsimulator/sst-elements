#!/bin/bash
if [[ $1 == "clean" ]] ; then
    rm -f ws
    exit
fi

extra=$1

mpic++ -Wall $extra -I../.. -I.. \
	driver.cc \
	ws.cc \
	../util.c \
	../stat_p.c \
	../../stats.cc \
	-o ws -lm
