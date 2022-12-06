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
DEBUG_NODE0 = 0
DEBUG_NODE1 = 0

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 0,
    "maxOutstanding" : 32,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
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
      "debug_level" : 10,
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 2,
    "maxOutstanding" : 32,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
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
      "debug_level" : 10,
      "debug" : DEBUG_L1 | DEBUG_CORE1 | DEBUG_NODE0
})
n0_bus = sst.Component("bus0", "memHierarchy.Bus")
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
      "cache_size" : "32 KB",
      "debug_level" : 10,
      "debug" : DEBUG_L2 | DEBUG_NODE0
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 4,
    "maxOutstanding" : 32,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
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
      "debug_level" : 10,
      "debug" : DEBUG_L1 | DEBUG_CORE2 | DEBUG_NODE1
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
cpu3.addParams({
    "memFreq" : 1,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 6,
    "maxOutstanding" : 32,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
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
      "debug_level" : 10,
      "debug" : DEBUG_L1 | DEBUG_CORE3 | DEBUG_NODE1
})
n1_bus = sst.Component("bus1", "memHierarchy.Bus")
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
      "cache_size" : "32 KB",
      "debug_level" : 10,
      "debug" : DEBUG_L2 | DEBUG_NODE1
})
n2_bus = sst.Component("bus2", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l3cache = sst.Component("l3cache.msi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug_level" : 10,
      "debug" : DEBUG_L3,
})

l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 1,
})

chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("directory.msi", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "entry_cache_size" : "1024",
      "debug" : DEBUG_DIR,
      "debug_level" : 10,
      "addr_range_start" : "0x0",
      "addr_range_end" : "0x1F000000",
})
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 2,
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_end" : 512*1024*1024-1,
})
memtoD = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "port", "1000ps"), (c0_l1cache, "high_network_0", "1000ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_0", "10000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "1000ps"), (c1_l1cache, "high_network_0", "1000ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_1", "10000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "low_network_0", "10000ps"), (n0_l2cache, "high_network_0", "10000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_0", "10000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "1000ps"), (c2_l1cache, "high_network_0", "1000ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_0", "10000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "1000ps"), (c3_l1cache, "high_network_0", "1000ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_1", "10000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "low_network_0", "10000ps"), (n1_l2cache, "high_network_0", "10000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "low_network_0", "10000ps"), (l3tol2, "port", "10000ps") )
# Network connections
link_cache_net = sst.Link("link_cache_net_0")
link_cache_net.connect( (l3NIC, "port", "10000ps"), (chiprtr, "port1", "2000ps") )
link_dir_net = sst.Link("link_dir_net_0")
link_dir_net.connect( (chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )
# Directory to memory
link_dir_mem = sst.Link("link_dir_mem_link")
link_dir_mem.connect( (dirtoM, "port", "10000ps"), (memtoD, "port", "10000ps") )
