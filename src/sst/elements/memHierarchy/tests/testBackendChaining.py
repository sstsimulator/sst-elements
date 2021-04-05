# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "clock" : "2.2GHz",
      "commFreq" : "4",
      "rngseed" : "101",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.memInterface")
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
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
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "clock" : "2.2GHz",
      "commFreq" : "4",
      "rngseed" : "301",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.memInterface")
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
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
comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : "2GHz"
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
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
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
comp_cpu2.addParams({
      "clock" : "2.2GHz",
      "commFreq" : "4",
      "rngseed" : "501",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface2 = comp_cpu2.setSubComponent("memory", "memHierarchy.memInterface")
comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams({
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
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
comp_cpu3.addParams({
      "clock" : "2.2GHz",
      "commFreq" : "4",
      "rngseed" : "701",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
iface3 = comp_cpu3.setSubComponent("memory", "memHierarchy.memInterface")
comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams({
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
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : "2GHz"
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
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
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2GHz"
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
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

l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
    "group" : 1,
})

comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "30GB/s",
      "link_bw" : "30GB/s",
      "input_buf_size" : "2KiB",
      "num_ports" : "2",
      "flit_size" : "36B",
      "output_buf_size" : "2KiB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "clock" : "1.5GHz",
    "coherence_protocol" : "MESI",
    "debug" : DEBUG_DIR,
    "debug_level" : DEBUG_LEVEL,
    "entry_cache_size" : "16384",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0",
})
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
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
link_c0_l1cache.connect( (iface0, "port", "100ps"), (comp_c0_l1cache, "high_network_0", "100ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "200ps"), (comp_n0_bus, "high_network_0", "200ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "100ps"), (comp_c1_l1cache, "high_network_0", "100ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "100ps"), (comp_n0_bus, "high_network_1", "200ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "low_network_0", "200ps"), (comp_n0_l2cache, "high_network_0", "200ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "200ps"), (comp_n2_bus, "high_network_0", "200ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "100ps"), (comp_c2_l1cache, "high_network_0", "100ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "200ps"), (comp_n1_bus, "high_network_0", "200ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "100ps"), (comp_c3_l1cache, "high_network_0", "100ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "200ps"), (comp_n1_bus, "high_network_1", "200ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "low_network_0", "200ps"), (comp_n1_l2cache, "high_network_0", "200ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "200ps"), (comp_n2_bus, "high_network_1", "200ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "200ps"), (l3tol2, "port", "200ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "200ps"), (comp_chiprtr, "port1", "150ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "150ps"), (dirNIC, "port", "150ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirtoM, "port", "200ps"), (memctrl, "direct_link", "200ps") )
# End of generated output.
