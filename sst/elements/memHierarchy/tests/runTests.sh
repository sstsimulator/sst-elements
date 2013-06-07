#!/bin/bash

#for sdl in sdl*.xml ;
#do
#    echo ${sdl}
#    rm -f ${sdl}.new
#    sst.x $sdl > ${sdl}.new 2>&1
#    diff -q ${sdl}.out ${sdl}.new && rm -f ${sdl}.new
#done

runCmd() {
    if [ -z $1 ]
    then
        echo "Need to pass a param."
        return 1
    fi
    rm -f ${1}.new
    echo ${1}
    sst.x ${1} > ${1}.new 2>&1
    diff -q ${1}.out ${1}.new && rm -f ${1}.new
}

export -f runCmd

find . -depth 1 -name sdl\*.xml -print0 | xargs -0 -n 1 -P 4 bash -c 'runCmd "$0"'



rm -f dramsim*log
