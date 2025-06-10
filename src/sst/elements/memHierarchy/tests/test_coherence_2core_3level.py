import sst
import sys
from mhlib import componentlist
from mhlib import Bus

# Part of test set that tests all combinations of coherence protocols
# with each other and as last-level and not and as distributed and not
#
# Covers test cases with a three-level hierarchy (3 cache levels or 2 cache levels + directory)

##################
## Options
##  0.  L2 (inclusive/private)                  + L3 (inclusive/shared)
##  1.  L2 (inclusive/shared/distributed)       + L3 (inclusive/shared/distributed)
##  2.  L2 (noninclusive/no tag dir)            + L3 inclusive/shared)
##  3.  L2 (noninclusive w/ tag dir)            + L3 (inclusive/shared)
##  4.  L2 (inclusive/shared)                   + L3 (noninclusive/no tag dir)
##  5.  L2 (inclusive/private)                  + L3 (noninclusive w/ tag dir/distributed)
##  6.  L2 (inclusive/private)                  + directory
##  7.  L2 (noninclusive w/ tag dir)            + L3 (noninclusive/no tag dir/distributed)
##  8.  L2 (noninclusive/no tag dir)            + directory (distributed)
##  9.  L2 (noninclusive/no tag dir)            + L3 (noninclusive w/tag dir)
##  10. L2 (noninclusive w/ tag dir/distributed + L3 (noninclusive w/ tag dir)
##  11. L2 (noninclusive w/ tag dir/distributed + directory

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_BUS = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 11

option = 0;
if len(sys.argv) < 6:
   print("Argument count is incorrect. Required: <test_case> <random_seed0> <random_seed1> <coherence_protocol> <enable_llsc>")
   sys.exit(-1)

option = int(sys.argv[1])
seeds = [int(sys.argv[2]), int(sys.argv[3])]
protocol = sys.argv[4]
llsc = sys.argv[5] == "yes"

outdir = ""
if len(sys.argv) >= 7:
    outdir = sys.argv[6]

cpu_params = {
    "memFreq" : 1,
    "memSize" : "16KiB",
    "verbose" : 0,
    "clock" : "3GHz",
    "maxOutstanding" : 8,
    "opCount" : 3000,
    "reqsPerIssue" : 4,
}
# Core 0
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams(cpu_params)
cpu0.addParams({
    "rngseed" : seeds[0],
    "write_freq" : 38, # 38% writes
    "read_freq" : 58,  # 58% reads
    "flushcache_freq" : 2, # 2% FlushAll
})
if llsc:
    cpu0.addParam("llsc_freq", 2)   # 2% LLSC
cpu0_iface = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 1
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams(cpu_params)
cpu1.addParams({
    "rngseed" : seeds[1],
    "write_freq" : 34, # 34% writes
    "read_freq" : 62,  # 62% reads
    "flushcache_freq" : 3,
})
if llsc:
    cpu1.addParam("llsc_freq", 1)   # 2% LLSC
cpu1_iface = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")

# L1s
l1cache_params = {
      "access_latency_cycles" : 2,
      "cache_frequency" : "3Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : protocol,
      "associativity" : 4,
      "mshr_num_entries" : 8,
      "cache_line_size" : "64",
      "debug_level" : DEBUG_LEVEL,
      "debug" : DEBUG_L1,
      "L1" : "1",
      "cache_size" : "1 KiB"
}

l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
l1cache0.addParams(l1cache_params)
l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
l1cache1.addParams(l1cache_params)

link0_cpu_l1 = sst.Link("link0_cpu_l1")
link0_cpu_l1.connect( (cpu0_iface, "lowlink", "100ps"), (l1cache0, "highlink", "100ps") )
link1_cpu_l1 = sst.Link("link1_cpu_l1")
link1_cpu_l1.connect( (cpu1_iface, "lowlink", "100ps"), (l1cache1, "highlink", "100ps") )

# Memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
    "backing" : "malloc",
    "backing_size_unit" : "1KiB",
    "backing_init_zero" : True,
    "backing_out_file" : "{}/test_memHierarchy_coherence_2core_3level_case{}_{}.malloc.mem".format(outdir, option, protocol),
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "512MiB",
      "access_time" : "80ns",
})


## Some parameter sets used below
cache_distributed_params = {
    "num_cache_slices" : 2,
    "slice_allocation_policy" : "rr"
}

