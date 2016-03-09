# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "10000ns")

# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
comp_cpu.addParams({
      "memSize" : "0x100000",
      "num_loadstore" : "10000",
      "commFreq" : "100",
      "do_write" : "1"
})
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
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
comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : ""
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
      "debug" : "",
      "network_address" : "1",
      "network_bw" : "25GB/s",
      "directory_at_next_level" : "1"
})
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "1",
      "debug_level" : "10",
      "network_address" : "0",
      "entry_cache_size" : "16384",
      "network_bw" : "25GB/s",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0"
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
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")

# Define the simulation links
link_cpu_l1cache_link = sst.Link("link_cpu_l1cache_link")
link_cpu_l1cache_link.connect( (comp_cpu, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_l1cache_l2cache_link = sst.Link("link_l1cache_l2cache_link")
link_l1cache_l2cache_link.connect( (comp_l1cache, "low_network_0", "10000ps"), (comp_l2cache, "high_network_0", "10000ps") )
link_l2cache_l3cache_link = sst.Link("link_l2cache_l3cache_link")
link_l2cache_l3cache_link.connect( (comp_l2cache, "low_network_0", "10000ps"), (comp_l3cache, "high_network_0", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "2000ps"), (comp_dirctrl, "network", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "10000ps"), (comp_memory, "direct_link", "10000ps") )
# End of generated output.
