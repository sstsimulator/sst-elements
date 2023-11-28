import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0
DEBUG_NODE0 = 0
DEBUG_NODE1 = 0

# 4 cores with private caches
# "Node0" is core+l1 0, core+l1 1, DC0, MC0, DC1, MC1
# "Node1" is core+l1 2, core+l1 3, DC2, MC2, DC3, MC3
# Cores are coherent on memory in their node, and can access the other node's memory incoherently via "noncacheable regions" at the cores' memH interfaces

####################
# Core 0 / Node 0
####################
node0_core0 = sst.Component("n0.core0", "memHierarchy.standardCPU")
node0_core0.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 1,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
node0_core0_iface = node0_core0.setSubComponent("memory", "memHierarchy.standardInterface")
# Cache addrs 0 : 50*1024-1, others are noncacheable
node0_core0_iface.addParam("noncacheable_regions", [50*1024, 100*1024-1]) # Address map to DC/MC 2 and 3

# L1 0
node0_l1cache0 = sst.Component("n0.l1cache0", "memHierarchy.Cache")
node0_l1cache0.addParams({
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
node0_l1cache0_to_core = node0_l1cache0.setSubComponent("cpulink", "memHierarchy.MemLink")
node0_l1cache0_to_network = node0_l1cache0.setSubComponent("memlink", "memHierarchy.MemNIC")
node0_l1cache0_to_network.addParams({
    "group" : 0,
    "network_bw" : "25GB/s",
})

####################
# Core 1 / Node 0
####################
node0_core1 = sst.Component("n0.core1", "memHierarchy.standardCPU")
node0_core1.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 2,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
node0_core1_iface = node0_core1.setSubComponent("memory", "memHierarchy.standardInterface")
# Cache addrs 0 : 50*1024-1, others are noncacheable
node0_core1_iface.addParam("noncacheable_regions", [50*1024, 100*1024-1]) # Address map to DC/MC 2 and 3

# L1 0
node0_l1cache1 = sst.Component("n0.l1cache1", "memHierarchy.Cache")
node0_l1cache1.addParams({
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
node0_l1cache1_to_core = node0_l1cache1.setSubComponent("cpulink", "memHierarchy.MemLink")
node0_l1cache1_to_network = node0_l1cache1.setSubComponent("memlink", "memHierarchy.MemNIC")
node0_l1cache1_to_network.addParams({
    "group" : 0,
    "network_bw" : "25GB/s",
})

####################
# Core 0 / Node 1
####################
node1_core0 = sst.Component("n1.core0", "memHierarchy.standardCPU")
node1_core0.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 3,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
node1_core0_iface = node1_core0.setSubComponent("memory", "memHierarchy.standardInterface")
# Cache addrs 50*1024 : 100*1024-1, others are noncacheable
node1_core0_iface.addParam("noncacheable_regions", [0, 50*1024-1]) # Address map to DC/MC 2 and 3

# L1 0
node1_l1cache0 = sst.Component("n1.l1cache0", "memHierarchy.Cache")
node1_l1cache0.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE1,
      "debug_level" : 10,
})
node1_l1cache0_to_core = node1_l1cache0.setSubComponent("cpulink", "memHierarchy.MemLink")
node1_l1cache0_to_network = node1_l1cache0.setSubComponent("memlink", "memHierarchy.MemNIC")
node1_l1cache0_to_network.addParams({
    "group" : 0,
    "network_bw" : "25GB/s",
})

####################
# Core 1 / Node 1
####################
node1_core1 = sst.Component("n1.core1", "memHierarchy.standardCPU")
node1_core1.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 4,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
node1_core1_iface = node1_core1.setSubComponent("memory", "memHierarchy.standardInterface")
# Cache addrs 50*1024 : 100*1024-1, others are noncacheable
node1_core1_iface.addParam("noncacheable_regions", [0, 50*1024-1]) # Address map to DC/MC 2 and 3

node1_l1cache1 = sst.Component("n1.l1cache1", "memHierarchy.Cache")
node1_l1cache1.addParams({
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
node1_l1cache1_to_core = node1_l1cache1.setSubComponent("cpulink", "memHierarchy.MemLink")
node1_l1cache1_to_network = node1_l1cache1.setSubComponent("memlink", "memHierarchy.MemNIC")
node1_l1cache1_to_network.addParams({
    "group" : 0,
    "network_bw" : "25GB/s",
})



####################
# Network-on-chip
####################
chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "8",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")

####################
# Directory/Memory
####################
# Directories
node0_dc0 = sst.Component("n0.directory0", "memHierarchy.DirectoryController")
node0_dc1 = sst.Component("n0.directory1", "memHierarchy.DirectoryController")
node1_dc0 = sst.Component("n1.directory0", "memHierarchy.DirectoryController")
node1_dc1 = sst.Component("n1.directory1", "memHierarchy.DirectoryController")
# Parameterize directories
node0_dc0.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR | DEBUG_NODE0,
      "debug_level" : 10,
      "entry_cache_size" : "32768",
      "addr_range_end" : 25*1024-1,
      "addr_range_start" : 0,
})
node0_dc1.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR | DEBUG_NODE0,
      "debug_level" : 10,
      "entry_cache_size" : "32768",
      "addr_range_end" : 50*1024-1,
      "addr_range_start" : 25*1024,
})
node1_dc0.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR | DEBUG_NODE1,
      "debug_level" : 10,
      "entry_cache_size" : "32768",
      "addr_range_end" : 75*1024-1,
      "addr_range_start" : 50*1024,
})
node1_dc1.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR | DEBUG_NODE1,
      "debug_level" : 10,
      "entry_cache_size" : "32768",
      "addr_range_end" : 100*1024-1,
      "addr_range_start" : 75*1024,
})
# Create and parameterize memNIC for each dir
node0_dc0_to_network = node0_dc0.setSubComponent("cpulink", "memHierarchy.MemNIC")
node0_dc1_to_network = node0_dc1.setSubComponent("cpulink", "memHierarchy.MemNIC")
node1_dc0_to_network = node1_dc0.setSubComponent("cpulink", "memHierarchy.MemNIC")
node1_dc1_to_network = node1_dc1.setSubComponent("cpulink", "memHierarchy.MemNIC")
node0_dc0_to_network.addParams({
      "network_bw" : "25GB/s",
      "group" : 1,
})
node0_dc1_to_network.addParams({
      "network_bw" : "25GB/s",
      "group" : 1,
})
node1_dc0_to_network.addParams({
      "network_bw" : "25GB/s",
      "group" : 1,
})
node1_dc1_to_network.addParams({
      "network_bw" : "25GB/s",
      "group" : 1,
})
# Create memlink (to memory controller) for each dir
node0_dc0_to_memory = node0_dc0.setSubComponent("memlink", "memHierarchy.MemLink")
node0_dc1_to_memory = node0_dc1.setSubComponent("memlink", "memHierarchy.MemLink")
node1_dc0_to_memory = node1_dc0.setSubComponent("memlink", "memHierarchy.MemLink")
node1_dc1_to_memory = node1_dc1.setSubComponent("memlink", "memHierarchy.MemLink")

