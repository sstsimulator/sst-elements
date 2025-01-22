import sst
import sys
from mhlib import componentlist

# Part of test set that tests all combinations of coherence protocols
# with each other and as last-level and not and as distributed and not
#
# Covers test cases with a zero, one or two-level hierarchy 
# (no caches, L1 only, L1+all combos of next level, and L1+L2+L3 for a specific case that only works as a private hierarchy)

## Options
# 0: L1
# 1: L1 + L2 (inclusive)
# 2: L1 + L2 (noninclusive/no tag dir)
# 3: L1 + L2 (noninclusive w/ tag dir)
# 4: L1 + Directory
# 5: L1 + L2 (noninclusive/no tag dir) + L3 (noninclusive/no tag dir)
# 6: No caches

# Define the simulation components
verbose = 2

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 11

option = 0

if len(sys.argv) < 5:
    print("Argument count is incorrect. Required: <test_case> <random_seed> <coherence_protocol> <enable_llsc>")
    exit(0)

option = int(sys.argv[1])
cpu_seed = int(sys.argv[2])
protocol = sys.argv[3]
llsc = sys.argv[4] == "yes"

outdir = ""
if len(sys.argv) >= 6:
    outdir = sys.argv[5]

cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
    "memFreq" : 2,
    "memSize" : "5KiB",
    "verbose" : 0,
    "clock" : "3.5GHz",
    "rngseed" : cpu_seed,
    "maxOutstanding" : 16,
    "opCount" : 2500,
    "reqsPerIssue" : 3,
    "write_freq" : 36, # 36% writes
    "read_freq" : 58,  # 60% reads
    "flushcache_freq" : 3
})
if llsc:
    cpu.addParam("llsc_freq", 3)   # 3% LLSC

iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

if option != 6:
    l1cache = sst.Component("l1cache", "memHierarchy.Cache")
    l1cache.addParams({
        "access_latency_cycles" : "3",
        "cache_frequency" : "3.5Ghz",
        "replacement_policy" : "lru",
        "coherence_protocol" : protocol,
        "associativity" : "4",
        "cache_line_size" : "64",
        "debug" : DEBUG_L1,
        "debug_level" : DEBUG_LEVEL,
        "verbose" : verbose,
        "L1" : "1",
        "cache_size" : "2KiB"
    })
    
    # Link core & l1cache
    link_cpu_l1 = sst.Link("link_cpu_l1")
    link_cpu_l1.connect( (iface, "lowlink", "400ps"), (l1cache, "highlink", "400ps") )


memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "verbose" : verbose,
    "addr_range_end" : 512*1024*1024-1,
    "backing" : "malloc",
    "backing_size_unit" : "1KiB",
    "backing_init_zero" : True,
    "backing_out_file" : "{}/test_memHierarchy_coherence_1core_case{}_{}.malloc".format(outdir, option, protocol),
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000ns",
    "mem_size" : "512MiB"
})


## Base L2 params
l2cache_params = {
        "cache_size" : "1KiB",
        "associativity" : 8,
        "access_latency_cycles" : 11,
        "cache_frequency" : "2.4GHz",
        "replacement_policy" : "nmru",
        "coherence_protocol" : protocol,
        "cache_line_size" : 64,
        "debug" : DEBUG_L2,
        "debug_level" : DEBUG_LEVEL
    }

### Options for level between l1 & memory, default is none

if option == 0: # L1 <-> Mem
    link_l1_mem = sst.Link("link_l1_mem")
    link_l1_mem.connect( (l1cache, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )

elif option == 1: # L1 <-> L2/inclusive <-> Mem
    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)
    l2cache.addParam("cache_size", "2KiB") # override base param since cache is inclusive
    
    link_l1_l2 = sst.Link("link_l1_l2")
    link_l1_l2.connect( (l1cache, "lowlink", "50ps"), (l2cache, "highlink", "50ps") )
    
    link_l2_mem = sst.Link("link_l2_mem")
    link_l2_mem.connect( (l2cache, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )

elif option == 2: # L1 <-> L2/priv noninclusive <-> Mem
    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)
    l2cache.addParam("cache_type", "noninclusive")
    
    link_l1_l2 = sst.Link("link_l1_l2")
    link_l1_l2.connect( (l1cache, "lowlink", "50ps"), (l2cache, "highlink", "50ps") )
    
    link_l2_mem = sst.Link("link_l2_mem")
    link_l2_mem.connect( (l2cache, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )

elif option == 3: # L1 <-> L2/shr noninclusive <-> Mem
    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)
    l2cache.addParams({"cache_type" : "noninclusive_with_directory", "noninclusive_directory_entries" : 40, "noninclusive_directory_associativity" : 4})
    
    link_l1_l2 = sst.Link("link_l1_l2")
    link_l1_l2.connect( (l1cache, "lowlink", "50ps"), (l2cache, "highlink", "50ps") )
    
    link_l2_mem = sst.Link("link_l2_mem")
    link_l2_mem.connect( (l2cache, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )

elif option == 4: # L1 <-> DC <-> Mem
    directory = sst.Component("directory", "memHierarchy.DirectoryController")
    directory.addParams({
        "clock" : "2.4GHz",
        "entry_cache_size" : 48,
        "coherence_protocol" : protocol,
        "mshr_num_entries" : 8,
        "access_latency_cycles" : 2,
        "mshr_latency_cycles" : 1,
        "max_requests_per_cycle" : 2,
        "debug_level" : DEBUG_LEVEL,
        "debug" : DEBUG_L2
        })
    
    link_l1_dc = sst.Link("link_l1_dc")
    link_l1_dc.connect( (l1cache, "lowlink", "50ps"), (directory, "highlink", "50ps") )
    
    link_dc_mem = sst.Link("link_dc_mem")
    link_dc_mem.connect( (directory, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )

elif option == 5: ## L1 <-> L2/priv noninclusive <-> 3/priv noninclusive <-> Mem
    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)
    l2cache.addParam("cache_type", "noninclusive")
    
    link_l1_l2 = sst.Link("link_l1_l2")
    link_l1_l2.connect( (l1cache, "lowlink", "50ps"), (l2cache, "highlink", "50ps") )

    l3cache = sst.Component("l3cache", "memHierarchy.Cache")
    l3cache.addParams(l2cache_params)
    l3cache.addParam("cache_type", "noninclusive")
    
    link_l2_l3 = sst.Link("link_l2_l3")
    link_l2_l3.connect( (l2cache, "lowlink", "50ps"), (l3cache, "highlink", "50ps") )

    link_l3_mem = sst.Link("link_l3_mem")
    link_l3_mem.connect( (l3cache, "lowlink", "50ps"), (memctrl, "highlink", "50ps") )

elif option == 6: ## Core <-> Mem
    link_cpu_mem = sst.Link("link_cpu_mem")
    link_cpu_mem.connect( (iface, "lowlink", "400ps"), (memctrl, "highlink", "400ps") )

else:
    print("Error, option=%d is not valid. Options 0-5 are allowed\n"%option)
    sys.exit(-1)

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

