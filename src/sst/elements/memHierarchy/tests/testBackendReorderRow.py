import sst
from mhlib import componentlist

cpu_params = {
    "memFreq" : 4,
    "clock" : "2.2GHz",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
}

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
cpu0.addParams(cpu_params)
cpu0.addParams({
    "rngseed" : "101",
})
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
      "debug" : "0"
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
cpu1.addParams(cpu_params)
cpu1.addParams({
    "rngseed" : "301",
})
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
      "debug" : "0"
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
      "debug" : "0"
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
cpu2.addParams(cpu_params)
cpu2.addParams({
    "rngseed" : "501",
})
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
      "debug" : "0"
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
cpu3.addParams(cpu_params)
cpu3.addParams({
    "rngseed" : "701",
})
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
      "debug" : "0"
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
      "debug" : "0"
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
    "debug" : "0",
})
l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "group" : 1,
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
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
    "debug" : "0",
    "entry_cache_size" : "16384",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0",
})
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "group" : 2,
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "500MHz",
    "backing" : "none",
    "addr_range_end" : 512*1024*1024-1,
})
memreorder = memctrl.setSubComponent("backend", "memHierarchy.reorderByRow")
memreorder.addParams({
    "max_requests_per_cycle" : 50,  # Num requests the backend can accept per cycle
    "max_issue_per_cycle" : 2,      # Num requests the backend can send per cycle
    "reorder_limit" : "20",
})
memory = memreorder.setSubComponent("backend", "memHierarchy.simpleDRAM")
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
link_c0_l1cache.connect( (iface0, "port", "100ps"), (c0_l1cache, "high_network_0", "100ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "low_network_0", "200ps"), (n0_bus, "high_network_0", "200ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "100ps"), (c1_l1cache, "high_network_0", "100ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "low_network_0", "100ps"), (n0_bus, "high_network_1", "200ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "low_network_0", "200ps"), (n0_l2cache, "high_network_0", "200ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "low_network_0", "200ps"), (n2_bus, "high_network_0", "200ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "100ps"), (c2_l1cache, "high_network_0", "100ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "low_network_0", "200ps"), (n1_bus, "high_network_0", "200ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "100ps"), (c3_l1cache, "high_network_0", "100ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "low_network_0", "200ps"), (n1_bus, "high_network_1", "200ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "low_network_0", "200ps"), (n1_l2cache, "high_network_0", "200ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "low_network_0", "200ps"), (n2_bus, "high_network_1", "200ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "low_network_0", "200ps"), (l3tol2, "port", "200ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "200ps"), (network, "port1", "150ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (network, "port0", "150ps"), (dirNIC, "port", "150ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirtoM, "port", "200ps"), (memctrl, "direct_link", "200ps") )
