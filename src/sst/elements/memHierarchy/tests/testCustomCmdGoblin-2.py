import sst
from mhlib import componentlist

debug = 0

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the simulation components
comp_cpu = sst.Component("cpu", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 0,
	"printStats" : 1,
})
iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")
gen = comp_cpu.setSubComponent("generator", "miranda.STREAMBenchGeneratorCustomCmd")
gen.addParams({
	"verbose" : 0,
	"startat" : 3,
	"count" : 500000,
	"max_address" : 512000,
        "write_cmd" : 10,
        "read_cmd" : 20,
})

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

# Enable statistics outputs
comp_cpu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "prefetcher" : "cassini.StridePrefetcher",
      "debug" : debug,
      "L1" : "1",
      "cache_size" : "2KB"
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

#comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory = sst.Component("memory", "memHierarchy.CoherentMemController")
comp_memory.addParams({
      "clock" : "1GHz",
      "customCmdHandler" : "memHierarchy.amoCustomCmdHandler",
      "addr_range_end" : 512*1024*1024-1,
})
memorybackend = comp_memory.setSubComponent("backend", "memHierarchy.goblinHMCSim")
memorybackend.addParams({
      "access_time" : "1000 ns",
      "mem_size" : "512MiB",
      "verbose" : "0",
      "trace-banks" : "1",
      "trace-queue" : "1",
      "trace-cmds" : "1",
      "trace-latency" : "1",
      "trace-stalls" : "1",
      "cmd-map" : "[CUSTOM:10:64:WR64,CUSTOM:20:64:RD64]"
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "port", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_cpu_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
