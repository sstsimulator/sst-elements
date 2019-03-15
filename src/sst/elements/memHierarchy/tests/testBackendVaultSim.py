# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "commFreq" : "100",
      "rngseed" : "101",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "commFreq" : "100",
      "rngseed" : "301",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
comp_cpu2.addParams({
      "commFreq" : "100",
      "rngseed" : "501",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
comp_c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
comp_cpu3.addParams({
      "commFreq" : "100",
      "rngseed" : "701",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
comp_c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
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
      "debug" : "0",
      "memNIC.network_bw" : "25GB/s",
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
      "debug" : "0",
      "entry_cache_size" : "32768",
      "memNIC.network_bw" : "25GB/s",
      "memNIC.addr_range_end" : "0x1F000000",
      "memNIC.addr_range_start" : "0x0"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "backend" : "memHierarchy.vaultsim",
      "backend.access_time" : "2 ns",   # Phy latency
      "backend.mem_size" : "512MiB",
      "clock" : "1GHz",
      "request_width" : "64"
})
comp_logic_layer = sst.Component("logic_layer", "VaultSimC.logicLayer")
comp_logic_layer.addParams({
    "clock" : "1GHz",
    "bwlimit" : "32",
    "vaults" : "8",
    "terminal" : 1,
    "llID" : 0,
    "LL_MASK" : 0
})

comp_vault0 = sst.Component("vault_0", "VaultSimC.VaultSimC")
comp_vault0.addParams({
    "clock" : "500MHz",
    "VaultID" : 0,
    "numVaults2" : 3
})

comp_vault1 = sst.Component("vault_1", "VaultSimC.VaultSimC")
comp_vault1.addParams({
    "clock" : "500MHz",
    "VaultID" : 1,
    "numVaults2" : 3
})

comp_vault2 = sst.Component("vault_2", "VaultSimC.VaultSimC")
comp_vault2.addParams({
    "clock" : "500MHz",
    "VaultID" : 2,
    "numVaults2" : 3
})

comp_vault3 = sst.Component("vault_3", "VaultSimC.VaultSimC")
comp_vault3.addParams({
    "clock" : "500MHz",
    "VaultID" : 3,
    "numVaults2" : 3
})

comp_vault4 = sst.Component("vault_4", "VaultSimC.VaultSimC")
comp_vault4.addParams({
    "clock" : "500MHz",
    "VaultID" : 4,
    "numVaults2" : 3
})

comp_vault5 = sst.Component("vault_5", "VaultSimC.VaultSimC")
comp_vault5.addParams({
    "clock" : "500MHz",
    "VaultID" : 5,
    "numVaults2" : 3
})

comp_vault6 = sst.Component("vault_6", "VaultSimC.VaultSimC")
comp_vault6.addParams({
    "clock" : "500MHz",
    "VaultID" : 6,
    "numVaults2" : 3
})

comp_vault7 = sst.Component("vault_7", "VaultSimC.VaultSimC")
comp_vault7.addParams({
    "clock" : "500MHz",
    "VaultID" : 7,
    "numVaults2" : 3
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_0", "10000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_1", "10000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "low_network_0", "10000ps"), (comp_n0_l2cache, "high_network_0", "10000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_0", "10000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (comp_cpu2, "mem_link", "1000ps"), (comp_c2_l1cache, "high_network_0", "1000ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_0", "10000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (comp_cpu3, "mem_link", "1000ps"), (comp_c3_l1cache, "high_network_0", "1000ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_1", "10000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "low_network_0", "10000ps"), (comp_n1_l2cache, "high_network_0", "10000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "10000ps"), (comp_l3cache, "high_network_0", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "2000ps"), (comp_dirctrl, "network", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "10000ps"), (comp_memory, "direct_link", "10000ps") )
link_dir_cube_link = sst.Link("link_dir_cube_link")
link_dir_cube_link.connect( (comp_memory, "cube_link", "2ns"), (comp_logic_layer, "toCPU", "2ns") )
link_logic_v0 = sst.Link("link_logic_v0")
link_logic_v0.connect( (comp_logic_layer, "bus_0", "500ps"), (comp_vault0, "bus", "500ps") )
link_logic_v1 = sst.Link("link_logic_v1")
link_logic_v1.connect( (comp_logic_layer, "bus_1", "500ps"), (comp_vault1, "bus", "500ps") )
link_logic_v2 = sst.Link("link_logic_v2")
link_logic_v2.connect( (comp_logic_layer, "bus_2", "500ps"), (comp_vault2, "bus", "500ps") )
link_logic_v3 = sst.Link("link_logic_v3")
link_logic_v3.connect( (comp_logic_layer, "bus_3", "500ps"), (comp_vault3, "bus", "500ps") )
link_logic_v4 = sst.Link("link_logic_v4")
link_logic_v4.connect( (comp_logic_layer, "bus_4", "500ps"), (comp_vault4, "bus", "500ps") )
link_logic_v5 = sst.Link("link_logic_v5")
link_logic_v5.connect( (comp_logic_layer, "bus_5", "500ps"), (comp_vault5, "bus", "500ps") )
link_logic_v6 = sst.Link("link_logic_v6")
link_logic_v6.connect( (comp_logic_layer, "bus_6", "500ps"), (comp_vault6, "bus", "500ps") )
link_logic_v7 = sst.Link("link_logic_v7")
link_logic_v7.connect( (comp_logic_layer, "bus_7", "500ps"), (comp_vault7, "bus", "500ps") )
# End of generated output.
