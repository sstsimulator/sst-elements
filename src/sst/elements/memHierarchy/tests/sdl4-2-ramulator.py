import sst
from mhlib import componentlist

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 989,
    "maxOutstanding" : 64,
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
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 987,
    "maxOutstanding" : 64,
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
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
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
      "debug" : "0"
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.ramulator")
memory.addParams({
      "mem_size" : "512MiB",
      "configFile" : "ramulator-ddr3.cfg",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (iface0, "port", "1000ps"), (c0_l1cache, "high_network_0", "1000ps") )
link_c0_l1_l2_link = sst.Link("link_c0_l1_l2_link")
link_c0_l1_l2_link.connect( (c0_l1cache, "low_network_0", "1000ps"), (bus, "high_network_0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (iface1, "port", "1000ps"), (c1_l1cache, "high_network_0", "1000ps") )
link_c1_l1_l2_link = sst.Link("link_c1_l1_l2_link")
link_c1_l1_l2_link.connect( (c1_l1cache, "low_network_0", "1000ps"), (bus, "high_network_1", "10000ps") )
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (bus, "low_network_0", "10000ps"), (l2cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l2cache, "low_network_0", "10000ps"), (memctrl, "direct_link", "10000ps") )
