import sst
from mhlib import componentlist

# Global variables
debugScratch = 0
debugL1 = 0
debugL2 = 0
debugL3 = 0
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
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

comp_l1_0 = sst.Component("l1_0", "memHierarchy.Cache")
comp_l1_0.addParams({
    "debug" : debugL1 | debugCore0,
    "debug_level" : 9,
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
    "cache_frequency" : core_clock,
    "cache_size" : "16KiB",
    "access_latency_cycles" : 5,
    "cache_type" : "noninclusive",
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "mru",
})
l3_0 = sst.Component("l3_0", "memHierarchy.Cache")
l3_0.addParams({
    "debug" : debugL3 | debugCore0,
    "debug_level" : 10,
    "cache_frequency" : core_clock,
    "cache_size" : "128KiB",
    "access_latency_cycles" : 7,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "nmru",
})
comp_scratch0 = sst.Component("scratch0", "memHierarchy.Scratchpad")
comp_scratch0.addParams({
    "debug" : debugScratch | debugCore0,
    "debug_level" : 5,
    "clock" : core_clock,
    "size" : "64KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})
scratch0_conv = comp_scratch0.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch0_back = scratch0_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch0_back.addParam("access_time", "10ns")
scratchlink0 = comp_scratch0.setSubComponent("cpulink", "memHierarchy.MemLink")
scratchnic0 = comp_scratch0.setSubComponent("memlink", "memHierarchy.MemNIC")
scratchnic0.addParams({
    "network_bw" : "50GB/s",
    "group" : 0
})


comp_cpu1 = sst.Component("cpu1", "memHierarchy.ScratchCPU")
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
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
    "rngseed" : 10
})
comp_l1_1 = sst.Component("l1_1", "memHierarchy.Cache")
comp_l1_1.addParams({
    "debug" : debugL1 | debugCore1,
    "debug_level" : 9,
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
    "cache_frequency" : core_clock,
    "cache_size" : "16KiB",
    "access_latency_cycles" : 5,
    "cache_type" : "noninclusive",
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "mru",
})
l3_1 = sst.Component("l3_1", "memHierarchy.Cache")
l3_1.addParams({
    "debug" : debugL3 | debugCore1,
    "debug_level" : 10,
    "cache_frequency" : core_clock,
    "cache_size" : "128KiB",
    "access_latency_cycles" : 7,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "associativity" : 8,
    "replacement_policy" : "nmru",
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 2200,    # A little less than the total cachelines to make it interesting
    "noninclusive_directory_associativity" : 8,
})
comp_scratch1 = sst.Component("scratch1", "memHierarchy.Scratchpad")
comp_scratch1.addParams({
    "debug" : debugScratch | debugCore1,
    "debug_level" : 5,
    "clock" : core_clock,
    "size" : "64KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})
scratch1_conv = comp_scratch1.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch1_back = scratch1_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch1_back.addParam("access_time", "10ns")
scratchlink1 = comp_scratch1.setSubComponent("cpulink", "memHierarchy.MemLink")
scratchnic1 = comp_scratch1.setSubComponent("memlink", "memHierarchy.MemNIC")
scratchnic1.addParams({
    "network_bw" : "50GB/s",
    "group" : 0
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
    "num_ports" : 4
})
comp_net.setSubComponent("topology","merlin.singlerouter")

memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
memctrl0.addParams({
    #"debug" : "1",
    #"debug_level" : 10,
    "backing" : "none",
    "clock" : "1GHz",
    "addr_range_start" : 0,
    "interleave_size" : "128B",
    "interleave_step" : "256B",
})
memory0 = memctrl0.setSubComponent("backend", "memHierarchy.simpleDRAM")
memory0.addParams({
    "access_time" : "50ns",
    "mem_size" : "512MiB",
})
memnic0 = memctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
memnic0.addParam("network_bw", "50GB/s")
memnic0.addParam("group", "1")

memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
memctrl1.addParams({
    #"debug" : "1",
    #"debug_level" : 10,
    "backing" : "none",
    "clock" : "1GHz",
    "addr_range_start" : 128,
    "interleave_size" : "128B",
    "interleave_step" : "256B"
})
memory1 = memctrl1.setSubComponent("backend", "memHierarchy.simpleDRAM")
memory1.addParams({
    "access_time" : "50ns",
    "mem_size" : "512MiB",
})
memnic1 = memctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
memnic1.addParam("network_bw", "50GB/s")
memnic1.addParam("group", "1")


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
link_l1_l2_0 = sst.Link("link_l1_l2_0")
link_l1_l2_0.connect( (comp_l1_0, "low_network_0", "100ps"), (comp_l2_0, "high_network_0", "100ps") )
link_l1_l2_1 = sst.Link("link_l1_l2_1")
link_l1_l2_1.connect( (comp_l1_1, "low_network_0", "100ps"), (comp_l2_1, "high_network_0", "100ps") )
link_l2_l3_0 = sst.Link("link_l2_l3_0")
link_l2_l3_0.connect( (comp_l2_0, "low_network_0", "100ps"), (l3_0, "high_network_0", "100ps") )
link_l2_l3_1 = sst.Link("link_l2_l3_1")
link_l2_l3_1.connect( (comp_l2_1, "low_network_0", "100ps"), (l3_1, "high_network_0", "100ps") )
link_l2_scratch0 = sst.Link("link_cpu0_scratch0")
link_l2_scratch0.connect( (l3_0, "low_network_0", "100ps"), (scratchlink0, "port", "100ps") )
link_l2_scratch1 = sst.Link("link_cpu1_scratch1")
link_l2_scratch1.connect( (l3_1, "low_network_0", "100ps"), (scratchlink1, "port", "100ps") )
link_scratch0_net = sst.Link("link_scratch0_net")
link_scratch0_net.connect( (scratchnic0, "port", "100ps"), (comp_net, "port0", "100ps") )
link_scratch1_net = sst.Link("link_scratch1_net")
link_scratch1_net.connect( (scratchnic1, "port", "100ps"), (comp_net, "port1", "100ps") )
link_mem0_net = sst.Link("link_mem0_net")
link_mem0_net.connect( (memnic0, "port", "100ps"), (comp_net, "port2", "100ps") )
link_mem1_net = sst.Link("link_mem1_net")
link_mem1_net.connect( (memnic1, "port", "100ps"), (comp_net, "port3", "100ps") )
# End of generated output.
