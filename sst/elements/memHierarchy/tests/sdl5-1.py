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
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
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
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
n0_l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
cpu2.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
c2_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
cpu3.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1"""
})
c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
c3_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
n1_l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """64 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """100""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "access_time" : """1000 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz""",
      "backend" : """memHierarchy.dramsim""",
      "device_ini" : """DDR3_micron_32M_8B_x4_sg125.ini""",
      "system_ini" : """system.ini"""
})


# Define the simulation links
bus_l3cache = sst.Link("bus_l3cache")
bus_l3cache.connect( (n2_bus, "low_network_0", "10000ps"), (l3cache, "high_network_0", "10000ps") )
c0_l1cache_l2cache_link = sst.Link("c0_l1cache_l2cache_link")
c0_l1cache_l2cache_link.connect( (c0_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_0", "10000ps") )
c1_l1cache_l2cache_link = sst.Link("c1_l1cache_l2cache_link")
c1_l1cache_l2cache_link.connect( (c1_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_1", "10000ps") )
c2_l1cache_l2cache_link = sst.Link("c2_l1cache_l2cache_link")
c2_l1cache_l2cache_link.connect( (c2_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_0", "10000ps") )
c3_l1cache_l2cache_link = sst.Link("c3_l1cache_l2cache_link")
c3_l1cache_l2cache_link.connect( (c3_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_1", "10000ps") )
cpu0_l1cache_link = sst.Link("cpu0_l1cache_link")
cpu0_l1cache_link.connect( (cpu0, "mem_link", "1000ps"), (c0_l1cache, "high_network_0", "1000ps") )
cpu1_l1cache_link = sst.Link("cpu1_l1cache_link")
cpu1_l1cache_link.connect( (cpu1, "mem_link", "1000ps"), (c1_l1cache, "high_network_0", "1000ps") )
cpu2_l1cache_link = sst.Link("cpu2_l1cache_link")
cpu2_l1cache_link.connect( (cpu2, "mem_link", "1000ps"), (c2_l1cache, "high_network_0", "1000ps") )
cpu3_l1cache_link = sst.Link("cpu3_l1cache_link")
cpu3_l1cache_link.connect( (cpu3, "mem_link", "1000ps"), (c3_l1cache, "high_network_0", "1000ps") )
mem_bus_link = sst.Link("mem_bus_link")
mem_bus_link.connect( (l3cache, "low_network_0", "10000ps"), (memory, "direct_link", "10000ps") )
n0_bus_l2cache = sst.Link("n0_bus_l2cache")
n0_bus_l2cache.connect( (n0_bus, "low_network_0", "10000ps"), (n0_l2cache, "high_network_0", "10000ps") )
n0_l2cache_l3cache = sst.Link("n0_l2cache_l3cache")
n0_l2cache_l3cache.connect( (n0_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_0", "10000ps") )
n1_bus_l2cache = sst.Link("n1_bus_l2cache")
n1_bus_l2cache.connect( (n1_bus, "low_network_0", "10000ps"), (n1_l2cache, "high_network_0", "10000ps") )
n1_l2cache_l3cache = sst.Link("n1_l2cache_l3cache")
n1_l2cache_l3cache.connect( (n1_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_1", "10000ps") )
# End of generated output.
