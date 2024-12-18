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
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "3KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 9067,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 58,  # 58% reads
    "llsc_freq" : 2,   # 2% LLSC
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

c0_l1cache = sst.Component("l1cache0.msi", "memHierarchy.Cache")
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

# Core 1 + L1
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "memFreq" : 1,
    "memSize" : "3KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 935,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 35, # 35% writes
    "read_freq" : 63,  # 63% reads
    "llsc_freq" : 2,   # 2% LLSC
})
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
c1_l1cache = sst.Component("l1cache.msi", "memHierarchy.Cache")
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

# Bus between L1s and L2
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
      "bus_frequency" : "2 Ghz"
})

# L2
l2cache = sst.Component("l2cache.msi.inclus", "memHierarchy.Cache")
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

# L3
l3cache = sst.Component("l3cache.msi.inclus", "memHierarchy.Cache")
l3cache.addParams({
    "access_latency_cycles" : 12,
    "cache_frequency" : "8GHz",
    "replacement_policy" : "lfu",
    "coherence_protocol" : "MSI",
    "associativity" : 8,
    "cache_line_size" : 64,
    "cache_size" : "4KiB"
})

l4cache = sst.Component("l4cache.msi.inclus", "memHierarchy.Cache")
l4cache.addParams({
    "access_latency_cycles" : 18,
    "cache_frequency" : "3GHz",
    "replacement_policy" : "lfu",
    "coherence_protocol" : "MSI",
    "associativity" : 16,
    "cache_line_size" : 64,
    "cache_size" : "8KiB"
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})

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
link_cpu0_l1cache.connect( (iface0, "lowlink", "1000ps"), (c0_l1cache, "highlink", "1000ps") )
# Core 1 to L1
link_cpu1_l1cache = sst.Link("link_cpu1_l1cache")
link_cpu1_l1cache.connect( (iface1, "lowlink", "1000ps"), (c1_l1cache, "highlink", "1000ps") )
# L1s to bus
link_c0_l1_l2 = sst.Link("link_c0_l1_l2")
link_c0_l1_l2.connect( (c0_l1cache, "lowlink", "1000ps"), (bus, "highlink0", "10000ps") )
link_c1_l1_l2 = sst.Link("link_c1_l1_l2_link")
link_c1_l1_l2.connect( (c1_l1cache, "lowlink", "1000ps"), (bus, "highlink1", "10000ps") )
# Bus to L2
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (bus, "lowlink0", "10000ps"), (l2cache, "highlink", "1000ps") )
# L2 to L3
link_l2_l3 = sst.Link("link_l2_l3")
link_l2_l3.connect( (l2cache, "lowlink", "100ps"), (l3cache, "highlink", "100ps") )
# L3 to L4
link_l3_l4 = sst.Link("link_l3_l4")
link_l3_l4.connect( (l3cache, "lowlink", "400ps"), (l4cache, "highlink", "400ps") )
# L4 to mem
link_l4_mem = sst.Link("link_l4_mem")
link_l4_mem.connect( (l4cache, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
