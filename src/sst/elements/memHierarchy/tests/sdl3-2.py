import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_L4 = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0

# Core 0 + L1
cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
cpu0.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.memInterface")

c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0,
      "cache_size" : "1 KB"
})
l1toC0 = c0_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1tol2_0 = c0_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# Core 1 + L1
cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
cpu1.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface1 = cpu1.setSubComponent("memory", "memHierarchy.memInterface")
c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE1,
      "cache_size" : "1 KB"
})
l1toC1 = c1_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1tol2_1 = c1_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# Bus between L1s and L2
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
      "bus_frequency" : "2 Ghz"
})

# L2
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "debug" : DEBUG_L2,
      "cache_size" : "2 KB"
})
l2tol1 = l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2tol3 = l2cache.setSubComponent("memlink", "memHierarchy.MemLink")

# L3
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
    "access_latency_cycles" : 12,
    "cache_frequency" : "8GHz",
    "replacement_policy" : "lfu",
    "coherence_protocol" : "MSI",
    "associativity" : 8,
    "cache_line_size" : 64,
    "cache_size" : "4KiB"
})
l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3tol4 = l3cache.setSubComponent("memlink", "memHierarchy.MemLink")

l4cache = sst.Component("l4cache", "memHierarchy.Cache")
l4cache.addParams({
    "access_latency_cycles" : 18,
    "cache_frequency" : "3GHz",
    "replacement_policy" : "lfu",
    "coherence_protocol" : "MSI",
    "associativity" : 16,
    "cache_line_size" : 64,
    "cache_size" : "8KiB"
})
l4tol3 = l4cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l4toM = l4cache.setSubComponent("memlink", "memHierarchy.MemLink")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})
mtol4 = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "512MiB",
      "access_time" : "80ns",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)



# Define the simulation links
# Core 0 to L1
link_cpu0_l1cache = sst.Link("link_cpu0_l1cache")
link_cpu0_l1cache.connect( (iface0, "port", "1000ps"), (l1toC0, "port", "1000ps") )
# Core 1 to L1
link_cpu1_l1cache = sst.Link("link_cpu1_l1cache")
link_cpu1_l1cache.connect( (iface1, "port", "1000ps"), (l1toC1, "port", "1000ps") )
# L1s to bus
link_c0_l1_l2 = sst.Link("link_c0_l1_l2")
link_c0_l1_l2.connect( (l1tol2_0, "port", "1000ps"), (bus, "high_network_0", "10000ps") )
link_c1_l1_l2 = sst.Link("link_c1_l1_l2_link")
link_c1_l1_l2.connect( (l1tol2_1, "port", "1000ps"), (bus, "high_network_1", "10000ps") )
# Bus to L2
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (bus, "low_network_0", "10000ps"), (l2tol1, "port", "1000ps") )
# L2 to L3
link_l2_l3 = sst.Link("link_l2_l3")
link_l2_l3.connect( (l2tol3, "port", "100ps"), (l3tol2, "port", "100ps") )
# L3 to L4
link_l3_l4 = sst.Link("link_l3_l4")
link_l3_l4.connect( (l3tol4, "port", "400ps"), (l4tol3, "port", "400ps") )
# L4 to mem
link_l4_mem = sst.Link("link_l4_mem")
link_l4_mem.connect( (l4toM, "port", "10000ps"), (mtol4, "port", "10000ps") )
