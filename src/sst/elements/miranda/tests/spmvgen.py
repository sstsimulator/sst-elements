import sst

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

gen0 = cpu0.setSubComponent("generator", "miranda.SPMVGenerator")
gen1 = cpu1.setSubComponent("generator", "miranda.SPMVGenerator")
dim = 30
elemSize = 8
ordSize = 4
nnz = 7
gen_params = {
    "matrix_nx" : dim,
    "matrix_ny" : dim,
    "element_width" : elemSize,
    "ordinal_width" : ordSize,
    "matrix_nnz_per_row" : nnz,
    "iterations" : 4 
}
gen0.addParams(gen_params)
gen1.addParams(gen_params)
gen0.addParams({
    "local_row_start" : 0,
    "local_row_end" : (dim // 2)
})
gen1.addParams({
    "local_row_start" : (dim // 2) + 1,
    "local_row_end" : dim
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

comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
    "clock" : "1GHz",
    "addr_range_end" : 4096 * 1024 * 1024 - 1
})
memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "4096MiB",
})

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
bus_l2cache_link = sst.Link("bus_l2cache_link")
bus_l2cache_link.connect( (bus, "low_network_0", "50ps"), (l2cache, "high_network_0", "50ps") )

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (l2cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps") )
