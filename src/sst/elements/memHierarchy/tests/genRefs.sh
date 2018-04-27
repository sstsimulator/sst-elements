#!/bin/bash

echo "Regenerating reference files..."
# SDL tests
echo "SDL..."
sst sdl-1.py > refFiles/test_memHierarchy_sdl_1.out &
sst sdl-2.py > refFiles/test_memHierarchy_sdl_2.out &           
sst sdl-3.py > refFiles/test_memHierarchy_sdl_3.out &
sst sdl2-1.py > refFiles/test_memHierarchy_sdl2_1.out &
sst sdl3-1.py > refFiles/test_memHierarchy_sdl3_1.out &          
sst sdl3-2.py > refFiles/test_memHierarchy_sdl3_2.out &           
sst sdl3-3.py > refFiles/test_memHierarchy_sdl3_3.out &
sst sdl4-1.py > refFiles/test_memHierarchy_sdl4_1.out &
sst sdl4-2.py > refFiles/test_memHierarchy_sdl4_2.out &
sst sdl5-1.py > refFiles/test_memHierarchy_sdl5_1.out &
sst sdl8-1.py > refFiles/test_memHierarchy_sdl8_1.out &
sst sdl8-3.py > refFiles/test_memHierarchy_sdl8_3.out &
sst sdl8-4.py > refFiles/test_memHierarchy_sdl8_4.out &
sst sdl9-1.py > refFiles/test_memHierarchy_sdl9_1.out &
sst sdl9-2.py > refFiles/test_memHierarchy_sdl9_2.out &
wait

# Some of the DRAMSim files have output that I don't see except on Jenkins
#sdl-2 
echo "== Loading device model file 'DDR3_micron_32M_8B_x4_sg125.ini' == " >> refFiles/test_memHierarchy_sdl_2.out
echo "== Loading system model file 'system.ini' == " >> refFiles/test_memHierarchy_sdl_2.out
echo "WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE" >> refFiles/test_memHierarchy_sdl_2.out
echo "===== MemorySystem 0 =====" >> refFiles/test_memHierarchy_sdl_2.out
echo "CH. 0 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl_2.out
echo "===== MemorySystem 1 =====" >> refFiles/test_memHierarchy_sdl_2.out
echo "CH. 1 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl_2.out
echo "DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz" >> refFiles/test_memHierarchy_sdl_2.out
#sdl-3
echo "== Loading device model file 'DDR3_micron_32M_8B_x4_sg125.ini' == " >> refFiles/test_memHierarchy_sdl_3.out
echo "== Loading system model file 'system.ini' == " >> refFiles/test_memHierarchy_sdl_3.out
echo "WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE" >> refFiles/test_memHierarchy_sdl_3.out
echo "===== MemorySystem 0 =====" >> refFiles/test_memHierarchy_sdl_3.out
echo "CH. 0 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl_3.out
echo "===== MemorySystem 1 =====" >> refFiles/test_memHierarchy_sdl_3.out
echo "CH. 1 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl_3.out
echo "DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz" >> refFiles/test_memHierarchy_sdl_3.out
#sdl3-2
echo "== Loading device model file 'DDR3_micron_32M_8B_x4_sg125.ini' == " >> refFiles/test_memHierarchy_sdl3_2.out
echo "== Loading system model file 'system.ini' == " >> refFiles/test_memHierarchy_sdl3_2.out
echo "WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE" >> refFiles/test_memHierarchy_sdl3_2.out
echo "===== MemorySystem 0 =====" >> refFiles/test_memHierarchy_sdl3_2.out
echo "CH. 0 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl3_2.out
echo "===== MemorySystem 1 =====" >> refFiles/test_memHierarchy_sdl3_2.out
echo "CH. 1 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl3_2.out
echo "DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz" >> refFiles/test_memHierarchy_sdl3_2.out
#sdl4-2
echo "== Loading device model file 'DDR3_micron_32M_8B_x4_sg125.ini' == " >> refFiles/test_memHierarchy_sdl4_2.out
echo "== Loading system model file 'system.ini' == " >> refFiles/test_memHierarchy_sdl4_2.out
echo "WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE" >> refFiles/test_memHierarchy_sdl4_2.out
echo "===== MemorySystem 0 =====" >> refFiles/test_memHierarchy_sdl4_2.out
echo "CH. 0 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl4_2.out
echo "===== MemorySystem 1 =====" >> refFiles/test_memHierarchy_sdl4_2.out
echo "CH. 1 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl4_2.out
echo "DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz" >> refFiles/test_memHierarchy_sdl4_2.out
# sdl5-1
echo "== Loading device model file 'DDR3_micron_32M_8B_x4_sg125.ini' == " >> refFiles/test_memHierarchy_sdl5_1.out
echo "== Loading system model file 'system.ini' == " >> refFiles/test_memHierarchy_sdl5_1.out
echo "WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE" >> refFiles/test_memHierarchy_sdl5_1.out
echo "===== MemorySystem 0 =====" >> refFiles/test_memHierarchy_sdl5_1.out
echo "CH. 0 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl5_1.out
echo "===== MemorySystem 1 =====" >> refFiles/test_memHierarchy_sdl5_1.out
echo "CH. 1 TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank" >> refFiles/test_memHierarchy_sdl5_1.out
echo "DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz" >> refFiles/test_memHierarchy_sdl5_1.out

