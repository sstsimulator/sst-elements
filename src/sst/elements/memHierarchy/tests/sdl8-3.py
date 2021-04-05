# Automatically generated SST Python input
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

# Core 0
cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
cpu0.addParams({
      "commFreq" : "100",
      "rngseed" : "101",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.memInterface")

# L1 0
c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0,
      "debug_level" : 10,
})
l1ToC_0 = c0_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1Tol2_0 = c0_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# Core 1
cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
cpu1.addParams({
      "commFreq" : "100",
      "rngseed" : "301",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface1 = cpu1.setSubComponent("memory", "memHierarchy.memInterface")

# L1 1
c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE1 | DEBUG_NODE0,
      "debug_level" : 10,
})
l1ToC_1 = c1_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1Tol2_1 = c1_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# L1/L2 bus 0
n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})

# L2 0
n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2 | DEBUG_NODE0,
      "debug_level" : 10,
})
l2Tol1_0 = n0_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2Tol3_0 = n0_l2cache.setSubComponent("memlink", "memHierarchy.MemLink")

# Core 2
cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
cpu2.addParams({
      "commFreq" : "100",
      "rngseed" : "501",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface2 = cpu2.setSubComponent("memory", "memHierarchy.memInterface")

# L1 2
c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE2 | DEBUG_NODE1,
      "debug_level" : 10,
})
l1ToC_2 = c2_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1Tol2_2 = c2_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# Core 3
cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
cpu3.addParams({
      "commFreq" : "100",
      "rngseed" : "701",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface3 = cpu3.setSubComponent("memory", "memHierarchy.memInterface")

# L1 3
c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE3 | DEBUG_NODE1,
      "debug_level" : 10,
})
l1ToC_3 = c3_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1Tol2_3 = c3_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# L1/L2 bus 1
n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})

# L2 1
n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2 | DEBUG_NODE1,
      "debug_level" : 10,
})
l2Tol1_1 = n1_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2Tol3_1 = n1_l2cache.setSubComponent("memlink", "memHierarchy.MemLink")

# L2/L3 bus
n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})

# L3
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : DEBUG_L3,
      "debug_level" : 10,
})
l3Tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "group" : 1,
    "network_bw" : "25GB/s",
})

# Network-on-chip
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

# Directory
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR,
      "debug_level" : 10,
      "entry_cache_size" : "32768",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0",
})
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 2,
})
dirLink = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")

# Memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_end" : 512*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "100 ns",
    "mem_size" : "512MiB",
})
memLink = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")

for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


#### Define the simulation links

# Cores to L1s
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "port", "1000ps"), (l1ToC_0, "port", "1000ps") )

link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "1000ps"), (l1ToC_1, "port", "1000ps") )

link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "1000ps"), (l1ToC_2, "port", "1000ps") )

link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "1000ps"), (l1ToC_3, "port", "1000ps") )

# L1s to buses
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (l1Tol2_0, "port", "10000ps"), (n0_bus, "high_network_0", "10000ps") )

link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (l1Tol2_1, "port", "10000ps"), (n0_bus, "high_network_1", "10000ps") )

link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (l1Tol2_2, "port", "10000ps"), (n1_bus, "high_network_0", "10000ps") )

link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (l1Tol2_3, "port", "10000ps"), (n1_bus, "high_network_1", "10000ps") )

# L1 buses to L2s
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "low_network_0", "10000ps"), (l2Tol1_0, "port", "10000ps") )

link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "low_network_0", "10000ps"), (l2Tol1_1, "port", "10000ps") )

# L2s to L3 via bus
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (l2Tol3_0, "port", "10000ps"), (n2_bus, "high_network_0", "10000ps") )

link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (l2Tol3_1, "port", "10000ps"), (n2_bus, "high_network_1", "10000ps") )

link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "low_network_0", "10000ps"), (l3Tol2, "port", "10000ps") )

# Network connections - l3 & directory
link_cache_net = sst.Link("link_cache_net_0")
link_cache_net.connect( (chiprtr, "port1", "2000ps"), (l3NIC, "port", "10000ps") )
link_dir_net = sst.Link("link_dir_net_0")
link_dir_net.connect( (chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )

# Directory to memory
link_dir_mem = sst.Link("link_dir_mem")
link_dir_mem.connect( (dirLink, "port", "10000ps"), (memLink, "port", "10000ps") )
