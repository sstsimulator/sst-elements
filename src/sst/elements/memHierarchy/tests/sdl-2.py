# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({
      "do_write" : "1",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000"
})
iface = cpu.setSubComponent("memory", "memHierarchy.memInterface")

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "32",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL,
      "L1" : "1",
      "cache_size" : "2 KB"
})

# Explicitly set the link subcomponents instead of having cache figure them out based on connected port names
l1toC = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1toM = l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

# Memory controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "request_width" : "32",
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "addr_range_end" : 512*1024*1024-1,
})
Mtol1 = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

# Memory model
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "20ns",
      "mem_size" : "512MiB",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "port", "1000ps"), (l1toC, "port", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l1toM, "port", "50ps"), (Mtol1, "port", "50ps") )

# End of generated output.
