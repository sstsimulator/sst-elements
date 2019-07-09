import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

memory_mb = 1024

# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "miranda.BaseCPU")
gen_cpu0 = comp_cpu0.setSubComponent("generator", "miranda.GUPSGenerator")
gen_cpu0.addParams({
	"verbose" : 0,
	"count" : 100000,
	"seed_a"  : 11,
	"seed_b"  : 177,
	"max_address" : ((memory_mb) / 2) * 1024 * 1024,
})

comp_cpu1 = sst.Component("cpu1", "miranda.BaseCPU")
gen_cpu1 = comp_cpu1.setSubComponent("generator", "miranda.GUPSGenerator")
gen_cpu1.addParams({
	"verbose" : 0,
	"count" : 100000,
	"seed_a"  : 10101110101,
	"seed_b"  : 101011,
	"max_address" : ((memory_mb) / 2) * 1024 * 1024,
})

# Enable statistics outputs
comp_cpu0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
gen_cpu0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
comp_cpu1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
gen_cpu1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
comp_l1cache0.addParams({
      "maxRequestDelay" : 100000,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8KB",
      "backing" : "none",
      "verbose" : 0
})
pref0 = comp_l1cache0.setSubComponent("prefetcher", "cassini.StridePrefetcher")
l1c0_cpulink = comp_l1cache0.setSubComponent("cpulink", "memHierarchy.MemLink")
l1c0_memlink = comp_l1cache0.setSubComponent("memlink", "memHierarchy.MemNIC")
l1c0_memlink.addParams({ "group" : 2 })
l1c0_linkctrl = l1c0_memlink.setSubComponent("linkcontrol", "shogun.ShogunNIC")

comp_l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
comp_l1cache1.addParams({
      "maxRequestDelay" : 100000,
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "8KB",
      "backing" : "none",
})
pref1 = comp_l1cache1.setSubComponent("prefetcher", "cassini.StridePrefetcher")
l1c1_cpulink = comp_l1cache1.setSubComponent("cpulink", "memHierarchy.MemLink")
l1c1_memlink = comp_l1cache1.setSubComponent("memlink", "memHierarchy.MemNIC")
l1c1_memlink.addParams({ "group" : 2 })
l1c1_linkctrl = l1c1_memlink.setSubComponent("linkcontrol", "shogun.ShogunNIC")

# Enable statistics outputs
comp_l1cache0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
comp_l1cache1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
pref0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
pref1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memctrl0.addParams({
      "request_width" : "64",
      "debug" : 0,
      "debug_level" : 10,
      "clock" : "1GHz",
})
memory0 = comp_memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
memory0.addParams({
      "access_time" : "1ns",
      "mem_size" : str(memory_mb * 1024 * 1024) + "B",
})

comp_memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memctrl1.addParams({
      "request_width" : "64",
      "debug" : 0,
      "debug_level" : 10,
      "clock" : "1GHz",
})
memory1 = comp_memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
memory1.addParams({
      "access_time" : "1ns",
      "mem_size" : str(memory_mb * 1024 * 1024) + "B",
})

comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "coherence_protocol" : "MESI",
      "entry_cache_size" : "1024",
      "addr_range_start" : "0x00",
      "addr_range_end" : (1024 * 1024 * 1024) - 64,
      "interleave_size" : "64B",
      "interleave_step" : "128B",
})

dc0_cpulink = comp_dirctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
dc0_memlink = comp_dirctrl0.setSubComponent("memlink", "memHierarchy.MemLink")
dc0_cpulink.addParams({ 
    "group" : 3,
    "debug" : 0,
    "debug_level" : 10,
})
dc0_linkctrl = dc0_cpulink.setSubComponent("linkcontrol", "shogun.ShogunNIC")

comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "coherence_protocol" : "MESI",
      "entry_cache_size" : "1024",
      "addr_range_start" : "0x40",
      "addr_range_end" : (1024 * 1024 * 1024),
      "interleave_size" : "64B",
      "interleave_step" : "128B",
})

dc1_cpulink = comp_dirctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
dc1_memlink = comp_dirctrl1.setSubComponent("memlink", "memHierarchy.MemLink")
dc1_cpulink.addParams({ 
    "group" : 3,
    "debug" : 0,
    "debug_level" : 10,
})
dc1_linkctrl = dc1_cpulink.setSubComponent("linkcontrol", "shogun.ShogunNIC")

shogun_xbar = sst.Component("shogunxbar", "shogun.ShogunXBar")
shogun_xbar.addParams({
       "clock" : "1.0GHz",
       "port_count" : 4,
       "verbose" : 0
})

shogun_xbar.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

# Define the simulation links
link_cpu_cache_link0 = sst.Link("link_cpu_cache_link0")
link_cpu_cache_link0.connect( (comp_cpu0, "cache_link", "100ps"), (l1c0_cpulink, "port", "100ps") )
link_cpu_cache_link0.setNoCut()

link_cpu_cache_link1 = sst.Link("link_cpu_cache_link1")
link_cpu_cache_link1.connect( (comp_cpu1, "cache_link", "100ps"), (l1c1_cpulink, "port", "100ps") )
link_cpu_cache_link1.setNoCut()

xbar_cpu0_link = sst.Link("xbar_cpu0_link")
xbar_cpu0_link.connect( (shogun_xbar, "port0", "100ps"), (l1c0_linkctrl, "port", "100ps") )

xbar_cpu1_link = sst.Link("xbar_cpu1_link")
xbar_cpu1_link.connect( (shogun_xbar, "port1", "100ps"), (l1c1_linkctrl, "port", "100ps") )

dir0_mem0_link = sst.Link("dir0_mem0_link")
dir0_mem0_link.connect( (dc0_memlink, "port", "500ps"), (comp_memctrl0, "direct_link", "50ps") )

dir0_net_link = sst.Link("dir0_net_link")
dir0_net_link.connect( (shogun_xbar, "port2", "100ps"), (dc0_linkctrl, "port", "200ps") )

dir1_mem1_link = sst.Link("dir1_mem1_link")
dir1_mem1_link.connect( (dc1_memlink, "port", "500ps"), (comp_memctrl1, "direct_link", "50ps") )

dir1_net_link = sst.Link("dir1_net_link")
dir1_net_link.connect( (shogun_xbar, "port3", "100ps"), (dc1_linkctrl, "port", "200ps") )
