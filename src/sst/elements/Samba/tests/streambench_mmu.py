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
	"clock" : "2.4GHz",
	"printStats" : 1,
})
cpugen = comp_cpu.setSubComponent("generator", "miranda.STREAMBenchGenerator")
cpugen.addParams({
        "verbose" : 0,
        "n" : 10000,
        "operandwidth" : 16,
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
      "debug" : "0",
      "L1" : "1",
      "cache_size" : "32KB"
})

# Enable statistics outputs
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "clock" : "1GHz"
})
mem = comp_memory.setSubComponent("backend", "memHierarchy.simpleMem")
mem.addParams({
      "access_time" : "100 ns",
      "mem_size" : "4096MiB",
})

mmu = sst.Component("mmu0", "Samba")
mmu.addParams({
        "corecount":  1,
        "page_size_L1": 4,
        "assoc_L1": 32,
        "size_L1": 32,
        "clock": "2.4GHz",
        "levels": 2,
        "max_width_L1": 2,
        "max_outstanding_L1": 2,
        "latency_L1": 1,
        "parallel_mode_L1": 0,
        "page_size_L2": 4,
        "assoc_L2": 8,
        "size_L2": 128,
        "max_outstanding_L2": 2,
        "max_width_L2": 2,
        "latency_L2": 10,
        "upper_link_L1": 1, # we assume same link latency of up and down traffic of the link
        "upper_link_L2": 1,
        "parallel_mode_L2": 0
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


link_cpu_mmu_link.connect( (comp_cpu, "cache_link", "50ps"), (mmu, "cpu_to_mmu0", "50ps") )
link_cpu_mmu_link.setNoCut()

link_mmu_cache_link.connect( (mmu, "mmu_to_cache0", "50ps"), (comp_l1cache, "high_network_0", "50ps") )
link_mmu_cache_link.setNoCut()



link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
                                                                                                             
