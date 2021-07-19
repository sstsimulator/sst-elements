import sst

DEBUG_L1 = 0

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.streamCPU")
comp_cpu.addParams({
      "do_write" : "1",
      "num_loadstore" : "100000",
      "commFreq" : "100",
      "memSize" : "524288"
})

iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "prefetcher" : "cassini.PalaPrefetcher",
      "debug" : DEBUG_L1,
      "L1" : "1",
      "cache_size" : "8 KB"
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({ "clock" : "1GHz" })
backend = comp_memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({
      "access_time" : "1000ns",
      "mem_size" : "512MiB",
})

# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "port", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
