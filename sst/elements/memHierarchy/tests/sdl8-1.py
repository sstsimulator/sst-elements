# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000ns")

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x100000""",
      "num_loadstore" : """10000""",
      "do_write" : """1"""
})
l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """4 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """5""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """32 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """8""",
      "access_latency_cycles" : """20""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1"""
})
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """64 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """16""",
      "access_latency_cycles" : """100""",
      "low_network_links" : """1""",
      "cache_line_size" : """64""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1""",
      "high_network_links" : """1""",
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
      "network_bw" : """1GHz""",
      "addr_range_start" : """0x0""",
      "addr_range_end" : """0x1F000000""",
      "entry_cache_size" : """16384""",
      "network_address" : """0"""
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
cache_net_0 = sst.Link("cache_net_0")
cache_net_0.connect( (l3cache, "directory", "10000ps"), (chiprtr, "port1", "10000ps") )
cpu_l1cache_link = sst.Link("cpu_l1cache_link")
cpu_l1cache_link.connect( (cpu, "mem_link", "1000ps"), (l1cache, "high_network_0", "1000ps") )
dir_mem_link = sst.Link("dir_mem_link")
dir_mem_link.connect( (dirctrl, "memory", "10000ps"), (memory, "direct_link", "10000ps") )
dir_net_0 = sst.Link("dir_net_0")
dir_net_0.connect( (chiprtr, "port0", "2000ps"), (dirctrl, "network", "2000ps") )
l1cache_l2cache_link = sst.Link("l1cache_l2cache_link")
l1cache_l2cache_link.connect( (l1cache, "low_network_0", "10000ps"), (l2cache, "high_network_0", "10000ps") )
l2cache_l3cache_link = sst.Link("l2cache_l3cache_link")
l2cache_l3cache_link.connect( (l2cache, "low_network_0", "10000ps"), (l3cache, "high_network_0", "10000ps") )
# End of generated output.
