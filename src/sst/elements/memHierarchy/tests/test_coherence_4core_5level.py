import sst
import sys
from mhlib import componentlist
from mhlib import Bus

# Part of test set that tests all combinations of coherence protocols
# with each other and as last-level and not and as distributed and not
#
# Covers test cases with a five-level hierarchy (4 cache levels + directory)

##################
## Options
## 0. L2 inclusive/private + L3 inclusive/shared + L4 noninclusive/private + directory
## 1. L2 noninclusive/private + L3 inclusive/shared + L4 noninclusive/shared + directory
## 2. L2 noninclusive/shared + L3 noninclusive/shared + L4 inclusive/shared + directory
## 3. L2 noninclusive/private + L3 noninclusive/private + L4 noninclusive/shared + directory

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_L4 = 0
DEBUG_DC = 0
DEBUG_MEM = 0

option = 0;
if len(sys.argv) != 7:
    print("Argument count is incorrect. Required: <test_case> <random_seed0> <random_seed1> <random_seed2> <random_seed3> <coherence_protocol>")
    
option = int(sys.argv[1])
seeds = [int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])]
protocol = sys.argv[6]

cpu_params = {
    "memFreq" : 1,
    "memSize" : "24KiB",
    "verbose" : 0,
    "clock" : "3GHz",
    "maxOutstanding" : 16,
    "opCount" : 2000,
    "reqsPerIssue" : 4,
}
# Core 0
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams(cpu_params)
cpu0.addParams({
    "rngseed" : seeds[0],
    "write_freq" : 38, # 38% writes
    "read_freq" : 59,  # 59% reads
    "llsc_freq" : 2,   # 2% LLSC
    "flushcache_freq" : 1, # 2% FlushAll
})
cpu0_iface = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 1
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams(cpu_params)
cpu1.addParams({
    "rngseed" : seeds[1],
    "write_freq" : 34, # 34% writes
    "read_freq" : 62,  # 62% reads
    "llsc_freq" : 1,   # 2% LLSC
    "flushcache_freq" : 3,
})
cpu1_iface = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 2
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams(cpu_params)
cpu2.addParams({
    "rngseed" : seeds[2],
    "write_freq" : 43, # 43% writes
    "read_freq" : 54,  # 54% reads
    "llsc_freq" : 2,   # 2% LLSC
    "flushcache_freq" : 1,
})
cpu2_iface = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 3
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
cpu3.addParams(cpu_params)
cpu3.addParams({
    "rngseed" : seeds[3],
    "write_freq" : 33, # 33% writes
    "read_freq" : 64,  # 64% reads
    "llsc_freq" : 1,   # 1% LLSC
    "flushcache_freq" : 2,
})
cpu3_iface = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")


# L1s
l1cache_params = {
      "access_latency_cycles" : 1,
      "cache_frequency" : "3Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : 4,
      "cache_line_size" : "64",
      "mshr_num_entries" : 16,
      "debug_level" : "10",
      "debug" : DEBUG_L1,
      "L1" : "1",
      "cache_size" : "1 KiB"
}

l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
l1cache0.addParams(l1cache_params)
l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
l1cache1.addParams(l1cache_params)
l1cache2 = sst.Component("l1cache2", "memHierarchy.Cache")
l1cache2.addParams(l1cache_params)
l1cache3 = sst.Component("l1cache3", "memHierarchy.Cache")
l1cache3.addParams(l1cache_params)

link0_cpu_l1 = sst.Link("link0_cpu_l1")
link0_cpu_l1.connect( (cpu0_iface, "lowlink", "100ps"), (l1cache0, "highlink", "100ps") )
link1_cpu_l1 = sst.Link("link1_cpu_l1")
link1_cpu_l1.connect( (cpu1_iface, "lowlink", "100ps"), (l1cache1, "highlink", "100ps") )
link2_cpu_l1 = sst.Link("link2_cpu_l1")
link2_cpu_l1.connect( (cpu2_iface, "lowlink", "100ps"), (l1cache2, "highlink", "100ps") )
link3_cpu_l1 = sst.Link("link3_cpu_l1")
link3_cpu_l1.connect( (cpu3_iface, "lowlink", "100ps"), (l1cache3, "highlink", "100ps") )

# Memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "512MiB",
      "access_time" : "80ns",
})

##################
## Options
## 0. L2 inclusive/private + L3 inclusive/shared + L4 noninclusive/private + directory
## 1. L2 noninclusive/private + L3 inclusive/shared + L4 noninclusive/shared + directory
## 2. L2 noninclusive/shared + L3 noninclusive/shared + L4 inclusive/shared + directory
## 3. L2 noninclusive/private + L3 noninclusive/private + L4 noninclusive/shared + directory

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
    "noninclusive_directory_entries" : 64, # 4KiB data covered 
    "noninclusive_directory_associativity" : 4
}

