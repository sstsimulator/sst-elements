import sst
from mhlib import componentlist

DEBUG_SCRATCH = 0
DEBUG_MEM = 0

# Define the simulation components
comp_cpu = sst.Component("core", "memHierarchy.ScratchCPU")
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
iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")
comp_scratch = sst.Component("scratch", "memHierarchy.Scratchpad")
comp_scratch.addParams({
    "debug" : DEBUG_SCRATCH,
    "debug_level" : 10,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 64,
    "backing" : "none",
})
scratch_conv = comp_scratch.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch_back = scratch_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch_back.addParams({
    "access_time" : "10ns",
})
scratch_conv.addParams({
    "debug_location" : 0,
    "debug_level" : 10,
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "clock" : "1GHz",
      "addr_range_start" : 0,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000 ns",
    "mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu_scratch = sst.Link("link_cpu_scratch")
link_cpu_scratch.connect( (iface, "lowlink", "1000ps"), (comp_scratch, "highlink", "1000ps") )
link_scratch_mem = sst.Link("link_scratch_mem")
link_scratch_mem.connect( (comp_scratch, "lowlink", "100ps"), (memctrl, "highlink", "100ps") )
