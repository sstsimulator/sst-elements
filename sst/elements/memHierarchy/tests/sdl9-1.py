# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "200000ns")

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "do_write" : """1""",
      "num_loadstore" : """1000""",
      "uncachedRangeStart" : """0x0000""",
      "uncachedRangeEnd" : """0x0100"""
})
l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """4""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "debug_level" : """8""",
      "statistics" : """1"""
})
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """16 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """10""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "debug_level" : """8""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "access_time" : """1000 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz"""
})


# Define the simulation links
cpu_l1cache_link = sst.Link("cpu_l1cache_link")
cpu_l1cache_link.connect( (cpu, "mem_link", "1000ps"), (l1cache, "high_network_0", "1000ps") )
l1cache_l2cache_link = sst.Link("l1cache_l2cache_link")
l1cache_l2cache_link.connect( (l1cache, "low_network_0", "10000ps"), (l2cache, "high_network_0", "10000ps") )
mem_bus_link = sst.Link("mem_bus_link")
mem_bus_link.connect( (l2cache, "low_network_0", "10000ps"), (memory, "direct_link", "10000ps") )
# End of generated output.
