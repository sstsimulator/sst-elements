# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "5000ns")

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "2 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "2 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_bus = sst.Component("bus", "memHierarchy.Bus")
comp_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "8 KB",
      "LL" : 1,
      "debug" : "0"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "backend.mem_size" : "512",
      "printStats" : "1",
      "clock" : "1GHz",
      "backend.access_time" : "100 ns"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0_l1_l2_link = sst.Link("link_c0_l1_l2_link")
link_c0_l1_l2_link.connect( (comp_c0_l1cache, "low_network_0", "1000ps"), (comp_bus, "high_network_0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1_l1_l2_link = sst.Link("link_c1_l1_l2_link")
link_c1_l1_l2_link.connect( (comp_c1_l1cache, "low_network_0", "1000ps"), (comp_bus, "high_network_1", "10000ps") )
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (comp_bus, "low_network_0", "10000ps"), (comp_l2cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l2cache, "low_network_0", "10000ps"), (comp_memory, "direct_link", "10000ps") )
# End of generated output.
