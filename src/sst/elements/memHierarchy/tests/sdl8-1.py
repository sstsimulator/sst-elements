# Automatically generated SST Python input
import sst

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0

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
      "debug" : DEBUG_L1,
      "debug_level" : 10,
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
      "debug" : DEBUG_L2,
      "debug_level" : 10,
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
      "debug" : DEBUG_L3,
      "debug_level" : 10,
})
l3_clink = comp_l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3_mlink = comp_l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3_mlink.addParams({
      "debug" : 0,
      "debug_level" : 10,
      "network_bw" : "25GB/s",
      "group" : 0,
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
    "debug" : DEBUG_DIR,
    "debug_level" : "10",
    "entry_cache_size" : "16384",
})
dir_clink = comp_dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dir_mlink = comp_dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dir_clink.addParams({
      "network_bw" : "25GB/s",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0",
      "debug" : 0,
      "debug_level" : 10,
      "group" : 1,
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "backend.access_time" : "100 ns",
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")

# Define the simulation links
link0 = sst.Link("link0")
link1 = sst.Link("link1")
link2 = sst.Link("link2")
link3 = sst.Link("link3")
link4 = sst.Link("link4")
link5 = sst.Link("link5")
link0.connect( (comp_cpu, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link1.connect( (comp_l1cache, "low_network_0", "10000ps"), (comp_l2cache, "high_network_0", "10000ps") )
link2.connect( (comp_l2cache, "low_network_0", "10000ps"), (l3_clink, "port", "10000ps") )
link3.connect( (l3_mlink, "port", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link4.connect( (comp_chiprtr, "port0", "2000ps"), (dir_clink, "port", "2000ps") )
link5.connect( (dir_mlink, "port", "10000ps"), (comp_memory, "direct_link", "10000ps") )
# End of generated output.
