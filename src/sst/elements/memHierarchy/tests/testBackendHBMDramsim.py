import sst
from mhlib import componentlist

# 4 CPUs with private L1s and semi-private L2s
# 2 memories - 1 DRAMSim & 1 HBMDramSim

clock = "2GHz"
netBW = "300GB/s"
debug = 0

cpu_params = {
    "memFreq" : 20,
    "clock" : clock,
    "memSize" : "32KiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
}

l1_params = {
      "access_latency_cycles" : 2,
      "cache_frequency" : clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : 2,
      "cache_size" : "1KiB", # Really small to force mem traffic
      "L1" : 1,
      "mshr_latency_cycles" : 2,
      "debug_level" : 10,
      "debug" : "0"
}

l2_params = {
      "access_latency_cycles" : 6,
      "mshr_latency_cycles" : 2,
      "cache_frequency" : clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : 8,
      "cache_size" : "4KB",
      "debug_level" : 10,
      "debug" : "0"
}

cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
cpu0.addParams(cpu_params)
cpu0.addParams({ "rngseed" : 43 })

c0_l1cache = sst.Component("l1cache0.msi", "memHierarchy.Cache")
c0_l1cache.addParams(l1_params)

cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
cpu1.addParams(cpu_params)
cpu1.addParams({ "rngseed" : 68 })

c1_l1cache = sst.Component("l1cache1.msi", "memHierarchy.Cache")
c1_l1cache.addParams(l1_params)

n0_bus = sst.Component("bus0", "memHierarchy.Bus")
n0_bus.addParams({ "bus_frequency" : clock })

n0_l2cache = sst.Component("l2cache0", "memHierarchy.Cache")
n0_l2cache.addParams(l2_params)

cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
cpu2.addParams(cpu_params)
cpu2.addParams({ "rngseed" : 8 })

c2_l1cache = sst.Component("l1cache2.msi", "memHierarchy.Cache")
c2_l1cache.addParams(l1_params)

cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
cpu3.addParams(cpu_params)
cpu3.addParams({ "rngseed" : 958 })

c3_l1cache = sst.Component("l1cache3.msi", "memHierarchy.Cache")
c3_l1cache.addParams(l1_params)

n1_bus = sst.Component("bus1", "memHierarchy.Bus")
n1_bus.addParams({ "bus_frequency" : "2 Ghz" })

n1_l2cache = sst.Component("l2cache1.msi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams(l2_params)

n2_bus = sst.Component("bus2", "memHierarchy.Bus")
n2_bus.addParams({ "bus_frequency" : "2 Ghz" })

# Shared L3 over network to two directories with line interleaving
l3cache = sst.Component("l3cache.msi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "9",
      "mshr_latency_cycles" : 2,
      "cache_frequency" : clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_size" : "64 KB",
      "debug_level" : 10,
      "debug" : 0,
})
l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "network_bw" : netBW,
    "group" : 1,
    "debug" : 0,
    "debug_level" : 10,
})
network = sst.Component("network", "merlin.hr_router")
network.addParams({
      "xbar_bw" : netBW,
      "link_bw" : netBW,
      "input_buf_size" : "512B",
      "output_buf_size" : "512B",
      "num_ports" : "3",
      "flit_size" : "36B",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
network.setSubComponent("topology","merlin.singlerouter")
dirctrl0 = sst.Component("directory0.msi", "memHierarchy.DirectoryController")
dirctrl0.addParams({
    "coherence_protocol" : "MSI",
    "debug" : 0,
    "debug_level" : 10,
    "entry_cache_size" : "1024",
    "addr_range_start" : "0x0",
    "addr_range_end" : "0x1EFFFFC0",
    "interleave_size" : "64B",
    "interleave_step" : "128B",
})
dirtoM0 = dirctrl0.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC0 = dirctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC0.addParams({
    "group" : 2,
    "network_bw" : netBW,
    "debug" : 0,
    "debug_level" : 10,
})
dirctrl1 = sst.Component("directory1.msi", "memHierarchy.DirectoryController")
dirctrl1.addParams({
    "coherence_protocol" : "MSI",
    "debug" : debug,
    "debug_level" : 10,
    "entry_cache_size" : "1024",
    "addr_range_start" : "0x40",
    "addr_range_end" : "0x1F000000",
    "interleave_size" : "64B",
    "interleave_step" : "128B",
})
dirtoM1 = dirctrl1.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC1 = dirctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC1.addParams({
    "group" : 2,
    "network_bw" : netBW,
    "debug" : 0,
    "debug_level" : 10,
})
memctrl1 = sst.Component("memory.ddr", "memHierarchy.MemController")
memctrl1.addParams({
    "debug" : debug,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_start" : "0x40",
    "addr_range_end" : "0x1EFFFFF0",
    "interleave_size" : "64B",
    "interleave_step" : "128B",
})
memtoD1 = memctrl1.setSubComponent("cpulink", "memHierarchy.MemLink")
memory1 = memctrl1.setSubComponent("backend", "memHierarchy.dramsim")
memory1.addParams({
    "mem_size" : "512MiB",
    "device_ini" : "ddr_device.ini",
    "system_ini" : "ddr_system.ini",
})
      
memctrl0 = sst.Component("memory.hbm", "memHierarchy.MemController")
memctrl0.addParams({
    "debug" : debug,
    "debug_level" : 10,
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_start" : "0x0",
    "addr_range_end" : "0x1EFFFFC0",
    "interleave_size" : "64B",
    "interleave_step" : "128B",
})
memtoD0 = memctrl0.setSubComponent("cpulink", "memHierarchy.MemLink")
memory0 = memctrl0.setSubComponent("backend", "memHierarchy.HBMDRAMSimMemory")
memory0.addParams({
    "mem_size" : "512MiB",
    "device_ini" : "hbm_device.ini",
    "system_ini" : "hbm_system.ini",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "port", "100ps"), (c0_l1cache, "high_network_0", "100ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "low_network_0", "500ps"), (n0_bus, "high_network_0", "500ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "100ps"), (c1_l1cache, "high_network_0", "100ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "low_network_0", "500ps"), (n0_bus, "high_network_1", "500ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "low_network_0", "500ps"), (n0_l2cache, "high_network_0", "500ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "low_network_0", "500ps"), (n2_bus, "high_network_0", "500ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "100ps"), (c2_l1cache, "high_network_0", "100ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "low_network_0", "500ps"), (n1_bus, "high_network_0", "500ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "100ps"), (c3_l1cache, "high_network_0", "100ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "low_network_0", "500ps"), (n1_bus, "high_network_1", "500ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "low_network_0", "500ps"), (n1_l2cache, "high_network_0", "500ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "low_network_0", "500ps"), (n2_bus, "high_network_1", "500ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "low_network_0", "500ps"), (l3tol2, "port", "500ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "500ps"), (network, "port2", "500ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (network, "port0", "200ps"), (dirNIC0, "port", "200ps") )
link_dir0_mem_link = sst.Link("link_dir0_mem_link")
link_dir0_mem_link.connect( (dirtoM0, "port", "500ps"), (memtoD0, "port", "500ps") )
link_dir_net_1 = sst.Link("link_dir_net_1")
link_dir_net_1.connect( (network, "port1", "200ps"), (dirNIC1, "port", "200ps") )
link_dir1_mem_link = sst.Link("link_dir1_mem_link")
link_dir1_mem_link.connect( (dirtoM1, "port", "500ps"), (memtoD1, "port", "500ps") )
