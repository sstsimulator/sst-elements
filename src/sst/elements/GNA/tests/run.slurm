#!/bin/bash
#SBATCH -N 1
# time sst ./test_2.py -- -n 10000 -c 32 -s 256 -m 32 -a 32 -g 2 -C .21
date
export argg="-c ${1} -s ${2} -m ${3} -a ${4} -g ${5} -C .${6}"
export file="c${1}-s${2}-m${3}-a${4}-g${5}-C${6}.txt"
echo "time sst test_2.py -- -n 10000 ${argg} |& tee $file"
time sst test_2.py -- -n 10000 ${argg} |& tee $file
date
echo
