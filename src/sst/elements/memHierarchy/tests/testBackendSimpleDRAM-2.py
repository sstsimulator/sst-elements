import sst
from mhlib import componentlist

# Test functions
# SimpleMemBackend w/ close page policy
# Memory controller connected to network
# backing = none
# MESI coherence protocol

DEBUG_LEVEL = 10
DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0

cpu_params = {
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 32,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
    "clock" : "2GHz",
    "memFreq" : 4,
    }

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
cpu0.addParams(cpu_params)
cpu0.addParams({
      "rngseed" : "5",
})
c0_l1cache = sst.Component("l1cache0.mesi", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "maxRequestDelay" : "1000000",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
cpu1.addParams(cpu_params)
cpu1.addParams({
      "rngseed" : "201",
})
c1_l1cache = sst.Component("l1cache1.mesi", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL,
      "maxRequestDelay" : "1000000"
})
n0_bus = sst.Component("bus0", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
n0_l2cache = sst.Component("l2cache0.mesi.inclus", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEVEL,
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
cpu2.addParams(cpu_params)
cpu2.addParams({
      "rngseed" : "401",
})
c2_l1cache = sst.Component("l1cache2.mesi", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "maxRequestDelay" : "1000000",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL,
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
cpu3.addParams(cpu_params)
cpu3.addParams({
      "rngseed" : "96",
})
c3_l1cache = sst.Component("l1cache3.mesi", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "maxRequestDelay" : "1000000",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL,
})
n1_bus = sst.Component("bus1", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
n1_l2cache = sst.Component("l2cache1.mesi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEVEL,
})
n2_bus = sst.Component("bus2", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l3cache = sst.Component("l3cache.mesi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : DEBUG_L3,
      "debug_level" : DEBUG_LEVEL,
})
l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "group" : 1,
    "network_bw" : "25GB/s",
#   "debug" : 1,
    "debug_level" : 10,
})
network = sst.Component("network", "merlin.hr_router")
network.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "2KB",
      "num_ports" : "3",
      "flit_size" : "72B",
      "output_buf_size" : "2KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
network.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("directory.mesi", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "coherence_protocol" : "MESI",
    "debug" : DEBUG_DIR,
    "debug_level" : DEBUG_LEVEL,
    "entry_cache_size" : "32768",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0",
})
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "group" : 2,
    "network_bw" : "25GB/s",
    "network_input_buffer_size" : "2KiB",
    "network_output_buffer_size" : "2KiB",
#   "debug" : 1,
    "debug_level" : 10,
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "backing" : "none",
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0",
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
memory.addParams({
    "mem_size" : "512MiB",
    "max_requests_per_cycle" : 1,
    "tCAS" : 3,
    "tRCD" : 3,
    "tRP" : 3,
    "cycle_time" : "5ns",
    "row_size" : "8KiB",
    "row_policy" : "closed",
})
memNIC = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
memNIC.addParams({
    "group" : 3,
#   "debug" : 1,
    "debug_level" : 10,
    "network_bw" : "25GB/s",
    "network_input_buffer_size" : "2KiB",
    "network_output_buffer_size" : "2KiB",
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
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "10000ps"), (network, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (network, "port0", "2000ps"), (dirNIC, "port", "2000ps") )
link_mem_net_0 = sst.Link("link_mem_net_0")
link_mem_net_0.connect( (network, "port2", "2000ps"), (memNIC, "port", "2000ps") )
