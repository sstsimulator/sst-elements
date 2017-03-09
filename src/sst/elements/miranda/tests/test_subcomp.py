import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the simulation components
comp_cpu = sst.Component("cpu", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 0,
	"printStats" : 1,
})

gen = comp_cpu.setSubComponent("generator", "miranda.CopyGenerator")
gen.addParams({
	"verbose" : 0,
    "read_start_address" : 0,
	"request_width" : 16,
	"request_count" : 65536,
})


# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

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
      "debug" : "1",
      "L1" : "1",
      "cache_size" : "2KB"
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MESI",
      "backend.access_time" : "1000 ns",
      "backend.mem_size" : "512MiB",
      "clock" : "1GHz"
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_cpu_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
