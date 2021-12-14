# Automatically generated SST Python input
import sst

# Define the simulation components
# cores with private L1/L2
# Shared distributed LLCs


cores = 6
caches = 3  # Number of LLCs on the network
memories = 2
coreclock = "2.4GHz"
uncoreclock = "1.4GHz"
coherence = "MESI"
network_bw = "60GB/s"

DEBUG_IFACE = 0
DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_NOC = 0
verbose = 2

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
comp_network.setSubComponent("topology","merlin.singlerouter")

for x in range(cores):
    comp_cpu = sst.Component("cpu" + str(x), "memHierarchy.standardCPU")
    comp_cpu.addParams({
        "clock" : coreclock,
        "memFreq" : 4, # issue request every 4th cycle
        "reqsPerIssue" : 2,
        "rngseed" : 687+x,
        "do_write" : 1,
        "opCount" : 1500,
        "memSize" : "4KiB",
        "read_freq" : 64,
        "write_freq" : 16,
        "flush_freq" : 4,
        "flushinv_freq" : 4,
        "maxOutstanding" : 16,
        "noncacheableRangeStart" : 0,
        "noncacheableRangeEnd" : "0x100",
    })
    iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")
    iface.addParams({"debug" : DEBUG_IFACE, "debug_level" : 10 })
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
        "verbose" : verbose,
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
        # MemNIC parameters
        "verbose" : verbose,
        "debug" : DEBUG_L2,
        "debug_level" : 10,
    })
    l2tol1 = l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
    l2NIC = l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")
    l2NIC.addParams({
        "group" : 1,
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
        # MemNIC parameters
        "verbose" : verbose,
        "debug" : DEBUG_L3,
        "debug_level" : 10,
    })

    l3NIC = l3cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
    l3NIC.addParams({
        "group" : 2,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
        "debug" : DEBUG_NOC,
        "debug_level" : 10
    })

    portid = x + cores
    l3_network_link = sst.Link("link_l3_network_" + str(x))
    l3_network_link.connect( (l3NIC, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )

for x in range(memories):
    directory = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    directory.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        "mshr_num_entries" : 16,
        # MemNIC parameters
        "verbose" : verbose,
        "debug" : DEBUG_DIR,
        "debug_level" : 10,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
    })

    dirtoM = directory.setSubComponent("memlink", "memHierarchy.MemLink")
    dirtoM.addParams({
        "debug" : DEBUG_NOC,
        "debug_level" : 10
    })
    dirNIC = directory.setSubComponent("cpulink", "memHierarchy.MemNIC")
    dirNIC.addParams({
        "group" : 3,
        "group" : 3, # L2 = 1, L3 = 2, dir = 3
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
        "debug" : DEBUG_NOC,
        "debug_level" : 10
    })

    memctrl = sst.Component("memory" + str(x), "memHierarchy.MemController")
    memctrl.addParams({
        "clock" : "500MHz",
        "backing" : "none",
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
        "verbose" : verbose,
        "debug" : DEBUG_MEM,
        "debug_level" : 10,
    })

    memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
    memory.addParams({
        "mem_size" : "512MiB",
        "tCAS" : 2,
        "tRCD" : 2,
        "tRP" : 3,
        "cycle_time" : "3ns",
        "row_size" : "4KiB",
        "row_policy" : "closed",
        "max_requests_per_cycle" : 2,
    })

    portid = x + caches + cores
    link_directory_network = sst.Link("link_directory_network_" + str(x))
    link_directory_network.connect( (dirNIC, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )
    
    link_directory_memory_network = sst.Link("link_directory_memory_" + str(x))
    link_directory_memory_network.connect( (dirtoM, "port", "400ps"), (memctrl, "direct_link", "400ps") )

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()

