import sst
from mhlib import componentlist

# Global variables
DEBUG_SCRATCH = 0
DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_BUS = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0
core_clock = "2GHz"

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.ScratchCPU")
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.scratchInterface")
iface0.addParams({ "scratchpad_size" : "64KiB" })
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
    "debug" : DEBUG_L1 | DEBUG_CORE0,
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
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.scratchInterface")
iface1.addParams({ "scratchpad_size" : "64KiB" })
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
    "debug" : DEBUG_L1 | DEBUG_CORE0,
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
    "debug" : DEBUG_L2 | DEBUG_CORE0,
    "debug_level" : 10,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "16KiB",
    "access_latency_cycles" : 5,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "mru",
})
l2_link_0 = comp_l2_0.setSubComponent("cpulink", "memHierarchy.MemLink")
l2_nic_0 = comp_l2_0.setSubComponent("memlink", "memHierarchy.MemNIC")
l2_nic_0.addParams({
    "network_bw" : "80GiB/s",
    "group" : 1,
})

comp_cpu2 = sst.Component("cpu2", "memHierarchy.ScratchCPU")
iface2 = comp_cpu2.setSubComponent("memory", "memHierarchy.scratchInterface")
iface2.addParams({ "scratchpad_size" : "64KiB" })
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
    "debug" : DEBUG_L1 | DEBUG_CORE1,
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
iface3 = comp_cpu3.setSubComponent("memory", "memHierarchy.scratchInterface")
iface3.addParams({ "scratchpad_size" : "64KiB" })
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
    "debug" : DEBUG_L1 | DEBUG_CORE1,
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
    "debug" : DEBUG_L2 | DEBUG_CORE1,
    "debug_level" : 10,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "16KiB",
    "access_latency_cycles" : 5,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "nmru",
})
l2_link_1 = comp_l2_1.setSubComponent("cpulink", "memHierarchy.MemLink")
l2_nic_1 = comp_l2_1.setSubComponent("memlink", "memHierarchy.MemNIC")
l2_nic_1.addParams({
    "network_bw" : "80GiB/s",
    "group" : 1,
})
comp_dir = sst.Component("dir", "memHierarchy.DirectoryController")
comp_dir.addParams({
    "debug" : DEBUG_DIR | DEBUG_CORE0 | DEBUG_CORE1,
    "debug_level" : 10,
    "verbose" : 2,
    "entry_cache_size" : 1024,
    "coherence_protocol" : "MESI",
    "addr_range_start" : 0,
})
dir_nic = comp_dir.setSubComponent("cpulink", "memHierarchy.MemNIC")
dir_nic.addParams({
    "network_bw" : "80GiB/s",
    "group" : 2,
})
scratch = sst.Component("scratch", "memHierarchy.Scratchpad")
scratch.addParams({
    "debug" : DEBUG_SCRATCH | DEBUG_CORE0 | DEBUG_CORE1,
    "debug_level" : 10,
    "verbose" : 2,
    "clock" : core_clock,
    "size" : "64KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})
conv = scratch.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch_backend = conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch_backend.addParam("access_time", "10ns")
scratch_nic = scratch.setSubComponent("cpulink", "memHierarchy.MemNIC")
scratch_nic.addParams({
    "network_bw" : "50GB/s",
    "group" : 3,
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
comp_net.setSubComponent("topology","merlin.singlerouter")

memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
memctrl0.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "backing" : "none",
    "clock" : "1GHz",
    "verbose" : 2,
    "addr_range_start" : 0,
    "interleave_size" : "128B",
    "interleave_step" : "256B",
})

memnic0 = memctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
memnic0.addParams({
    "network_bw" : "50GB/s",
    "group" : 4,
})

mem0 = memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
mem0.addParams({
    "access_time" : "50ns",
    "mem_size" : "512MiB",
})

memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
memctrl1.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "backing" : "none",
    "clock" : "1GHz",
    "verbose" : 2,
    "addr_range_start" : 128,
    "interleave_size" : "128B",
    "interleave_step" : "256B",
})

