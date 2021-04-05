import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({
      "num_loadstore" : "10000",
      "commFreq" : "100",
      "memSize" : "0x100000",
      "noncacheableRangeStart" : "0",
      "do_write" : "1",
      "noncacheableRangeEnd" : "0x100"
})
iface = cpu.setSubComponent("memory", "memHierarchy.memInterface")

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "cache_size" : "4 KB"
})
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "debug" : DEBUG_L2,
      "cache_size" : "32 KB"
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "debug_level" : "8",
      "debug" : DEBUG_L3,
      "cache_size" : "64 KB"
})
memctrl = sst.Component("memctrl", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache")
link_cpu_l1cache.connect( (iface, "port", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_l1cache_l2cache = sst.Link("link_l1cache_l2cache")
link_l1cache_l2cache.connect( (l1cache, "low_network_0", "10000ps"), (l2cache, "high_network_0", "10000ps") )
link_l2cache_l3cache = sst.Link("link_l2cache_l3cache")
link_l2cache_l3cache.connect( (l2cache, "low_network_0", "10000ps"), (l3cache, "high_network_0", "10000ps") )
link_mem_bus = sst.Link("link_mem_bus")
link_mem_bus.connect( (l3cache, "low_network_0", "10000ps"), (memctrl, "direct_link", "10000ps") )
