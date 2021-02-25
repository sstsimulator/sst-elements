# Automatically generated SST Python input
import sst
import os

noncache = 0

os.environ['OMP_NUM_THREADS']="8"
Executable = os.getenv('OMP_EXE', "ompbarrier/ompbarrier.x")

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "100ms")

# Define the simulation components
ariel_cpus = sst.Component("cpus", "ariel.ariel")
ariel_cpus.addParams({
    "verbose"           : 0,
    "clock"             : "2 Ghz",
    "maxcorequeue"      : 256,
    "maxissuepercycle"  : 4,
    "pipetimeout"       : 0,
    "corecount"         : 8,
    "arielmode"         : 1,
    "launchparamcount"  : 1,
    "launchparam0"      : "-ifeellucky",
    "memmgr.memorylevels"      : 1,
    "memmgr.pagecount0"        : 262144,
    "memmgr.defaultlevel"      : 0,
    "executable"        : Executable
})

comp_c0_l1Dcache = sst.Component("c0.l1Dcache", "memHierarchy.Cache")
comp_c0_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c1_l1Dcache = sst.Component("c1.l1Dcache", "memHierarchy.Cache")
comp_c1_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c2_l1Dcache = sst.Component("c2.l1Dcache", "memHierarchy.Cache")
comp_c2_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c3_l1Dcache = sst.Component("c3.l1Dcache", "memHierarchy.Cache")
comp_c3_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c4_l1Dcache = sst.Component("c4.l1Dcache", "memHierarchy.Cache")
comp_c4_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c5_l1Dcache = sst.Component("c5.l1Dcache", "memHierarchy.Cache")
comp_c5_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c6_l1Dcache = sst.Component("c6.l1Dcache", "memHierarchy.Cache")
comp_c6_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c7_l1Dcache = sst.Component("c7.l1Dcache", "memHierarchy.Cache")
comp_c7_l1Dcache.addParams({
      "debug" : "0",
      "force_noncacheable_reqs" : noncache,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_bus = sst.Component("bus", "memHierarchy.Bus")
comp_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "debug" : "0",
      "access_latency_cycles" : "6",
      "cache_frequency" : "2.0 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "mshr_num_entries" : "4096"
})
comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
      "debug" : "0",
      "clock" : "2GHz",
      "request_width" : "64",
})
comp_memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory.addParams({
    "mem_size" : "1024MiB",
    "access_time" : "25 ns",
})


# Define the simulation links
link_core0_dcache = sst.Link("link_core0_dcache")
link_core0_dcache.connect( (ariel_cpus, "cache_link_0", "500ps"), (comp_c0_l1Dcache, "high_network_0", "500ps") )
link_core1_dcache = sst.Link("link_core1_dcache")
link_core1_dcache.connect( (ariel_cpus, "cache_link_1", "500ps"), (comp_c1_l1Dcache, "high_network_0", "500ps") )
link_core2_dcache = sst.Link("link_core2_dcache")
link_core2_dcache.connect( (ariel_cpus, "cache_link_2", "500ps"), (comp_c2_l1Dcache, "high_network_0", "500ps") )
link_core3_dcache = sst.Link("link_core3_dcache")
link_core3_dcache.connect( (ariel_cpus, "cache_link_3", "500ps"), (comp_c3_l1Dcache, "high_network_0", "500ps") )
link_core4_dcache = sst.Link("link_core4_dcache")
link_core4_dcache.connect( (ariel_cpus, "cache_link_4", "500ps"), (comp_c4_l1Dcache, "high_network_0", "500ps") )
link_core5_dcache = sst.Link("link_core5_dcache")
link_core5_dcache.connect( (ariel_cpus, "cache_link_5", "500ps"), (comp_c5_l1Dcache, "high_network_0", "500ps") )
link_core6_dcache = sst.Link("link_core6_dcache")
link_core6_dcache.connect( (ariel_cpus, "cache_link_6", "500ps"), (comp_c6_l1Dcache, "high_network_0", "500ps") )
link_core7_dcache = sst.Link("link_core7_dcache")
link_core7_dcache.connect( (ariel_cpus, "cache_link_7", "500ps"), (comp_c7_l1Dcache, "high_network_0", "500ps") )
link_c0dcache_bus_link = sst.Link("link_c0dcache_bus_link")
link_c0dcache_bus_link.connect( (comp_c0_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_0", "100ps") )
link_c1dcache_bus_link = sst.Link("link_c1dcache_bus_link")
link_c1dcache_bus_link.connect( (comp_c1_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_1", "100ps") )
link_c2dcache_bus_link = sst.Link("link_c2dcache_bus_link")
link_c2dcache_bus_link.connect( (comp_c2_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_2", "100ps") )
link_c3dcache_bus_link = sst.Link("link_c3dcache_bus_link")
link_c3dcache_bus_link.connect( (comp_c3_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_3", "100ps") )
link_c4dcache_bus_link = sst.Link("link_c4dcache_bus_link")
link_c4dcache_bus_link.connect( (comp_c4_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_4", "100ps") )
link_c5dcache_bus_link = sst.Link("link_c5dcache_bus_link")
link_c5dcache_bus_link.connect( (comp_c5_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_5", "100ps") )
link_c6dcache_bus_link = sst.Link("link_c6dcache_bus_link")
link_c6dcache_bus_link.connect( (comp_c6_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_6", "100ps") )
link_c7dcache_bus_link = sst.Link("link_c7dcache_bus_link")
link_c7dcache_bus_link.connect( (comp_c7_l1Dcache, "low_network_0", "100ps"), (comp_bus, "high_network_7", "100ps") )
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (comp_bus, "low_network_0", "100ps"), (comp_l2cache, "high_network_0", "100ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l2cache, "low_network_0", "100ps"), (comp_memctrl, "direct_link", "100ps") )
# End of generated output.
