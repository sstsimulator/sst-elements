# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "500000ns")

# Define the simulation components
cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
cpu0.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
c0_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
cpu1.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
c1_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
cpu2.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
c2_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
cpu3.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
c3_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
n0_l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu4 = sst.Component("cpu4", "memHierarchy.trivialCPU")
cpu4.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c4_l1cache = sst.Component("c4.l1cache", "memHierarchy.Cache")
c4_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu5 = sst.Component("cpu5", "memHierarchy.trivialCPU")
cpu5.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c5_l1cache = sst.Component("c5.l1cache", "memHierarchy.Cache")
c5_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu6 = sst.Component("cpu6", "memHierarchy.trivialCPU")
cpu6.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c6_l1cache = sst.Component("c6.l1cache", "memHierarchy.Cache")
c6_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
cpu7 = sst.Component("cpu7", "memHierarchy.trivialCPU")
cpu7.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "num_loadstore" : """1000""",
      "do_write" : """1""",
      "uncacheRangeStart" : """0""",
      "uncacheRangeEnd" : """0x100"""
})
c7_l1cache = sst.Component("c7.l1cache", "memHierarchy.Cache")
c7_l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
n1_l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """64 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """16""",
      "access_latency_cycles" : """100""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "directory_at_next_level" : """1""",
      "network_address" : """1"""
})
chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
      "num_ports" : """2""",
      "num_vcs" : """3""",
      "link_bw" : """5GHz""",
      "xbar_bw" : """5GHz""",
      "topology" : """merlin.singlerouter""",
      "id" : """0"""
})
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "network_address" : """0""",
      "network_bw" : """1GHz""",
      "addr_range_start" : """0x0""",
      "addr_range_end" : """0x1F000000""",
      "backing_store_size" : """0x1000000""",
      "entry_cache_size" : """8192"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "access_time" : """100 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz"""
})


# Define the simulation links
c0l1cache_bus = sst.Link("c0l1cache_bus")
c0l1cache_bus.connect( (c0_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_0", "10000ps") )
c0l1cache_link = sst.Link("c0l1cache_link")
c0l1cache_link.connect( (cpu0, "mem_link", "1000ps"), (c0_l1cache, "high_network_0", "1000ps") )
c1l1cache_bus = sst.Link("c1l1cache_bus")
c1l1cache_bus.connect( (c1_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_1", "10000ps") )
c1l1cache_link = sst.Link("c1l1cache_link")
c1l1cache_link.connect( (cpu1, "mem_link", "1000ps"), (c1_l1cache, "high_network_0", "1000ps") )
c2l1cache_bus = sst.Link("c2l1cache_bus")
c2l1cache_bus.connect( (c2_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_2", "10000ps") )
c2l1cache_link = sst.Link("c2l1cache_link")
c2l1cache_link.connect( (cpu2, "mem_link", "1000ps"), (c2_l1cache, "high_network_0", "1000ps") )
c3l1cache_bus = sst.Link("c3l1cache_bus")
c3l1cache_bus.connect( (c3_l1cache, "low_network_0", "10000ps"), (n0_bus, "high_network_3", "10000ps") )
c3l1cache_link = sst.Link("c3l1cache_link")
c3l1cache_link.connect( (cpu3, "mem_link", "1000ps"), (c3_l1cache, "high_network_0", "1000ps") )
c4l1cache_bus = sst.Link("c4l1cache_bus")
c4l1cache_bus.connect( (c4_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_0", "10000ps") )
c4l1cache_link = sst.Link("c4l1cache_link")
c4l1cache_link.connect( (cpu4, "mem_link", "1000ps"), (c4_l1cache, "high_network_0", "1000ps") )
c5l1cache_bus = sst.Link("c5l1cache_bus")
c5l1cache_bus.connect( (c5_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_1", "10000ps") )
c5l1cache_link = sst.Link("c5l1cache_link")
c5l1cache_link.connect( (cpu5, "mem_link", "1000ps"), (c5_l1cache, "high_network_0", "1000ps") )
c6l1cache_bus = sst.Link("c6l1cache_bus")
c6l1cache_bus.connect( (c6_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_2", "10000ps") )
c6l1cache_link = sst.Link("c6l1cache_link")
c6l1cache_link.connect( (cpu6, "mem_link", "1000ps"), (c6_l1cache, "high_network_0", "1000ps") )
c7l1cache_bus = sst.Link("c7l1cache_bus")
c7l1cache_bus.connect( (c7_l1cache, "low_network_0", "10000ps"), (n1_bus, "high_network_3", "10000ps") )
c7l1cache_link = sst.Link("c7l1cache_link")
c7l1cache_link.connect( (cpu7, "mem_link", "1000ps"), (c7_l1cache, "high_network_0", "1000ps") )
cache_net_0 = sst.Link("cache_net_0")
cache_net_0.connect( (l3cache, "directory", "10000ps"), (chiprtr, "port1", "10000ps") )
dir_mem_link = sst.Link("dir_mem_link")
dir_mem_link.connect( (dirctrl, "memory", "10000ps"), (memory, "direct_link", "10000ps") )
dir_net_0 = sst.Link("dir_net_0")
dir_net_0.connect( (chiprtr, "port0", "2000ps"), (dirctrl, "network", "2000ps") )
n0bus_l2cache = sst.Link("n0bus_l2cache")
n0bus_l2cache.connect( (n0_bus, "low_network_0", "10000ps"), (n0_l2cache, "high_network_0", "10000ps") )
n0l2cache_bus = sst.Link("n0l2cache_bus")
n0l2cache_bus.connect( (n0_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_0", "10000ps") )
n1bus_l2cache = sst.Link("n1bus_l2cache")
n1bus_l2cache.connect( (n1_bus, "low_network_0", "10000ps"), (n1_l2cache, "high_network_0", "10000ps") )
n1l2cache_bus = sst.Link("n1l2cache_bus")
n1l2cache_bus.connect( (n1_l2cache, "low_network_0", "10000ps"), (n2_bus, "high_network_1", "10000ps") )
n2bus_l3cache = sst.Link("n2bus_l3cache")
n2bus_l3cache.connect( (n2_bus, "low_network_0", "10000ps"), (l3cache, "high_network_0", "10000ps") )
# End of generated output.
