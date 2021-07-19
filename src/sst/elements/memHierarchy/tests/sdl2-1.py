# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
verbose = 2

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface = cpu.setSubComponent("memory", "memHierarchy.memInterface")
l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
    "access_latency_cycles" : "4",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "cache_size" : "2 KiB",
    "L1" : "1",
    "verbose" : verbose,
    "debug" : DEBUG_L1,
    "debug_level" : "10"
})
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
    "access_latency_cycles" : "10",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "16 KiB",
    "verbose" : verbose,
    "debug" : DEBUG_L2,
    "debug_level" : "10"
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "backend.access_time" : "100 ns",
    "verbose" : verbose,
    "debug" : DEBUG_MEM,
    "debug_level" : "10",
    "addr_range_end" : 512*1024*1024-1,
})
    
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "100 ns",
    "mem_size" : "512MiB",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache_link")
link_cpu_l1cache.connect( (iface, "port", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_l1cache_l2cache = sst.Link("link_l1cache_l2cache_link")
link_l1cache_l2cache.connect( (l1cache, "low_network_0", "10000ps"), (l2cache, "high_network_0", "1000ps") )
link_mem_bus = sst.Link("link_mem_bus_link")
link_mem_bus.connect( (l2cache, "low_network_0", "10000ps"), (memctrl, "direct_link", "10000ps") )
# End of generated output.
