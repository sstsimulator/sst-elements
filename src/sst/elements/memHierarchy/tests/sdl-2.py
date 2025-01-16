import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# Define the simulation components
cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
    "memFreq" : 2,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "3.5GHz",
    "rngseed" : 2,
    "maxOutstanding" : 16,
    "opCount" : 2500,
    "reqsPerIssue" : 3,
    "write_freq" : 36, # 36% writes
    "read_freq" : 60,  # 60% reads
    "llsc_freq" : 4,   # 4% LLSC
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

l1cache = sst.Component("l1cache.mesi", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "nmru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "32",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL,
      "L1" : "1",
      "cache_size" : "2KiB"
})

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

# Memory model
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "24ns",
      "mem_size" : "512MiB",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "lowlink", "100ps"), (l1cache, "highlink", "100ps") )
link_mem_cache_link = sst.Link("link_mem_cache_link")
link_mem_cache_link.connect( (memctrl, "highlink", "50ps"), (l1cache, "lowlink", "50ps") )