# Memory controllers
node0_mc0 = sst.Component("n0.memory0", "memHierarchy.MemController")
node0_mc1 = sst.Component("n0.memory1", "memHierarchy.MemController")
node1_mc0 = sst.Component("n1.memory0", "memHierarchy.MemController")
node1_mc1 = sst.Component("n1.memory1", "memHierarchy.MemController")
# Parameterize memory controllers
node0_mc0.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_start" : 0,
    "addr_range_end" : 25*1024-1,
})
node0_mc1.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_start" : 25*1024,
    "addr_range_end" : 50*1024-1,
})
node1_mc0.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_start" : 50*1024,
    "addr_range_end" : 75*1024-1,
})
node1_mc1.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_start" : 75*1024,
    "addr_range_end" : 100*1024-1,
})
# Create memory backends for each memory controller
node0_mem0 = node0_mc0.setSubComponent("backend", "memHierarchy.simpleMem")
node0_mem1 = node0_mc1.setSubComponent("backend", "memHierarchy.simpleMem")
node1_mem0 = node1_mc0.setSubComponent("backend", "memHierarchy.simpleMem")
node1_mem1 = node1_mc1.setSubComponent("backend", "memHierarchy.simpleMem")

node0_mem0.addParams( { "access_time" : "100 ns", "mem_size" : "50KiB" } )
node0_mem1.addParams( { "access_time" : "100 ns", "mem_size" : "50KiB" } )
node1_mem0.addParams( { "access_time" : "100 ns", "mem_size" : "50KiB" } )
node1_mem1.addParams( { "access_time" : "100 ns", "mem_size" : "50KiB" } )

