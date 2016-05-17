# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "100ms")

# Define the simulation components
comp_system = sst.Component("system", "m5C.M5")
comp_system.addParams({
      "info" : "no",
      "mem_initializer_port" : "core0-dcache",
      "configFile" : "exampleM5.xml",
      "frequency" : "2 Ghz",
      "statFile" : "out.txt",
      "debug" : "0",
      "memory_trace" : "0",
      "registerExit" : "yes"
})
comp_c0_l1Dcache = sst.Component("c0.l1Dcache", "memHierarchy.Cache")
comp_c0_l1Dcache.addParams({
      "debug" : "0",
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c0_l1Icache = sst.Component("c0.l1Icache", "memHierarchy.Cache")
comp_c0_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c1_l1Icache = sst.Component("c1.l1Icache", "memHierarchy.Cache")
comp_c1_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c2_l1Icache = sst.Component("c2.l1Icache", "memHierarchy.Cache")
comp_c2_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c3_l1Icache = sst.Component("c3.l1Icache", "memHierarchy.Cache")
comp_c3_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c4_l1Icache = sst.Component("c4.l1Icache", "memHierarchy.Cache")
comp_c4_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c5_l1Icache = sst.Component("c5.l1Icache", "memHierarchy.Cache")
comp_c5_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c6_l1Icache = sst.Component("c6.l1Icache", "memHierarchy.Cache")
comp_c6_l1Icache.addParams({
      "debug" : "0",
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
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "1",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8 KB"
})
comp_c7_l1Icache = sst.Component("c7.l1Icache", "memHierarchy.Cache")
comp_c7_l1Icache.addParams({
      "debug" : "0",
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
      "L1" : "0",
      "cache_size" : "64 KB",
      "mshr_num_entries" : "4096"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : "0",
      "coherence_protocol" : "MSI",
      "backend.mem_size" : "1024",
      "backend.access_time" : "25 ns",
      "clock" : "2GHz",
      "request_width" : "64",
      "rangeStart" : "0"
})


# Define the simulation links
link_core0_dcache = sst.Link("link_core0_dcache")
link_core0_dcache.connect( (comp_system, "core0-dcache", "1000ps"), (comp_c0_l1Dcache, "high_network_0", "1000ps") )
link_core0_icache = sst.Link("link_core0_icache")
link_core0_icache.connect( (comp_system, "core0-icache", "1000ps"), (comp_c0_l1Icache, "high_network_0", "1000ps") )
link_core1_dcache = sst.Link("link_core1_dcache")
link_core1_dcache.connect( (comp_system, "core1-dcache", "1000ps"), (comp_c1_l1Dcache, "high_network_0", "1000ps") )
link_core1_icache = sst.Link("link_core1_icache")
link_core1_icache.connect( (comp_system, "core1-icache", "1000ps"), (comp_c1_l1Icache, "high_network_0", "1000ps") )
link_core2_dcache = sst.Link("link_core2_dcache")
link_core2_dcache.connect( (comp_system, "core2-dcache", "1000ps"), (comp_c2_l1Dcache, "high_network_0", "1000ps") )
link_core2_icache = sst.Link("link_core2_icache")
link_core2_icache.connect( (comp_system, "core2-icache", "1000ps"), (comp_c2_l1Icache, "high_network_0", "1000ps") )
link_core3_dcache = sst.Link("link_core3_dcache")
link_core3_dcache.connect( (comp_system, "core3-dcache", "1000ps"), (comp_c3_l1Dcache, "high_network_0", "1000ps") )
link_core3_icache = sst.Link("link_core3_icache")
link_core3_icache.connect( (comp_system, "core3-icache", "1000ps"), (comp_c3_l1Icache, "high_network_0", "1000ps") )
link_core4_dcache = sst.Link("link_core4_dcache")
link_core4_dcache.connect( (comp_system, "core4-dcache", "1000ps"), (comp_c4_l1Dcache, "high_network_0", "1000ps") )
link_core4_icache = sst.Link("link_core4_icache")
link_core4_icache.connect( (comp_system, "core4-icache", "1000ps"), (comp_c4_l1Icache, "high_network_0", "1000ps") )
link_core5_dcache = sst.Link("link_core5_dcache")
link_core5_dcache.connect( (comp_system, "core5-dcache", "1000ps"), (comp_c5_l1Dcache, "high_network_0", "1000ps") )
link_core5_icache = sst.Link("link_core5_icache")
link_core5_icache.connect( (comp_system, "core5-icache", "1000ps"), (comp_c5_l1Icache, "high_network_0", "1000ps") )
link_core6_dcache = sst.Link("link_core6_dcache")
link_core6_dcache.connect( (comp_system, "core6-dcache", "1000ps"), (comp_c6_l1Dcache, "high_network_0", "1000ps") )
link_core6_icache = sst.Link("link_core6_icache")
link_core6_icache.connect( (comp_system, "core6-icache", "1000ps"), (comp_c6_l1Icache, "high_network_0", "1000ps") )
link_core7_dcache = sst.Link("link_core7_dcache")
link_core7_dcache.connect( (comp_system, "core7-dcache", "1000ps"), (comp_c7_l1Dcache, "high_network_0", "1000ps") )
link_core7_icache = sst.Link("link_core7_icache")
link_core7_icache.connect( (comp_system, "core7-icache", "1000ps"), (comp_c7_l1Icache, "high_network_0", "1000ps") )
link_c0dcache_bus_link = sst.Link("link_c0dcache_bus_link")
link_c0dcache_bus_link.connect( (comp_c0_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_0", "50ps") )
link_c0icache_bus_link = sst.Link("link_c0icache_bus_link")
link_c0icache_bus_link.connect( (comp_c0_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_1", "50ps") )
link_c1dcache_bus_link = sst.Link("link_c1dcache_bus_link")
link_c1dcache_bus_link.connect( (comp_c1_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_2", "50ps") )
link_c1icache_bus_link = sst.Link("link_c1icache_bus_link")
link_c1icache_bus_link.connect( (comp_c1_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_3", "50ps") )
link_c2dcache_bus_link = sst.Link("link_c2dcache_bus_link")
link_c2dcache_bus_link.connect( (comp_c2_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_4", "50ps") )
link_c2icache_bus_link = sst.Link("link_c2icache_bus_link")
link_c2icache_bus_link.connect( (comp_c2_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_5", "50ps") )
link_c3dcache_bus_link = sst.Link("link_c3dcache_bus_link")
link_c3dcache_bus_link.connect( (comp_c3_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_6", "50ps") )
link_c3icache_bus_link = sst.Link("link_c3icache_bus_link")
link_c3icache_bus_link.connect( (comp_c3_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_7", "50ps") )
link_c4dcache_bus_link = sst.Link("link_c4dcache_bus_link")
link_c4dcache_bus_link.connect( (comp_c4_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_8", "50ps") )
link_c4icache_bus_link = sst.Link("link_c4icache_bus_link")
link_c4icache_bus_link.connect( (comp_c4_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_9", "50ps") )
link_c5dcache_bus_link = sst.Link("link_c5dcache_bus_link")
link_c5dcache_bus_link.connect( (comp_c5_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_10", "50ps") )
link_c5icache_bus_link = sst.Link("link_c5icache_bus_link")
link_c5icache_bus_link.connect( (comp_c5_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_11", "50ps") )
link_c6dcache_bus_link = sst.Link("link_c6dcache_bus_link")
link_c6dcache_bus_link.connect( (comp_c6_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_12", "50ps") )
link_c6icache_bus_link = sst.Link("link_c6icache_bus_link")
link_c6icache_bus_link.connect( (comp_c6_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_13", "50ps") )
link_c7dcache_bus_link = sst.Link("link_c7dcache_bus_link")
link_c7dcache_bus_link.connect( (comp_c7_l1Dcache, "low_network_0", "50ps"), (comp_bus, "high_network_14", "50ps") )
link_c7icache_bus_link = sst.Link("link_c7icache_bus_link")
link_c7icache_bus_link.connect( (comp_c7_l1Icache, "low_network_0", "50ps"), (comp_bus, "high_network_15", "50ps") )
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (comp_bus, "low_network_0", "50ps"), (comp_l2cache, "high_network_0", "50ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l2cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
# End of generated output.
