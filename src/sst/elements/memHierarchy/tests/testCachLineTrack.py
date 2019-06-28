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

# Create merlin network - this is just simple single router
comp_network = sst.Component("network", "merlin.hr_router")
comp_network.addParams({
      "xbar_bw" : network_bw,
      "link_bw" : network_bw,
      "input_buf_size" : "2KiB",
      "num_ports" : cores + caches + memories,
      "flit_size" : "36B",
      "output_buf_size" : "2KiB",
      "id" : "0",  
      "topology" : "merlin.singlerouter"
})

for x in range(cores):
    comp_cpu = sst.Component("cpu" + str(x), "memHierarchy.streamCPU")
    iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")
    comp_cpu.addParams({
        "clock" : coreclock,
        "commFreq" : 4, # issue request every 4th cycle
        "rngseed" : 99+x,
        "do_write" : 1,
        "num_loadstore" : 1500,
        "addressoffset" : 1024, # Stream between addresses 1024 & 16384
        "memSize" : 1024*4
    })
    
    comp_l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
    comp_l1cache.addParams({
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
        "debug_level" : 10,
        # prefetcher
        "prefetcher" : "cassini.cacheLineTrack",
        #"prefetcher" : "cassini.NextBlockPrefetcher",
        #"max_outstanding_prefetch" : 2, # No more than 2 outstanding prefetches at a time; only set since L1 mshr is unlimited in size (otherwise defaults to 1/2 mshr size)
    })

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
        # Prefetch parameters
        #"prefetcher" : "cassini.NextBlockPrefetcher",
        #"drop_prefetch_mshr_level" : 5, # Drop prefetch when total misses > 5
})
    l2tol1 = l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
    l2NIC = l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")
    l2NIC.addParams({
        "group" : 1,
        # MemNIC parameters
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    cpu_l1_link = sst.Link("link_cpu_cache_" + str(x))
    cpu_l1_link.connect ( (iface, "port", "500ps"), (comp_l1cache, "high_network_0", "500ps") )
    
    l1_l2_link = sst.Link("link_l1_l2_" + str(x))
    l1_l2_link.connect( (comp_l1cache, "low_network_0", "100ps"), (l2tol1, "port", "100ps") )

    l2_network_link = sst.Link("link_l2_network_" + str(x))
    l2_network_link.connect( (l2NIC, "port", "100ps"), (comp_network, "port" + str(x), "100ps") )

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
    })
    l3NIC = l3cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
    l3NIC.addParams({
        "group" : 2,
        # MemNIC parameters
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    portid = x + cores
    l3_network_link = sst.Link("link_l3_network_" + str(x))
    l3_network_link.connect( (l3NIC, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )

for x in range(memories):
    dirctrl = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    dirctrl.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        "mshr_num_entries" : 16,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
    })
    dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
    dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
    dirNIC.addParams({
        "group" : 3,
        # MemNIC parameters
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    comp_memory = sst.Component("memory" + str(x), "memHierarchy.MemController")
    comp_memory.addParams({
        "clock" : "500MHz",
        "backing" : "none",
    })
    memorybackend = comp_memory.setSubComponent("backend", "memHierarchy.simpleDRAM")
    memorybackend.addParams({
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
    link_directory_network.connect( (dirNIC, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )
    
    link_directory_memory_network = sst.Link("link_directory_memory_" + str(x))
    link_directory_memory_network.connect( (dirtoM, "port", "400ps"), (comp_memory, "direct_link", "400ps") )

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")

sst.enableStatisticForComponentType("memHierarchy.Cache",
                                    "hist_reads_log2",
                                        {"type":"sst.HistogramStatistic",
                                         "minvalue" : "0",
                                         "numbins"  : "30", 
                                         "binwidth" : "1",
                                         "includeoutofbounds" : "1"
                                         })

sst.enableStatisticForComponentType("memHierarchy.Cache",
                                    "hist_writes_log2",
                                        {"type":"sst.HistogramStatistic",
                                         "minvalue" : "0",
                                         "numbins"  : "30", 
                                         "binwidth" : "1",
                                         "includeoutofbounds" : "1"
                                         })

sst.enableStatisticForComponentType("memHierarchy.Cache",
                                    "hist_age_log2",
                                        {"type":"sst.HistogramStatistic",
                                         "minvalue" : "0",
                                         "numbins"  : "32", 
                                         "binwidth" : "1",
                                         "includeoutofbounds" : "1"
                                         })

sst.enableStatisticForComponentType("memHierarchy.Cache",
                                    "hist_word_accesses",
                                        {"type":"sst.HistogramStatistic",
                                         "minvalue" : "0",
                                         "numbins"  : "9", 
                                         "binwidth" : "1",
                                         "includeoutofbounds" : "1"
                                         })

