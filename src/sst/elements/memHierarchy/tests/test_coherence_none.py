import sst
import sys
from mhlib import componentlist
from mhlib import Bus, MemLink

# Part of test set that tests all combinations of coherence protocols
# with each other and as last-level and not and as distributed and not
#
# Covers test cases with incoherent caches
# (L1 only, L1+L2, and L1+L2+L3)

## Options
# 0: L1
# 1: L1 + L2
# 2: L1 + L2/distributed
# 3: L1 + L2 + L3
# 4: L1 + L2/distributed + L3/distributed

# Define the simulation components
verbose = 2

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 11

option = 0

if len(sys.argv) < 3:
    print("Argument count is incorrect. Required: <test_case> <random_seed>")
    exit(0)

option = int(sys.argv[1])
cpu_seed = int(sys.argv[2])

outdir = ""
if len(sys.argv) >= 4:
    outdir = sys.argv[3]

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
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
    #"flushcache_freq" : 3 # Incoherent does not yet handle flush
})

iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
    "access_latency_cycles" : "3",
    "cache_frequency" : "3.5Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "none",
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
})

if outdir != "":
    memctrl.addParam("backing_out_file", "{}/test_memHierarchy_coherence_none_case{}_none.malloc.mem".format(outdir, option))

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
        "coherence_protocol" : "none",
        "cache_type" : "noninclusive",
        "cache_line_size" : 64,
        "debug" : DEBUG_L2,
        "debug_level" : DEBUG_LEVEL
    }


## Base L3 params
l3cache_params = {
        "cache_size" : "1KiB",
        "associativity" : 4,
        "access_latency_cycles" : 16,
        "cache_frequency" : "2.4GHz",
        "replacement_policy" : "nmru",
        "coherence_protocol" : "none",
        "cache_type" : "noninclusive",
        "cache_line_size" : 64,
        "debug" : DEBUG_L3,
        "debug_level" : DEBUG_LEVEL
    }


# Parameter sets used in some cases
cache_distributed_params_two_way = {
    "num_cache_slices" : 2,
    "slice_allocation_policy" : "rr"
}
cache_distributed_params_four_way = {
    "num_cache_slices" : 4,
    "slice_allocation_policy" : "rr"
}

bus_params = {"bus_frequency" : "2GHz"}

# Convenience link constructor
memLink = MemLink("50ps")

### Options for level between l1 & memory, default is none
if option == 0: # L1 <-> Mem
    memLink.connect(l1cache, memctrl)

elif option == 1: # L1 <-> L2 <-> Mem
    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)

    memLink.connect(l1cache, l2cache)
    memLink.connect(l2cache, memctrl)

elif option == 2: # L1 <-> L2/distrib <-> Mem
    cache_bus = Bus("cachebus", bus_params, "50ps", [l1cache] )
    mem_bus = Bus("membus", bus_params, "50ps", lowcomps=[memctrl])

    for x in range(4):
        l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
        l2cache.addParams(l2cache_params)
        l2cache.addParams(cache_distributed_params_four_way)
        l2cache.addParam("slice_id", x)
        cache_bus.connectLow(l2cache)
        mem_bus.connectHigh(l2cache)

elif option == 3: # L1 <-> L2 <-> L3
    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)

    l3cache = sst.Component("l3cache", "memHierarchy.Cache")
    l3cache.addParams(l3cache_params)

    memLink.connect(l1cache, l2cache)
    memLink.connect(l2cache, l3cache)
    memLink.connect(l3cache, memctrl)

elif option == 4: # L1 <-> L2/distrib <-> L3/distr <-> Mem
    l1_bus = Bus("l1_bus", bus_params, "50ps", highcomps=[l1cache])
    l2_bus = Bus("l2_bus", bus_params, "50ps")
    l3_bus = Bus("l3_bus", bus_params, "50ps", lowcomps=[memctrl])

    for x in range(4):
        l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
        l2cache.addParams(l2cache_params)
        l2cache.addParams(cache_distributed_params_four_way)
        l2cache.addParam("slice_id", x)
        l1_bus.connectLow(l2cache)
        l2_bus.connectHigh(l2cache)

    for x in range(2):
        l3cache = sst.Component("l3cache" + str(x), "memHierarchy.Cache")
        l3cache.addParams(l3cache_params)
        l3cache.addParams(cache_distributed_params_two_way)
        l3cache.addParam("slice_id", x)
        l2_bus.connectLow(l3cache)
        l3_bus.connectHigh(l3cache)
else:
    print("Error, option=%d is not valid. Options 0-4 are allowed\n"%option)
    sys.exit(-1)

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