cache_noninclusive_private_params = {
    "cache_type" : "noninclusive"
}

l2cache_noninclusive_shared_params = {
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 128, # Cover 2x 1KiB L1 + 1x 2KiB L2 (or 2x 2KiB L2)
    "noninclusive_directory_associativity" : 4
}

l3cache_noninclusive_shared_params = {
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 128,
    "noninclusive_directory_associativity" : 4
}

bus_params = {"bus_frequency" : "3GHz", "debug" : DEBUG_BUS, "debug_level" : DEBUG_LEVEL}


#########################
## Construct L2 level
#########################

l2cache_base_params = {
    "access_latency_cycles" : "11",
    "cache_frequency" : "3GHz",
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "mshr_num_entries" : 16,
    "cache_line_size" : "64",
    "debug_level" : DEBUG_LEVEL,
    "debug" : DEBUG_L2,
}
l2cache_size_full = {"cache_size" : "4KiB"} # Single (shared) inclusive cache
l2cache_size_half = {"cache_size" : "2KiB"} # Multiple (private or shared/distributed) caches, single shared noninclusive
l2cache_size_quarter = {"cache_size" : "1KiB"} # Distributed shared noninclusive

l2_bus = None # Only need an L2 bus if we have multiple L2s and/or multiple L3s

if option == 0 or option == 5 or option == 6:   ##  L2 inclusive/private/single
    params = dict(l2cache_base_params)
    params.update(l2cache_size_half)

    l2cache0 = sst.Component("l2cache0", "memHierarchy.Cache")
    l2cache0.addParams(params)

    l2cache1 = sst.Component("l2cache1", "memHierarchy.Cache")
    l2cache1.addParams(params)

    # Connect L1<->L2
    link0_l1_l2 = sst.Link("link0_l1_l2")
    link1_l1_l2 = sst.Link("link1_l1_l2")
    link0_l1_l2.connect((l1cache0, "lowlink", "100ps"), (l2cache0, "highlink", "100ps"))
    link1_l1_l2.connect((l1cache1, "lowlink", "100ps"), (l2cache1, "highlink", "100ps"))

    # Create L2 bus and connect L2s. Will connect L3(s) or directory(s) later.
    l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache0, l2cache1])

elif option == 1:                               ##  L2 inclusive/shared/distributed
    params = dict(l2cache_base_params)
    params.update(l2cache_size_half)
    params.update(cache_distributed_params)

    l2cache0 = sst.Component("l2cache0", "memHierarchy.Cache")
    l2cache0.addParams(params)
    l2cache0.addParam("slice_id", 0)

    l2cache1 = sst.Component("l2cache1", "memHierarchy.Cache")
    l2cache1.addParams(params)
    l2cache1.addParam("slice_id", 1)

    # Create L1 bus and connect L1s to L2s
    Bus("l1bus", bus_params, "100ps", [l1cache0, l1cache1], [l2cache0, l2cache1])

    # Create L2 bus and connect L2s. Will connect L3(s) or directory(s) later.
    l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache0, l2cache1])

elif option == 4:                               ##  L2 inclusive/shared-single
    params = dict(l2cache_base_params)
    params.update(l2cache_size_full)

    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(params)

    # Create L1 bus and connect L1s to L2
    Bus("l1bus", bus_params, "100ps", [l1cache0, l1cache1], [l2cache])

elif option == 2 or option == 8 or option == 9: ##  L2 noninclusive/private
    params = dict(l2cache_base_params)
    params.update(l2cache_size_half)
    params.update(cache_noninclusive_private_params)

    l2cache0 = sst.Component("l2cache0", "memHierarchy.Cache")
    l2cache0.addParams(params)

    l2cache1 = sst.Component("l2cache1", "memHierarchy.Cache")
    l2cache1.addParams(params)

    # Connect L1<->L2
    link0_l1_l2 = sst.Link("link0_l1_l2")
    link1_l1_l2 = sst.Link("link1_l1_l2")
    link0_l1_l2.connect((l1cache0, "lowlink", "100ps"), (l2cache0, "highlink", "100ps"))
    link1_l1_l2.connect((l1cache1, "lowlink", "100ps"), (l2cache1, "highlink", "100ps"))

    # Create L2 bus and connect L2s. Will connect L3(s) or directory(s) later.
    l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache0, l2cache1])

