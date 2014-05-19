# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "100ms")

# Define the simulation components
system = sst.Component("system", "m5C.M5")
system.addParams({
      "debug" : """0""",
      "memory_trace" : """0""",
      "info" : """no""",
      "registerExit" : """yes""",
      "frequency" : """2 Ghz""",
      "statFile" : """out.txt""",
      "configFile" : """exampleM5.xml""",
      "mem_initializer_port" : """core0-dcache"""
})
c0_l1Dcache = sst.Component("c0.l1Dcache", "memHierarchy.Cache")
c0_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c0_l1Icache = sst.Component("c0.l1Icache", "memHierarchy.Cache")
c0_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c1_l1Dcache = sst.Component("c1.l1Dcache", "memHierarchy.Cache")
c1_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c1_l1Icache = sst.Component("c1.l1Icache", "memHierarchy.Cache")
c1_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c2_l1Dcache = sst.Component("c2.l1Dcache", "memHierarchy.Cache")
c2_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c2_l1Icache = sst.Component("c2.l1Icache", "memHierarchy.Cache")
c2_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c3_l1Dcache = sst.Component("c3.l1Dcache", "memHierarchy.Cache")
c3_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c3_l1Icache = sst.Component("c3.l1Icache", "memHierarchy.Cache")
c3_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c4_l1Dcache = sst.Component("c4.l1Dcache", "memHierarchy.Cache")
c4_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c4_l1Icache = sst.Component("c4.l1Icache", "memHierarchy.Cache")
c4_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c5_l1Dcache = sst.Component("c5.l1Dcache", "memHierarchy.Cache")
c5_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c5_l1Icache = sst.Component("c5.l1Icache", "memHierarchy.Cache")
c5_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c6_l1Dcache = sst.Component("c6.l1Dcache", "memHierarchy.Cache")
c6_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c6_l1Icache = sst.Component("c6.l1Icache", "memHierarchy.Cache")
c6_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c7_l1Dcache = sst.Component("c7.l1Dcache", "memHierarchy.Cache")
c7_l1Dcache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
c7_l1Icache = sst.Component("c7.l1Icache", "memHierarchy.Cache")
c7_l1Icache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """1""",
      "cache_size" : """8 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """2""",
      "mshr_num_entries" : """4096""",
      "L1" : """1"""
})
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
      "debug" : """0""",
      "cache_frequency" : """2.0 Ghz""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "cache_size" : """64 KB""",
      "cache_line_size" : """64""",
      "low_network_links" : """1""",
      "access_latency_cycles" : """6""",
      "mshr_num_entries" : """4096""",
      "L1" : """0""",
      "high_network_links" : """1"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "debug" : """0""",
      "coherence_protocol" : """MSI""",
      "rangeStart" : """0""",
      "request_width" : """64""",
      "access_time" : """25 ns""",
      "mem_size" : """1024""",
      "clock" : """2GHz"""
})


# Define the simulation links
bus_l2cache = sst.Link("bus_l2cache")
bus_l2cache.connect( (bus, "low_network_0", "50ps"), (l2cache, "high_network_0", "50ps") )
c0dcache_bus_link = sst.Link("c0dcache_bus_link")
c0dcache_bus_link.connect( (c0_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_0", "50ps") )
c0icache_bus_link = sst.Link("c0icache_bus_link")
c0icache_bus_link.connect( (c0_l1Icache, "low_network_0", "50ps"), (bus, "high_network_1", "50ps") )
c1dcache_bus_link = sst.Link("c1dcache_bus_link")
c1dcache_bus_link.connect( (c1_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_2", "50ps") )
c1icache_bus_link = sst.Link("c1icache_bus_link")
c1icache_bus_link.connect( (c1_l1Icache, "low_network_0", "50ps"), (bus, "high_network_3", "50ps") )
c2dcache_bus_link = sst.Link("c2dcache_bus_link")
c2dcache_bus_link.connect( (c2_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_4", "50ps") )
c2icache_bus_link = sst.Link("c2icache_bus_link")
c2icache_bus_link.connect( (c2_l1Icache, "low_network_0", "50ps"), (bus, "high_network_5", "50ps") )
c3dcache_bus_link = sst.Link("c3dcache_bus_link")
c3dcache_bus_link.connect( (c3_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_6", "50ps") )
c3icache_bus_link = sst.Link("c3icache_bus_link")
c3icache_bus_link.connect( (c3_l1Icache, "low_network_0", "50ps"), (bus, "high_network_7", "50ps") )
c4dcache_bus_link = sst.Link("c4dcache_bus_link")
c4dcache_bus_link.connect( (c4_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_8", "50ps") )
c4icache_bus_link = sst.Link("c4icache_bus_link")
c4icache_bus_link.connect( (c4_l1Icache, "low_network_0", "50ps"), (bus, "high_network_9", "50ps") )
c5dcache_bus_link = sst.Link("c5dcache_bus_link")
c5dcache_bus_link.connect( (c5_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_10", "50ps") )
c5icache_bus_link = sst.Link("c5icache_bus_link")
c5icache_bus_link.connect( (c5_l1Icache, "low_network_0", "50ps"), (bus, "high_network_11", "50ps") )
c6dcache_bus_link = sst.Link("c6dcache_bus_link")
c6dcache_bus_link.connect( (c6_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_12", "50ps") )
c6icache_bus_link = sst.Link("c6icache_bus_link")
c6icache_bus_link.connect( (c6_l1Icache, "low_network_0", "50ps"), (bus, "high_network_13", "50ps") )
c7dcache_bus_link = sst.Link("c7dcache_bus_link")
c7dcache_bus_link.connect( (c7_l1Dcache, "low_network_0", "50ps"), (bus, "high_network_14", "50ps") )
c7icache_bus_link = sst.Link("c7icache_bus_link")
c7icache_bus_link.connect( (c7_l1Icache, "low_network_0", "50ps"), (bus, "high_network_15", "50ps") )
core0_dcache = sst.Link("core0_dcache")
core0_dcache.connect( (system, "core0-dcache", "1000ps"), (c0_l1Dcache, "high_network_0", "1000ps") )
core0_icache = sst.Link("core0_icache")
core0_icache.connect( (system, "core0-icache", "1000ps"), (c0_l1Icache, "high_network_0", "1000ps") )
core1_dcache = sst.Link("core1_dcache")
core1_dcache.connect( (system, "core1-dcache", "1000ps"), (c1_l1Dcache, "high_network_0", "1000ps") )
core1_icache = sst.Link("core1_icache")
core1_icache.connect( (system, "core1-icache", "1000ps"), (c1_l1Icache, "high_network_0", "1000ps") )
core2_dcache = sst.Link("core2_dcache")
core2_dcache.connect( (system, "core2-dcache", "1000ps"), (c2_l1Dcache, "high_network_0", "1000ps") )
core2_icache = sst.Link("core2_icache")
core2_icache.connect( (system, "core2-icache", "1000ps"), (c2_l1Icache, "high_network_0", "1000ps") )
core3_dcache = sst.Link("core3_dcache")
core3_dcache.connect( (system, "core3-dcache", "1000ps"), (c3_l1Dcache, "high_network_0", "1000ps") )
core3_icache = sst.Link("core3_icache")
core3_icache.connect( (system, "core3-icache", "1000ps"), (c3_l1Icache, "high_network_0", "1000ps") )
core4_dcache = sst.Link("core4_dcache")
core4_dcache.connect( (system, "core4-dcache", "1000ps"), (c4_l1Dcache, "high_network_0", "1000ps") )
core4_icache = sst.Link("core4_icache")
core4_icache.connect( (system, "core4-icache", "1000ps"), (c4_l1Icache, "high_network_0", "1000ps") )
core5_dcache = sst.Link("core5_dcache")
core5_dcache.connect( (system, "core5-dcache", "1000ps"), (c5_l1Dcache, "high_network_0", "1000ps") )
core5_icache = sst.Link("core5_icache")
core5_icache.connect( (system, "core5-icache", "1000ps"), (c5_l1Icache, "high_network_0", "1000ps") )
core6_dcache = sst.Link("core6_dcache")
core6_dcache.connect( (system, "core6-dcache", "1000ps"), (c6_l1Dcache, "high_network_0", "1000ps") )
core6_icache = sst.Link("core6_icache")
core6_icache.connect( (system, "core6-icache", "1000ps"), (c6_l1Icache, "high_network_0", "1000ps") )
core7_dcache = sst.Link("core7_dcache")
core7_dcache.connect( (system, "core7-dcache", "1000ps"), (c7_l1Dcache, "high_network_0", "1000ps") )
core7_icache = sst.Link("core7_icache")
core7_icache.connect( (system, "core7-icache", "1000ps"), (c7_l1Icache, "high_network_0", "1000ps") )
mem_bus_link = sst.Link("mem_bus_link")
mem_bus_link.connect( (l2cache, "low_network_0", "50ps"), (memory, "direct_link", "50ps") )
# End of generated output.
