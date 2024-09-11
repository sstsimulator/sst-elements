import os
import sst

mh_debug_level=10
mh_debug=0

verbose = 2

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

cpu = sst.Component("core", "memHierarchy.standardCPU") # if this is a parameter, it determines whether this should be in memh or vanadis
cpu.addParams({
    "memFreq": 2,
    "memSize": "4KiB",
    "verbose": 0,
    "clock": "3.5GHz",
    "rngseed": 111,
    "maxOutstanding": 16,
    "opCount": 2500,
    "reqsPerIssue": 3,
    "write_freq": 36,
    "read_freq": 60,
    "llsc_Freq": 4
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

l1cache = sst.Component("l1cache.msi", "memHierarchy.Cache")
l1cache.addParams({
    "access_latency_cycles" : "3",
    "cache_frequency" : "3.5Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "debug" : DEBUG_L1,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
    "L1" : "1",
    "cache_size" : "2KiB"
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "verbose" : verbose,
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000ns",
    "mem_size" : "512MiB"
})

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")
protocol="MESI"

l2cacheParams = {
    "access_latency_cycles" : "14",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "16",
    "cache_line_size" : "64",
    "cache_size" : "1MB",
    "mshr_latency_cycles": 3,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

l3cacheParams = {
    
}

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")

# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (iface, "port", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l1cache, "low_network_0", "50ps"), (memctrl, "direct_link", "50ps") )
