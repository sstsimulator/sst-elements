import sst
from mhlib import componentlist

debug = 0

# Define the simulation components
comp_cpu = sst.Component("core", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 0,
	"printStats" : 1,
})

iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")
gen = comp_cpu.setSubComponent("generator", "miranda.STREAMBenchGeneratorCustomCmd")
gen.addParams({
	"verbose" : 0,
        "n" : 10000,
        "write_cmd" : 10,
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
      "debug_level" : 10,
      "L1" : "1",
      "cache_size" : "2KB"
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memctrl = sst.Component("memory", "memHierarchy.CoherentMemController")
comp_memctrl.addParams({
      "clock" : "1GHz",
      "customCmdHandler" : "memHierarchy.defCustomCmdHandler",
      "addr_range_end" : 512*1024*1024-1,
      "debug" : debug,
      "debug_level" : 10,
})

comp_memory = comp_memctrl.setSubComponent("backend", "memHierarchy.goblinHMCSim")
comp_memory.addParams({
    "verbose" : "0",
    "trace_banks" : "1",
    "trace_queue" : "1",
    "trace_cmds" : "1",
    "trace_latency" : "1",
    "trace_stalls" : "1",
    "cmd_map" : "[CUSTOM:10:8:WR16]",
    "access_time" : "1000 ns",
    "mem_size" : "512MiB",
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "port", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_cpu_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps") )
