# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "1000000ns")

# Define the simulation components
cpu = sst.Component("cpu", "prospero.prospero")
cpu.addParams({
      "outputlevel" : """0""",
      "tracetype" : """file""",
      "trace" : """traces/pinatrace.out""",
      "cache_line" : """64""",
      "physlimit" : """512"""
})
l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """64 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """1""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "mshr_num_entries" : """4096""",
      "L1" : """1""",
      "statistics" : """1"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "coherence_protocol" : """MSI""",
      "access_time" : """1000 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz"""
})


# Define the simulation links
cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (cpu, "cache_link", "1000ps"), (l1cache, "high_network_0", "1000ps") )
mem_bus_link = sst.Link("mem_bus_link")
mem_bus_link.connect( (l1cache, "low_network_0", "50ps"), (memory, "direct_link", "50ps") )
# End of generated output.
