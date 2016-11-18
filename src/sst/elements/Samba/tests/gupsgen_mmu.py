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
	"verbose" : 1,
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


mmu = sst.Component("mmu0", "Samba")
mmu.addParams({
        "os_page_size": 2048,
        "corecount": 1,
        "sizes_L1": 3,
        "page_size1_L1": 4,
        "page_size2_L1": 2048,
        "page_size3_L1": 1024*1024,
        "assoc1_L1": 4,
        "size1_L1": 64,
        "assoc2_L1": 4,
        "size2_L1": 32,
        "assoc3_L1": 4,
        "size3_L1": 4,
        "sizes_L2": 3,
        "page_size1_L2": 4,
        "page_size2_L2": 2048,
        "page_size3_L2": 1024*1024,
        "assoc1_L2": 12,
        "size1_L2": 1536,
        "assoc2_L2": 12,
        "size2_L2": 1536,
        "assoc3_L2": 4,
        "size3_L2": 16,
        "clock": "2 Ghz",
        "levels": 2,
        "max_width_L1": 16,
        "max_outstanding_L1": 2,
        "latency_L1": 1,
        "parallel_mode_L1": 1,
        "max_outstanding_L2": 2,
        "max_width_L2": 4,
        "latency_L2": 10,
        "parallel_mode_L2": 0,
        "page_walk_latency": 50,

});




mmu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

# Define the simulation links
link_cpu_mmu_link = sst.Link("link_cpu_mmu_link")

link_mmu_cache_link = sst.Link("link_mmu_cache_link")


'''
arielMMULink = sst.Link("cpu_mmu_link_" + str(next_core_id))
                MMUCacheLink = sst.Link("mmu_cache_link_" + str(next_core_id))
                arielMMULink.connect((ariel, "cache_link_%d"%next_core_id, ring_latency), (mmu, "cpu_to_mmu%d"%next_core_id, ring_latency))
                MMUCacheLink.connect((mmu, "mmu_to_cache%d"%next_core_id, ring_latency), (l1, "high_network_0", ring_latency))
                arielMMULink.setNoCut()
                MMUCacheLink.setNoCut()
'''


link_cpu_mmu_link.connect( (comp_cpu, "cache_link", "0ps"), (mmu, "cpu_to_mmu0", "0ps") )
link_cpu_mmu_link.setNoCut()

link_mmu_cache_link.connect( (mmu, "mmu_to_cache0", "50ps"), (comp_l1cache, "high_network_0", "50ps") )
link_mmu_cache_link.setNoCut()



link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
