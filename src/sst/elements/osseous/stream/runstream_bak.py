import sst
import os
import time 

time.sleep(15)
sst.setProgramOption("timebase", "1ps")

sst_root = os.getenv( "SST_ROOT" )

ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams({
        "verbose" : "0",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
        "executable" : "/home/shubham/ECE633_Independent_Project/shubham/sst-tools/tools/ariel/femlm/examples/stream/mlmstream",
        "arielmode" : "1",
        "arieltool" : sst_root + "/sst-elements/src/sst/elements/ariel/fesimple.so",
        "memmgr.memorylevels" : "1",
        "memmgr.defaultlevel" : "0"
        })

corecount = 1;

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
        "cache_frequency" : "2 Ghz",
        "cache_size" : "64 KB",
        "coherence_protocol" : "MSI",
        "replacement_policy" : "lru",
        "associativity" : "8",
        "access_latency_cycles" : "1",
        "low_network_links" : "1",
        "cache_line_size" : "64",
        "L1" : "1",
        "debug" : "0",
        "statistics" : "1"
	})

vecshiftregister = sst.Component("vecshiftregister", "vecshiftreg.vecShiftReg")
vecshiftregister.addParams({
        "ExecFreq" : "1 GHz",
        "maxCycles" : "100"
	})

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
        "coherence_protocol" : "MSI",
        "access_time" : "10ns",
        "backend.mem_size" : "2048",
        "clock" : "1GHz",
        "use_dramsim" : "0",
        "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
        "system_ini" : "system.ini"
        })

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cache, "high_network_0", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (l1cache, "low_network_0", "50ps"), (memory, "direct_link", "50ps") )

cpu_rtl_link = sst.Link("cpu_rtl_link")
cpu_rtl_link.connect( (ariel, "rtl_link_0", "50ps"), (vecshiftregister, "Rtllink", "50ps") )
