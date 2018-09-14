# Automatically generated SST Python input
import sst

# Define the simulation components
# 4 cores with non-inclusive L1/L2 hierarchies
# 2 inclusive L3s

cores = 8
caches = 4  # Number of LLCs on the network
memories = 2
coreclock = "2.4GHz"
uncoreclock = "1.4GHz"
coherence = "MESI"
network_bw = "60GB/s"

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
    comp_cpu = sst.Component("cpu" + str(x), "memHierarchy.trivialCPU")
    comp_cpu.addParams({
        "clock" : coreclock,
        "commFreq" : 4, # issue request every 4th cycle
        "rngseed" : 15+x,
        "do_write" : 1,
        "num_loadstore" : 1500,
        "memSize" : 1024*1024*1024
    })
    
    comp_l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
    comp_l1cache.addParams({
        "cache_frequency" : coreclock,
        "access_latency_cycles" : 3,
        "tag_access_latency_cycles" : 1,
        "mshr_latency_cycles" : 2,
        "replacement_policy" : "lru",
        "coherence_protocol" : coherence,
        "cache_size" : "2KiB",  # super tiny for lots of traffic
        "associativity" : 2,
        "L1" : 1,
    })

    comp_l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
    comp_l2cache.addParams({
        "cache_frequency" : coreclock,
        "access_latency_cycles" : 9,
        "tag_access_latency_cycles" : 2,
        "mshr_latency_cycles" : 4,
        "replacement_policy" : "nmru",
        "coherence_protocol" : coherence,
        "cache_size" : "4KiB",
        "associativity" : 4,
        "cache_type" : "noninclusive",
        "max_requests_per_cycle" : 1,
        "mshr_num_entries" : 4,
        # MemNIC parameters
        "memNIC.network_bw" : network_bw,
        "memNIC.network_input_buffer_size" : "2KiB",
        "memNIC.network_output_buffer_size" : "2KiB",
    })

    cpu_l1_link = sst.Link("link_cpu_cache_" + str(x))
    cpu_l1_link.connect ( (comp_cpu, "mem_link", "500ps"), (comp_l1cache, "high_network_0", "500ps") )
    
    l1_l2_link = sst.Link("link_l1_l2_" + str(x))
    l1_l2_link.connect( (comp_l1cache, "low_network_0", "100ps"), (comp_l2cache, "high_network_0", "100ps") )

    l2_network_link = sst.Link("link_l2_network_" + str(x))
    l2_network_link.connect( (comp_l2cache, "cache", "100ps"), (comp_network, "port" + str(x), "100ps") )

for x in range(caches):
    comp_l3cache = sst.Component("l3cache" + str(x), "memHierarchy.Cache")
    comp_l3cache.addParams({
        "cache_frequency" : uncoreclock,
        "access_latency_cycles" : 6,
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
        "memNIC.network_bw" : network_bw,
        "memNIC.network_input_buffer_size" : "2KiB",
        "memNIC.network_output_buffer_size" : "2KiB",
    })

    portid = x + cores
    l3_network_link = sst.Link("link_l3_network_" + str(x))
    l3_network_link.connect( (comp_l3cache, "directory", "100ps"), (comp_network, "port" + str(portid), "100ps") )

for x in range(memories):
    comp_directory = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    comp_directory.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        "mshr_num_entries" : 16,
        # MemNIC parameters
        "memNIC.interleave_size" : "64B",    # Interleave at line granularity between memories
        "memNIC.interleave_step" : str(memories * 64) + "B",
        "memNIC.network_bw" : network_bw,
        "memNIC.addr_range_start" : x*64,
        "memNIC.addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
        "memNIC.network_input_buffer_size" : "2KiB",
        "memNIC.network_output_buffer_size" : "2KiB",
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
    })

    portid = x + caches + cores
    link_directory_network = sst.Link("link_directory_network_" + str(x))
    link_directory_network.connect( (comp_directory, "network", "100ps"), (comp_network, "port" + str(portid), "100ps") )
    
    link_directory_memory_network = sst.Link("link_directory_memory_" + str(x))
    link_directory_memory_network.connect( (comp_directory, "memory", "400ps"), (comp_memory, "direct_link", "400ps") )

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()

