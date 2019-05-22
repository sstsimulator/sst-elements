import sst

# 4 CPUs with private L1s and semi-private L2s
# 2 memories - 1 DRAMSim & 1 HBMDramSim

clock = "2GHz"
netBW = "300GB/s"
debug = 0

cpu_params = {
        "memSize" : "0x8000",
        "num_loadstore" : 1000,
        "commFreq" : 20,
        "do_write" : 1,
        "clock" : clock,
        "verbose" : 0
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

comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu0.addParams(cpu_params)
comp_cpu0.addParams({ "rngseed" : 43 })

comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams(l1_params)

comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu1.addParams(cpu_params)
comp_cpu1.addParams({ "rngseed" : 68 })

comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams(l1_params)

comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({ "bus_frequency" : clock })

comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams(l2_params)

comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
iface2 = comp_cpu2.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu2.addParams(cpu_params)
comp_cpu2.addParams({ "rngseed" : 8 })

comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams(l1_params)

comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
iface3 = comp_cpu3.setSubComponent("memory", "memHierarchy.memInterface")
comp_cpu3.addParams(cpu_params)
comp_cpu3.addParams({ "rngseed" : 958 })

comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams(l1_params)

comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({ "bus_frequency" : "2 Ghz" })

comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams(l2_params)

comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({ "bus_frequency" : "2 Ghz" })

# Shared L3 over network to two directories with line interleaving
comp_l3cache = sst.Component("l3cache", "memHierarchy.Cache")
comp_l3cache.addParams({
      "access_latency_cycles" : "9",
      "mshr_latency_cycles" : 2,
      "cache_frequency" : clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_size" : "64 KB",
      "debug_level" : 10,
      "debug" : 0,
      "memNIC.network_bw" : netBW,
      "memNIC.debug" : 0,
      "memNIC.debug_level" : 10,
})
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : netBW,
      "link_bw" : netBW,
      "input_buf_size" : "512B",
      "output_buf_size" : "512B",
      "num_ports" : "3",
      "flit_size" : "36B",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "coherence_protocol" : "MSI",
      "debug" : 0,
      "debug_level" : 10,
      "entry_cache_size" : "1024",
      "memNIC.network_bw" : netBW,
      "memNIC.addr_range_start" : "0x0",
      "memNIC.addr_range_end" : "0x1EFFFFC0",
      "memNIC.interleave_size" : "64B",
      "memNIC.interleave_step" : "128B",
      "memNIC.debug" : 0,
      "memNIC.debug_level" : 10,
})
comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "coherence_protocol" : "MSI",
      "debug" : debug,
      "debug_level" : 10,
      "entry_cache_size" : "1024",
      "memNIC.network_bw" : netBW,
      "memNIC.addr_range_start" : "0x40",
      "memNIC.addr_range_end" : "0x1F000000",
      "memNIC.interleave_size" : "64B",
      "memNIC.interleave_step" : "128B",
      "memNIC.debug" : 0,
      "memNIC.debug_level" : 10,
})
comp_memory1 = sst.Component("ddr", "memHierarchy.MemController")
comp_memory1.addParams({
      "debug" : debug,
      "debug_level" : 10,
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB",
      "request_width" : "64",
      "backend" : "memHierarchy.dramsim",
      "backend.device_ini" : "ddr_device.ini",
      "backend.system_ini" : "ddr_system.ini",
      "memNIC.addr_range_start" : "0x40",
      "memNIC.addr_range_end" : "0x1F000000",
      "memNIC.interleave_size" : "64B",
      "memNIC.interleave_step" : "128B",

})
      
comp_memory0 = sst.Component("hbm", "memHierarchy.MemController")
comp_memory0.addParams({
      "debug" : debug,
      "debug_level" : 10,
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB",
      "request_width" : "64",
      "backend" : "memHierarchy.HBMDRAMSimMemory",
      "backend.device_ini" : "hbm_device.ini",
      "backend.system_ini" : "hbm_system.ini",
      "memNIC.addr_range_start" : "0x0",
      "memNIC.addr_range_end" : "0x1EFFFFC0",
      "memNIC.interleave_size" : "64B",
      "memNIC.interleave_step" : "128B",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")

# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "port", "100ps"), (comp_c0_l1cache, "high_network_0", "100ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "500ps"), (comp_n0_bus, "high_network_0", "500ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "port", "100ps"), (comp_c1_l1cache, "high_network_0", "100ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "500ps"), (comp_n0_bus, "high_network_1", "500ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "low_network_0", "500ps"), (comp_n0_l2cache, "high_network_0", "500ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "500ps"), (comp_n2_bus, "high_network_0", "500ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "port", "100ps"), (comp_c2_l1cache, "high_network_0", "100ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "500ps"), (comp_n1_bus, "high_network_0", "500ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "port", "100ps"), (comp_c3_l1cache, "high_network_0", "100ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "500ps"), (comp_n1_bus, "high_network_1", "500ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "low_network_0", "500ps"), (comp_n1_l2cache, "high_network_0", "500ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "500ps"), (comp_n2_bus, "high_network_1", "500ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "500ps"), (comp_l3cache, "high_network_0", "500ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "500ps"), (comp_chiprtr, "port2", "500ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "200ps"), (comp_dirctrl0, "network", "200ps") )
link_dir0_mem_link = sst.Link("link_dir0_mem_link")
link_dir0_mem_link.connect( (comp_dirctrl0, "memory", "500ps"), (comp_memory0, "direct_link", "500ps") )
link_dir_net_1 = sst.Link("link_dir_net_1")
link_dir_net_1.connect( (comp_chiprtr, "port1", "200ps"), (comp_dirctrl1, "network", "200ps") )
link_dir1_mem_link = sst.Link("link_dir1_mem_link")
link_dir1_mem_link.connect( (comp_dirctrl1, "memory", "500ps"), (comp_memory1, "direct_link", "500ps") )
