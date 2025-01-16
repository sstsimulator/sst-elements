import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 987,
    "maxOutstanding" : 8,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 25, # 25% writes
    "read_freq" : 75,  # 75% reads
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
      "cache_size" : "4KiB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0,
      "debug_level" : 10,
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 989,
    "maxOutstanding" : 8,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 25, # 25% writes
    "read_freq" : 75,  # 75% reads
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
      "cache_size" : "4KiB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE1,
      "debug_level" : 10,
})
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l2cache = sst.Component("l2cache.msi.inclus", "memHierarchy.Cache")
l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : 10,
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
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
link_cpu0_l1cache_link.connect( (iface0, "lowlink", "1000ps"), (c0_l1cache, "highlink", "1000ps") )
link_c0_l1_l2_link = sst.Link("link_c0_l1_l2_link")
link_c0_l1_l2_link.connect( (c0_l1cache, "lowlink", "1000ps"), (bus, "highlink0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (iface1, "lowlink", "1000ps"), (c1_l1cache, "highlink", "1000ps") )
link_c1_l1_l2_link = sst.Link("link_c1_l1_l2_link")
link_c1_l1_l2_link.connect( (c1_l1cache, "lowlink", "1000ps"), (bus, "highlink1", "10000ps") )
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (bus, "lowlink0", "10000ps"), (l2cache, "highlink", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l2cache, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