memnic1 = memctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
memnic1.addParams({
    "network_bw" : "50GB/s",
    "group" : 4,
})

mem1 = memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
mem1.addParams({
    "access_time" : "50ns",
    "mem_size" : "512MiB",
})

comp_bus_0 = sst.Component("bus_0", "memHierarchy.Bus")
comp_bus_0.addParams({
    "bus_frequency" : "2GHz",
    "debug" : DEBUG_BUS,
    "debug_level" : 5,
})
comp_bus_1 = sst.Component("bus_1", "memHierarchy.Bus")
comp_bus_1.addParams({
    "bus_frequency" : "2GHz",
    "debug" : DEBUG_BUS,
    "debug_level" : 5,
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu0_l1 = sst.Link("link_cpu0_l1")
link_cpu0_l1.connect( (iface0, "port", "100ps"), (comp_l1_0, "high_network_0", "100ps") )

link_cpu1_l1 = sst.Link("link_cpu1_l1")
link_cpu1_l1.connect( (iface1, "port", "100ps"), (comp_l1_1, "high_network_0", "100ps") )

link_cpu2_l1 = sst.Link("link_cpu2_l1")
link_cpu2_l1.connect( (iface2, "port", "100ps"), (comp_l1_2, "high_network_0", "100ps") )

link_cpu3_l1 = sst.Link("link_cpu3_l1")
link_cpu3_l1.connect( (iface3, "port", "100ps"), (comp_l1_3, "high_network_0", "100ps") )

link_cpu0_bus = sst.Link("link_cpu0_bus")
link_cpu0_bus.connect( (comp_l1_0, "low_network_0", "100ps"), (comp_bus_0, "high_network_0", "100ps") )

link_cpu1_bus = sst.Link("link_cpu1_bus")
link_cpu1_bus.connect( (comp_l1_1, "low_network_0", "100ps"), (comp_bus_0, "high_network_1", "100ps") )

link_cpu2_bus = sst.Link("link_cpu2_bus")
link_cpu2_bus.connect( (comp_l1_2, "low_network_0", "100ps"), (comp_bus_1, "high_network_0", "100ps") )

link_cpu3_bus = sst.Link("link_cpu3_bus")
link_cpu3_bus.connect( (comp_l1_3, "low_network_0", "100ps"), (comp_bus_1, "high_network_1", "100ps") )

link_bus_l2_0 = sst.Link("link_bus_l2_0")
link_bus_l2_0.connect( (comp_bus_0, "low_network_0", "100ps"), (l2_link_0, "port", "100ps") )

link_bus_l2_1 = sst.Link("link_bus_l2_1")
link_bus_l2_1.connect( (comp_bus_1, "low_network_0", "100ps"), (l2_link_1, "port", "100ps") )

link_l2_net0 = sst.Link("link_l2_net_0")
link_l2_net0.connect( (l2_nic_0, "port", "100ps"), (comp_net, "port0", "100ps") )

link_l2_net1 = sst.Link("link_l2_net_1")
link_l2_net1.connect( (l2_nic_1, "port", "100ps"), (comp_net, "port1", "100ps") )

link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (dir_nic, "port", "100ps"), (comp_net, "port2", "100ps") )

link_scratch_net = sst.Link("link_scratch_net")
link_scratch_net.connect( (scratch_nic, "port", "100ps"), (comp_net, "port3", "100ps") )

link_mem0_net = sst.Link("link_mem0_net")
link_mem0_net.connect( (memnic0, "port", "100ps"), (comp_net, "port4", "100ps") )

link_mem1_net = sst.Link("link_mem1_net")
link_mem1_net.connect( (memnic1, "port", "100ps"), (comp_net, "port5", "100ps") )
# End of generated output.
