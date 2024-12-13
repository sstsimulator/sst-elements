import sst
import os

from mhlib import Bus

# Define SST core options
sst.setProgramOption("stop-at", "100000ns")

# Define the simulation components
cpu_params = {
    "memFreq" : 10,
    "memSize" : "1MiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "maxOutstanding" : 16,
    "opCount" : 1000,
    "reqsPerIssue" : 2,
    "write_freq" : 40,  # 40% writes
    "read_freq" : 60,   # 60% reads
}

comp_cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
comp_cpu0.addParams(cpu_params)
comp_cpu0.addParams({
    "rngseed" : 0
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : 2,
      "cache_frequency" : "2GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
comp_cpu1.addParams(cpu_params)
comp_cpu1.addParams({
    "rngseed" : 0
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})

bus_params = { "bus_frequency" : "2GHz" }

comp_bus = Bus("bus", bus_params, "10000ps")

comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "access_latency_cycles" : "8",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "L1" : "0",
      "debug" : "0",
      "mshr_num_entries" : "4096"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : "0",
      "clock" : "1GHz",
      "access_time" : "1000 ns",
      "addr_range_start" : 0,
})
comp_hybridsim = comp_memory.setSubComponent("backend", "memHierarchy.hybridsim")
comp_hybridsim.addParams({
      "access_time" : "1000 ns",
      "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
      "system_ini" : os.environ['SST_HYBRIDSIM_LIB_DIR'] + '/ini/hybridsim.ini',
      "mem_size" : "512MiB",
})

subcomp_iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
subcomp_iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.standardInterface")

# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (subcomp_iface0, "lowlink", "1000ps"), (comp_c0_l1cache, "highlink", "1000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (subcomp_iface1, "lowlink", "1000ps"), (comp_c1_l1cache, "highlink", "1000ps") )

comp_bus.connect(highcomps=[comp_c0_l1cache,comp_c1_l1cache], lowcomps=[comp_l2cache])

link_mem = sst.Link("link_mem_link")
link_mem.connect( (comp_l2cache, "lowlink", "10000ps"), (comp_memory, "highlink", "10000ps") )
