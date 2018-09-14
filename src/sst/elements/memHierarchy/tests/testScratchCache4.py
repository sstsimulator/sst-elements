import sst

# Global variables
debugScratch = 0
debugL1 = 0
debugL2 = 0
debugL3 = 0
debugBus = 0
debugDir = 0
debugCore0 = 0
debugCore1 = 0
core_clock = "2GHz"

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.ScratchCPU")
comp_cpu0.addParams({
    "scratchSize" : 65536,   # 64K scratch
    "maxAddr" : 2097152,       # 2M mem
    "scratchLineSize" : 64,
    "memLineSize" : 128,
    "clock" : core_clock,
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 1000,
    "verbose" : 1,
    "rngseed" : 8
})
comp_l1_0 = sst.Component("l1_0", "memHierarchy.Cache")
comp_l1_0.addParams({
    "debug" : debugL1 | debugCore0,
    "debug_level" : 9,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "4KiB",
    "access_latency_cycles" : 1,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})


comp_cpu1 = sst.Component("cpu1", "memHierarchy.ScratchCPU")
comp_cpu1.addParams({
    "scratchSize" : 65536,   # 64K scratch
    "maxAddr" : 2097152,       # 2M mem
    "scratchLineSize" : 64,
    "memLineSize" : 128,
    "clock" : core_clock,
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 1000,
    "verbose" : 1,
    "rngseed" : 84
})
comp_l1_1 = sst.Component("l1_1", "memHierarchy.Cache")
comp_l1_1.addParams({
    "debug" : debugL1 | debugCore0,
    "debug_level" : 9,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "4KiB",
    "access_latency_cycles" : 1,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})

comp_l2_0 = sst.Component("l2_0", "memHierarchy.Cache")
comp_l2_0.addParams({
    "debug" : debugL2 | debugCore0,
    "debug_level" : 10,
    "debug_addr" : "[0x1340]",
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "16KiB",
    "access_latency_cycles" : 5,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "mru",
    "memNIC.network_bw" : "80GiB/s",
    "memNIC.group" : 1,
})
comp_cpu2 = sst.Component("cpu2", "memHierarchy.ScratchCPU")
comp_cpu2.addParams({
    "scratchSize" : 65536,   # 64K scratch
    "maxAddr" : 2097152,       # 2M mem
    "scratchLineSize" : 64,
    "memLineSize" : 128,
    "clock" : core_clock,
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 1000,
    "verbose" : 1,
    "rngseed" : 10
})
comp_l1_2 = sst.Component("l1_2", "memHierarchy.Cache")
comp_l1_2.addParams({
    "debug" : debugL1 | debugCore1,
    "debug_level" : 9,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "4KiB",
    "access_latency_cycles" : 4,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})
comp_cpu3 = sst.Component("cpu3", "memHierarchy.ScratchCPU")
comp_cpu3.addParams({
    "scratchSize" : 65536,   # 64K scratch
    "maxAddr" : 2097152,       # 2M mem
    "scratchLineSize" : 64,
    "memLineSize" : 128,
    "clock" : core_clock,
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 1000,
    "verbose" : 1,
    "rngseed" : 109
})
comp_l1_3 = sst.Component("l1_3", "memHierarchy.Cache")
comp_l1_3.addParams({
    "debug" : debugL1 | debugCore1,
    "debug_level" : 9,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "4KiB",
    "access_latency_cycles" : 4,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})
comp_l2_1 = sst.Component("l2_1", "memHierarchy.Cache")
comp_l2_1.addParams({
    "debug" : debugL2 | debugCore1,
    "debug_level" : 10,
    "debug_addr" : "[0x1340]",
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "16KiB",
    "access_latency_cycles" : 5,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "nmru",
    "memNIC.network_bw" : "80GiB/s",
    "memNIC.group" : 1,
})
comp_dir = sst.Component("dir", "memHierarchy.DirectoryController")
comp_dir.addParams({
    "debug" : debugDir | debugCore0 | debugCore1,
    "debug_level" : 10,
    "debug_addr" : "[0x1340]",
    "verbose" : 2,
    "entry_cache_size" : 1024,
    "coherence_protocol" : "MESI",
    "memNIC.network_bw" : "80GiB/s",
    "memNIC.addr_range_start" : 0,
    "memNIC.group" : 2,
    "net_memory_name" : "scratch"
})
comp_scratch = sst.Component("scratch", "memHierarchy.Scratchpad")
comp_scratch.addParams({
    "debug" : debugScratch | debugCore0 | debugCore1,
    "debug_level" : 10,
    "debug_addr" : "[0x1340]",
    "verbose" : 2,
    "clock" : core_clock,
    "size" : "64KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
    "backendConvertor" : "memHierarchy.simpleMemScratchBackendConvertor",
    "backendConvertor.backend" : "memHierarchy.simpleMem",
    "backendConvertor.backend.access_time" : "10ns",
    "memNIC.network_bw" : "50GB/s",
    "memNIC.group" : 3,
})
comp_net = sst.Component("network", "merlin.hr_router")
comp_net.addParams({
    "xbar_bw" : "50GB/s",
    "link_bw" : "50GB/s",
    "input_buf_size" : "1KiB",
    "output_buf_size" : "1KiB",
    "flit_size" : "72B",
    "id" : "0",
    "topology" : "merlin.singlerouter",
    "num_ports" : 6
})