l3cache_noninclusive_shared_params = { 
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 128,
    "noninclusive_directory_associativity" : 4
}

l4cache_noninclusive_shared_params = { 
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 320,
    "noninclusive_directory_associativity" : 8
}

bus_params = {"bus_frequency" : "3GHz"}


#########################
## Construct L2 level
#########################

l2cache_base_params = {
    "access_latency_cycles" : 3,
    "cache_frequency" : "3GHz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "8",
    "cache_line_size" : "64",
    "mshr_num_entries" : 24,
    "debug_level" : "10",
    "debug" : DEBUG_L2,
}
l2cache_size_full = {"cache_size" : "2KiB"} # Inclusive
l2cache_size_half = {"cache_size" : "1KiB"} # Noninclusive

l2_bus = None # Only need an L2 bus if we have multiple L2s and/or multiple L3s

## 0. L2 inclusive/private    + L3 inclusive/shared + L4 noninclusive/private + directory
##      L2=2KiB
##      L3=12KiB
##      L4=4KiB
## 1. L2 noninclusive/private + L3 inclusive/shared + L4 noninclusive/shared + directory
##      L2=1KiB
##      L3=12KiB
##      L4=4KiB, 16KiB tag
## 2. L2 noninclusive/shared  + L3 noninclusive/shared + L4 inclusive/shared + directory 
##      L2=1KiB, 4KiB tag
##      L3=2KiB, 8KiB tag
##      L4=12KiB
## 3. L2 noninclusive/private + L3 noninclusive/private + L4 noninclusive/shared + directory
##      L2=2KiB
##      L3=2KiB
##      L4=4KiB, 20KiB tag

if option == 0 or option == 1 or option == 3: ## L2 inclusive/private or noninclusive/private
    params = dict(l2cache_base_params)
    if option == 1:
        params.update(l2cache_size_half)
        params.update(cache_noninclusive_private_params)
    else:
        params.update(l2cache_size_full)

    l2cache0 = sst.Component("l2cache0", "memHierarchy.Cache")
    l2cache0.addParams(params)
    
    l2cache1 = sst.Component("l2cache1", "memHierarchy.Cache")
    l2cache1.addParams(params)
    
    l2cache2 = sst.Component("l2cache2", "memHierarchy.Cache")
    l2cache2.addParams(params)

    l2cache3 = sst.Component("l2cache3", "memHierarchy.Cache")
    l2cache3.addParams(params)

    link0_l1_l2 = sst.Link("link0_l1_l2")
    link1_l1_l2 = sst.Link("link1_l1_l2")
    link2_l1_l2 = sst.Link("link2_l1_l2")
    link3_l1_l2 = sst.Link("link3_l1_l2")
    link0_l1_l2.connect((l1cache0, "lowlink", "100ps"), (l2cache0, "highlink", "100ps"))
    link1_l1_l2.connect((l1cache1, "lowlink", "100ps"), (l2cache1, "highlink", "100ps"))
    link2_l1_l2.connect((l1cache2, "lowlink", "100ps"), (l2cache2, "highlink", "100ps"))
    link3_l1_l2.connect((l1cache3, "lowlink", "100ps"), (l2cache3, "highlink", "100ps"))
    
    if option != 3:
        l2_bus = Bus("l2bus", bus_params, "100ps", [l2cache0, l2cache1, l2cache2, l2cache3])

elif option == 2: ## L2 noninclusive/shared
    params = dict(l2cache_base_params)
    params.update(l2cache_noninclusive_shared_params)
    params.update(l2cache_size_half)

    l2cache = sst.Component("l2cache", "memHierarchy.Cache")
    l2cache.addParams(params)

    l1_bus = Bus("l1bus", bus_params, "100ps", [l1cache0, l1cache1, l1cache2, l1cache3], [l2cache])

else:
    print("Error, option=%d is not valid. Options 0-3 are allowed\n"%option)
    sys.exit(-1)


#########################
## Construct L3 level
#########################
l3cache_base_params = {
    "access_latency_cycles" : 7,
    "cache_frequency" : "3 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "8",
    "mshr_num_entries" : 48,
    "cache_line_size" : "64",
    "debug_level" : "10",
    "debug" : DEBUG_L3,
}

l3cache_size_inclus = {"cache_size" : "12KiB"} # Inclusive cache, to cover upper levels
l3cache_size_noninclus = {"cache_size" : "2KiB"} # Noninclusive cache

l3_bus = None # Only need an L3 bus if we have multiple L3s

