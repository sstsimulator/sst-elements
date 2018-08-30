# Automatically generated SST Python input
import sst

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
verbose = 2

# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
comp_cpu.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
    "access_latency_cycles" : "4",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "cache_size" : "2 KiB",
    "L1" : "1",
    "verbose" : verbose,
    "debug" : DEBUG_L1,
    "debug_level" : "10"
})
comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
    "access_latency_cycles" : "10",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "16 KiB",
    "verbose" : verbose,
    "debug" : DEBUG_L2,
    "debug_level" : "10"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
    "coherence_protocol" : "MSI",
    "backend.access_time" : "100 ns",
    "clock" : "1GHz",
    "backend.mem_size" : "512MiB",
    "verbose" : verbose,
    "debug" : DEBUG_MEM,
    "debug_level" : "10"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_cpu_l1cache_link = sst.Link("link_cpu_l1cache_link")
link_cpu_l1cache_link.connect( (comp_cpu, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_l1cache_l2cache_link = sst.Link("link_l1cache_l2cache_link")
link_l1cache_l2cache_link.connect( (comp_l1cache, "low_network_0", "10000ps"), (comp_l2cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l2cache, "low_network_0", "10000ps"), (comp_memory, "direct_link", "10000ps") )
# End of generated output.
