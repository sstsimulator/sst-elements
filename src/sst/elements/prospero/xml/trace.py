# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "1000000ns")

# Define the simulation components
comp_cpu = sst.Component("cpu", "prospero.prospero")
comp_cpu.addParams({
      "physlimit" : """512""",
      "tracetype" : """file""",
      "outputlevel" : """0""",
      "cache_line" : """64""",
      "trace" : """traces/pinatrace.out"""
})
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : """1""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : """lru""",
      "coherence_protocol" : """MSI""",
      "associativity" : """8""",
      "cache_line_size" : """64""",
      "statistics" : """1""",
      "L1" : """1""",
      "cache_size" : """64 KB"""
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : """MSI""",
      "access_time" : """1000 ns""",
      "mem_size" : """512""",
      "clock" : """1GHz"""
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
# End of generated output.
