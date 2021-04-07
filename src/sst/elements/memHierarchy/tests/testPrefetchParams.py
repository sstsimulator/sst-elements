# Automatically generated SST Python input
import sst
from mhlib import componentlist

# Define the simulation components
# cores with private L1/L2
# Shared distributed LLCs
# All caches have prefetchers and limit prefetching


cores = 6
caches = 3  # Number of LLCs on the network
memories = 2
coreclock = "2.4GHz"
uncoreclock = "1.4GHz"
coherence = "MESI"
network_bw = "60GB/s"

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0

# Create merlin network - this is just simple single router
network = sst.Component("network", "merlin.hr_router")
network.addParams({
      "xbar_bw" : network_bw,
      "link_bw" : network_bw,
      "input_buf_size" : "2KiB",
      "num_ports" : cores + caches + memories,
      "flit_size" : "36B",
      "output_buf_size" : "2KiB",
      "id" : "0",  
      "topology" : "merlin.singlerouter"
})
network.setSubComponent("topology","merlin.singlerouter")

for x in range(cores):
    cpu = sst.Component("cpu" + str(x), "memHierarchy.streamCPU")
    cpu.addParams({
        "clock" : coreclock,
        "commFreq" : 4, # issue request every 4th cycle
        "rngseed" : 99+x,
        "do_write" : 1,
        "num_loadstore" : 1500,
        "addressoffset" : 1024, # Stream between addresses 1024 & 16384
        "memSize" : 1024*4
    })
    iface = cpu.setSubComponent("memory", "memHierarchy.memInterface")
    
    l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
    l1cache.addParams({
        "cache_frequency" : coreclock,
        "access_latency_cycles" : 3,
        "tag_access_latency_cycles" : 1,
        "mshr_latency_cycles" : 2,
        "replacement_policy" : "lfu",
        "coherence_protocol" : coherence,
        "cache_size" : "2KiB",  # super tiny for lots of traffic
        "associativity" : 2,
        "L1" : 1,
        "debug" : DEBUG_L1,
        #"debug_addr" : "[1152]",
        "debug_level" : 10,
        "max_outstanding_prefetch" : 2, # No more than 2 outstanding prefetches at a time; only set since L1 mshr is unlimited in size (otherwise defaults to 1/2 mshr size)
    })
    l1cache.setSubComponent("prefetcher", "cassini.NextBlockPrefetcher")

    l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
    l2cache.addParams({
        "cache_frequency" : coreclock,
        "access_latency_cycles" : 9,
        "tag_access_latency_cycles" : 2,
        "mshr_latency_cycles" : 4,
        "replacement_policy" : "nmru",
        "coherence_protocol" : coherence,
        "cache_size" : "4KiB",
        "associativity" : 4,
        "max_requests_per_cycle" : 1,
        "mshr_num_entries" : 8,
        "drop_prefetch_mshr_level" : 5, # Drop prefetch when total misses > 5
        "debug" : DEBUG_L2,
        #"debug_addr" : "[1152]",
        "debug_level" : 10,
    })
    l2cache.setSubComponent("prefetcher", "cassini.NextBlockPrefetcher")
    l2tl1 = l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
    l2nic = l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")
    l2nic.addParams({
        "group" : 1,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    cpu_l1_link = sst.Link("link_cpu_cache_" + str(x))
    cpu_l1_link.connect ( (iface, "port", "500ps"), (l1cache, "high_network_0", "500ps") )
    
    l1_l2_link = sst.Link("link_l1_l2_" + str(x))
    l1_l2_link.connect( (l1cache, "low_network_0", "100ps"), (l2tl1, "port", "100ps") )

    l2_network_link = sst.Link("link_l2_network_" + str(x))
    l2_network_link.connect( (l2nic, "port", "100ps"), (network, "port" + str(x), "100ps") )

for x in range(caches):
    l3cache = sst.Component("l3cache" + str(x), "memHierarchy.Cache")
    l3cache.addParams({
        "cache_frequency" : uncoreclock,
        "access_latency_cycles" : 14,
        "tag_access_latency_cycles" : 6,
        "mshr_latency_cycles" : 12,
        "replacement_policy" : "random",
        "coherence_protocol" : coherence,
        "cache_size" : "1MiB",
        "associativity" : 32,
        "mshr_num_entries" : 8,
        # Distributed cache parameters
        "num_cache_slices" : caches,
        "slice_allocation_policy" : "rr", # Round-robin
        "slice_id" : x,
        "debug" : DEBUG_L3,
        #"debug_addr" : "[1152]",
        "debug_level" : 10,
    })
    l3nic = l3cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
    l3nic.addParams({
        "group" : 2,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    portid = x + cores
    l3_network_link = sst.Link("link_l3_network_" + str(x))
    l3_network_link.connect( (l3nic, "port", "100ps"), (network, "port" + str(portid), "100ps") )

for x in range(memories):
    directory = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    directory.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        "mshr_num_entries" : 16,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
        "debug" : DEBUG_DIR,
        #"debug_addr" : "[1152]",
        "debug_level" : 10,
    })
    dirtoM = directory.setSubComponent("memlink", "memHierarchy.MemLink")
    dirnic = directory.setSubComponent("cpulink", "memHierarchy.MemNIC")
    dirnic.addParams({
        "group" : 3,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })
    
    memctrl = sst.Component("memory" + str(x), "memHierarchy.MemController")
    memctrl.addParams({
        "clock" : "500MHz",
        "backing" : "none",
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
    })
    memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
    memory.addParams({
        "max_requests_per_cycle" : 2,
        "mem_size" : "512MiB",
        "tCAS" : 2,
        "tRCD" : 2,
        "tRP" : 3,
        "cycle_time" : "3ns",
        "row_size" : "4KiB",
        "row_policy" : "closed",
    })

    portid = x + caches + cores
    link_directory_network = sst.Link("link_directory_network_" + str(x))
    link_directory_network.connect( (dirnic, "port", "100ps"), (network, "port" + str(portid), "100ps") )
    
    link_directory_memory_network = sst.Link("link_directory_memory_" + str(x))
    link_directory_memory_network.connect( (dirtoM, "port", "400ps"), (memctrl, "direct_link", "400ps") )

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

