#!/bin/tcsh

#foreach c (16 32 64 128)
foreach c (${1})
    foreach s (32 64 128 256)
        set fileN = test-${c}K-${s}.out
        echo "running $fileN"
        rm -f $fileN
        time sst ./test.py -- -n 20000 -c $c -s $s -m 32 >& $fileN &
        sleep 1
    end
    wait
    echo "done cache = ${c}K"
end

