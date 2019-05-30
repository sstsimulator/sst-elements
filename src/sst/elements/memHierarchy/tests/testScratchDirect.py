# Automatically generated SST Python input
import sst

DEBUG_SCRATCH = 0
DEBUG_MEM = 0

# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.ScratchCPU")
comp_cpu.addParams({
    "scratchSize" : 1024,   # 1K scratch
    "maxAddr" : 4096,       # 4K mem
    "scratchLineSize" : 64,
    "memLineSize" : 64,
    "clock" : "1GHz",
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 500,
    "verbose" : 1
})
iface = comp_cpu.setSubComponent("memory", "memHierarchy.scratchInterface")
iface.addParams({ "scratchpad_size" : "1KiB" })
comp_scratch = sst.Component("scratch", "memHierarchy.Scratchpad")
comp_scratch.addParams({
    "debug" : DEBUG_SCRATCH,
    "debug_level" : 10,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 64,
    "backing" : "none",
    "backendConvertor" : "memHierarchy.simpleMemScratchBackendConvertor",
    "backendConvertor.backend" : "memHierarchy.simpleMem",
    "backendConvertor.backend.access_time" : "10ns",
    "backendConvertor.debug_location" : 1,
    "backendConvertor.debug_level" : 10,
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "backend.access_time" : "1000 ns",
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Scratchpad")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_cpu_scratch = sst.Link("link_cpu_scratch")
link_cpu_scratch.connect( (iface, "port", "1000ps"), (comp_scratch, "cpu", "1000ps") )
link_scratch_mem = sst.Link("link_scratch_mem")
link_scratch_mem.connect( (comp_scratch, "memory", "100ps"), (comp_memory, "direct_link", "100ps") )
# End of generated output.
