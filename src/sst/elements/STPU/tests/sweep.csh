#!/bin/tcsh

#foreach c (1 4 16 64)
foreach c (${1})
    foreach s (32 128 512 2048)
        set fileN = test-${c}K-${s}.out
        echo "running $fileN"
        rm -f $fileN
        time sst ./test.py -- -n 2500 -c $c -s $s -m $s >& $fileN &
        sleep 1
    end
    wait
    echo "done cache = ${c}K"
end

