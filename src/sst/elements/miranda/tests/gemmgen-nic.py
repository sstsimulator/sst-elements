import sst
from mhlib import componentlist

# Define the simulation components
cpu0 = sst.Component("cpu0", "miranda.BaseCPU")
cpu1 = sst.Component("cpu1", "miranda.BaseCPU")
cpu_params = {
	"verbose" : 0,
	"clock" : "2GHz",
	"printStats" : 1,
}
cpu0.addParams(cpu_params)
cpu1.addParams(cpu_params)

gen0 = cpu0.setSubComponent("generator", "miranda.GEMMGenerator")
gen1 = cpu1.setSubComponent("generator", "miranda.GEMMGenerator")
gen_params = {
    "verbose" : 2,
    "iterations" : 1,
    "matrix_M" : 64,
    "matrix_N" : 64,
    "matrix_K" : 64,
}
gen0.addParams(gen_params)
gen1.addParams(gen_params)
gen0.addParams({
    "b_ptr" : """0x00100000""",
    "thread_id" : 0,
    "num_threads" : 2,
})
gen1.addParams({
    "start_addr" : """0x00100000""",
    "a_ptr" : 0x00,
    "thread_id" : 1,
    "num_threads" : 2,
})

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

# Enable statistics outputs
cpu0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
cpu1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
l1cache_params = {
    "access_latency_cycles" : "2",
    "cache_frequency" : "2 GHz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "prefetcher" : "cassini.StridePrefetcher",
    "debug" : "0",
    "L1" : "1",
    "cache_size" : "32KB"
}
l1cache0.addParams(l1cache_params)
l1cache1.addParams(l1cache_params)

# Enable statistics outputs
l1cache0.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
l1cache1.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({"bus_frequency" : "2GHz"})

l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
    "access_latency_cycles" : 8,
    "cache_frequency" : "2GHz",
    "replacement_policy" : "lru",
    "associativity" : 8,
    "cache_line_size" : 64,
    "cache_size" : "256KB",
})
cpulink_l2cache = l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2_nic = l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l2_nic.addParams({"group" : 1, "network_bw" : "25GB/s"})

# comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
# comp_memctrl.addParams({
#     "clock" : "1GHz",
#     "addr_range_end" : 4096 * 1024 * 1024 - 1
# })
# memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
# memory.addParams({
#       "access_time" : "100 ns",
#       "mem_size" : "4096MiB",
# })

######### set up NIC router ###############
comp_chipRtr = sst.Component("chipRtr", "merlin.hr_router")
comp_chipRtr.addParams({
      "debug" : 1,
      "input_buf_size" : """2KB""",
      "num_ports" : """6""",
      "id" : """0""",
      "output_buf_size" : """2KB""",
      "flit_size" : """64B""",
      "xbar_bw" : """51.2 GB/s""",
      "link_bw" : """25.6 GB/s""",
})
comp_chipRtr.setSubComponent("topology", "merlin.singlerouter")

####### set up directory controller 1 ############
comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "debug" : """0""",
      "coherence_protocol" : "MSI",
      "entry_cache_size" : """32768""",
      "addr_range_start" : """0x0""",
      "addr_range_end" : """0x000FFFFF""",
      "mshr_num_entries" : "2",
})

######### set up mem NIC and mem link ############
mem_nic0 = comp_dirctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
memlink_dirctrl0 = comp_dirctrl0.setSubComponent("memlink", "memHierarchy.MemLink")
mem_nic0.addParams({
    "group" : 2,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})


####### set up memory controller 1 ############
comp_memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memctrl0.addParams({
      "debug" : """0""",
      "clock" : """1.6GHz""",
      "addr_range_start" : """0x0""",
      "addr_range_end" : """0x000FFFFF""",
})
comp_memory0 = comp_memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory0.addParams({
      "mem_size" : "512MiB",
      "access_time" : "5 ns"
})


####### set up directory controller 2 ############
comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "debug" : 0,
      "coherence_protocol" : "MSI",
      "entry_cache_size" : 32768,
      "addr_range_start" : 0x00100000,
      "addr_range_end" : 0x3FFFFFFF,
      "mshr_num_entries" : 2,
})
mem_nic1 = comp_dirctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
memlink_dirctrl1 = comp_dirctrl1.setSubComponent("memlink", "memHierarchy.MemLink")
mem_nic1.addParams({
    "group" : 2,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memctrl1.addParams({
      "debug" : """0""",
      "clock" : """1.6GHz""",
      "addr_range_start" : """0x00100000""",
      "addr_range_end" : """0x3FFFFFFF""",
})
comp_memory1 = comp_memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory1.addParams({
      "mem_size" : "512MiB",
      "access_time" : "5 ns"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

#       cpu0      cpu1
#         |         |
#      l1cache0   l1cache1
#         |         |
#       L2cache/l2_nic
#             |
#   ------ chiprtr  ---------
#   |                       |
#  memNic0/memLink0 memNic1/memLink1
#         |             |
#  memDirctrl0        memDrictrl1
#         |             |
#       mem0            mem1

# Define the simulation links
cpu0_cache_link = sst.Link("cpu0_cache_link")
cpu1_cache_link = sst.Link("cpu1_cache_link")
cpu0_cache_link.connect( (cpu0, "cache_link", "1000ps"), (l1cache0, "high_network_0", "1000ps") )
cpu1_cache_link.connect( (cpu1, "cache_link", "1000ps"), (l1cache1, "high_network_0", "1000ps") )
cpu0_cache_link.setNoCut()
cpu1_cache_link.setNoCut()

l1cache0_bus_link = sst.Link("l1cache0_bus_link")
l1cache1_bus_link = sst.Link("l1cache1_bus_link")
l1cache0_bus_link.connect( (l1cache0, "low_network_0", "50ps"), (bus, "high_network_0", "50ps") )
l1cache1_bus_link.connect( (l1cache1, "low_network_0", "50ps"), (bus, "high_network_1", "50ps") )

# bus_l2cache_link = sst.Link("bus_l2cache_link")
# bus_l2cache_link.connect( (bus, "low_network_0", "50ps"), (l2cache, "high_network_0", "50ps") )

link_bus_l2cache = sst.Link("link_bus_l2cache")
link_bus_l2cache.connect( (bus, "low_network_0", "100ps"), (cpulink_l2cache, "port", "100ps") )

link_l2_rtr = sst.Link("link_l2")
link_l2_rtr.connect( (l2_nic, "port", '1000ps'), (comp_chipRtr, "port1", "1000ps") )

# link_mem_bus_link = sst.Link("link_mem_bus_link")
# link_mem_bus_link.connect( (l2cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps") )

link_mem_rtr0 = sst.Link("link_mem0")
link_mem_rtr0.connect( (mem_nic0, "port", "1000ps"), (comp_chipRtr, "port2", "1000ps") )
link_mem_rtr1 = sst.Link("link_mem1")
link_mem_rtr1.connect( (mem_nic1, "port", "1000ps"), (comp_chipRtr, "port3", "1000ps") )

link_dirctrl0_mem = sst.Link("link_dirctrl0_mem")
link_dirctrl0_mem.connect( (memlink_dirctrl0, "port", "100ps"), (comp_memctrl0, "direct_link", "100ps") )
link_dirctrl1_mem = sst.Link("link_dirctrl1_mem")
link_dirctrl1_mem.connect( (memlink_dirctrl1, "port", "100ps"), (comp_memctrl1, "direct_link", "100ps") )
