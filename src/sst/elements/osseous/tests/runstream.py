import sst
import os

clock = "1GHz"
sst.setProgramOption("timebase", "0.5ps")

sst_root = os.getenv( "SST_ROOT" )
#sst.setProgramOption("timebase", "1ps")
sst_workdir = os.getcwd();
app = sst_workdir + "/testbench"

if not os.path.exists(app):
    app = os.getenv( "OMP_EXE" )
    print("OS PATH DOESN'T EXIST")
ariel = sst.Component("A0", "ariel.ariel")
ariel.addParams({
        "verbose" : "1",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
        "executable" : app,
        "arielmode" : "1",
        "clock" : "1GHz",
        "arielinterceptcalls" : "1",
        "launchparamcount" : 1,
        "writepayloadtrace" : 1,
        "launchparam0" : "-ifeellucky",
        })

memmgr = ariel.setSubComponent("memmgr", "ariel.MemoryManagerSimple")

corecount = 1;

rtl = sst.Component("rtlaximodel", "rtlcomponent.Rtlmodel")
rtl.addParams({
        "ExecFreq" : "1GHz",
        "maxCycles" : "100"
	})

rtlmemmgr = rtl.setSubComponent("memmgr", "rtlaximodel.MemoryManagerSimple")

l1cpucache = sst.Component("l1cpucache", "memHierarchy.Cache")
l1cpucache.addParams({
        "cache_frequency" : "1GHz",
        "cache_size" : "64 KB",
        "cache_type" : "inclusive",
        "coherence_protocol" : "MSI",
        "replacement_policy" : "lru",
        "associativity" : "8",
        "access_latency_cycles" : "1",
        "cache_line_size" : "64",
        "L1" : "1",
        "debug" : "0",
})

l1rtlcache = sst.Component("l1rtlcache", "memHierarchy.Cache")
l1rtlcache.addParams({
        "cache_frequency" : "1GHz",
        "cache_size" : "64 KB",
        "coherence_protocol" : "MSI",
        "replacement_policy" : "lru",
        "associativity" : "8",
        "access_latency_cycles" : "1",
        "cache_line_size" : "64",
        "L1" : "1",
        "debug" : "0",
})

# Bus between private L1s and L2
membus = sst.Component("membus", "memHierarchy.Bus")
membus.addParams( { "bus_frequency" : clock,
                    "debug" : 2,
                    "debug_level" : 10
} )

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cpucache, "high_network_0", "50ps") )

rtl_cache_link = sst.Link("rtl_cache_link")
rtl_cache_link.connect( (rtl, "RtlCacheLink", "50ps"), (l1rtlcache, "high_network_0", "50ps") )

l1cpu_bus_link = sst.Link("l1cpu_bus_link")
l1cpu_bus_link.connect( (l1cpucache, "low_network_0", "50ps"), (membus, "high_network_0", "50ps") )

l1rtl_bus_link = sst.Link("l1rtl_bus_link")
l1rtl_bus_link.connect( (l1rtlcache, "low_network_0", "50ps"), (membus, "high_network_1", "50ps") )

# Shared L2
# 1MB*cores, 16-way set associative, 64B line, 15 cycle access
# MSI coherence with NMRU (not-most-recently-used) replacement
l2 = sst.Component("l2cache", "memHierarchy.Cache")
l2.addParams( {
    "cache_frequency" : clock,
    "access_latency_cycles" : 15,
    "cache_size" : str(corecount) + "MB",   # 1MB/core
    "associativity" : 16,
    "cache_line_size" : 64,
    "replacement_policy" : "nmru",
    "coherence_protocol" : "MSI"
} )

l2_bus_link = sst.Link("l2_bus_link")
l2_bus_link.connect( (l2, "high_network_0", "50ps"), (membus, "low_network_0", "50ps") )

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
        "clock" : "1GHz",
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
        "access_time" : "10ns",
        "mem_size" : "2048MiB",
})

cpu_rtl_link = sst.Link("cpu_rtl_link")
cpu_rtl_link.connect( (ariel, "rtl_link_0", "50ps"), (rtl, "ArielRtllink", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (l2, "low_network_0", "50ps"), (memctrl, "direct_link", "50ps") )

# Set the Statistic Load Level; Statistics with Enable Levels (set in
# elementInfoStatistic) lower or equal to the load can be enabled (default = 0)
sst.setStatisticLoadLevel(16)

# Set the desired Statistic Output (sst.statOutputConsole is default)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./TestOutput.txt"
#                                            })
#sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv",
#                                                         "separator" : ", "
#                                            })

# Enable Individual Statistics for the Component with output at end of sim
# Statistic defaults to Accumulator
ariel.enableStatistics([
      "cycles",
      "active_cycles",
      "instruction_count",
      "read_requests",
      "write_requests"
])

l1cpucache.enableStatistics([
      #"CacheHits",
      "latency_GetS_hit",
      "latency_GetX_hit",
      "latency_GetS_miss",
      "latency_GetX_miss",
      "GetSHit_Arrival",
      "GetSHit_Blocked",
      "CacheMisses"
])
