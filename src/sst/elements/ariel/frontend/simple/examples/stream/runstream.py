import sst
import os

sst.setProgramOption("timebase", "1ps")

stream_app = os.getenv("ARIEL_TEST_STREAM_APP")
if stream_app == None:
    sst_root = os.getenv( "SST_ROOT" )
    app = sst_root + "/sst-elements/src/sst/elements/ariel/frontend/simple/examples/stream/stream"
else:
    app = stream_app

if not os.path.exists(app):
    app = os.getenv( "OMP_EXE" )

ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams({
        "verbose" : "0",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
        "executable" : app,
        "arielmode" : "1",
        "launchparamcount" : 1,
        "launchparam0" : "-ifeellucky",
        })

memmgr = ariel.setSubComponent("memmgr", "ariel.MemoryManagerSimple")


corecount = 1;

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
        "cache_frequency" : "2 Ghz",
        "cache_size" : "64 KB",
        "coherence_protocol" : "MSI",
        "replacement_policy" : "lru",
        "associativity" : "8",
        "access_latency_cycles" : "1",
        "cache_line_size" : "64",
        "L1" : "1",
        "debug" : "0",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
        "clock" : "1GHz",
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
        "access_time" : "10ns",
        "mem_size" : "2048MiB",
})

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cache, "highlink", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (l1cache, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )


# Set the Statistic Load Level; Statistics with Enable Levels (set in
# elementInfoStatistic) lower or equal to the load can be enabled (default = 0)
sst.setStatisticLoadLevel(5)

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

l1cache.enableStatistics([
      "CacheHits",
      "CacheMisses"
])

