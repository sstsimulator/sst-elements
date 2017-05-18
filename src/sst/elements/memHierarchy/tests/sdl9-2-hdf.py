# Automatically generated SST Python input
import sst

# Define the simulation components

l1stats = sst.StatisticGroup("L1Stats")
l1stats.setOutput(sst.StatisticOutput("sst.statOutputHDF5", {"filepath": "out.h5"}))
l1stats.setFrequency("500Mhz")
l1stats.addStatistic("CacheMisses", {"resetOnRead": True})
l1stats.addStatistic("CacheHits", {"resetOnRead": True})

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
      "debug" : "1",
      "debug_level" : "1",
      "network_address" : "1",
      "network_bw" : "25GB/s"
})
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "id" : "0",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "link_bw" : "1GB/s",
      "topology" : "merlin.singlerouter"
})
comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "network_address" : "0",
      "entry_cache_size" : "8192",
      "network_bw" : "25GB/s",
      "addr_range_start" : "0x0",
      "addr_range_end" : "0x1F000000"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "backend.access_time" : "100 ns",
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB"
})

link_n2bus_l3cache = sst.Link("link_n2bus_l3cache")
link_n2bus_l3cache.connect( (comp_n2_bus, "low_network_0", "10000ps"), (comp_l3cache, "high_network_0", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "2000ps"), (comp_dirctrl, "network", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "10000ps"), (comp_memory, "direct_link", "10000ps") )

for node in range(2):
    comp_bus = sst.Component("n%d.bus"%node, "memHierarchy.Bus")
    comp_bus.addParams({
      "bus_frequency" : "2 Ghz"
    })
    comp_l2cache = sst.Component("n%d.l2cache"%node, "memHierarchy.Cache")
    comp_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
    })
    link_l2cache_bus = sst.Link("link_n%dl2cache_bus"%node)
    link_l2cache_bus.connect( (comp_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_%d"%node, "10000ps") )
    link_l2node_bus = sst.Link("link_n%dl2node_bus"%node)
    link_l2node_bus.connect( (comp_bus, "low_network_0", "10000ps"), (comp_l2cache, "high_network_0", "10000ps") )
    for i in range(4):
        comp_cpu = sst.Component("cpu%d"%(node*4+i), "memHierarchy.trivialCPU")
        comp_cpu.addParams({
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000",
      "do_write" : "1",
      "noncacheableRangeStart" : "0",
      "noncacheableRangeEnd" : "0x100"
        })
        comp_l1cache = sst.Component("c%d.l1cache"%(node*4+i), "memHierarchy.Cache")
        comp_l1cache.setCoordinates(i, node)
        l1stats.addComponent(comp_l1cache)
        comp_l1cache.addParams({
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
        l1Link = sst.Link("c%dl1"%(node*4+i))
        l1Link.connect( (comp_cpu, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
        busLink = sst.Link("l1%dbus"%(node*4+i))
        busLink.connect( (comp_l1cache, "low_network_0", "10000ps"), (comp_bus, "high_network_%d"%i, "10000ps") )






# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.setStatisticOutput("sst.statOutputHDF5", {"filepath": "out.h5"})
#sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
#sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


