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
                    sdl8-1.py
                    sdl8-3.py
                    sdl8-4.py
                    sdl9-1.py
                    sdl9-2.py
                    )
declare -a sdl_ref_arr=(refFiles/test_memHierarchy_sdl_1.out
                    refFiles/test_memHierarchy_sdl_2.out
                    refFiles/test_memHierarchy_sdl_3.out
                    refFiles/test_memHierarchy_sdl2_1.out
                    refFiles/test_memHierarchy_sdl3_1.out
                    refFiles/test_memHierarchy_sdl3_2.out
                    refFiles/test_memHierarchy_sdl3_3.out
                    refFiles/test_memHierarchy_sdl4_1.out
                    refFiles/test_memHierarchy_sdl8_1.out
                    refFiles/test_memHierarchy_sdl8_3.out
                    refFiles/test_memHierarchy_sdl8_4.out
                    refFiles/test_memHierarchy_sdl9_1.out
                    refFiles/test_memHierarchy_sdl9_2.out
                    )
declare -a bk_arr=( testBackendChaining.py
                    testBackendDelayBuffer.py
                    testBackendReorderRow.py
                    testBackendReorderSimple.py
                    testBackendSimpleDRAM-1.py
                    testBackendSimpleDRAM-2.py
                    testBackendTimingDRAM-1.py
                    testBackendVaultSim.py
                    )
declare -a bk_ref_arr=(refFiles/test_memHA_BackendChaining.out
                    refFiles/test_memHA_BackendDelayBuffer.out
                    refFiles/test_memHA_BackendReorderRow.out
                    refFiles/test_memHA_BackendReorderSimple.out
                    refFiles/test_memHA_BackendSimpleDRAM_1.out
                    refFiles/test_memHA_BackendSimpleDRAM_2.out
                    refFiles/test_memHA_BackendTimingDRAM_1.out
                    refFiles/test_memHA_BackendVaultSim.out
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
declare -a ca_ref_arr=(refFiles/test_memHA_DistributedCaches.out
                    refFiles/test_memHA_Flushes_2.out
                    refFiles/test_memHA_Flushes.out
                    refFiles/test_memHA_HashXor.out
                    refFiles/test_memHA_Incoherent.out
                    refFiles/test_memHA_Noninclusive_1.out
                    refFiles/test_memHA_Noninclusive_2.out
                    refFiles/test_memHA_PrefetchParams.out
                    refFiles/test_memHA_ThroughputThrottling.out
                    )
#declare -a scr_arr=(testScratchCache1.py
#                    testScratchCache2.py
#                    testScratchCache3.py
#                    testScratchCache4.py
#                    testScratchDirect.py
#                    testScratchNetwork.py
#                    )
#declare -a scr_ref_arr=(refFiles/testScratchCache1.out
#                    refFiles/test_memHA_ScratchCache2.out
#                    refFiles/test_memHA_ScratchCache3.out
#                    refFiles/testScratchCache4.out
#                    refFiles/testScratchDirect.out
#                    refFiles/testScratchNetwork.out
#                    )

arr=()
refarr=()
docheck=0
while getopts dscba option
do
    case "${option}"
    in
    d) 
        arr+=( "${sdl_arr[@]}" )
        refarr+=( "${sdl_ref_arr[@]}" )
        ;;
#    s) 
#        arr+=( "${scr_arr[@]}" )
#        refarr+=( "${scr_ref_arr[@]}" )
#        ;;
    c) 
        arr+=( "${ca_arr[@]}" );
        refarr+=( "${ca_ref_arr[@]}" )
        ;;
    b) 
        arr+=( "${bk_arr[@]}" )
        refarr+=( "${bk_ref_arr[@]}" )
        ;;
    a) 
        arr+=( "${sdl_arr[@]}" "${bk_arr[@]}" "${ca_arr[@]}" )
        refarr+=( "${sdl_ref_arr[@]}" "${bk_ref_arr[@]}" "${ca_ref_arr[@]}" )
#        arr+=( "${sdl_arr[@]}" "${bk_arr[@]}" "${ca_arr[@]}" "${scr_arr[@]}" )
#        refarr+=( "${sdl_ref_arr[@]}" "${bk_ref_arr[@]}" "${ca_ref_arr[@]}" "${scr_ref_arr[@]}" )
        ;;
    h) docheck=1
    esac
done

if [ -z "$arr" ]; then
    arr+=( "${sdl_arr[@]}" "${bk_arr[@]}" "${ca_arr[@]}" )
    refarr+=( "${sdl_ref_arr[@]}" "${bk_ref_arr[@]}" "${ca_ref_arr[@]}" )
#    arr+=( "${sdl_arr[@]}" "${bk_arr[@]}" "${ca_arr[@]}" "${scr_arr[@]}" )
#    refarr+=( "${sdl_ref_arr[@]}" "${bk_ref_arr[@]}" "${ca_ref_arr[@]}" "${scr_ref_arr[@]}" )
fi

for i in "${!arr[@]}"
do
    echo "Running ${arr[$i]}"
    if timeout 60 sst ${arr[$i]} > log; then
        if grep -q "Simulation is complete, simulated time" log; then
            if diff log ${refarr[$i]}; then
                echo "  Complete"
            else
                cp log diffed_${arr[$i]}.log
                echo "  Complete but diffed"
            fi
        else
            echo "  FAILED"
            cp log fail_${arr[$i]}.log
        fi
    else
        echo "  FAILED"
        cp log fail_${arr[$i]}.log
    fi
done


