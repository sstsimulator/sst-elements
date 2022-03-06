# Automatically generated SST Python input
import sst
from mhlib import componentlist

# Global variables
debugScratch = 0
debugL1 = 0
debugCore0 = 0
debugCore1 = 0
debugNIC = 0
DEBUG_MEM = 0
core_clock = "2GHz"

#######################################################################################################################
# Define the simulation components
#######################################################################################################################
# CPU0 
# ---------------------------------------------------------------------------------------------------------------------
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
    "rngseed" : 11
})

# CPU0 uses the standardInterface to interface to the memory hierarchy
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
#######################################################################################################################


#######################################################################################################################
# CPU0 
# ---------------------------------------------------------------------------------------------------------------------
comp_l1_0 = sst.Component("l1_0", "memHierarchy.Cache")
comp_l1_0.addParams({
    #"debug" : debugL1 | debugCore0,
    "debug_level" : 10,
    "cache_frequency" : core_clock,
    "cache_size" : "4KiB",
    "access_latency_cycles" : 4,
    "coherence_protocol" : "MSI",
    "cache_line_size" : 64,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})
# The L1 uses default (direct) links to the CPU and scratchpad, so we don't explicitly specify their type
# (e.g., MemLink or MemNIC)
#######################################################################################################################

#######################################################################################################################
# Scratchpad0 
# ---------------------------------------------------------------------------------------------------------------------
comp_scratch0 = sst.Component("scratch0", "memHierarchy.Scratchpad")
comp_scratch0.addParams({
    "debug" : debugScratch | debugCore0,
    "debug_level" : 10,
    "clock" : core_clock,
    "size" : "64KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})

# Scratchpad0 uses the 'simpleMem' backend for computing access time
scratch0_conv = comp_scratch0.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch0_backend = scratch0_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch0_backend.addParam("access_time", "10ns")

# Scratchpad0 uses a MemLink on its link towards the CPU
scratch0_link_cpu = comp_scratch0.setSubComponent("cpulink", "memHierarchy.MemLink")

# Scratchpad0 uses a MemNIC on its link towards the lower memory system
scratch0_link_mem = comp_scratch0.setSubComponent("memlink", "memHierarchy.MemNIC")
scratch0_link_mem.addParam("network_bw", "50GB/s")
# We put scratchpads in group 0 and memories in group 1 so that routing is (scratchpads -> memories)
scratch0_link_mem.addParam("group", "0") # Sources are 'group - 1' and destinations are 'group + 1' 
scratch0_link_mem.addParam("debug", debugNIC)
scratch0_link_mem.addParam("debug_level", 10)
#######################################################################################################################


#######################################################################################################################
# CPU1
# ---------------------------------------------------------------------------------------------------------------------
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
    "rngseed" : 1
})

# CPU1 uses the standardInterface to interface to the memory hierarchy
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
#######################################################################################################################


#######################################################################################################################
# L1 for CPU1
# ---------------------------------------------------------------------------------------------------------------------
comp_l1_1 = sst.Component("l1_1", "memHierarchy.Cache")
comp_l1_1.addParams({
    "debug" : debugL1 | debugCore1,
    "debug_level" : 10,
    "cache_frequency" : core_clock,
    "cache_size" : "4KiB",
    "access_latency_cycles" : 4,
    "coherence_protocol" : "MESI",
    "cache_line_size" : 64,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})
#######################################################################################################################


#######################################################################################################################
# Scratchpad1
# ---------------------------------------------------------------------------------------------------------------------
comp_scratch1 = sst.Component("scratch1", "memHierarchy.Scratchpad")
comp_scratch1.addParams({
    "debug" : debugScratch | debugCore1,
    "debug_level" : 10,
    "clock" : core_clock,
    "size" : "64KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 128,
    "backing" : "none",
})

# Scratchpad1 uses the 'simpleMem' backend for computing access time
scratch1_conv = comp_scratch1.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch1_backend = scratch1_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch1_backend.addParam("access_time", "10ns")

# Scratchpad1 uses a MemLink on its link towards the CPU
scratch1_link_cpu = comp_scratch1.setSubComponent("cpulink", "memHierarchy.MemLink")

# Scratchpad1 uses a MemNIC on its link towards the lower memory system
scratch1_link_mem = comp_scratch1.setSubComponent("memlink", "memHierarchy.MemNIC")
scratch1_link_mem.addParam("network_bw", "50GB/s")
# We put scratchpads in group 0 and memories in group 1 so that routing is (scratchpads -> memories)
scratch1_link_mem.addParam("group", "0") # Sources are 'group - 1' and destinations are 'group + 1' 
scratch1_link_mem.addParam("debug", debugNIC)
scratch1_link_mem.addParam("debug_level", 10)
#######################################################################################################################



