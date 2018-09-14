"""
Sample script
Simulating System
Every core with Private L1 and L2 caches. 
All L2 cache are connected to shared L3 cache via bus. 
L3 connects to MainMemory through memory Controller.

core_0   core_1   core_2   core_3   core_4   core_5   core_6   core_7
L1_0     L1_1     L1_2     L1_3     L1_4     L1_5     L1_6     L1_7
L2_0     L2_1     L2_2     L2_3     L2_4     L2_5     L2_6     L2_7
                                BUS
                             Shared L3
                           MemoryController
                         Main Memory (DRAMSIM)
"""

import sst 
import os

## Flags
memDebug = 0
memDebugLevel = 10
coherenceProtocol = "MESI"
rplPolicy = "lru"
busLat = "50 ps"
cacheFrequency = "2 Ghz"
defaultLevel = 0
cacheLineSize = 64

corecount = 8

## Application Info
os.environ['SIM_DESC'] = 'EIGHT_CORES'
os.environ['OMP_NUM_THREADS'] = str(corecount)

sst_root = os.getenv( "SST_ROOT" )

## Application Info:
## Executable  -> exe_file
## appargcount -> Number of commandline arguments after <exec_file> name
## apparg<#>   -> arguments 
## Commandline execution for the below example would be 
## /home/amdeshp/arch/benchmarks/PathFinder_1.0.0/PathFinder_ref/PathFinder.x -x /home/amdeshp/arch/benchmarks/PathFinder_1.0.0/generatedData/small1.adj_list
## AppArgs = ({
##    "executable"  : "/home/amdeshp/arch/benchmarks/PathFinder_1.0.0/PathFinder_ref/PathFinder.x",
##    "appargcount" : "0",
##    "apparg0"     : "-x",
##    "apparg1"     : "/home/amdeshp/arch/benchmarks/PathFinder_1.0.0/generatedData/small1.adj_list",
## })

## Processor Model
ariel = sst.Component("A0", "ariel.ariel")
## ariel.addParams(AppArgs)
ariel.addParams({
   "verbose"             : "0",
   "maxcorequeue"        : "256",
   "maxissuepercycle"    : "2",
   "pipetimeout"         : "0",
   "executable" : sst_root + "/sst-elements/src/sst/elements/ariel/frontend/simple/examples/stream/stream",
   "memorylevels"        : "1",
   "arielinterceptcalls" : "1",
   "arielmode"           : "1",
   "corecount"           : corecount,
   "defaultlevel"        : defaultLevel,
})
 
## MemHierarchy 

def genMemHierarchy(cores):
   
   membus = sst.Component("membus", "memHierarchy.Bus")
   membus.addParams({
       "bus_frequency" : cacheFrequency,
   })

   memory = sst.Component("memory", "memHierarchy.MemController")
   memory.addParams({
        "range_start"           : "0",
        "coherence_protocol"    : coherenceProtocol,
        "debug"                 : memDebug,
        "clock"                 : "1Ghz",
        "verbose"               : 2,
        "backend.device_ini"    : "DDR3_micron_32M_8B_x4_sg125.ini",
        "backend.system_ini"    : "system.ini",
        "backend.mem_size"      : "512MiB",
        "request_width"         : cacheLineSize,
        "backend"               : "memHierarchy.dramsim",
        "device_ini"            : "DDR3_micron_32M_8B_x4_sg125.ini",
        "system_ini"            : "system.ini"
   })

   for core in range (cores):
       l1 = sst.Component("l1cache_%d"%core, "memHierarchy.Cache")
       l1.addParams({
            "cache_frequency"       : cacheFrequency,
            "cache_size"            : "32KB",
            "cache_line_size"       : cacheLineSize,
            "associativity"         : "8",
            "access_latency_cycles" : "4",
            "coherence_protocol"    : coherenceProtocol,
            "replacement_policy"    : rplPolicy,
            "L1"                    : "1",
            "debug"                 : memDebug,  
            "debug_level"           : memDebugLevel, 
            "verbose"               : 2,
       })

       l2 = sst.Component("l2cache_%d"%core, "memHierarchy.Cache")
       l2.addParams({
            "cache_frequency"       : cacheFrequency,
            "cache_size"            : "256 KB",
            "cache_line_size"       : cacheLineSize,
            "associativity"         : "8",
            "access_latency_cycles" : "10",
            "coherence_protocol"    : coherenceProtocol,
            "replacement_policy"    : rplPolicy,
            "L1"                    : "0",
            "debug"                 : memDebug,  
            "debug_level"           : memDebugLevel, 
            "verbose"               : 2,
            "mshr_num_entries"      : "16",
       })
       
       ## SST Links
       # Ariel -> L1(PRIVATE) -> L2(PRIVATE)  -> L3 (SHARED) -> DRAM 
       ArielL1Link = sst.Link("cpu_cache_%d"%core)
       ArielL1Link.connect((ariel, "cache_link_%d"%core, busLat), (l1, "high_network_0", busLat))
       L1L2Link = sst.Link("l1_l2_%d"%core)
       L1L2Link.connect((l1, "low_network_0", busLat), (l2, "high_network_0", busLat))
       L2MembusLink = sst.Link("l2_membus_%d"%core)
       L2MembusLink.connect((l2, "low_network_0", busLat), (membus, "high_network_%d"%core, busLat))
   

   l3 = sst.Component("L3cache", "memHierarchy.Cache")
   l3.addParams({
        "cache_frequency"       : cacheFrequency,
        "cache_size"            : "8 MB",
        "cache_line_size"       : cacheLineSize,
        "associativity"         : "8",
        "access_latency_cycles" : "20",
        "coherence_protocol"    : coherenceProtocol,
        "replacement_policy"    : rplPolicy,
        "L1"                    : "0",
        "debug"                 : memDebug,  
        "debug_level"           : memDebugLevel, 
        "mshr_num_entries"      : "16",
        "verbose"               : 2,
   })

   # Bus to L3 and L3 <-> MM
   BusL3Link = sst.Link("bus_L3")
   BusL3Link.connect((membus, "low_network_0", busLat), (l3, "high_network_0", busLat))
   L3MemCtrlLink = sst.Link("L3MemCtrl")
   L3MemCtrlLink.connect((l3, "low_network_0", busLat), (memory, "direct_link", busLat))

genMemHierarchy(corecount)        

