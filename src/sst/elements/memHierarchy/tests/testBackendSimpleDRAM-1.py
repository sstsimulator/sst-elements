import sst
from mhlib import componentlist

cpu_params = {
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 32,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
}

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
cpu0.addParams(cpu_params)
cpu0.addParams({
      "memFreq" : "10",
      "rngseed" : "10",
      "clock" : "3GHz",
})
c0_l1cache = sst.Component("l1cache0.mesi", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
cpu1.addParams(cpu_params)
cpu1.addParams({
      "clock" : "2GHz",
      "memFreq" : "8",
      "rngseed" : "301",
})
c1_l1cache = sst.Component("l1cache1.mesi", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
bus0 = sst.Component("bus0", "memHierarchy.Bus")
bus0.addParams({
      "bus_frequency" : "2Ghz"
})
n0_l2cache = sst.Component("l2cache0.mesi.inclus", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "9",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
cpu2.addParams(cpu_params)
cpu2.addParams({
      "clock" : "2.5GHz",
      "memFreq" : "2",
      "rngseed" : "501",
})
c2_l1cache = sst.Component("l1cache2.mesi", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
cpu3.addParams(cpu_params)
cpu3.addParams({
    "clock" : "1.7GHz",
    "memFreq" : "20",
    "rngseed" : "701",
})
c3_l1cache = sst.Component("l1cache3.mesi", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
bus1 = sst.Component("bus1", "memHierarchy.Bus")
bus1.addParams({
      "bus_frequency" : "2Ghz"
})
n1_l2cache = sst.Component("l2cache1.mesi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "16",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
bus2 = sst.Component("bus2", "memHierarchy.Bus")
bus2.addParams({
      "bus_frequency" : "2Ghz"
})
l3cache = sst.Component("l3cache.mesi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "30",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : "0",
})
l3NIC = l3cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "group" : 1,
    "network_bw" : "25GB/s",
})
network = sst.Component("network", "merlin.hr_router")
network.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
network.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("directory.mesi", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "coherence_protocol" : "MESI",
    "debug" : "0",
    "entry_cache_size" : "32768",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0"
})
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "group" : 2,
    "network_bw" : "25GB/s",
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "backing" : "none",
    "addr_range_end" : 512*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
memory.addParams({
    "max_requests_per_cycle" : 1,
    "mem_size" : "512MiB",
    "tCAS" : 3, # 11@800MHz roughly coverted to 200MHz
    "tRCD" : 3,
    "tRP" : 3,
    "cycle_time" : "5ns",
    "row_size" : "8KiB",
    "row_policy" : "open"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "lowlink", "1000ps"), (c0_l1cache, "highlink", "1000ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "lowlink", "10000ps"), (bus0, "highlink0", "10000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "lowlink", "1000ps"), (c1_l1cache, "highlink", "1000ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "lowlink", "10000ps"), (bus0, "highlink1", "10000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (bus0, "lowlink0", "10000ps"), (n0_l2cache, "highlink", "10000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "lowlink", "10000ps"), (bus2, "highlink0", "10000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "lowlink", "1000ps"), (c2_l1cache, "highlink", "1000ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "lowlink", "10000ps"), (bus1, "highlink0", "10000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "lowlink", "1000ps"), (c3_l1cache, "highlink", "1000ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "lowlink", "10000ps"), (bus1, "highlink1", "10000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (bus1, "lowlink0", "10000ps"), (n1_l2cache, "highlink", "10000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "lowlink", "10000ps"), (bus2, "highlink1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (bus2, "lowlink0", "10000ps"), (l3cache, "highlink", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "10000ps"), (network, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (network, "port0", "2000ps"), (dirNIC, "port", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirctrl, "lowlink", "10000ps"), (memctrl, "highlink", "10000ps") )
