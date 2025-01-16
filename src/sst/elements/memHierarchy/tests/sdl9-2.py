import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0
DEBUG_CORE2 = 0
DEBUG_CORE3 = 0
DEBUG_CORE4 = 0
DEBUG_CORE5 = 0
DEBUG_CORE6 = 0
DEBUG_CORE7 = 0
DEBUG_NODE0 = 0
DEBUG_NODE1 = 0

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 6,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
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
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 8,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
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
      "debug" : DEBUG_L1 | DEBUG_CORE1 | DEBUG_NODE0
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 10,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
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
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE2 | DEBUG_NODE0
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
cpu3.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 12,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
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
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE3 | DEBUG_NODE0
})
comp_n0_bus = sst.Component("bus0", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n0_l2cache = sst.Component("l2cache0.msi.inclus", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "mshr_latency_cycles" : 5,
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2 | DEBUG_NODE0
})
cpu4 = sst.Component("core4", "memHierarchy.standardCPU")
cpu4.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 14,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
})
iface4 = cpu4.setSubComponent("memory", "memHierarchy.standardInterface")
c4_l1cache = sst.Component("l1cache4.msi", "memHierarchy.Cache")
c4_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE4 | DEBUG_NODE1
})
cpu5 = sst.Component("core5", "memHierarchy.standardCPU")
cpu5.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 16,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
})
iface5 = cpu5.setSubComponent("memory", "memHierarchy.standardInterface")
c5_l1cache = sst.Component("l1cache5.msi", "memHierarchy.Cache")
c5_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE5 | DEBUG_NODE1
})
cpu6 = sst.Component("core6", "memHierarchy.standardCPU")
cpu6.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 18,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
})
iface6 = cpu6.setSubComponent("memory", "memHierarchy.standardInterface")
c6_l1cache = sst.Component("l1cache6.msi", "memHierarchy.Cache")
c6_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE6 | DEBUG_NODE1
})
cpu7 = sst.Component("core7", "memHierarchy.standardCPU")
cpu7.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 20,
    "maxOutstanding" : 32,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
    "noncacheableRangeStart" : "0x0",
    "noncacheableRangeEnd" : "0x100",
})
iface7 = cpu7.setSubComponent("memory", "memHierarchy.standardInterface")
c7_l1cache = sst.Component("l1cache7.msi", "memHierarchy.Cache")
c7_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE6 | DEBUG_NODE1
})
comp_n1_bus = sst.Component("bus1", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n1_l2cache = sst.Component("l2cache1.msi.inclus", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "mshr_latency_cycles" : 5,
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2 | DEBUG_NODE1
})
comp_n2_bus = sst.Component("bus2", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l3cache = sst.Component("l3cache.msi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "mshr_latency_cycles" : 20,
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : DEBUG_L3,
      "debug_level" : "3",
})
l3NIC = l3cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
l3NIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 1,
})
chiprtr = sst.Component("network", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "id" : "0",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "link_bw" : "1GB/s",
      "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("directory.msi", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "coherence_protocol" : "MSI",
    "debug" : DEBUG_DIR,
    "debug_level" : 10,
    "entry_cache_size" : "8192",
    "addr_range_start" : "0x0",
    "addr_range_end" : "0x1F000000",
})
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "group" : 2,
    "network_bw" : "25GB/s",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_c0l1cache_link = sst.Link("link_c0l1cache_link")
