import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

memory_mb = 1024

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "miranda.BaseCPU")
comp_cpu0.addParams({
	"generator" : "miranda.GUPSGenerator",
	"generatorParams.verbose" : 0,
	"generatorParams.count" : 100000,
	"generatorParams.seed_a"  : 11,
	"generatorParams.seed_b"  : 177,
	"generatorParams.max_address" : ((memory_mb) / 2) * 1024 * 1024,
})

comp_cpu1 = sst.Component("cpu1", "miranda.BaseCPU")
comp_cpu1.addParams({
	"generator" : "miranda.GUPSGenerator",
	"generatorParams.verbose" : 0,
	"generatorParams.count" : 100000,
	"generatorParams.seed_a"  : 10101110101,
	"generatorParams.seed_b"  : 101011,
	"generatorParams.max_address" : ((memory_mb) / 2) * 1024 * 1024,
})

# Enable statistics outputs
comp_cpu0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
comp_cpu1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
comp_l1cache0.addParams({
      "memNIC.network_link_control" : "shogun.ShogunNIC",
      "maxRequestDelay" : 100000,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "prefetcher" : "cassini.StridePrefetcher",
      "L1" : "1",
      "cache_size" : "8KB",
      "backing" : "none",
      "verbose" : 0
})

comp_l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
comp_l1cache1.addParams({
      "memNIC.network_link_control" : "shogun.ShogunNIC",
      "maxRequestDelay" : 100000,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "prefetcher" : "cassini.StridePrefetcher",
      "L1" : "1",
      "cache_size" : "8KB",
      "backing" : "none",
})

# Enable statistics outputs
comp_l1cache0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
comp_l1cache1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memory0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memory0.addParams({
      "coherence_protocol" : "MESI",
      "backend.access_time" : "1ns",
      "backend.mem_size" : str(memory_mb * 1024 * 1024) + "B",
      "request_width" : "64",
      "debug" : 0,
      "debug_level" : 10,
      "clock" : "1GHz",
})

comp_memory1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memory1.addParams({
      "coherence_protocol" : "MESI",
      "backend.access_time" : "1ns",
      "backend.mem_size" : str(memory_mb * 1024 * 1024) + "B",
      "request_width" : "64",
      "debug" : 0,
      "debug_level" : 10,
      "clock" : "1GHz",
})

comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "coherence_protocol" : "MESI",
      "entry_cache_size" : "1024",
      "memNIC.network_link_control" : "shogun.ShogunNIC",
      "memNIC.network_bw" : "10GB/s",
      "memNIC.addr_range_start" : "0x00",
      "memNIC.addr_range_end" : (1024 * 1024 * 1024) - 64,
      "memNIC.interleave_size" : "64B",
      "memNIC.interleave_step" : "128B",
      "memNIC.debug" : 0,
      "memNIC.debug_level" : 10,
})

comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "coherence_protocol" : "MESI",
      "entry_cache_size" : "1024",
      "memNIC.network_link_control" : "shogun.ShogunNIC",
      "memNIC.network_bw" : "10GB/s",
      "memNIC.addr_range_start" : "0x40",
      "memNIC.addr_range_end" : (1024 * 1024 * 1024),
      "memNIC.interleave_size" : "64B",
      "memNIC.interleave_step" : "128B",
      "memNIC.debug" : 0,
      "memNIC.debug_level" : 10,
})

shogun_xbar = sst.Component("shogunxbar", "shogun.ShogunXBar")
shogun_xbar.addParams({
       "clock" : "1.0GHz",
       "port_count" : 4,
       "verbose" : 0
})

shogun_xbar.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

# Define the simulation links
link_cpu_cache_link0 = sst.Link("link_cpu_cache_link0")
link_cpu_cache_link0.connect( (comp_cpu0, "cache_link", "100ps"), (comp_l1cache0, "high_network_0", "100ps") )
link_cpu_cache_link0.setNoCut()

link_cpu_cache_link1 = sst.Link("link_cpu_cache_link1")
link_cpu_cache_link1.connect( (comp_cpu1, "cache_link", "100ps"), (comp_l1cache1, "high_network_0", "100ps") )
link_cpu_cache_link1.setNoCut()

xbar_cpu0_link = sst.Link("xbar_cpu0_link")
xbar_cpu0_link.connect( (shogun_xbar, "port0", "100ps"), (comp_l1cache0, "directory", "100ps") )

xbar_cpu1_link = sst.Link("xbar_cpu1_link")
xbar_cpu1_link.connect( (shogun_xbar, "port1", "100ps"), (comp_l1cache1, "directory", "100ps") )

dir0_mem0_link = sst.Link("dir0_mem0_link")
dir0_mem0_link.connect( (comp_dirctrl0, "memory", "500ps"), (comp_memory0, "direct_link", "50ps") )

dir0_net_link = sst.Link("dir0_net_link")
dir0_net_link.connect( (shogun_xbar, "port2", "100ps"), (comp_dirctrl0, "network", "200ps") )

dir1_mem1_link = sst.Link("dir1_mem1_link")
dir1_mem1_link.connect( (comp_dirctrl1, "memory", "500ps"), (comp_memory1, "direct_link", "50ps") )

dir1_net_link = sst.Link("dir1_net_link")
dir1_net_link.connect( (shogun_xbar, "port3", "100ps"), (comp_dirctrl1, "network", "200ps") )
