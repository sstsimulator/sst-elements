#!/bin/bash

# clean up from previous
rm dramsim*.log

# Create array of all tests
declare -a sdl_arr=(sdl-1.py
                    sdl-2.py
                    sdl-3.py
                    sdl2-1.py
                    sdl3-1.py
                    sdl3-2.py
                    sdl3-3.py
                    sdl4-1.py
                    sdl4-2.py
                    sdl5-1.py
                    sdl8-1.py
                    sdl8-3.py
                    sdl8-4.py
                    sdl9-1.py
                    sdl9-2.py
                    )

declare -a bk_arr=( testBackendChaining.py
                    testBackendDelayBuffer.py
                    testBackendPagedMulti.py
                    testBackendReorderRow.py
                    testBackendReorderSimple.py
                    testBackendSimpleDRAM-1.py
                    testBackendSimpleDRAM-2.py
                    testBackendTimingDRAM.py
                    testBackendVaultSim.py
                    )
declare -a ca_arr=(testDistributedCaches.py
                    testFlushes-2.py
                    testFlushes.py
                    testHashXor.py
                    testIncoherent.py
                    testNoninclusive-1.py
                    testNoninclusive-2.py
                    testPrefetchParams.py
                    testThroughputThrottling.py
                    )
declare -a scr_arr=(testScratchCache1.py
                    testScratchCache2.py
                    testScratchCache3.py
                    testScratchCache4.py
                    testScratchDirect.py
                    testScratchNetwork.py
                    )

arr=()
while getopts dscba option
do
    case "${option}"
    in
    d) arr+=( "${sdl_arr[@]}" );;
    s) arr+=( "${scr_arr[@]}" );;
    c) arr+=( "${ca_arr[@]}" );;
    b) arr+=( "${bk_arr[@]}" );;
    a) arr+=( "${sdl_arr[@]}" "${bk_arr[@]}" "${ca_arr[@]}" "${scr_arr[@]}" ) ;;
    esac
done

if [ -z "$arr" ]; then
    arr+=( "${sdl_arr[@]}" "${bk_arr[@]}" "${ca_arr[@]}" "${scr_arr[@]}" )
fi

for i in "${arr[@]}"
do
    echo "Running $i"
    if timeout 60 sst $i > log; then
        if grep -q "Simulation is complete, simulated time" log; then
            echo "  Complete"
        else
            echo "  FAILED"
            cp log fail_${i}.log
        fi
    else
        echo "  FAILED"
        cp log fail_${i}.log
    fi
done


