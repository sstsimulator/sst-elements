import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0

# Define the simulation components
cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 5,
    "maxOutstanding" : 16,
    "opCount" : 10000,
    "reqsPerIssue" : 4,
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 3,   # 3% llsc
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

l1cache = sst.Component("l1cache.msi", "memHierarchy.Cache")
l1cache.addParams({
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
      "verbose" : 2,
})

l2cache = sst.Component("l2cache.msi.inclus", "memHierarchy.Cache")
l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : 10,
      "verbose" : 2,
})

l3cache = sst.Component("l3cache.msi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : DEBUG_L3,
      "debug_level" : 10,
      "verbose" : 2,
})

# Must use subcomponent slot instead of lowlink port since l3 connects to a network
l3NIC = l3cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
l3NIC.addParams({
      #"debug" : 1,
      #"debug_level" : 10,
      "network_bw" : "25GB/s",
      "group" : 1,
      "verbose" : 2,
})

chiprtr = sst.Component("network", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")

dirctrl = sst.Component("directory.msi", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR,
      "debug_level" : "10",
      "entry_cache_size" : "16384",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0",
      "verbose" : 2,
})
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 2,
      "verbose" : 2,
      #"debug" : 1,
      #"debug_level" : 10,
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "verbose" : 2,
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache")
link_cpu_l1cache.connect( (iface, "lowlink", "1000ps"), (l1cache, "highlink", "1000ps") )

link_l1cache_l2cache = sst.Link("link_l1cache_l2cache")
link_l1cache_l2cache.connect( (l1cache, "lowlink", "10000ps"), (l2cache, "highlink", "10000ps") )

link_l2cache_l3cache = sst.Link("link_l2cache_l3cache")
link_l2cache_l3cache.connect( (l2cache, "lowlink", "10000ps"), (l3cache, "highlink", "10000ps") )

link_cache_net = sst.Link("link_cache_net")
link_cache_net.connect( (l3NIC, "port", "10000ps"), (chiprtr, "port1", "2000ps") )

link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )

link_dir_mem = sst.Link("link_dir_mem")
link_dir_mem.connect( (dirctrl, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )

