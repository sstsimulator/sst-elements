import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

# Define the simulation components
comp_cpu = sst.Component("cpu", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 0,
	"generator" : "miranda.STREAMBenchGeneratorCustomCmd",
	"clock" : "2.4GHz",
	"generatorParams.verbose" : 0,
	"generatorParams.n" : 10000,
        "generatorParams.operandwidth" : 16,
        "generatorParams.write_cmd" : 10,
        "generatorParams.read_cmd" : 20,
	"printStats" : 1,
})

# Enable statistics outputs
comp_cpu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : "2.4 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "prefetcher" : "cassini.StridePrefetcher",
      "debug" : "1",
      "L1" : "1",
      "cache_size" : "32KB"
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MESI",
      "backend.access_time" : "100 ns",
      "backend.mem_size" : "4096MiB",
      "clock" : "1GHz",
      "customCmdHandler" : "memHierarchy.amoCustomCmdHandler",
      "backendConvertor" : "memHierarchy.extMemBackendConvertor",
      "backend" : "memHierarchy.goblinHMCSim",
      "backend.cmd-map" : "[CUSTOM:20:64:RD64,CUSTOM:10:64:WR64]",
      "backend.verbose" : 1
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_cpu_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
