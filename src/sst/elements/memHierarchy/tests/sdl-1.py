# Automatically generated SST Python input
import sst

# Define the simulation components
verbose = 2

comp_cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
comp_cpu.addParams({
      "do_write" : "1",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000"
})
iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")

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
    "verbose" : verbose,
    "L1" : "1",
    "cache_size" : "2KiB"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
    "coherence_protocol" : "MSI",
    #"debug" : "1",
    "backend.access_time" : "1000 ns",
    "clock" : "1GHz",
    "backend.mem_size" : "512MiB",
    "verbose" : verbose,
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "port", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
# End of generated output.
