import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "101",
    "memSize" : "1MiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
c0_l1cache = sst.Component("l1cache0.mesi", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "301",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
c1_l1cache = sst.Component("l1cache1.mesi", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
n0_bus = sst.Component("bus0", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2GHz"
})
n0_l2cache = sst.Component("l2cache0.mesi.inclus", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "11",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEVEL
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "501",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
c2_l1cache = sst.Component("l1cache2.mesi", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
cpu3.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "701",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
c3_l1cache = sst.Component("l1cache3.mesi", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
n1_bus = sst.Component("bus1", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2GHz"
})
n1_l2cache = sst.Component("l2cache1.mesi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "11",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEVEL
})
n2_bus = sst.Component("bus2", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2GHz"
})
l3cache = sst.Component("l3cache.mesi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "19",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "nmru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64KiB",
      "debug" : DEBUG_L3,
      "debug_level" : DEBUG_LEVEL
})

l3NIC = l3cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
    "group" : 1,
})

network = sst.Component("network", "merlin.hr_router")
network.addParams({
      "xbar_bw" : "30GB/s",
      "link_bw" : "30GB/s",
      "input_buf_size" : "2KiB",
      "num_ports" : "2",
      "flit_size" : "36B",
      "output_buf_size" : "2KiB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
network.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("directory.mesi", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "clock" : "1.5GHz",
    "coherence_protocol" : "MESI",
    "debug" : DEBUG_DIR,
    "debug_level" : DEBUG_LEVEL,
    "entry_cache_size" : "16384",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0",
})
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
    "group" : 2,
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "500MHz",
    "backing" : "none",
    "addr_range_end" : 512*1024*1024-1,
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL
})

# Backends: delay -> reorder -> simpleDRAM

# Delays requests by n cycles
memdelay = memctrl.setSubComponent("backend", "memHierarchy.DelayBuffer")
# Reorder buffer for requests
memreorder = memdelay.setSubComponent("backend", "memHierarchy.reorderByRow")
# Memory model
memory = memreorder.setSubComponent("backend", "memHierarchy.simpleDRAM")
memdelay.addParams({
    "max_requests_per_cycle" : 50,
    "request_delay" : "20ns",
})
memreorder.addParams({
    "max_issue_per_cycle" : 2,
    "reorder_limit" : "20",
})
memory.addParams({
    "mem_size" : "512MiB",
    "tCAS" : 3, # 11@800MHz roughly coverted to 200MHz
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
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "lowlink", "100ps"), (c0_l1cache, "highlink", "100ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "lowlink", "200ps"), (n0_bus, "highlink0", "200ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "lowlink", "100ps"), (c1_l1cache, "highlink", "100ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "lowlink", "100ps"), (n0_bus, "highlink1", "200ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "lowlink0", "200ps"), (n0_l2cache, "highlink", "200ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "lowlink", "200ps"), (n2_bus, "highlink0", "200ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "lowlink", "100ps"), (c2_l1cache, "highlink", "100ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "lowlink", "200ps"), (n1_bus, "highlink0", "200ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "lowlink", "100ps"), (c3_l1cache, "highlink", "100ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "lowlink", "200ps"), (n1_bus, "highlink1", "200ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "lowlink0", "200ps"), (n1_l2cache, "highlink", "200ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "lowlink", "200ps"), (n2_bus, "highlink1", "200ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "lowlink0", "200ps"), (l3cache, "highlink", "200ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "200ps"), (network, "port1", "150ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (network, "port0", "150ps"), (dirNIC, "port", "150ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirctrl, "lowlink", "200ps"), (memctrl, "highlink", "200ps") )
