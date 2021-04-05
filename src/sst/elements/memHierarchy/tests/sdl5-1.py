# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0
DEBUG_CORE0 = 0
DEBUG_CORE1 = 0
DEBUG_CORE2 = 0
DEBUG_CORE3 = 0
DEBUG_NODE0 = 0
DEBUG_NODE1 = 0

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.memInterface")
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0,
      "cache_size" : "4 KB"
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.memInterface")
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE0 | DEBUG_NODE0,
      "cache_size" : "4 KB"
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
      "debug_level" : "10",
      "debug" : DEBUG_L2 | DEBUG_NODE0,
      "cache_size" : "32 KB"
})
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
comp_cpu2.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface2 = comp_cpu2.setSubComponent("memory", "memHierarchy.memInterface")
comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE2 | DEBUG_NODE1,
      "cache_size" : "4 KB"
})
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
comp_cpu3.addParams({
      "memSize" : "0x1000",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface3 = comp_cpu3.setSubComponent("memory", "memHierarchy.memInterface")
comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "L1" : "1",
      "debug" : DEBUG_L1 | DEBUG_CORE3 | DEBUG_NODE1,
      "cache_size" : "4 KB"
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
      "debug_level" : "10",
      "debug" : DEBUG_L2 | DEBUG_NODE1,
      "cache_size" : "32 KB"
})
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "debug_level" : "10",
      "debug" : DEBUG_L3,
      "cache_size" : "64 KB"
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : "10",
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.dramsim")
memory.addParams({
      "mem_size" : "512MiB",
      "access_time" : "1000 ns",
      "system_ini" : "system.ini",
      "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (iface0, "port", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0_l1cache_l2cache_link = sst.Link("link_c0_l1cache_l2cache_link")
link_c0_l1cache_l2cache_link.connect( (comp_c0_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (iface1, "port", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1_l1cache_l2cache_link = sst.Link("link_c1_l1cache_l2cache_link")
link_c1_l1cache_l2cache_link.connect( (comp_c1_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_1", "10000ps") )
link_n0_bus_l2cache = sst.Link("link_n0_bus_l2cache")
link_n0_bus_l2cache.connect( (comp_n0_bus, "low_network_0", "10000ps"), (comp_n0_l2cache, "high_network_0", "1000ps") )
link_n0_l2cache_l3cache = sst.Link("link_n0_l2cache_l3cache")
link_n0_l2cache_l3cache.connect( (comp_n0_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_0", "10000ps") )
link_cpu2_l1cache_link = sst.Link("link_cpu2_l1cache_link")
link_cpu2_l1cache_link.connect( (iface2, "port", "1000ps"), (comp_c2_l1cache, "high_network_0", "1000ps") )
link_c2_l1cache_l2cache_link = sst.Link("link_c2_l1cache_l2cache_link")
link_c2_l1cache_l2cache_link.connect( (comp_c2_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_0", "10000ps") )
link_cpu3_l1cache_link = sst.Link("link_cpu3_l1cache_link")
link_cpu3_l1cache_link.connect( (iface3, "port", "1000ps"), (comp_c3_l1cache, "high_network_0", "1000ps") )
link_c3_l1cache_l2cache_link = sst.Link("link_c3_l1cache_l2cache_link")
link_c3_l1cache_l2cache_link.connect( (comp_c3_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_1", "10000ps") )
link_n1_bus_l2cache = sst.Link("link_n1_bus_l2cache")
link_n1_bus_l2cache.connect( (comp_n1_bus, "low_network_0", "10000ps"), (comp_n1_l2cache, "high_network_0", "1000ps") )
link_n1_l2cache_l3cache = sst.Link("link_n1_l2cache_l3cache")
link_n1_l2cache_l3cache.connect( (comp_n1_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "10000ps"), (l3cache, "high_network_0", "10000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l3cache, "low_network_0", "10000ps"), (memctrl, "direct_link", "10000ps") )
# End of generated output.
