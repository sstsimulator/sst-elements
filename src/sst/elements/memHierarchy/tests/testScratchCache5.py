# Automatically generated SST Python input
import sst

# Global variables
debugScratch = 1
debugL1 = 1
debugL2 = 1
debugDir = 1
debugCore0 = 1
debugCore1 = 1
DEBUG_MEM = 0

# Parameters
core_clock = "2GHz"
scratchSize = "16384" # Bytes
scratchLine = 8
memLine = 64

# Define the simulation components
# ---------- CPUs ---------- #
comp_cpu0 = sst.Component("cpu0", "memHierarchy.ScratchCPU")
comp_cpu0.addParams({
    "scratchSize" : scratchSize,   # 64K scratch
    "maxAddr" : 2097152,       # 2M mem
    "scratchLineSize" : scratchLine,
    "memLineSize" :memLine,
    "clock" : core_clock,
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 1000,
    "verbose" : 1,
    "rngseed" : 11
})
comp_scr_inter0 = comp_cpu0.setSubComponent("scratchpad", "memHierarchy.scratchInterface")
comp_scr_inter0.addParams({
    "scratchpad_size" : str(scratchSize) + "B"
})
comp_ca_inter0 = comp_cpu0.setSubComponent("cache", "memHierarchy.memInterface")

comp_cpu1 = sst.Component("cpu1", "memHierarchy.ScratchCPU")
comp_cpu1.addParams({
    "scratchSize" : scratchSize,   # 64K scratch
    "maxAddr" : 2097152,       # 2M mem
    "scratchLineSize" : scratchLine,
    "memLineSize" : memLine,
    "clock" : core_clock,
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 1000,
    "verbose" : 1,
    "rngseed" : 10
})

comp_scr_inter1 = comp_cpu1.setSubComponent("scratchpad", "memHierarchy.scratchInterface")
comp_scr_inter1.addParams({
    "scratchpad_size" : str(scratchSize) + "B"
})
comp_ca_inter1 = comp_cpu1.setSubComponent("cache", "memHierarchy.memInterface")

# -------- Scratch --------- #
comp_scratch0 = sst.Component("scratch0", "memHierarchy.Scratchpad")
comp_scratch0.addParams({
    "debug" : debugScratch | debugCore0,
    "debug_level" : 10,
    "clock" : core_clock,
    "size" : str(scratchSize) + "B",
    "scratch_line_size" : scratchLine,
    "memory_line_size" : memLine,
    "backing" : "none",
    "backendConvertor" : "memHierarchy.simpleMemScratchBackendConvertor",
    "backendConvertor.backend" : "memHierarchy.simpleMem",
    "backendConvertor.backend.access_time" : "10ns",
})

comp_scratch1 = sst.Component("scratch1", "memHierarchy.Scratchpad")
comp_scratch1.addParams({
    "debug" : debugScratch | debugCore0,
    "debug_level" : 10,
    "clock" : core_clock,
    "size" : str(scratchSize) + "B",
    "scratch_line_size" : scratchLine,
    "memory_line_size" : memLine,
    "backing" : "none",
    "backendConvertor" : "memHierarchy.simpleMemScratchBackendConvertor",
    "backendConvertor.backend" : "memHierarchy.simpleMem",
    "backendConvertor.backend.access_time" : "10ns",
})

# ---------- L1s ----------- #
comp_l1_0 = sst.Component("l1_0", "memHierarchy.Cache")
comp_l1_0.addParams({
    "debug" : debugL1 | debugCore0,
    "debug_level" : 10,
    "cache_frequency" : core_clock,
    "cache_size" : "64KiB",
    "access_latency_cycles" : 4,
    "coherence_protocol" : "MESI",
    "cache_line_size" : memLine,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})
comp_l1_1 = sst.Component("l1_1", "memHierarchy.Cache")
comp_l1_1.addParams({
    "debug" : debugL1 | debugCore0,
    "debug_level" : 10,
    "cache_frequency" : core_clock,
    "cache_size" : "64KiB",
    "access_latency_cycles" : 4,
    "coherence_protocol" : "MESI",
    "cache_line_size" : memLine,
    "L1" : 1,
    "associativity" : 4,
    "replacement_policy" : "lru",
})

# --------- Buses ---------- #
comp_bus0 = sst.Component("bus0", "memHierarchy.Bus")
comp_bus0.addParams({ "bus_frequency" : core_clock })
comp_bus1 = sst.Component("bus1", "memHierarchy.Bus")
comp_bus1.addParams({ "bus_frequency" : core_clock })
comp_bus2 = sst.Component("bus2", "memHierarchy.Bus")
comp_bus2.addParams({ "bus_frequency" : core_clock })

