#!/bin/sh

for sdl in sdl*.xml ;
do
    echo ${sdl}
    rm -f ${sdl}.new
    sst.x $sdl > ${sdl}.new 2>&1
    diff -q ${sdl}.out ${sdl}.new && rm -f ${sdl}.new
done

rm -f dramsim*log
