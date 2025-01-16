import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
verbose = 2

# Define the simulation components
cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
    "memFreq" : 2,
    "memSize" : "1KiB",
    "verbose" : 0,
    "clock" : "3.5GHz",
    "rngseed" : 111,
    "maxOutstanding" : 16,
    "opCount" : 2500,
    "reqsPerIssue" : 3,
    "write_freq" : 36, # 36% writes
    "read_freq" : 60,  # 60% reads
    "llsc_freq" : 4,   # 4% LLSC
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
l1cache = sst.Component("l1cache.msi", "memHierarchy.Cache")
l1cache.addParams({
    "access_latency_cycles" : "4",
    "cache_frequency" : "3Ghz",
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
l2cache = sst.Component("l2cache.msi.inclus", "memHierarchy.Cache")
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
link_cpu_l1cache.connect( (iface, "lowlink", "1000ps"), (l1cache, "highlink", "1000ps") )
link_l1cache_l2cache = sst.Link("link_l1cache_l2cache_link")
link_l1cache_l2cache.connect( (l1cache, "lowlink", "10000ps"), (l2cache, "highlink", "1000ps") )
link_mem_cache = sst.Link("link_mem_cache_link")
link_mem_cache.connect( (l2cache, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