# ----------- L2 ----------- #
comp_l2 = sst.Component("l2", "memHierarchy.Cache")
comp_l2.addParams({
    "debug" : debugL2 | debugCore0,
    "debug_level" : 10,
    "verbose" : 2,
    "cache_frequency" : core_clock,
    "cache_size" : "1MiB",
    "access_latency_cycles" : 5,
    "coherence_protocol" : "MESI",
    "cache_line_size" : memLine,
    "associativity" : 8,
    "replacement_policy" : "lru",
    "memNIC.network_bw" : "80GiB/s",
    "memNIC.group" : 0,
})

# ---------- Dir ----------- #
comp_dir = sst.Component("dir", "memHierarchy.DirectoryController")
comp_dir.addParams({
    "debug" : debugDir | debugCore0 | debugCore1,
    "debug_level" : 10,
    "verbose" : 2,
    "entry_cache_size" : 1024,
    "coherence_protocol" : "MESI",
    "memNIC.network_bw" : "50GiB/s",
    "memNIC.addr_range_start" : 0,
    "memNIC.group" : 1,
    "net_memory_name" : "mem"
})

# ---------- Mem ----------- #
comp_mem = sst.Component("mem", "memHierarchy.MemController")
comp_mem.addParams({
      "debug" : "1",
      "debug_level" : 10,
      "backing" : "none",
      "clock" : "1GHz",
      "verbose" : 2,
      #"backendConvertor.debug_location" : 1,
      #"backendConvertor.debug_level" : 10,
      "backend.access_time" : "50ns",
      "backend.mem_size" : "512MiB",
      "memNIC.network_bw" : "50GB/s",
      "memNIC.addr_range_start" : 0,
      #"memNIC.interleave_size" : "128B",
      #"memNIC.interleave_step" : "256B",
      "memNIC.group" : 2,
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

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Scratchpad")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")


# Define the simulation links
link_cpu0_bus = sst.Link("link_cpu0_bus")
link_cpu0_bus.connect( (comp_ca_inter0, "port", "100ps"), (comp_bus0, "high_network_0", "100ps") )

link_cpu0_scr = sst.Link("link_cpu0_scr")
link_cpu0_scr.connect( (comp_scr_inter0, "port", "100ps"), (comp_scratch0, "cpu", "100ps") )

link_bus0_scr = sst.Link("link_bus0_scr")
link_bus0_scr.connect( (comp_bus0, "high_network_1", "100ps"), (comp_scratch0, "memory", "100ps") )

link_l1_bus0 = sst.Link("link_bus0_l1")
link_l1_bus0.connect( (comp_l1_0, "high_network_0", "100ps"), (comp_bus0, "low_network_0", "100ps") )

link_cpu1_bus = sst.Link("link_cpu1_bus")
link_cpu1_bus.connect( (comp_ca_inter1, "port", "100ps"), (comp_bus1, "high_network_0", "100ps") )

link_cpu1_scr = sst.Link("link_cpu1_scr")
link_cpu1_scr.connect( (comp_scr_inter1, "port", "100ps"), (comp_scratch1, "cpu", "100ps") )

link_bus1_scr = sst.Link("link_bus1_scr")
link_bus1_scr.connect( (comp_bus1, "high_network_1", "100ps"), (comp_scratch1, "memory", "100ps") )

link_bus1_l1 = sst.Link("link_bus1_l1")
link_bus1_l1.connect( (comp_l1_1, "high_network_0", "100ps"), (comp_bus1, "low_network_0", "100ps") )

link_l1_l2_0 = sst.Link("link_l1_l2_0")
link_l1_l2_1 = sst.Link("link_l1_l2_1")
link_l1_l2_0.connect( (comp_l1_0, "low_network_0", "100ps"), (comp_bus2, "high_network_0", "100ps") )
link_l1_l2_1.connect( (comp_l1_1, "low_network_0", "100ps"), (comp_bus2, "high_network_1", "100ps") )

link_l2_bus = sst.Link("link_l2_bus")
link_l2_bus.connect( (comp_bus2, "low_network_0", "100ps"), (comp_l2, "high_network_0", "100ps") )

link_l2_net = sst.Link("link_l2_net")
link_l2_net.connect( (comp_l2, "directory", "100ps"), (comp_net, "port0", "100ps") )

link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (comp_dir, "network", "100ps"), (comp_net, "port1", "100ps") )

link_mem_net = sst.Link("link_mem_net")
link_mem_net.connect( (comp_mem, "network", "100ps"), (comp_net, "port2", "100ps") )



# End of generated output.