if option != 3: ## L3 inclusive/shared or noninclusive/shared
    params = dict(l3cache_base_params)
    if option != 2:
        params.update(l3cache_size_inclus)
    else:
        params.update(l3cache_size_noninclus)
        params.update(l3cache_noninclusive_shared_params)
    
    l3cache = sst.Component("l3cache", "memHierarchy.Cache")
    l3cache.addParams(params)
    
    if l2_bus:
        l2_bus.connect(lowcomps=[l3cache])
    else:
        link = sst.Link("link_l2_l3")
        link.connect( (l2cache, "lowlink", "100ps"), (l3cache, "highlink", "100ps") )

else: ## option == 3:  L3 noninclusive/private
    params = dict(l3cache_base_params)
    params.update(cache_noninclusive_private_params)
    params.update(l3cache_size_noninclus)

    l3cache0 = sst.Component("l3cache0", "memHierarchy.Cache")
    l3cache0.addParams(params)
    
    l3cache1 = sst.Component("l3cache1", "memHierarchy.Cache")
    l3cache1.addParams(params)
    
    l3cache2 = sst.Component("l3cache2", "memHierarchy.Cache")
    l3cache2.addParams(params)

    l3cache3 = sst.Component("l3cache3", "memHierarchy.Cache")
    l3cache3.addParams(params)

    link0_l2_l3 = sst.Link("link0_l2_l3")
    link1_l2_l3 = sst.Link("link1_l2_l3")
    link2_l2_l3 = sst.Link("link2_l2_l3")
    link3_l2_l3 = sst.Link("link3_l2_l3")
    link0_l2_l3.connect( (l2cache0, "lowlink", "100ps"), (l3cache0, "highlink", "100ps") )
    link1_l2_l3.connect( (l2cache1, "lowlink", "100ps"), (l3cache1, "highlink", "100ps") )
    link2_l2_l3.connect( (l2cache2, "lowlink", "100ps"), (l3cache2, "highlink", "100ps") )
    link3_l2_l3.connect( (l2cache3, "lowlink", "100ps"), (l3cache3, "highlink", "100ps") )
    
    l3_bus = Bus("l3bus", bus_params, "100ps", [l3cache0, l3cache1, l3cache2, l3cache3])

#########################
## Construct L4 level
#########################
l4cache_base_params = {
    "access_latency_cycles" : 7,
    "cache_frequency" : "3 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "8",
    "mshr_num_entries" : 128,
    "cache_line_size" : "64",
    "debug_level" : "10",
    "debug" : DEBUG_L4,
}

l4cache_size_inclus = { "cache_size" : "12KiB" }
l4cache_size_noninclus = { "cache_size" : "4KiB" }

## 0. L2 inclusive/private    + L3 inclusive/shared + L4 noninclusive/private + directory
##      L4=4KiB
## 1. L2 noninclusive/private + L3 inclusive/shared + L4 noninclusive/shared + directory
##      L4=4KiB, 16KiB tag
## 2. L2 noninclusive/shared  + L3 noninclusive/shared + L4 inclusive/shared + directory 
##      L4=12KiB
## 3. L2 noninclusive/private + L3 noninclusive/private + L4 noninclusive/shared + directory
##      L4=4KiB, 20KiB tag

params = dict(l4cache_base_params)
if option == 0:
    params.update(l4cache_size_noninclus)
    params.update(cache_noninclusive_private_params)
elif option == 1 or option == 3:
    params.update(l4cache_size_noninclus)
    params.update(l4cache_noninclusive_shared_params)
else: # option == 2:
    params.update(l4cache_size_inclus)

l4cache = sst.Component("l4cache", "memHierarchy.Cache")
l4cache.addParams(params)

directory = sst.Component("directory", "memHierarchy.DirectoryController")
directory.addParams({
    "clock" : "2GHz",
    "entry_cache_size" : 512,
    "coherence_protocol" : "MESI",
    "mshr_num_entries" : 256,
    "access_latency_cycles" : 1,
    "mshr_latency_cycles" : 1,
    "max_requests_per_cycle" : 2,
    "debug_level" : "10",
    "debug" : DEBUG_DC,
})

# Connect l3cache(s) <-> l4cache
if l3_bus:
    l3_bus.connect(lowcomps=[l4cache])
else:
    link = sst.Link("link_l3_l4")
    link.connect( (l3cache, "lowlink", "100ps"), (l4cache, "highlink", "100ps") ) 

# Connect l4cache <-> directory
link = sst.Link("link_l4_dir")
link.connect( (l4cache, "lowlink", "100ps"), (directory, "highlink", "100ps") ) 

# Connect directory <-> memory controller
link = sst.Link("link_dir_mem")
link.connect( (directory, "lowlink", "100ps"), (memctrl, "highlink", "100ps") ) 


# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