# Create memlink (to directories) for each memory controller
node0_mem0_to_dc = node0_mc0.setSubComponent("cpulink", "memHierarchy.MemLink")
node0_mem1_to_dc = node0_mc1.setSubComponent("cpulink", "memHierarchy.MemLink")
node1_mem0_to_dc = node1_mc0.setSubComponent("cpulink", "memHierarchy.MemLink")
node1_mem1_to_dc = node1_mc1.setSubComponent("cpulink", "memHierarchy.MemLink")

####################
# Enable statistics
####################
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")

for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


####################
# Link components
####################

# Cores to private L1s
link_node0_core0 = sst.Link("link_n0_core0")
link_node0_core1 = sst.Link("link_n0_core1")
link_node1_core0 = sst.Link("link_n1_core0")
link_node1_core1 = sst.Link("link_n1_core1")
link_node0_core0.connect( (node0_core0_iface, "port", "1000ps"), (node0_l1cache0_to_core, "port", "1000ps") )
link_node0_core1.connect( (node0_core1_iface, "port", "1000ps"), (node0_l1cache1_to_core, "port", "1000ps") )
link_node1_core0.connect( (node1_core0_iface, "port", "1000ps"), (node1_l1cache0_to_core, "port", "1000ps") )
link_node1_core1.connect( (node1_core1_iface, "port", "1000ps"), (node1_l1cache1_to_core, "port", "1000ps") )


# L1s to network
link_node0_l1cache0_net = sst.Link("link_n0_l1cache0_net")
link_node0_l1cache1_net = sst.Link("link_n0_l1cache1_net")
link_node1_l1cache0_net = sst.Link("link_n1_l1cache0_net")
link_node1_l1cache1_net = sst.Link("link_n1_l1cache1_net")
link_node0_l1cache0_net.connect( (node0_l1cache0_to_network, "port", "2000ps"), (chiprtr, "port0", "2000ps") )
link_node0_l1cache1_net.connect( (node0_l1cache1_to_network, "port", "2000ps"), (chiprtr, "port1", "2000ps") )
link_node1_l1cache0_net.connect( (node1_l1cache0_to_network, "port", "2000ps"), (chiprtr, "port2", "2000ps") )
link_node1_l1cache1_net.connect( (node1_l1cache1_to_network, "port", "2000ps"), (chiprtr, "port3", "2000ps") )

# Directories to network
link_node0_dc0_net = sst.Link("link_n0_dc0_net")
link_node0_dc1_net = sst.Link("link_n0_dc1_net")
link_node1_dc0_net = sst.Link("link_n1_dc0_net")
link_node1_dc1_net = sst.Link("link_n1_dc1_net")
link_node0_dc0_net.connect( (node0_dc0_to_network, "port", "2000ps"), (chiprtr, "port4", "2000ps") )
link_node0_dc1_net.connect( (node0_dc1_to_network, "port", "2000ps"), (chiprtr, "port5", "2000ps") )
link_node1_dc0_net.connect( (node1_dc0_to_network, "port", "2000ps"), (chiprtr, "port6", "2000ps") )
link_node1_dc1_net.connect( (node1_dc1_to_network, "port", "2000ps"), (chiprtr, "port7", "2000ps") )

# Directories to memories
link_node0_dc0_mc0 = sst.Link("link_n0_dc0_m0")
link_node0_dc1_mc1 = sst.Link("link_n0_dc1_m1")
link_node1_dc0_mc0 = sst.Link("link_n1_dc0_m0")
link_node1_dc1_mc1 = sst.Link("link_n1_dc1_m1")
link_node0_dc0_mc0.connect( (node0_dc0_to_memory, "port", "1000ps"), (node0_mem0_to_dc, "port", "1000ps") )
link_node0_dc1_mc1.connect( (node0_dc1_to_memory, "port", "1000ps"), (node0_mem1_to_dc, "port", "1000ps") )
link_node1_dc0_mc0.connect( (node1_dc0_to_memory, "port", "1000ps"), (node1_mem0_to_dc, "port", "1000ps") )
link_node1_dc1_mc1.connect( (node1_dc1_to_memory, "port", "1000ps"), (node1_mem1_to_dc, "port", "1000ps") )