elif option == 3 or option == 7:                ##  L2 noninclusive/shared-single
    params = dict(l2cache_base_params)
    params.update(l2cache_size_half)
    params.update(l2cache_noninclusive_shared_params)

    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(params)

    # Create L1 bus and connect L1s to L2
    Bus("l1bus", bus_params, "100ps", [l1cache0, l1cache1], [l2cache])


elif option == 10 or option == 11:              ##  L2 noninclusive/shared-distributed
    params = dict(l2cache_base_params)
    params.update(l2cache_size_quarter)
    params.update(cache_distributed_params)
    params.update(l2cache_noninclusive_shared_params)

    l2cache0 = sst.Component("l2cache0", "memHierarchy.Cache")
    l2cache0.addParams(params)
    l2cache0.addParam("slice_id", 0)

    l2cache1 = sst.Component("l2cache1", "memHierarchy.Cache")
    l2cache1.addParams(params)
    l2cache1.addParam("slice_id", 1)

    # Create L1 bus and connect L1s to L2s
    Bus("l1bus", bus_params, "100ps", [l1cache0, l1cache1], [l2cache0, l2cache1])

    # Create L2 bus and connect L2s. Will connect L3(s) or directory(s) later.
    l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache0, l2cache1])


else:
    print("Error, option=%d is not valid. Options 0-11 are allowed\n"%option)
    sys.exit(-1)


#########################
## Construct L3 level
#########################
l3cache_base_params = {
    "access_latency_cycles" : "18",
    "cache_frequency" : "3 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "12",
    "cache_line_size" : "64",
    "mshr_num_entries" : 32,
    "debug_level" : DEBUG_LEVEL,
    "debug" : DEBUG_L2,
}

l3cache_size_full = {"cache_size" : "6KiB"} # Single (shared) cache
l3cache_size_half = {"cache_size" : "3KiB"} # Multiple (private or shared/distributed) caches
l3cache_size_quarter = {"cache_size" : "1.5KiB"} # Multiple distributed noninclusive caches


if option == 0 or option == 2 or option == 3: ## L3 inclusive/shared/single
    l3cache = sst.Component("l3cache", "memHierarchy.Cache")
    l3cache.addParams(l3cache_base_params)
    l3cache.addParams(l3cache_size_full)

    # Connect l3cache to l2bus
    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache])
    else:
        link = sst.Link("link_l2_l3")
        link.connect( (l2cache, "lowlink", "100ps"), (l3cache, "highlink", "100ps") )

    link = sst.Link("link_l3_memory")
    link.connect( (l3cache, "lowlink", "100ps"), (memctrl, "highlink", "100ps") )

elif option == 1: ## L3 inclusive/shared/distributed
    params = dict(l3cache_base_params)
    params.update(cache_distributed_params)
    params.update(l3cache_size_half)

    l3cache0 = sst.Component("l3cache0", "memHierarchy.Cache")
    l3cache0.addParams(params)
    l3cache0.addParam("slice_id", 0)

    l3cache1 = sst.Component("l3cache1", "memHierarchy.Cache")
    l3cache1.addParams(params)
    l3cache1.addParam("slice_id", 1)

    # Connect l2 <-> l3 via a bus
    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache0,l3cache1])
    else:
        l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache], [l3cache0, l3cache1])

    # Connect l3 <-> memory controller via a bus
    mem_bus = Bus("membus", bus_params, "100ps", [l3cache0, l3cache1], [memctrl])

elif option == 4:   ##  4. L3 noninclusive/private/single
    params = dict(l3cache_base_params)
    params.update(l3cache_size_half)
    params.update(cache_noninclusive_private_params)

    l3cache = sst.Component("l3cache", "memHierarchy.Cache")
    l3cache.addParams(params)

    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache])
    else:
        link = sst.Link("link_l2_l3")
        link.connect( (l2cache, "lowlink", "100ps"), (l3cache, "highlink", "100ps") )

    link = sst.Link("link_l3_memory")
    link.connect( (l3cache, "lowlink", "100ps"), (memctrl, "highlink", "100ps") )

