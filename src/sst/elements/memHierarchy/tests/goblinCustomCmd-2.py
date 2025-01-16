import sst

# Define the simulation components
comp_cpu = sst.Component("core", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 0,
	"generator" : "miranda.STREAMBenchGeneratorCustomCmd",
	"generatorParams.verbose" : 0,
	"generatorParams.startat" : 3,
	"generatorParams.count" : 500000,
	"generatorParams.max_address" : 512000,
        "generatorParams.read_cmd" : 20,
	"printStats" : 1,
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

comp_memory = sst.Component("memory", "memHierarchy.CoherentMemController")
comp_memory.addParams({
      "coherence_protocol" : "MESI",
      "backend.access_time" : "1000 ns",
      "backend.mem_size" : "512MiB",
      "clock" : "1GHz",
      "customCmdHandler" : "memHierarchy.defCustomCmdHandler",
      "backendConvertor" : "memHierarchy.extMemBackendConvertor",
      "backend" : "memHierarchy.goblinHMCSim",
      "backend.verbose" : "0",
      "backend.trace_banks" : "1",
      "backend.trace_queue" : "1",
      "backend.trace_cmds" : "1",
      "backend.trace_latency" : "1",
      "backend.trace_stalls" : "1",

      "backend.cmd_map" : "[CUSTOM:20:64:RD64]"
})


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "highlink", "1000ps") )
link_cpu_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "lowlink", "50ps"), (comp_memory, "highlink", "50ps") )
