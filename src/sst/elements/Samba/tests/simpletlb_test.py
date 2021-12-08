import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")


memory_mb = 1024 #1GB
virt_region_size_mb = 128

# Define the simulation components
comp_cpu = sst.Component("cpu", "miranda.BaseCPU")
comp_cpu.addParams({
	"verbose" : 1,
})
cpugen = comp_cpu.setSubComponent("generator", "miranda.GUPSGenerator")
cpugen.addParams({
	"verbose" : 0,
	"count" : 10000,
	"max_address" : virt_region_size_mb
})


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
        "debug" : "1",          
        "debug_level" : "10",    
        "debug_addresses": "[]",
        "verbose": "1",
})

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(2)
# Enable statistics outputs
comp_cpu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
comp_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
#comp_l1cache.enableStatistics(["GetS_recv","GetX_recv"])



# ====== Memory
comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
      "clock" : "1GHz"
})
memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "50 ns",
    "mem_size" : str(memory_mb * 1024 * 1024) + "B",
})


# ====== Custom TLB component
comp_tlb = sst.Component("comp_TLB", "Samba.SimpleTLB")
comp_tlb.addParams({
    "verbose": 1,
    "fixed_mapping_va_start": "0x0",
    "fixed_mapping_pa_start": "0xF000000",
    "fixed_mapping_len": "128MB",
})




#=========== SetupLinks
# Define the simulation links
#link_cpu_mmu_link = sst.Link("link_cpu_mmu_link")
#link_cpu_mmu_link.connect( (comp_cpu, "cache_link", "50ps"), (mmu, "cpu_to_mmu0", "50ps") )
#
#link_mmu_cache_link = sst.Link("link_mmu_cache_link")
#link_mmu_cache_link.connect( (mmu, "mmu_to_cache0", "50ps"), (comp_l1cache, "high_network_0", "50ps") )

def makeLink(name, portA, portB):
    'ports should be tuple like `(component, "port_name", "50ps")`'
    link = sst.Link(name)
    link.connect(portA,portB)

#===== Interpose TLB between cpu and cache
makeLink("link_cpu_tlb",
        (comp_cpu, "cache_link", "50ps"), (comp_tlb, "high_network", "50ps"))

makeLink("link_tlb_l1cache",
        (comp_tlb, "low_network", "50ps"), (comp_l1cache, "high_network_0", "50ps"))


##===== Alternately, self-loop the TLB and connect cpu to cache
#makeLink("link_cpu_l1cache",
#        (comp_cpu, "cache_link", "50ps"), (comp_l1cache, "high_network_0", "50ps"))
#
#makeLink("linkself_tlb",
#        (comp_tlb, "link_high", "50ps"), (comp_tlb, "link_low", "50ps"))


# === CPU to mem
makeLink("link_l1cache_membus",
        (comp_l1cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps"))

