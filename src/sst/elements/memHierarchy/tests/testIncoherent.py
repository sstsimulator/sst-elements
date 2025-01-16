import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
DEBUG_LEV = 10

# Define the simulation components
comp_cpu = sst.Component("core", "memHierarchy.standardCPU")
comp_cpu.addParams({
    "clock" : "2GHz",
    "memFreq" : "4",
    "rngseed" : "101",
    "memSize" : "1MiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "none",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "2 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEV
})
comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "access_latency_cycles" : "10",
      "mshr_latency_cycles" : 2,
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "none",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "16 KB",
      "cache_type" : "noninclusive",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEV
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEV
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
link_cpu_l1cache_link = sst.Link("link_cpu_l1cache_link")
link_cpu_l1cache_link.connect( (iface, "lowlink", "1000ps"), (comp_l1cache, "highlink", "1000ps") )
link_l1cache_l2cache_link = sst.Link("link_l1cache_l2cache_link")
link_l1cache_l2cache_link.connect( (comp_l1cache, "lowlink", "10000ps"), (comp_l2cache, "highlink", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l2cache, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
