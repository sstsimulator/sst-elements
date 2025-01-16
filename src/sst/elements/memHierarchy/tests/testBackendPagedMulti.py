import sst
from mhlib import componentlist

# Testing
# Different simpleDRAM parameters from simpleDRAM tests
# mru/lru/nmru cache replacement
# Lower latencies
# DelayBuffer backend 

cpu_params = {
    "memFreq" : 20,
    "clock" : "2GHz",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 10000,
    "reqsPerIssue" : 1,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
}

# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
cpu0.addParams(cpu_params)
cpu0.addParams({
    "rngseed" : "1",
})
c0_l1cache = sst.Component("l1cache0.mesi", "memHierarchy.Cache")
c0_l1cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "mru",
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
    "rngseed" : "301",
})
c1_l1cache = sst.Component("l1cache1.mesi", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
n0_bus = sst.Component("bus0", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2Ghz"
})
n0_l2cache = sst.Component("l2cache0.mesi.inclus", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "6",
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
    "rngseed" : "501",
})
c2_l1cache = sst.Component("l1cache2.mesi", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "mru",
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
    "rngseed" : "701",
})
c3_l1cache = sst.Component("l1cache3.mesi", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
n1_bus = sst.Component("bus1", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2Ghz"
})
n1_l2cache = sst.Component("l2cache1.mesi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "7",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
n2_bus = sst.Component("bus2", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2Ghz"
})
l3cache = sst.Component("l3cache.mesi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "24",
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "nmru",
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
    "entry_cache_size" : "16384",
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
    "clock" : "500MHz",
    "backing" : "none",
    "addr_range_end" : 512*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.pagedMulti")
memory.addParams({
    "max_requests_per_cycle" : 1,
    "mem_size" : "512MiB",
    "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
    "system_ini" : "system.ini",
    "access_time" : "30ns",
    "dramBackpressure" : "1",
    "max_fast_pages" : 4,
    "quantum" : "30us", # Test runs ~1.7ms
    "page_shift" : "10",
    "collect_stats" : "0",
    "transfer_delay" : "0",
    "threshold" : 1,
    "page_add_strategy": "RAND",
    "page_replace_strategy": "FIFO",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "lowlink", "500ps"), (c0_l1cache, "highlink", "500ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "lowlink", "1000ps"), (n0_bus, "highlink0", "1000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "lowlink", "500ps"), (c1_l1cache, "highlink", "500ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "lowlink", "1000ps"), (n0_bus, "highlink1", "1000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "lowlink0", "1000ps"), (n0_l2cache, "highlink", "1000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "lowlink", "1000ps"), (n2_bus, "highlink0", "1000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "lowlink", "500ps"), (c2_l1cache, "highlink", "500ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "lowlink", "1000ps"), (n1_bus, "highlink0", "1000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "lowlink", "500ps"), (c3_l1cache, "highlink", "500ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "lowlink", "1000ps"), (n1_bus, "highlink1", "1000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "lowlink0", "1000ps"), (n1_l2cache, "highlink", "1000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "lowlink", "1000ps"), (n2_bus, "highlink1", "1000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "lowlink0", "1000ps"), (l3cache, "highlink", "1000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "1000ps"), (network, "port1", "1000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (network, "port0", "1000ps"), (dirNIC, "port", "1000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirctrl, "lowlink", "1000ps"), (memctrl, "highlink", "1000ps") )
