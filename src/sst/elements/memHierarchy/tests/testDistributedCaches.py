# Automatically generated SST Python input
import sst

# Define the simulation components

cores = 8
caches = 4  # Number of LLCs on the network
memories = 2
coreclock = "2.4GHz"
uncoreclock = "1.4GHz"
coherence = "MESI"
network_bw = "60GB/s"

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0

# Create merlin network - this is just simple single router
comp_network = sst.Component("network", "merlin.hr_router")
comp_network.addParams({
      "xbar_bw" : network_bw,
      "link_bw" : network_bw,
      "input_buf_size" : "2KiB",
      "num_ports" : cores + caches + (memories*2),
      "flit_size" : "36B",
      "output_buf_size" : "2KiB",
      "id" : "0",  
      "topology" : "merlin.singlerouter"
})
comp_network.setSubComponent("topology","merlin.singlerouter")

for x in range(cores):
    comp_cpu = sst.Component("cpu" + str(x), "memHierarchy.trivialCPU")
    comp_cpu.addParams({
        "clock" : coreclock,
        "commFreq" : 4, # issue request every 4th cycle
        "rngseed" : 20+x,
        "do_write" : 1,
        "num_loadstore" : 1200,
        "memSize" : 1024*1024*1024
    })
    iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")

    l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
    l1cache.addParams({
        "cache_frequency" : coreclock,
        "access_latency_cycles" : 3,
        "replacement_policy" : "lru",
        "coherence_protocol" : coherence,
        "cache_size" : "2KiB",  # super tiny for lots of traffic
        "associativity" : 2,
        "L1" : 1,
        # Debug parameters
        "debug" : DEBUG_L1,
        "debug_level" : 10,
    })
    l1toC = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
    l1NIC = l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")
    l1NIC.addParams({
        "group" : 1,
        "network_bw" : network_bw,
    })

    cpu_l1_link = sst.Link("link_cpu_cache_" + str(x))
    cpu_l1_link.connect ( (iface, "port", "500ps"), (l1toC, "port", "500ps") )

    l1_network_link = sst.Link("link_l1_network_" + str(x))
    l1_network_link.connect( (l1NIC, "port", "100ps"), (comp_network, "port" + str(x), "100ps") )

for x in range(caches):
    l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
    l2cache.addParams({
        "cache_frequency" : uncoreclock,
        "access_latency_cycles" : 6,
        "replacement_policy" : "random",
        "coherence_protocol" : coherence,
        "cache_size" : "1MiB",
        "associativity" : 16,
        # Distributed cache parameters
        "num_cache_slices" : caches,
        "slice_allocation_policy" : "rr", # Round-robin
        "slice_id" : x,
        # MemNIC parameters
        # Debug parameters
        "debug" : DEBUG_L2,
        "debug_level" : 10,
        "verbose" : 2,
    })
    l2NIC = l2cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
    l2NIC.addParams({
        "group" : 2,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    portid = x + cores
    l2_network_link = sst.Link("link_l2_network_" + str(x))
    l2_network_link.connect( (l2NIC, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )

for x in range(memories):
    dirctrl = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    dirctrl.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        # Debug parameters
        "debug" : DEBUG_DIR,
        "debug_level" : 10,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
    })
    dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
    dirNIC.addParams({
        "group" : 3,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
        #"debug" : 1,
        #"debug_level" : 10,
    })

    memctrl = sst.Component("memory" + str(x), "memHierarchy.MemController")
    memctrl.addParams({
        "clock" : "500MHz",
        "backing" : "none",
        # Debug parameters
        "debug" : DEBUG_MEM,
        "debug_level" : 10,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
    })
    memNIC = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
    memNIC.addParams({
        "group" : 4,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
#        "debug" : 1,
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
    
    portid = x + caches + cores + memories
    link_memory_network = sst.Link("link_memory_network_" + str(x))
    link_memory_network.connect( (memNIC, "port", "100ps",), (comp_network, "port" + str(portid), "100ps") )


# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()