link_c0l1cache_link.connect( (iface0, "lowlink", "1000ps"), (c0_l1cache, "highlink", "1000ps") )
link_c0l1cache_bus = sst.Link("link_c0l1cache_bus")
link_c0l1cache_bus.connect( (c0_l1cache, "lowlink", "10000ps"), (comp_n0_bus, "highlink0", "10000ps") )
link_c1l1cache_link = sst.Link("link_c1l1cache_link")
link_c1l1cache_link.connect( (iface1, "lowlink", "1000ps"), (c1_l1cache, "highlink", "1000ps") )
link_c1l1cache_bus = sst.Link("link_c1l1cache_bus")
link_c1l1cache_bus.connect( (c1_l1cache, "lowlink", "10000ps"), (comp_n0_bus, "highlink1", "10000ps") )
link_c2l1cache_link = sst.Link("link_c2l1cache_link")
link_c2l1cache_link.connect( (iface2, "lowlink", "1000ps"), (c2_l1cache, "highlink", "1000ps") )
link_c2l1cache_bus = sst.Link("link_c2l1cache_bus")
link_c2l1cache_bus.connect( (c2_l1cache, "lowlink", "10000ps"), (comp_n0_bus, "highlink2", "10000ps") )
link_c3l1cache_link = sst.Link("link_c3l1cache_link")
link_c3l1cache_link.connect( (iface3, "lowlink", "1000ps"), (c3_l1cache, "highlink", "1000ps") )
link_c3l1cache_bus = sst.Link("link_c3l1cache_bus")
link_c3l1cache_bus.connect( (c3_l1cache, "lowlink", "10000ps"), (comp_n0_bus, "highlink3", "10000ps") )
link_n0bus_l2cache = sst.Link("link_n0bus_l2cache")
link_n0bus_l2cache.connect( (comp_n0_bus, "lowlink0", "10000ps"), (comp_n0_l2cache, "highlink", "10000ps") )
link_n0l2cache_bus = sst.Link("link_n0l2cache_bus")
link_n0l2cache_bus.connect( (comp_n0_l2cache, "lowlink", "10000ps"), (comp_n2_bus, "highlink0", "10000ps") )
link_c4l1cache_link = sst.Link("link_c4l1cache_link")
link_c4l1cache_link.connect( (iface4, "lowlink", "1000ps"), (c4_l1cache, "highlink", "1000ps") )
link_c4l1cache_bus = sst.Link("link_c4l1cache_bus")
link_c4l1cache_bus.connect( (c4_l1cache, "lowlink", "10000ps"), (comp_n1_bus, "highlink0", "10000ps") )
link_c5l1cache_link = sst.Link("link_c5l1cache_link")
link_c5l1cache_link.connect( (iface5, "lowlink", "1000ps"), (c5_l1cache, "highlink", "1000ps") )
link_c5l1cache_bus = sst.Link("link_c5l1cache_bus")
link_c5l1cache_bus.connect( (c5_l1cache, "lowlink", "10000ps"), (comp_n1_bus, "highlink1", "10000ps") )
link_c6l1cache_link = sst.Link("link_c6l1cache_link")
link_c6l1cache_link.connect( (iface6, "lowlink", "1000ps"), (c6_l1cache, "highlink", "1000ps") )
link_c6l1cache_bus = sst.Link("link_c6l1cache_bus")
link_c6l1cache_bus.connect( (c6_l1cache, "lowlink", "10000ps"), (comp_n1_bus, "highlink2", "10000ps") )
link_c7l1cache_link = sst.Link("link_c7l1cache_link")
link_c7l1cache_link.connect( (iface7, "lowlink", "1000ps"), (c7_l1cache, "highlink", "1000ps") )
link_c7l1cache_bus = sst.Link("link_c7l1cache_bus")
link_c7l1cache_bus.connect( (c7_l1cache, "lowlink", "10000ps"), (comp_n1_bus, "highlink3", "10000ps") )
link_n1bus_l2cache = sst.Link("link_n1bus_l2cache")
link_n1bus_l2cache.connect( (comp_n1_bus, "lowlink0", "10000ps"), (comp_n1_l2cache, "highlink", "10000ps") )
link_n1l2cache_bus = sst.Link("link_n1l2cache_bus")
link_n1l2cache_bus.connect( (comp_n1_l2cache, "lowlink", "10000ps"), (comp_n2_bus, "highlink1", "10000ps") )
link_n2bus_l3cache = sst.Link("link_n2bus_l3cache")
link_n2bus_l3cache.connect( (comp_n2_bus, "lowlink0", "10000ps"), (l3cache, "highlink", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "10000ps"), (chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirctrl, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
