import sst
from mhlib import componentlist

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
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "L1" : "1",
      "debug" : "0",
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
})
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "L1" : "1",
      "debug" : "0",
      "cache_size" : "4 KB"
})
n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "debug" : "0",
      "cache_size" : "32 KB"
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 490,
    "maxOutstanding" : 16,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
})
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "L1" : "1",
      "debug" : "0",
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
})
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "L1" : "1",
      "debug" : "0",
      "cache_size" : "4 KB"
})
n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "debug" : "0",
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
      "debug_level" : "8",
      "debug" : "0",
      "cache_size" : "64 KB"
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
link_cpu0_l1cache_link.connect( (iface0, "lowlink", "1000ps"), (c0_l1cache, "highlink", "1000ps") )
link_c0_l1cache_l2cache_link = sst.Link("link_c0_l1cache_l2cache_link")
link_c0_l1cache_l2cache_link.connect( (c0_l1cache, "lowlink", "10000ps"), (n0_bus, "highlink0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (iface1, "lowlink", "1000ps"), (c1_l1cache, "highlink", "1000ps") )
link_c1_l1cache_l2cache_link = sst.Link("link_c1_l1cache_l2cache_link")
link_c1_l1cache_l2cache_link.connect( (c1_l1cache, "lowlink", "10000ps"), (n0_bus, "highlink1", "10000ps") )
link_n0_bus_l2cache = sst.Link("link_n0_bus_l2cache")
link_n0_bus_l2cache.connect( (n0_bus, "lowlink0", "10000ps"), (n0_l2cache, "highlink", "1000ps") )
link_n0_l2cache_l3cache = sst.Link("link_n0_l2cache_l3cache")
link_n0_l2cache_l3cache.connect( (n0_l2cache, "lowlink", "10000ps"), (n2_bus, "highlink0", "10000ps") )
link_cpu2_l1cache_link = sst.Link("link_cpu2_l1cache_link")
link_cpu2_l1cache_link.connect( (iface2, "lowlink", "1000ps"), (c2_l1cache, "highlink", "1000ps") )
link_c2_l1cache_l2cache_link = sst.Link("link_c2_l1cache_l2cache_link")
link_c2_l1cache_l2cache_link.connect( (c2_l1cache, "lowlink", "10000ps"), (n1_bus, "highlink0", "10000ps") )
link_cpu3_l1cache_link = sst.Link("link_cpu3_l1cache_link")
link_cpu3_l1cache_link.connect( (iface3, "lowlink", "1000ps"), (c3_l1cache, "highlink", "1000ps") )
link_c3_l1cache_l2cache_link = sst.Link("link_c3_l1cache_l2cache_link")
link_c3_l1cache_l2cache_link.connect( (c3_l1cache, "lowlink", "10000ps"), (n1_bus, "highlink1", "10000ps") )
link_n1_bus_l2cache = sst.Link("link_n1_bus_l2cache")
link_n1_bus_l2cache.connect( (n1_bus, "lowlink0", "10000ps"), (n1_l2cache, "highlink", "1000ps") )
link_n1_l2cache_l3cache = sst.Link("link_n1_l2cache_l3cache")
link_n1_l2cache_l3cache.connect( (n1_l2cache, "lowlink", "10000ps"), (n2_bus, "highlink1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "lowlink0", "10000ps"), (l3cache, "highlink", "10000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l3cache, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