comp_memory0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memory0.addParams({
      #"debug" : "1",
      #"debug_level" : 10,
      "backing" : "none",
      "clock" : "1GHz",
      "verbose" : 2,
      #"backendConvertor.debug_location" : 1,
      #"backendConvertor.debug_level" : 10,
      "backend.access_time" : "50ns",
      "backend.mem_size" : "512MiB",
      "memNIC.network_bw" : "50GB/s",
      "memNIC.addr_range_start" : 0,
      "memNIC.interleave_size" : "128B",
      "memNIC.interleave_step" : "256B",
      "memNIC.group" : 4,
})
comp_memory1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memory1.addParams({
      #"debug" : "1",
      #"debug_level" : 10,
      "backing" : "none",
      "backend.access_time" : "50ns",
      "clock" : "1GHz",
      "verbose" : 2,
      "backend.mem_size" : "512MiB",
      "memNIC.network_bw" : "50GB/s",
      "memNIC.addr_range_start" : 128,
      "memNIC.interleave_size" : "128B",
      "memNIC.interleave_step" : "256B",
      "memNIC.group" : 4,
})

comp_bus_0 = sst.Component("bus_0", "memHierarchy.Bus")
comp_bus_0.addParams({
        "bus_frequency" : "2GHz",
        "debug" : debugBus,
        "debug_level" : 5,
})
comp_bus_1 = sst.Component("bus_1", "memHierarchy.Bus")
comp_bus_1.addParams({
        "bus_frequency" : "2GHz",
        "debug" : debugBus,
        "debug_level" : 5,
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Scratchpad")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_cpu0_l1 = sst.Link("link_cpu0_l1")
link_cpu0_l1.connect( (comp_cpu0, "mem_link", "100ps"), (comp_l1_0, "high_network_0", "100ps") )

link_cpu1_l1 = sst.Link("link_cpu1_l1")
link_cpu1_l1.connect( (comp_cpu1, "mem_link", "100ps"), (comp_l1_1, "high_network_0", "100ps") )

link_cpu2_l1 = sst.Link("link_cpu2_l1")
link_cpu2_l1.connect( (comp_cpu2, "mem_link", "100ps"), (comp_l1_2, "high_network_0", "100ps") )

link_cpu3_l1 = sst.Link("link_cpu3_l1")
link_cpu3_l1.connect( (comp_cpu3, "mem_link", "100ps"), (comp_l1_3, "high_network_0", "100ps") )

link_cpu0_bus = sst.Link("link_cpu0_bus")
link_cpu0_bus.connect( (comp_l1_0, "low_network_0", "100ps"), (comp_bus_0, "high_network_0", "100ps") )

link_cpu1_bus = sst.Link("link_cpu1_bus")
link_cpu1_bus.connect( (comp_l1_1, "low_network_0", "100ps"), (comp_bus_0, "high_network_1", "100ps") )

link_cpu2_bus = sst.Link("link_cpu2_bus")
link_cpu2_bus.connect( (comp_l1_2, "low_network_0", "100ps"), (comp_bus_1, "high_network_0", "100ps") )

link_cpu3_bus = sst.Link("link_cpu3_bus")
link_cpu3_bus.connect( (comp_l1_3, "low_network_0", "100ps"), (comp_bus_1, "high_network_1", "100ps") )

link_bus_l2_0 = sst.Link("link_bus_l2_0")
link_bus_l2_0.connect( (comp_bus_0, "low_network_0", "100ps"), (comp_l2_0, "high_network_0", "100ps") )

link_bus_l2_1 = sst.Link("link_bus_l2_1")
link_bus_l2_1.connect( (comp_bus_1, "low_network_0", "100ps"), (comp_l2_1, "high_network_0", "100ps") )

link_l2_net0 = sst.Link("link_l2_net_0")
link_l2_net0.connect( (comp_l2_0, "directory", "100ps"), (comp_net, "port0", "100ps") )

link_l2_net1 = sst.Link("link_l2_net_1")
link_l2_net1.connect( (comp_l2_1, "directory", "100ps"), (comp_net, "port1", "100ps") )

link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (comp_dir, "network", "100ps"), (comp_net, "port2", "100ps") )

link_scratch_net = sst.Link("link_scratch_net")
link_scratch_net.connect( (comp_scratch, "network", "100ps"), (comp_net, "port3", "100ps") )

link_mem0_net = sst.Link("link_mem0_net")
link_mem0_net.connect( (comp_memory0, "network", "100ps"), (comp_net, "port4", "100ps") )

link_mem1_net = sst.Link("link_mem1_net")
link_mem1_net.connect( (comp_memory1, "network", "100ps"), (comp_net, "port5", "100ps") )
# End of generated output.
