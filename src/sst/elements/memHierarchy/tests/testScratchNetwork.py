import sst
from mhlib import componentlist

DEBUG_SCRATCH = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0

# Define the simulation components
comp_cpu0 = sst.Component("core0", "memHierarchy.ScratchCPU")
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
comp_cpu0.addParams({
    "scratchSize" : 1024,   # 1K scratch
    "maxAddr" : 4096,       # 4K mem
    "scratchLineSize" : 64,
    "memLineSize" : 128,
    "clock" : "1GHz",
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 500,
    "verbose" : 1,
    "rngseed" : 10
})
comp_scratch0 = sst.Component("scratch0", "memHierarchy.Scratchpad")
comp_scratch0.addParams({
    "debug" : DEBUG_SCRATCH | DEBUG_CORE0,
    "debug_level" : 5,
    "verbose" : 2,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})
scratch0_conv = comp_scratch0.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch0_back = scratch0_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch0_back.addParams({"access_time" : "10ns"})
scratch0_link = comp_scratch0.setSubComponent("cpulink", "memHierarchy.MemLink")
scratch0_nic = comp_scratch0.setSubComponent("memlink", "memHierarchy.MemNIC")
scratch0_nic.addParams({"network_bw" : "50GB/s", "group" : 0})

comp_cpu1 = sst.Component("core1", "memHierarchy.ScratchCPU")
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
comp_cpu1.addParams({
    "scratchSize" : 1024,   # 1K scratch
    "maxAddr" : 4096,       # 4K mem
    "scratchLineSize" : 64,
    "memLineSize" : 128,
    "clock" : "1GHz",
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 500,
    "verbose" : 1,
    "rngseed" : 2
})
comp_scratch1 = sst.Component("scratch1", "memHierarchy.Scratchpad")
comp_scratch1.addParams({
    "debug" : DEBUG_SCRATCH | DEBUG_CORE1,
    "debug_level" : 5,
    "verbose" : 2,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})
scratch1_conv = comp_scratch1.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch1_back = scratch1_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch1_back.addParams({"access_time" : "10ns"})
scratch1_link = comp_scratch1.setSubComponent("cpulink", "memHierarchy.MemLink")
scratch1_nic = comp_scratch1.setSubComponent("memlink", "memHierarchy.MemNIC")
scratch1_nic.addParams({"network_bw" : "50GB/s", "group" : 0})

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
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "backing" : "none",
    "clock" : "1GHz",
    "verbose" : 2,
    "addr_range_start" : 0,
    "interleave_size" : "128B",
    "interleave_step" : "256B",
})
memory0 = memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
memory0.addParams({
    "access_time" : "75ns",
    "mem_size" : "512MiB",
})
memnic0 = memctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
memnic0.addParams({
    "network_bw" : "50GB/s",
    "group" : 1
})


memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
memctrl1.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "verbose" : 2,
    "backing" : "none",
    "clock" : "1GHz",
    "addr_range_start" : 128,
    "interleave_size" : "128B",
    "interleave_step" : "256B"
})
memory1 = memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
memory1.addParams({
    "access_time" : "75ns",
    "mem_size" : "512MiB",
})
memnic1 = memctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
memnic1.addParams({
    "network_bw" : "50GB/s",
    "group" : 1
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu0_scratch0 = sst.Link("link_cpu0_scratch0")
link_cpu0_scratch0.connect( (iface0, "port", "1000ps"), (scratch0_link, "port", "1000ps") )
link_cpu0_scratch1 = sst.Link("link_cpu1_scratch1")
link_cpu0_scratch1.connect( (iface1, "port", "1000ps"), (scratch1_link, "port", "1000ps") )
link_scratch0_net = sst.Link("link_scratch0_net")
link_scratch0_net.connect( (scratch0_nic, "port", "100ps"), (comp_net, "port0", "100ps") )
link_scratch1_net = sst.Link("link_scratch1_net")
link_scratch1_net.connect( (scratch1_nic, "port", "100ps"), (comp_net, "port1", "100ps") )
link_mem0_net = sst.Link("link_mem0_net")
link_mem0_net.connect( (memnic0, "port", "100ps"), (comp_net, "port2", "100ps") )
link_mem1_net = sst.Link("link_mem1_net")
link_mem1_net.connect( (memnic1, "port", "100ps"), (comp_net, "port3", "100ps") )
