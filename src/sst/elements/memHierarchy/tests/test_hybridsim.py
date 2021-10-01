# Automatically generated SST Python input
import sst
import os

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "100000ns")

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "workPerCycle" : """1000""",
      "do_write" : """1""",
      "commFreq" : """100""",
      "memSize" : """0x1000"""
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : """lru""",
      "coherence_protocol" : """MSI""",
      "associativity" : """4""",
      "cache_line_size" : """64""",
      "cache_size" : """4 KB""",
      "L1" : """1""",
      "debug" : """0"""
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "workPerCycle" : """1000""",
      "do_write" : """1""",
      "commFreq" : """100""",
      "memSize" : """0x1000"""
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : """lru""",
      "coherence_protocol" : """MSI""",
      "associativity" : """4""",
      "cache_line_size" : """64""",
      "cache_size" : """4 KB""",
      "L1" : """1""",
      "debug" : """0"""
})
comp_bus = sst.Component("bus", "memHierarchy.Bus")
comp_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "access_latency_cycles" : """8""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : """lru""",
      "coherence_protocol" : """MSI""",
      "associativity" : """4""",
      "cache_line_size" : """64""",
      "cache_size" : """32 KB""",
      "L1" : """0""",
      "debug" : """0""",
      "mshr_num_entries" : """4096"""
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : """0""",
      "clock" : """1GHz""",
      "access_time" : """1000 ns""",
      "addr_range_start" : 0,
})
comp_hybridsim = comp_memory.setSubComponent("backend", "memHierarchy.hybrdisim")
comp_hybridsim.addParams({
      "access_time" : "1000 ns",
      "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
      "system_ini" : os.environ['SST_HYBRIDSIM_LIB_DIR'] + '/ini/hybridsim.ini',
      "mem_size" : "512MiB",
})

# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0_l1cache_bus_link = sst.Link("link_c0_l1cache_bus_link")
link_c0_l1cache_bus_link.connect( (comp_c0_l1cache, "low_network_0", "10000ps"), (comp_bus, "high_network_0", "10000ps") )
link_cpu1_l1cache_link = sst.Link("link_cpu1_l1cache_link")
link_cpu1_l1cache_link.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1_l1cache_bus_link = sst.Link("link_c1_l1cache_bus_link")
link_c1_l1cache_bus_link.connect( (comp_c1_l1cache, "low_network_0", "10000ps"), (comp_bus, "high_network_1", "10000ps") )
link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (comp_bus, "low_network_0", "10000ps"), (comp_l2cache, "high_network_0", "10000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l2cache, "low_network_0", "10000ps"), (comp_memory, "direct_link", "10000ps") )
# End of generated output.
