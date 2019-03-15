# Automatically generated SST Python input
import sst

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
      "debug" : "0"
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
      "debug" : "0"
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
      "debug" : "0"
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
      "debug" : "0"
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
      "debug" : "0"
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
      "debug" : "0"
})
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2GHz"
})
comp_l3cache = sst.Component("l3cache", "memHierarchy.Cache")
comp_l3cache.addParams({
      "access_latency_cycles" : "19",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "nmru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64KiB",
      "debug" : "0",
      "memNIC.network_bw" : "40GB/s",
      "memNIC.input_buffer_size" : "2KiB",
      "memNIC.output_buffer_size" : "2KiB",
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
comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "clock" : "1.5GHz",
      "coherence_protocol" : "MESI",
      "debug" : "0",
      "entry_cache_size" : "16384",
      "memNIC.addr_range_end" : "0x1F000000",
      "memNIC.addr_range_start" : "0x0",
      "memNIC.network_bw" : "40GB/s",
      "memNIC.input_buffer_size" : "2KiB",
      "memNIC.output_buffer_size" : "2KiB",
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
    "backend.mem_size" : "512MiB",
    "clock" : "500MHz",
    "max_requests_per_cycle" : 50,
    "backing" : "none",
    "backend" : "memHierarchy.reorderByRow",
    "backend.max_issue_per_cycle" : 2,
    "backend.reorder_limit" : "20",
    "backend.backend" : "memHierarchy.simpleDRAM",
    "backend.backend.tCAS" : 3, # 11@800MHz roughly coverted to 200MHz
    "backend.backend.tRCD" : 3,
    "backend.backend.tRP" : 3,
    "backend.backend.cycle_time" : "5ns",
    "backend.backend.row_size" : "8KiB",
    "backend.backend.row_policy" : "open"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (comp_cpu0, "mem_link", "100ps"), (comp_c0_l1cache, "high_network_0", "100ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "200ps"), (comp_n0_bus, "high_network_0", "200ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (comp_cpu1, "mem_link", "100ps"), (comp_c1_l1cache, "high_network_0", "100ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "100ps"), (comp_n0_bus, "high_network_1", "200ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "low_network_0", "200ps"), (comp_n0_l2cache, "high_network_0", "200ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "200ps"), (comp_n2_bus, "high_network_0", "200ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (comp_cpu2, "mem_link", "100ps"), (comp_c2_l1cache, "high_network_0", "100ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "200ps"), (comp_n1_bus, "high_network_0", "200ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (comp_cpu3, "mem_link", "100ps"), (comp_c3_l1cache, "high_network_0", "100ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "200ps"), (comp_n1_bus, "high_network_1", "200ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "low_network_0", "200ps"), (comp_n1_l2cache, "high_network_0", "200ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "200ps"), (comp_n2_bus, "high_network_1", "200ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "200ps"), (comp_l3cache, "high_network_0", "200ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "200ps"), (comp_chiprtr, "port1", "150ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "150ps"), (comp_dirctrl, "network", "150ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "200ps"), (comp_memory, "direct_link", "200ps") )
# End of generated output.
