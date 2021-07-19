# Automatically generated SST Python input
import sst
from mhlib import componentlist

# Define shared parameters
cpu_params = {
    "commFreq" : "10",
    "do_write" : "1",
    "num_loadstore" : "10000",
    "memSize" : "0x40000000",
}

l1_params = {
    "access_latency_cycles" : "1",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "cache_size" : "4 KB",
    "L1" : "1",
    "debug" : "0",
}

l2_params = {
    "access_latency_cycles" : "8",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "debug" : "0",
}
# Define the simulation components
# Core 0
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu0.addParams(cpu_params)
comp_cpu0.addParams({ "rngseed" : 101 })

comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams(l1_params)
comp_c0_l1cache.setSubComponent("hash", "memHierarchy.hash.xor")

# Core 1
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu1.addParams(cpu_params)
comp_cpu1.addParams({ "rngseed" : 301 })

comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams(l1_params)
comp_c1_l1cache.setSubComponent("hash", "memHierarchy.hash.xor")

# Node 0
comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams(l2_params)
comp_n0_l2cache.setSubComponent("hash", "memHierarchy.hash.xor")

# Core 2
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
iface2 = comp_cpu2.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu2.addParams(cpu_params)
comp_cpu2.addParams({ "rngseed" : 501 })

comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams(l1_params)
comp_c2_l1cache.setSubComponent("hash", "memHierarchy.hash.xor")

# Core 3
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
iface3 = comp_cpu3.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu3.addParams(cpu_params)
comp_cpu3.addParams({ "rngseed" : 701 })

comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams(l1_params)
comp_c3_l1cache.setSubComponent("hash", "memHierarchy.hash.xor")

# Node 1
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams(l2_params)
comp_n1_l2cache.setSubComponent("hash", "memHierarchy.hash.xor")

# Uncore
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})

l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "12",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : "0",
      "memNIC.network_bw" : "25GB/s",
})
l3cache.setSubComponent("hash", "memHierarchy.hash.xor")
l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "group" : 1,
    "network_bw" : " 25GB/s"
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
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "coherence_protocol" : "MESI",
    "debug" : "0",
    "entry_cache_size" : "32768",
    "addr_range_end" : "0x40000000",
    "addr_range_start" : "0x0"
})
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "group" : 2,
    "network_bw" : "25GB/s",
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_end" : 1024*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "30ns",
    "mem_size" : "1GiB",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "port", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "1000ps"), (comp_n0_bus, "high_network_0", "1000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "1000ps"), (comp_n0_bus, "high_network_1", "1000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "low_network_0", "1000ps"), (comp_n0_l2cache, "high_network_0", "1000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "1000ps"), (comp_n2_bus, "high_network_0", "1000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "1000ps"), (comp_c2_l1cache, "high_network_0", "1000ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "1000ps"), (comp_n1_bus, "high_network_0", "1000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "1000ps"), (comp_c3_l1cache, "high_network_0", "1000ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "1000ps"), (comp_n1_bus, "high_network_1", "1000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "low_network_0", "1000ps"), (comp_n1_l2cache, "high_network_0", "1000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "1000ps"), (comp_n2_bus, "high_network_1", "1000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "1000ps"), (l3tol2, "port", "1000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "1000ps"), (comp_chiprtr, "port1", "100ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "100ps"), (dirNIC, "port", "100ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirtoM, "port", "1000ps"), (memctrl, "direct_link", "1000ps") )
# End of generated output.