elif option == 5:   ##  L3 noninclusive/shared/distributed
    params = dict(l3cache_base_params)
    params.update(l3cache_size_quarter)
    params.update(l3cache_noninclusive_shared_params)
    params.update(cache_distributed_params)

    l3cache0 = sst.Component("l3cache0", "memHierarchy.Cache")
    l3cache0.addParams(params)
    l3cache0.addParam("slice_id", 0)

    l3cache1 = sst.Component("l3cache1", "memHierarchy.Cache")
    l3cache1.addParams(params)
    l3cache1.addParam("slice_id", 1)

    # Connect l2 <-> l3 via a bus
    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache0,l3cache1])
    else:
        l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache], [l3cache0, l3cache1])

    # Connect l3 <-> memory controller via a bus
    mem_bus = Bus("membus", bus_params, "100ps", [l3cache0, l3cache1], [memctrl])

elif option == 6 or option == 11:   ##  directory/single
    directory = sst.Component("directory", "memHierarchy.DirectoryController")
    directory.addParams({
        "clock" : "2GHz",
        "entry_cache_size" : 256,
        "coherence_protocol" : protocol,
        "mshr_num_entries" : 32,
        "access_latency_cycles" : 1,
        "mshr_latency_cycles" : 1,
        "max_requests_per_cycle" : 2,
        "debug_level" : DEBUG_LEVEL,
        "debug" : DEBUG_L3,
    })

    # Connect l2 <-> l3 via a bus
    if l2_bus:
        l2_bus.connect(lowcomps=[directory])
    else:
        link = sst.Link("link_l2_dir")
        link.connect( (l2cache, "lowlink", "100ps"), (directory, "highlink", "100ps") )

    # Connect directory <-> memory controller
    link = sst.Link("link_dir_mem")
    link.connect( (directory, "lowlink", "100ps"), (memctrl, "highlink", "100ps") )

elif option == 7: ##  L3 noninclusive/private/distributed
    params = dict(l3cache_base_params)
    params.update(l3cache_size_quarter)
    params.update(cache_noninclusive_private_params)
    params.update(cache_distributed_params)

    l3cache0 = sst.Component("l3cache0", "memHierarchy.Cache")
    l3cache0.addParams(params)
    l3cache0.addParam("slice_id", 0)

    l3cache1 = sst.Component("l3cache1", "memHierarchy.Cache")
    l3cache1.addParams(params)
    l3cache1.addParam("slice_id", 1)

    # Connect l2 <-> l3 via a bus
    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache0,l3cache1])
    else:
        l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache], [l3cache0, l3cache1])

    # Connect l3 <-> memory controller via a bus
    mem_bus = Bus("membus", bus_params, "100ps", [l3cache0, l3cache1], [memctrl])

elif option == 8: ##  8. directory/distributed
    params = {
        "clock" : "2GHz",
        "entry_cache_size" : 256,
        "coherence_protocol" : protocol,
        "mshr_num_entries" : 32,
        "access_latency_cycles" : 1,
        "mshr_latency_cycles" : 1,
        "max_requests_per_cycle" : 2,
        "debug_level" : DEBUG_LEVEL,
        "debug" : DEBUG_L3,
        "interleave_step" : "256B",
        "interleave_size" : "128B", # interleave 128B chunks (2 cachelines)
    }

    directory0 = sst.Component("directory0", "memHierarchy.DirectoryController")
    directory0.addParams(params)
    directory0.addParam("addr_range_start", 0)

    directory1 = sst.Component("directory1", "memHierarchy.DirectoryController")
    directory1.addParams(params)
    directory1.addParam("addr_range_start", 128)

    # Connect l2 <-> l3 via a bus
    if l2_bus:
        l2_bus.connect(lowcomps=[directory0, directory1])
    else:
        Bus("l2bus", bus_params, "100ps", [l2cache], [directory0, directory1])

    # Connect directory <-> memory controller
    mem_bus = Bus("membus", bus_params, "100ps", [directory0, directory1], [memctrl])

elif option == 9 or option == 10:   ## L3 noninclusive/shared/single
    params = dict(l3cache_base_params)
    params.update(l3cache_size_half)
    params.update(l3cache_noninclusive_shared_params)

    l3cache = sst.Component("l3cache", "memHierarchy.Cache")
    l3cache.addParams(params)

    # Connect l3cache to l2bus
    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache])
    else:
        link = sst.Link("link_l2_l3")
        link.connect( (l2cache, "lowlink", "100ps"), (l3cache, "highlink", "100ps") )

    link = sst.Link("link_l3_memory")
    link.connect( (l3cache, "lowlink", "100ps"), (memctrl, "highlink", "100ps") )

else:
    print("Error, option=%d is not valid. Options 0-11 are allowed\n"%option)
    sys.exit(-1)

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

