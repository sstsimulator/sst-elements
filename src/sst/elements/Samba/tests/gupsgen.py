import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

memory_mb = 1024

# Define the simulation components
comp_cpu = sst.Component("cpu", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 0,
	"generator" : "miranda.GUPSGenerator",
	"generatorParams.verbose" : 0,
	"generatorParams.count" : 10000,
	"generatorParams.max_address" : ((memory_mb) / 2) * 1024 * 1024,
})

# Enable statistics outputs
comp_cpu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "prefetcher" : "cassini.StridePrefetcher",
      "L1" : "1",
      "cache_size" : "8KB",
      "do_not_back" : 1
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MESI",
      "backend.access_time" : "50 ns",
      "backend.mem_size" : str(memory_mb * 1024 * 1024) + "B",
      "clock" : "1GHz"
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "50ps"), (comp_l1cache, "high_network_0", "50ps") )
link_cpu_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
