import sst
import os

sst.setProgramOption("timebase", "1ps")

sst_root = os.getenv( "SST_ROOT" )

l2PrefetchParams = {
        "prefetcher": "cassini.NextBlockPrefetcher"
        }

ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams({
        "verbose" : "0",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
        "executable" : sst_root + "/sst-elements/src/sst/elements/ariel/frontend/simple/examples/stream/stream",
        "arielmode" : "1",
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
        "cache_line_size" : "64",
        "L1" : "1",
        "debug" : "0",
	})

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
        "coherence_protocol" : "MSI",
        "backend.access_time" : "10ns",
        "backend.mem_size" : "2048MiB",
        "clock" : "1GHz",
        "use_dramsim" : "0",
        "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
        "system_ini" : "system.ini"
        })

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cache, "high_network_0", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (l1cache, "low_network_0", "50ps"), (memory, "direct_link", "50ps") )

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
      "instruction_count",
      "read_requests",
      "write_requests"
])

l1cache.enableStatistics([
      "CacheHits",
      "CacheMisses"
])

