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

cpu_lsq = cpu.setSubComponent("lsq", "VanadisBasicLoadStoreQueue")
cpu_lsq.addParams({
    "verbose" : verbose,
    "address_mask" : 0xFFFFFFFF,
    "max_stores" : 16,
    "max_loads" : 8,
})

cpu_l1_cache_interface = cpu_lsq.setSubComponent("memory", "memHierarchy.standardInterface")

l1_d_cache = sst.Component("core.l1ecache.msi", "memHierarchy.Cache")
l1_d_cache.addParams({
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

l1_d_cache_to_cpu = l1_d_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1_d_cache_to_l2_cache = l1_d_cache.setSubComponent("l1memlink", "memHierarchy.MemLink")

l1_i_cache = sst.Component("core.l1icache", "memHierarcy.Cache")
l1_i_cache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
})

l1_i_cache_to_cpu = l1_i_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1_i_cache_to_l2_cache = l1_i_cache.setSubComponent("memlink", "memHierarchy.Cache")

l2_cache = sst.Component("l1cache.msi", "vanadis.Cache")
l2_cache.addParams({
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
})

l2_cache_to_l1_caches = l2_cache.setSubComponenet("cpulink", "memHierarchy.MemLink")
l2_cache_to_memory = l2_cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l2_cache_to_memory.addParams({ 
    "group" : 1,
    "network_bw" : "25GB/s" 
})

# L1 to L2 bus
l1_l2_cache_bus = l2_cache.setSubComponent("l1_l2_cache.bus", "memHierarchy.Bus")
l1_l2_cache_bus.addParams( { 
    "bus_frequency" : cpu_clock 
})

# data l1 -> bus
l1_d_cache_to_l2_cache_link = sst.Link("cpul1l2cache.l1_d_cache_to_l2_cache_link")
l1_d_cache_to_l2_cache_link.connect((l1_d_cache_to_l2_cache, "port", "1ns"), (l1_l2_cache_bus, "high_network_0", "1ns"))
l1_d_cache_to_l2_cache_link.setNoCut()

# instruction l1 -> bus
l1_i_cache_to_l2_cache_link = sst.Link("cpul1l2cache.l1_i_cache_to_l2_cache_link")
l1_i_cache_to_l2_cache_link.connect((l1_i_cache_to_l2_cache, "port", "1ns"), (l1_l2_cache_bus, "high_network_1", "1ns"))
l1_i_cache_to_l2_cache_link.setNoCut()

# bus to l2 cache
bus_l2_cache_link = sst.Link("cpul1l2cache.bus_l2_cache_link")
bus_l2_cache_link.connect((l1_l2_cache_bus, "port", "1ns"), (l2_cache_to_l1_caches, "port", "1ns"))
bus_l2_cache_link.setNoCut()

l3cacheParams = {
    "access_latency_cycles" : "20",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "16",
    "cache_line_size" : "64",
    "chache_size" : "1MB",
    "mshr_latency_cycles" : 5,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level
}

l3cache = sst.Component("l2cache.msi", "vanadis.Cache'")
l3cache.addparams(l3cacheParams)

busParams = { 
    "bus_frequency" : cpu_clock, 
}

l2memLinkParams = { 
    "group" : 1,
    "network_bw" : "25GB/s" 
}

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")

# Define the simulation links
link_cpu_l1cache_link = sst.Link("link_cpu_l1cache_link")
link_cpu_l1cache_link.connect( (iface, "port", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_mem_bus_l1cache = sst.Link("link_mem_bus_l1cache")
link_mem_bus_l1cache.connect( (l1cache, "low_network_0", "50ps"), (memctrl, "direct_link", "50ps") )

# Link the l2cache to the CPU interface
link_cpu_l2cache_link = sst.Link("link_cpu_l2cache_link")
link_cpu_l2cache_link.connect((iface, "port", "1ns"), (l2cache, "port", "1ns"))
link_mem_bus_l2cache = sst.Link("link_mem_bus_l2cache")
link_mem_bus_l2cache.connect( (l2cache, ) );