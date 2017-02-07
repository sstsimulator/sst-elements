# Automatically generated SST Python input
import sst

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.ScratchCPU")
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
    #"debug" : 1,
    #"debug_level" : 5,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "do_not_back" : 1,
    "backendConvertor" : "memHierarchy.simpleMemScratchBackendConvertor",
    "backendConvertor.backend" : "memHierarchy.simpleMem",
    "backendConvertor.backend.access_time" : "10ns",
    "network_bw" : "50GB/s",
    "network_address" : 0
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.ScratchCPU")
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
    #"debug" : 1,
    #"debug_level" : 5,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "do_not_back" : 1,
    "backendConvertor" : "memHierarchy.simpleMemScratchBackendConvertor",
    "backendConvertor.backend" : "memHierarchy.simpleMem",
    "backendConvertor.backend.access_time" : "10ns",
    "network_bw" : "50GB/s",
    "network_address" : 1
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

comp_memory0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memory0.addParams({
      #"debug" : "1",
      #"debug_level" : 10,
      "do_not_back" : 1,
      "clock" : "1GHz",
      #"backendConvertor.debug_location" : 1,
      #"backendConvertor.debug_level" : 10,
      "backend.access_time" : "75ns",
      "backend.mem_size" : "512MiB",
      "memNIC.network_bw" : "50GB/s",
      "memNIC.network_address" : 2,
      "memNIC.range_start" : 0,
      "memNIC.interleave_size" : "128B",
      "memNIC.interleave_step" : "256B",
})
comp_memory1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memory1.addParams({
      #"debug" : "1",
      #"debug_level" : 10,
      "do_not_back" : 1,
      "backend.access_time" : "75ns",
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB",
      "memNIC.network_bw" : "50GB/s",
      "memNIC.network_address" : 3,
      "memNIC.range_start" : 128,
      "memNIC.interleave_size" : "128B",
      "memNIC.interleave_step" : "256B"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Scratchpad")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_cpu0_scratch0 = sst.Link("link_cpu0_scratch0")
link_cpu0_scratch0.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_scratch0, "cpu", "1000ps") )
link_cpu0_scratch1 = sst.Link("link_cpu1_scratch1")
link_cpu0_scratch1.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_scratch1, "cpu", "1000ps") )
link_scratch0_net = sst.Link("link_scratch0_net")
link_scratch0_net.connect( (comp_scratch0, "network", "100ps"), (comp_net, "port0", "100ps") )
link_scratch1_net = sst.Link("link_scratch1_net")
link_scratch1_net.connect( (comp_scratch1, "network", "100ps"), (comp_net, "port1", "100ps") )
link_mem0_net = sst.Link("link_mem0_net")
link_mem0_net.connect( (comp_memory0, "network", "100ps"), (comp_net, "port2", "100ps") )
link_mem1_net = sst.Link("link_mem1_net")
link_mem1_net.connect( (comp_memory1, "network", "100ps"), (comp_net, "port3", "100ps") )
# End of generated output.