#######################################################################################################################
# The network-on-chip is modeled using the Merlin library's 'hr_router
# This is a crossbar

comp_net = sst.Component("network", "merlin.hr_router")
comp_net.addParams({
    "xbar_bw" : "50GB/s",
    "link_bw" : "50GB/s",
    "input_buf_size" : "1KiB",
    "output_buf_size" : "1KiB",
    "flit_size" : "72B",
    "id" : "0",
    "num_ports" : 4,
})
# We use a single router to model a crossbar, so topology is just 'merlin.singlerouter'
comp_net.setSubComponent("topology","merlin.singlerouter")
#######################################################################################################################


#######################################################################################################################
# Memory0
# Addresses are interleaved between memories at 128B granularity
memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
memctrl0.addParams({
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "backing" : "none",
      "clock" : "1GHz",
      "addr_range_start" : 0,
      "interleave_size" : "128B",
      "interleave_step" : "256B",
})

# The timing model for this memory is simpleMem (constant latency access)
memory0 = memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
memory0.addParams({
    "access_time" : "50ns",
    "mem_size" : "512MiB"
})

# The memory sits directly on the NoC so it uses a MemNIC on its link
memNIC0 = memctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
memNIC0.addParam("network_bw", "50GB/s")
# The source for the memory is the scratchpads (group 0) so we are group 0 + 1 = 1
memNIC0.addParam("group", "1") # Sources are 'group - 1' and destinations are 'group + 1' 
memNIC0.addParam("debug", debugNIC)
memNIC0.addParam("debug_level", 10)
#######################################################################################################################


#######################################################################################################################
# Memory1
# Addresses are interleaved between memories at 128B granularity
memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
memctrl1.addParams({
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "backing" : "none",
      "clock" : "1GHz",
      "addr_range_start" : 128,
      "interleave_size" : "128B",
      "interleave_step" : "256B"
})

# The timing model for this memory is simpleMem (constant latency access)
memory1 = memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
memory1.addParams({
    "access_time" : "50ns",
    "mem_size" : "512MiB"
})

# The memory sits directly on the NoC so it uses a MemNIC on its link
memNIC1 = memctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
memNIC1.addParam("network_bw", "50GB/s")
# The source for the memory is the scratchpads (group 0) so we are group 0 + 1 = 1
memNIC1.addParam("group", "1") # Sources are 'group - 1' and destinations are 'group + 1' 
memNIC1.addParam("debug", debugNIC)
memNIC1.addParam("debug_level", 10)
#######################################################################################################################


#######################################################################################################################
# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)
#######################################################################################################################


#######################################################################################################################
# Define the simulation links
#######################################################################################################################
# Connect CPU0 to L1_0
link_cpu0_l1 = sst.Link("link_cpu0_l1")
link_cpu0_l1.connect( (iface0, "port", "100ps"), (comp_l1_0, "high_network_0", "100ps") )
# Connect CPU1 to L1_1
link_cpu1_l1 = sst.Link("link_cpu1_l1")
link_cpu1_l1.connect( (iface1, "port", "100ps"), (comp_l1_1, "high_network_0", "100ps") )
# Connect L1_0 to Scratchpad0
link_l1_scratch0 = sst.Link("link_cpu0_scratch0")
link_l1_scratch0.connect( (comp_l1_0, "low_network_0", "100ps"), (scratch0_link_cpu, "port", "100ps") )
# Connect L1_1 to Scratchpad1
link_l1_scratch1 = sst.Link("link_cpu1_scratch1")
link_l1_scratch1.connect( (comp_l1_1, "low_network_0", "100ps"), (scratch1_link_cpu, "port", "100ps") )
# Connect Scratch0's MemNIC to network
link_scratch0_net = sst.Link("link_scratch0_net")
link_scratch0_net.connect( (scratch0_link_mem, "port", "100ps"), (comp_net, "port0", "100ps") )
# Connect Scratch1's MemNIC to network
link_scratch1_net = sst.Link("link_scratch1_net")
link_scratch1_net.connect( (scratch1_link_mem, "port", "100ps"), (comp_net, "port1", "100ps") )
# Connect Memory0's MemNIC to network
link_mem0_net = sst.Link("link_mem0_net")
link_mem0_net.connect( (memNIC0, "port", "100ps"), (comp_net, "port2", "100ps") )
# Connect Memory1's MemNIC to network
link_mem1_net = sst.Link("link_mem1_net")
link_mem1_net.connect( (memNIC1, "port", "100ps"), (comp_net, "port3", "100ps") )
#######################################################################################################################
