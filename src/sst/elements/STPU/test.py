import sst

# Define the simulation components
comp_stpu = sst.Component("STPU", "STPU.STPU")
comp_stpu.addParams({
    "verbose" : 1,
    "clock" : "1GHz",
    "BWPperTic" : 1,
    "STSDispatch" : 1,
    "STSParallelism" : 1
})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
    "access_latency_cycles" : "4",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    #"debug" : "1",
    #"debug_level" : "10",
    "verbose" : 1,
    "L1" : "1",
    "cache_size" : "2KiB"
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : 1,
      "coherence_protocol" : "MSI",
      "debug_level" : 10,
      "backend.access_time" : "100 ns",
      "backing" : "malloc", 
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_stpu_cache = sst.Link("link_stpu_mem")
link_stpu_cache.connect( (comp_stpu, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )

