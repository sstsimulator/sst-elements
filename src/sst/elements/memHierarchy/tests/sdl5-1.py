import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0
DEBUG_CORE2 = 0
DEBUG_CORE3 = 0
DEBUG_NODE0 = 0
DEBUG_NODE1 = 0

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 498,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% LLSC
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
c0_l1cache = sst.Component("l1cache0.msi", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0,
      "cache_size" : "4 KB"
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 498,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% LLSC
})
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
c1_l1cache = sst.Component("l1cache1.msi", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0,
      "cache_size" : "4 KB"
})
n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
n0_l2cache = sst.Component("l2cache0.msi.inclus", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "debug" : DEBUG_L2 | DEBUG_NODE0,
      "cache_size" : "32 KB"
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 498,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% LLSC
})
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
c2_l1cache = sst.Component("l1cache2.msi", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE2 | DEBUG_NODE1,
      "cache_size" : "4 KB"
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
cpu3.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 498,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% LLSC
})
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
c3_l1cache = sst.Component("l1cache3.msi", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE3 | DEBUG_NODE1,
      "cache_size" : "4 KB"
})
n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
n1_l2cache = sst.Component("l2cache1.msi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "debug" : DEBUG_L2 | DEBUG_NODE1,
      "cache_size" : "32 KB"
})
n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "debug" : DEBUG_L3,
      "cache_size" : "64 KB"
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : "10",
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
memory.addParams({
    "max_requests_per_cycle" : 1,
    "mem_size" : "512MiB",
    "tCAS" : 3,
    "tRCD" : 3,
    "tRP" : 3,
    "cycle_time" : "5ns",
    "row_size" : "8KiB",
    "row_policy" : "open"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (iface0, "port", "1000ps"), (c0_l1cache, "high_network_0", "1000ps") )
link_c0_l1cache_l2cache_link = sst.Link("link_c0_l1cache_l2cache_link")
link_c0_l1cache_l2cache_link.connect( (c0_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (iface1, "port", "1000ps"), (c1_l1cache, "high_network_0", "1000ps") )
link_c1_l1cache_l2cache_link = sst.Link("link_c1_l1cache_l2cache_link")
link_c1_l1cache_l2cache_link.connect( (c1_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_1", "10000ps") )
link_n0_bus_l2cache = sst.Link("link_n0_bus_l2cache")
link_n0_bus_l2cache.connect( (n0_bus, "low_network_0", "10000ps"), (n0_l2cache, "high_network_0", "1000ps") )
link_n0_l2cache_l3cache = sst.Link("link_n0_l2cache_l3cache")
link_n0_l2cache_l3cache.connect( (n0_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_0", "10000ps") )
link_cpu2_l1cache_link = sst.Link("link_cpu2_l1cache_link")
link_cpu2_l1cache_link.connect( (iface2, "port", "1000ps"), (c2_l1cache, "high_network_0", "1000ps") )
link_c2_l1cache_l2cache_link = sst.Link("link_c2_l1cache_l2cache_link")
link_c2_l1cache_l2cache_link.connect( (c2_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_0", "10000ps") )
link_cpu3_l1cache_link = sst.Link("link_cpu3_l1cache_link")
link_cpu3_l1cache_link.connect( (iface3, "port", "1000ps"), (c3_l1cache, "high_network_0", "1000ps") )
link_c3_l1cache_l2cache_link = sst.Link("link_c3_l1cache_l2cache_link")
link_c3_l1cache_l2cache_link.connect( (c3_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_1", "10000ps") )
link_n1_bus_l2cache = sst.Link("link_n1_bus_l2cache")
link_n1_bus_l2cache.connect( (n1_bus, "low_network_0", "10000ps"), (n1_l2cache, "high_network_0", "1000ps") )
link_n1_l2cache_l3cache = sst.Link("link_n1_l2cache_l3cache")
link_n1_l2cache_l3cache.connect( (n1_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "low_network_0", "10000ps"), (l3cache, "high_network_0", "10000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l3cache, "low_network_0", "10000ps"), (memctrl, "direct_link", "10000ps") )
