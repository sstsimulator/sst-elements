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

    comp_l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
    comp_l1cache.addParams({
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
    l1_clink = comp_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
    l1_mlink = comp_l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")
    l1_mlink.addParams({
        "group" : 1,
        "network_bw" : network_bw,
    })

    cpu_l1_link = sst.Link("link_cpu_cache_" + str(x))
    cpu_l1_link.connect ( (comp_cpu, "mem_link", "500ps"), (l1_clink, "port", "500ps") )

    l1_network_link = sst.Link("link_l1_network_" + str(x))
    l1_network_link.connect( (l1_mlink, "port", "100ps"), (comp_network, "port" + str(x), "100ps") )

for x in range(caches):
    comp_l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
    comp_l2cache.addParams({
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
        # Debug parameters
        "debug" : DEBUG_L2,
        "debug_level" : 10,
    })
    l2_link = comp_l2cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
    l2_link.addParams({
        "group" : 2,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })
    portid = x + cores
    l2_network_link = sst.Link("link_l2_network_" + str(x))
    l2_network_link.connect( (l2_link, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )

for x in range(memories):
    comp_directory = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    comp_directory.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        "net_memory_name" : "memory" + str(x),
        # Debug parameters
        "debug" : DEBUG_DIR,
        "debug_level" : 10,
    })
    dir_link = comp_directory.setSubComponent("cpulink", "memHierarchy.MemNIC")
    dir_link.addParams({
        "group" : 3,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
        "network_bw" : network_bw,
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
        #"debug" : 1,
        #"debug_level" : 10,
    })

    comp_memory = sst.Component("memory" + str(x), "memHierarchy.MemController")
    comp_memory.addParams({
        "clock" : "500MHz",
        "max_requests_per_cycle" : 2,
        "backing" : "none",
        # Backend parameters
        "backend" : "memHierarchy.simpleDRAM",
        "backend.mem_size" : "512MiB",
        "backend.tCAS" : 2,
        "backend.tRCD" : 2,
        "backend.tRP" : 3,
        "backend.cycle_time" : "3ns",
        "backend.row_size" : "4KiB",
        "backend.row_policy" : "closed",
        # Debug parameters
        "debug" : DEBUG_MEM,
        "debug_level" : 10,
    })
    mem_link = comp_memory.setSubComponent("cpulink", "memHierarchy.MemNIC")
    mem_link.addParams({
        "group" : 4,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
#        "debug" : 1,
        "debug_level" : 10,
    })
    
    portid = x + caches + cores
    link_directory_network = sst.Link("link_directory_network_" + str(x))
    link_directory_network.connect( (dir_link, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )
    
    portid = x + caches + cores + memories
    link_memory_network = sst.Link("link_memory_network_" + str(x))
    link_memory_network.connect( (mem_link, "port", "100ps",), (comp_network, "port" + str(portid), "100ps") )


# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()