# SDL multithread
echo "MC..."
sst -n2 sdl9-1.py > refFiles/test_memHierarchy_sdl9_1_MC.out &
wait

# Backend tests
echo "Backend..."
sst testBackendChaining.py > refFiles/test_memHA_BackendChaining.out &
sst testBackendDelayBuffer.py > refFiles/test_memHA_BackendDelayBuffer.out &     
sst testBackendGoblinHMC.py > refFiles/test_memHA_BackendGoblinHMC.out & 
sst testBackendPagedMulti.py > refFiles/test_memHA_BackendPagedMulti.out &     
sst testBackendReorderRow.py > refFiles/test_memHA_BackendReorderRow.out &    
sst testBackendReorderSimple.py > refFiles/test_memHA_BackendReorderSimple.out &    
sst testBackendSimpleDRAM-1.py > refFiles/test_memHA_BackendSimpleDRAM_1.out &  
sst testBackendSimpleDRAM-2.py > refFiles/test_memHA_BackendSimpleDRAM_2.out &     
sst testBackendTimingDRAM-1.py > refFiles/test_memHA_BackendTimingDRAM_1.out &    
sst testBackendTimingDRAM-2.py > refFiles/test_memHA_BackendTimingDRAM_2.out &    
sst testBackendTimingDRAM-3.py > refFiles/test_memHA_BackendTimingDRAM_3.out &    
sst testBackendTimingDRAM-4.py > refFiles/test_memHA_BackendTimingDRAM_4.out &    
sst testBackendVaultSim.py > refFiles/test_memHA_BackendVaultSim.out &
wait

# Backend multithread
echo "MC..."
sst -n2 testBackendChaining.py > refFiles/test_memHA_BackendChaining_MC.out &
sst -n2 testBackendDelayBuffer.py > refFiles/test_memHA_BackendDelayBuffer_MC.out &
sst -n2 testBackendGoblinHMC.py > refFiles/test_memHA_BackendGoblinHMC_MC.out &
sst -n2 testBackendPagedMulti.py > refFiles/test_memHA_BackendPagedMulti_MC.out &
sst -n2 testBackendReorderRow.py > refFiles/test_memHA_BackendReorderRow_MC.out & 
sst -n2 testBackendReorderSimple.py > refFiles/test_memHA_BackendReorderSimple_MC.out &
sst -n2 testBackendSimpleDRAM-1.py > refFiles/test_memHA_BackendSimpleDRAM_1_MC.out &
sst -n2 testBackendSimpleDRAM-2.py > refFiles/test_memHA_BackendSimpleDRAM_2_MC.out & 
sst -n2 testBackendVaultSim.py > refFiles/test_memHA_BackendVaultSim_MC.out &
wait

# Misc tests
echo "Misc..."
sst testCustomCmdGoblin-1.py > refFiles/test_memHA_CustomCmdGoblin_1.out &   
sst testCustomCmdGoblin-2.py > refFiles/test_memHA_CustomCmdGoblin_2.out &   
sst testCustomCmdGoblin-3.py > refFiles/test_memHA_CustomCmdGoblin_3.out &   
sst testDistributedCaches.py > refFiles/test_memHA_DistributedCaches.out &
sst testFlushes.py > refFiles/test_memHA_Flushes.out &      
sst testFlushes-2.py > refFiles/test_memHA_Flushes_2.out &
sst testHashXor.py > refFiles/test_memHA_HashXor.out &    
sst testIncoherent.py > refFiles/test_memHA_Incoherent.out &
sst testNoninclusive-1.py > refFiles/test_memHA_Noninclusive_1.out &   
sst testNoninclusive-2.py > refFiles/test_memHA_Noninclusive_2.out &   
sst testPrefetchParams.py > refFiles/test_memHA_PrefetchParams.out &
sst testThroughputThrottling.py > refFiles/test_memHA_ThroughputThrottling.out &  
wait

# Misc multithread
echo "MC..."
sst -n2 testCustomCmdGoblin-1.py > refFiles/test_memHA_CustomCmdGoblin_1_MC.out &   
sst -n2 testCustomCmdGoblin-2.py > refFiles/test_memHA_CustomCmdGoblin_2_MC.out &
sst -n2 testCustomCmdGoblin-3.py > refFiles/test_memHA_CustomCmdGoblin_3_MC.out &   
sst -n2 testDistributedCaches.py > refFiles/test_memHA_DistributedCaches_MC.out &
sst -n2 testFlushes.py > refFiles/test_memHA_Flushes_MC.out &   
sst -n2 testFlushes-2.py > refFiles/test_memHA_Flushes_2_MC.out &         
sst -n2 testHashXor.py > refFiles/test_memHA_HashXor_MC.out & 
sst -n2 testIncoherent.py > refFiles/test_memHA_Incoherent_MC.out &
sst -n2 testNoninclusive-1.py > refFiles/test_memHA_Noninclusive_1_MC.out &
sst -n2 testNoninclusive-2.py > refFiles/test_memHA_Noninclusive_2_MC.out &
sst -n2 testPrefetchParams.py > refFiles/test_memHA_PrefetchParams_MC.out &
sst -n2 testThroughputThrottling.py > refFiles/test_memHA_ThroughputThrottling_MC.out &
wait

echo "Done!"

