import sst

# Configuration describes: Single core, L1, L2, L3, Memory

## Debug & output controls
verbose = 2
### These do nothing unless sst-core was configured with `--enable-debug`
DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# Core model - may replace with anything that implements StdMem
## StandardCPU is a test component that generates random memory addresses
cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
    "memFreq" : 2,          # On average, issue requests every 2 cycles
    "memSize" : "1MiB",     # Use memory addresses between 0 & 1MiB 
    "verbose" : 0,          # No extra output
    "clock" : "3.5GHz",     # Run at 3.5GHz
    "rngseed" : 111,        # Random seed for access type and address generation
    "maxOutstanding" : 16,  # No more than 16 memory accesses outstanding at a time
    "opCount" : 2500,       # Stop after 2500 accesses
    "reqsPerIssue" : 3,     # Issue up to 3 requests in a cycle
    "write_freq" : 36,      # 36% of accesses should be writes
    "read_freq" : 63,       # 63% of accesses should be reads
    "llsc_freq" : 1,        # 1% of accesses should be LLSC pairs
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
#iface.addParams({"debug" : 1, "debug_level" : 10})

# L1 cache
## 16KiB, 4-way set associative with 64B lines
## MESI coherence, LRU replacement policy
## 3 cycle hit latency, 3.5GHz clock
l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
    "access_latency_cycles" : "3",
    "cache_frequency" : "3.5Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "mesi",
    "associativity" : "4",
    "cache_line_size" : "64",
    "L1" : "1",
    "cache_size" : "16KiB",
    "debug" : DEBUG_L1,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
})

# L2 cache
## 32KiB, 8-way set associative with 64B lines
## MESI coherence, LRU replacement policy
## 5 cycle hit latency, 3.5GHz clock
## Non-inclusive
l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams({
    "access_latency_cycles" : "5",
    "cache_frequency" : "3.5Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "mesi",
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32KiB",
    "cache_type" : "noninclusive",
    "debug" : DEBUG_L2,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
})

# L3 cache
## 512KiB, 16-way set associative with 64B lines
## MESI coherence, LRU replacement policy
## 12 cycle hit latency, 3.5GHz clock
## Inclusive
l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
    "access_latency_cycles" : "12",
    "cache_frequency" : "3.5Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "mesi",
    "associativity" : "16",
    "cache_line_size" : "64",
    "cache_size" : "512KiB",
    "debug" : DEBUG_L3,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
})

# Memory controller
## Addresses 0-512MiB map to this controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "verbose" : verbose,
    "addr_range_end" : 512*1024*1024-1,
})

# Memory access timing model - may replace with any available backend
## SimpleMem has a constant access latency for all accesses
## 512MiB memory with 1000ns access latency
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000ns",
    "mem_size" : "512MiB"
})

# Define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache")
link_cpu_l1cache.connect( (iface, "lowlink", "500ps"), (l1cache, "highlink", "500ps") )

link_l1cache_l2cache = sst.Link("link_l1cache_l2cache")
link_l1cache_l2cache.connect( (l1cache, "lowlink", "500ps"), (l2cache, "highlink", "500ps") )

link_l2cache_l3cache = sst.Link("link_l2cache_l3cache")
link_l2cache_l3cache.connect( (l2cache, "lowlink", "500ps"), (l3cache, "highlink", "500ps") )

link_cache_mem = sst.Link("link_cache_mem")
link_cache_mem.connect( (l3cache, "lowlink", "500ps"), (memctrl, "highlink", "500ps") )

# Enable statistics for all three components
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()


