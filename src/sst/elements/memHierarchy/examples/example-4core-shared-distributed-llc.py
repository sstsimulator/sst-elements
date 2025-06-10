import sst

# Configuration describes: Four cores, priave L1s, shared distributed L2, Memory
# Identical to example-4core-shared-llc.py except that the L2 is distributed across four slices
# This requires an extra bus between the L2 and memory (since we have multiple components connecting to one)

## Debug & output controls
verbose = 2
### These do nothing unless sst-core was configured with `--enable-debug`
DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# This script constructs the SST (sub)components and then links them at the end
# Therefore, we need to store some objects so we can connect them
cpu_iface_obj = []  # Store core interfaces
l1cache_obj = []    # Store l1 interfaces
l2cache_obj = []

# Core models - may replace with anything that implements StdMem
## StandardCPU is a test component that generates random memory addresses

cpu_params = {
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
}

### Create four cores
for x in range(4):
    cpu = sst.Component("core" + str(x), "memHierarchy.standardCPU")
    cpu.addParams(cpu_params)
    cpu.addParam("rngseed", 111+x) # Give each core a unique rngseed
    iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
    #iface.addParams({"debug" : 1, "debug_level" : 10})
    cpu_iface_obj.append(iface)

# L1 cache
## 16KiB, 4-way set associative with 64B lines
## MESI coherence, LRU replacement policy
## 3 cycle hit latency, 3.5GHz clock
l1_params = {
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
}
for x in range(4):
    l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
    l1cache.addParams(l1_params)
    l1cache_obj.append(l1cache)

# L2 cache
## 384KiB, 12-way set associative with 64B lines
## Distributed across four slices (96KiB each)
## MESI coherence, LRU replacement policy
## 10 cycle hit latency, 3.5GHz clock
## Inclusive
l2cache_params = {
    "access_latency_cycles" : "10",
    "cache_frequency" : "3.5Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "mesi",
    "associativity" : "12",
    "cache_line_size" : "64",
    "cache_size" : "96KiB",
    ## These next two parameters are needed for distributing
    "num_cache_slices" : 4,
    "slice_allocation_policy" : "rr", # Round-robin
    "debug" : DEBUG_L2,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
}

for x in range(4):
    l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
    l2cache.addParams(l2cache_params)
    l2cache.addParam("slice_id", x)
    l2cache_obj.append(l2cache)

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


# In this example, the 4 L1s will need to connect to 4 L2s
# And the 4 L2s need to connect to 1 memory
# For that, we need 2 buses, one on each side of the l2s
# Looks like: cpu0 -> l1cache0 -> bus0 -> l2cache0 -> bus1 -> memory
#             cpu1 -> l1cache1 ---^    ^- l2cache1 ---^
#             cpu2 -> l1cache2 ---^    ^- l2cache2 ---^
#             cpu3 -> l1cache3 ---^    ^- l2cache3 ---^

bus0 = sst.Component("bus0", "memHierarchy.Bus")
bus0.addParam("bus_frequency", "3.5GHz")

bus1 = sst.Component("bus1", "memHierarchy.Bus")
bus1.addParam("bus_frequency", "3.5GHz")

# Define the simulation links
for x in range(4):
    link_cpu_l1cache = sst.Link("link_cpu_l1cache" + str(x))
    link_cpu_l1cache.connect( (cpu_iface_obj[x], "lowlink", "500ps"), (l1cache_obj[x], "highlink", "500ps") )

    link_l1cache_bus = sst.Link("link_l1cache_bus" + str(x))
    link_l1cache_bus.connect( (l1cache_obj[x], "lowlink", "500ps"), (bus0, "highlink" + str(x), "500ps") )

    link_bus_l2cache = sst.Link("link_bus_l2cache" + str(x))
    link_bus_l2cache.connect( (bus0, "lowlink" + str(x), "500ps"), (l2cache_obj[x], "highlink", "500ps") )

    link_l2cache_bus = sst.Link("link_l2cache_bus" + str(x))
    link_l2cache_bus.connect( (l2cache_obj[x], "lowlink", "500ps"), (bus1, "highlink" + str(x), "500ps") )


link_bus_mem = sst.Link("link_bus_mem")
link_bus_mem.connect( (bus1, "lowlink0", "500ps"), (memctrl, "highlink", "500ps") )

# Enable statistics for all three components
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()


