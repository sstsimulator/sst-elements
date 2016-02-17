# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "500000ns")

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
comp_cpu2.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
comp_cpu3.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : ""
})
comp_cpu4 = sst.Component("cpu4", "memHierarchy.trivialCPU")
comp_cpu4.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c4_l1cache = sst.Component("c4.l1cache", "memHierarchy.Cache")
comp_c4_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_cpu5 = sst.Component("cpu5", "memHierarchy.trivialCPU")
comp_cpu5.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c5_l1cache = sst.Component("c5.l1cache", "memHierarchy.Cache")
comp_c5_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_cpu6 = sst.Component("cpu6", "memHierarchy.trivialCPU")
comp_cpu6.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c6_l1cache = sst.Component("c6.l1cache", "memHierarchy.Cache")
comp_c6_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_cpu7 = sst.Component("cpu7", "memHierarchy.trivialCPU")
comp_cpu7.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
})
comp_c7_l1cache = sst.Component("c7.l1cache", "memHierarchy.Cache")
comp_c7_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : ""
})
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : ""
})
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_l3cache = sst.Component("l3cache", "memHierarchy.Cache")
comp_l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : "1",
      "debug_level" : "1",
      "network_address" : "1",
      "network_bw" : "25GB/s"
})
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "id" : "0",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "link_bw" : "1GB/s",
      "topology" : "merlin.singlerouter"
})
comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "",
      "network_address" : "0",
      "entry_cache_size" : "8192",
      "network_bw" : "25GB/s",
      "addr_range_start" : "0x0",
      "backing_store_size" : "0x1000000",
      "addr_range_end" : "0x1F000000"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "",
      "backend.access_time" : "100 ns",
      "clock" : "1GHz",
      "backend.mem_size" : "512"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_c0l1cache_link = sst.Link("link_c0l1cache_link")
link_c0l1cache_link.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0l1cache_bus = sst.Link("link_c0l1cache_bus")
link_c0l1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_0", "10000ps") )
link_c1l1cache_link = sst.Link("link_c1l1cache_link")
link_c1l1cache_link.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1l1cache_bus = sst.Link("link_c1l1cache_bus")
link_c1l1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_1", "10000ps") )
link_c2l1cache_link = sst.Link("link_c2l1cache_link")
link_c2l1cache_link.connect( (comp_cpu2, "mem_link", "1000ps"), (comp_c2_l1cache, "high_network_0", "1000ps") )
link_c2l1cache_bus = sst.Link("link_c2l1cache_bus")
link_c2l1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_2", "10000ps") )
link_c3l1cache_link = sst.Link("link_c3l1cache_link")
link_c3l1cache_link.connect( (comp_cpu3, "mem_link", "1000ps"), (comp_c3_l1cache, "high_network_0", "1000ps") )
link_c3l1cache_bus = sst.Link("link_c3l1cache_bus")
link_c3l1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_3", "10000ps") )
link_n0bus_l2cache = sst.Link("link_n0bus_l2cache")
link_n0bus_l2cache.connect( (comp_n0_bus, "low_network_0", "10000ps"), (comp_n0_l2cache, "high_network_0", "10000ps") )
link_n0l2cache_bus = sst.Link("link_n0l2cache_bus")
link_n0l2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_0", "10000ps") )
link_c4l1cache_link = sst.Link("link_c4l1cache_link")
link_c4l1cache_link.connect( (comp_cpu4, "mem_link", "1000ps"), (comp_c4_l1cache, "high_network_0", "1000ps") )
link_c4l1cache_bus = sst.Link("link_c4l1cache_bus")
link_c4l1cache_bus.connect( (comp_c4_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_0", "10000ps") )
link_c5l1cache_link = sst.Link("link_c5l1cache_link")
link_c5l1cache_link.connect( (comp_cpu5, "mem_link", "1000ps"), (comp_c5_l1cache, "high_network_0", "1000ps") )
link_c5l1cache_bus = sst.Link("link_c5l1cache_bus")
link_c5l1cache_bus.connect( (comp_c5_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_1", "10000ps") )
link_c6l1cache_link = sst.Link("link_c6l1cache_link")
link_c6l1cache_link.connect( (comp_cpu6, "mem_link", "1000ps"), (comp_c6_l1cache, "high_network_0", "1000ps") )
link_c6l1cache_bus = sst.Link("link_c6l1cache_bus")
link_c6l1cache_bus.connect( (comp_c6_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_2", "10000ps") )
link_c7l1cache_link = sst.Link("link_c7l1cache_link")
link_c7l1cache_link.connect( (comp_cpu7, "mem_link", "1000ps"), (comp_c7_l1cache, "high_network_0", "1000ps") )
link_c7l1cache_bus = sst.Link("link_c7l1cache_bus")
link_c7l1cache_bus.connect( (comp_c7_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_3", "10000ps") )
link_n1bus_l2cache = sst.Link("link_n1bus_l2cache")
link_n1bus_l2cache.connect( (comp_n1_bus, "low_network_0", "10000ps"), (comp_n1_l2cache, "high_network_0", "10000ps") )
link_n1l2cache_bus = sst.Link("link_n1l2cache_bus")
link_n1l2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_1", "10000ps") )
link_n2bus_l3cache = sst.Link("link_n2bus_l3cache")
link_n2bus_l3cache.connect( (comp_n2_bus, "low_network_0", "10000ps"), (comp_l3cache, "high_network_0", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "2000ps"), (comp_dirctrl, "network", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "10000ps"), (comp_memory, "direct_link", "10000ps") )
# End of generated output.
