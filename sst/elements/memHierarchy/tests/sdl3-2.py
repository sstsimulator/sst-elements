# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "300000ns")

# Define the simulation components
cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
cpu0.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
c0_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """1 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """2""",
      "access_latency_cycles" : """3""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "debug_level" : """6""",
      "statistics" : """1"""
})
cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
cpu1.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
c1_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """1 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """2""",
      "access_latency_cycles" : """3""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "debug_level" : """6""",
      "statistics" : """1"""
})
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """2 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "debug_level" : """6""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "access_time" : """100 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz""",
      "backend" : """memHierarchy.dramsim""",
      "device_ini" : """DDR3_micron_32M_8B_x4_sg125.ini""",
      "system_ini" : """system.ini"""
})


# Define the simulation links
bus_l2cache = sst.Link("bus_l2cache")
bus_l2cache.connect( (bus, "low_network_0", "10000ps"), (l2cache, "high_network_0", "10000ps") )
c0_l1_l2_link = sst.Link("c0_l1_l2_link")
c0_l1_l2_link.connect( (c0_l1cache, "low_network_0", "1000ps"), (bus, "high_network_0", "1000ps") )
c1_l1_l2_link = sst.Link("c1_l1_l2_link")
c1_l1_l2_link.connect( (c1_l1cache, "low_network_0", "1000ps"), (bus, "high_network_1", "1000ps") )
cpu0_l1cache_link = sst.Link("cpu0_l1cache_link")
cpu0_l1cache_link.connect( (cpu0, "mem_link", "1000ps"), (c0_l1cache, "high_network_0", "1000ps") )
cpu1_l1cache_link = sst.Link("cpu1_l1cache_link")
cpu1_l1cache_link.connect( (cpu1, "mem_link", "1000ps"), (c1_l1cache, "high_network_0", "1000ps") )
mem_bus_link = sst.Link("mem_bus_link")
mem_bus_link.connect( (l2cache, "low_network_0", "10000ps"), (memory, "direct_link", "10000ps") )
# End of generated output.
