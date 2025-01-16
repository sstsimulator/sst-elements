import sst
from mhlib import componentlist

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
comp_network.setSubComponent("topology","merlin.singlerouter")

for x in range(cores):
    comp_cpu = sst.Component("core" + str(x), "memHierarchy.standardCPU")
    comp_cpu.addParams({
        "clock" : coreclock,
        "rngseed" : 15+x,
        "memFreq" : "4",
        "memSize" : "1GiB",
        "verbose" : 0,
        "clock" : "2GHz",
        "maxOutstanding" : 16,
        "opCount" : 5000,
        "reqsPerIssue" : 2,
        "write_freq" : 40, # 40% writes
        "read_freq" : 60,  # 60% reads
    })
    iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")
    
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
        "cache_type" : "noninclusive",
        "max_requests_per_cycle" : 1,
        "mshr_num_entries" : 4,
        # MemNIC parameters
    })

    l2nic = l2cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
    l2nic.addParams({
        "group" : 1,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    cpu_l1_link = sst.Link("link_cpu_cache_" + str(x))
    cpu_l1_link.connect ( (iface, "lowlink", "500ps"), (comp_l1cache, "highlink", "500ps") )
    
    l1_l2_link = sst.Link("link_l1_l2_" + str(x))
    l1_l2_link.connect( (comp_l1cache, "lowlink", "100ps"), (l2cache, "highlink", "100ps") )

    l2_network_link = sst.Link("link_l2_network_" + str(x))
    l2_network_link.connect( (l2nic, "port", "100ps"), (comp_network, "port" + str(x), "100ps") )

for x in range(caches):
    l3cache = sst.Component("l3cache" + str(x), "memHierarchy.Cache")
    l3cache.addParams({
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
    })

    l3nic = l3cache.setSubComponent("highlink", "memHierarchy.MemNIC")
    l3nic.addParams({
        "group" : 2,
        "network_bw" : network_bw,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
    })

    portid = x + cores
    l3_network_link = sst.Link("link_l3_network_" + str(x))
    l3_network_link.connect( (l3nic, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )

for x in range(memories):
    directory = sst.Component("directory" + str(x), "memHierarchy.DirectoryController")
    directory.addParams({
        "clock" : uncoreclock,
        "coherence_protocol" : coherence,
        "entry_cache_size" : 32768,
        "mshr_num_entries" : 16,
        "addr_range_start" : x*64,
        "addr_range_end" :  1024*1024*1024 - ((memories - x) * 64) + 63,
        "interleave_size" : "64B",    # Interleave at line granularity between memories
        "interleave_step" : str(memories * 64) + "B",
    })
    dirNic = directory.setSubComponent("highlink", "memHierarchy.MemNIC")
    dirNic.addParams({
        "group" : 3,
        "network_input_buffer_size" : "2KiB",
        "network_output_buffer_size" : "2KiB",
        "network_bw" : network_bw,
    })

    memctrl = sst.Component("memory" + str(x), "memHierarchy.MemController")
    memctrl.addParams({
        "clock" : "500MHz",
        "backing" : "none",
        "addr_range_start" : x*64,
        "addr_range_end" : 1024*1024*1024 - ((memories -x) * 64) + 63,
        "interleave_size" : "64B",
        "interleave_step" : str(memories * 64) + "B",
    })

    memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
    memory.addParams({
        "max_requests_per_cycle" : 2,
        "mem_size" : "1GiB",
        "tCAS" : 2,
        "tRCD" : 2,
        "tRP" : 3,
        "cycle_time" : "3ns",
        "row_size" : "4KiB",
        "row_policy" : "closed",
    })

    portid = x + caches + cores
    link_directory_network = sst.Link("link_directory_network_" + str(x))
    link_directory_network.connect( (dirNic, "port", "100ps"), (comp_network, "port" + str(portid), "100ps") )
    
    link_directory_memory_network = sst.Link("link_directory_memory_" + str(x))
    link_directory_memory_network.connect( (directory, "lowlink", "400ps"), (memctrl, "highlink", "400ps") )

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

