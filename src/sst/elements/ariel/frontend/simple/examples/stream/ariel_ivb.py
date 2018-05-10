import sst
import os

next_core_id = 0
next_network_id = 0
next_memory_ctrl_id = 0

clock = "2300MHz"
memory_clock = "225MHz"
coherence_protocol = "MESI"

cores_per_group = 3
active_cores_per_group = cores_per_group
memory_controllers_per_group = 1
groups = 4

l3cache_blocks_per_group = 5
l3cache_block_size = "1MB"

ring_latency = "50ps"
ring_bandwidth = "85GB/s"
ring_flit_size = "72B"

memory_network_bandwidth = "85GB/s"

mem_interleave_size = 4096	# Do 4K page level interleaving
memory_capacity = 16384 	# Size of memory in MBs

streamN = 1000000

l1_prefetch_params = {
        }

l2_prefetch_params = {
       	"prefetcher": "cassini.StridePrefetcher",
       	"reach": 16,
	"detect_range" : 1
       	}

ringstop_params = {
        "torus:shape" : groups * (cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group),
        "output_latency" : "100ps",
        "xbar_bw" : ring_bandwidth,
        "input_buf_size" : "2KB",
        "input_latency" : "100ps",
        "num_ports" : "3",
        "debug" : "0",
        "torus:local_ports" : "1",
        "flit_size" : ring_flit_size,
       	"output_buf_size" : "2KB",
       	"link_bw" : ring_bandwidth,
       	"torus:width" : "1",
        "topology" : "merlin.torus"
}

l1_params = {
	"coherence_protocol": coherence_protocol,
        "cache_frequency": clock,
        "replacement_policy": "lru",
        "cache_size": "32KB",
        "maxRequestDelay" : "1000000",
        "associativity": 8,
        "cache_line_size": 64,
        "access_latency_cycles": 4,
       	"L1": 1,
       	"debug": 0
}

l2_params = {
        "coherence_protocol": coherence_protocol,
        "cache_frequency": clock,
        "replacement_policy": "lru",
        "cache_size": "256KB",
        "associativity": 8,
        "cache_line_size": 64,
        "access_latency_cycles": 8,
        "mshr_num_entries" : 16,
        "L1": 0,
        "debug": 0,
}

l3_params = {
	"debug" : "0",
      	"access_latency_cycles" : "6",
      	"cache_frequency" : "2GHz",
      	"replacement_policy" : "lru",
      	"coherence_protocol" : coherence_protocol,
      	"associativity" : "4",
      	"cache_line_size" : "64",
      	"debug_level" : "10",
      	"L1" : "0",
      	"cache_size" : "128 KB",
      	"mshr_num_entries" : "4096",
      	"num_cache_slices" : str(groups * l3cache_blocks_per_group),
      	"slice_allocation_policy" : "rr",
}

mem_params = {
	"coherence_protocol" : coherence_protocol,
	"backend.access_time" : "30ns",
	"backing" : "none",
	"rangeStart" : 0,
	"backend.mem_size" : str(memory_capacity / (groups * memory_controllers_per_group)) + "MiB",
	"clock" : memory_clock,
}

dc_params = {
	"coherence_protocol": coherence_protocol,
        "memNIC.network_bw": memory_network_bandwidth,
        "memNIC.interleave_size": str(mem_interleave_size) + "B",
        "memNIC.interleave_step": str((groups * memory_controllers_per_group) * mem_interleave_size) + "B",
        "entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
        "clock": memory_clock,
       	"debug": 1,
}

print "Configuring Ariel processor model (" + str(groups * cores_per_group) + " cores)..."

ariel = sst.Component("A0", "ariel.ariel")
ariel.addParams({
        "verbose"             : "0",
        "maxcorequeue"        : "256",
        "maxtranscore"        : "16",
        "maxissuepercycle"    : "2",
        "pipetimeout"         : "0",
        "executable"          : str(os.environ['OMP_EXE']),
        "appargcount"         : "0",
       	"memmgr.memorylevels" : "1",
        "arielinterceptcalls" : "1",
       	"arielmode"           : "1",
	"memmgr.pagecount0"   : "1048576",
        "corecount"           : groups * cores_per_group,
        "memmgr.defaultlevel" : 0,
        "clock"               : str(clock)
})

router_map = {}

print "Configuring OMP_NUM_THREADS, set to " + str(groups * cores_per_group)
os.environ["OMP_NUM_THREADS"] = str(groups * cores_per_group)

print "Configuring ring network..."

for next_ring_stop in range((cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) * groups):
	ring_rtr = sst.Component("rtr." + str(next_ring_stop), "merlin.hr_router")
        ring_rtr.addParams(ringstop_params)
        ring_rtr.addParams({
               	"id" : next_ring_stop
               	})
        router_map["rtr." + str(next_ring_stop)] = ring_rtr

for next_ring_stop in range((cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) * groups):
	if next_ring_stop == 0:
               	rtr_link_positive = sst.Link("rtr_pos_" + str(next_ring_stop))
               	rtr_link_positive.connect( (router_map["rtr.0"], "port0", ring_latency), (router_map["rtr.1"], "port1", ring_latency) )
               	rtr_link_negative = sst.Link("rtr_neg_" + str(next_ring_stop))
                rtr_link_negative.connect( (router_map["rtr.0"], "port1", ring_latency), (router_map["rtr." + str(((cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) * groups) - 1)], "port0", ring_latency) )
       	elif next_ring_stop == ((cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) * groups) - 1:
               	rtr_link_positive = sst.Link("rtr_pos_" + str(next_ring_stop))
               	rtr_link_positive.connect( (router_map["rtr." + str(next_ring_stop)], "port0", ring_latency), (router_map["rtr.0"], "port1", ring_latency) )
               	rtr_link_negative = sst.Link("rtr_neg_" + str(next_ring_stop))
               	rtr_link_negative.connect( (router_map["rtr." + str(next_ring_stop)], "port1", ring_latency), (router_map["rtr." + str(next_ring_stop-1)], "port0", ring_latency) )
       	else:
               	rtr_link_positive = sst.Link("rtr_pos_" + str(next_ring_stop))
               	rtr_link_positive.connect( (router_map["rtr." + str(next_ring_stop)], "port0", ring_latency), (router_map["rtr." + str(next_ring_stop+1)], "port1", ring_latency) )
               	rtr_link_negative = sst.Link("rtr_neg_" + str(next_ring_stop))
                rtr_link_negative.connect( (router_map["rtr." + str(next_ring_stop)], "port1", ring_latency), (router_map["rtr." + str(next_ring_stop-1)], "port0", ring_latency) )

for next_group in range(groups):
	print "Configuring core and memory controller group " + str(next_group) + "..."

	for next_active_core in range(active_cores_per_group):
		print "Creating active core " + str(next_active_core) + " in group " + str(next_group)

		l1 = sst.Component("l1cache_" + str(next_core_id), "memHierarchy.Cache")
		l1.addParams(l1_params)
		l1.addParams(l1_prefetch_params)

		l2 = sst.Component("l2cache_" + str(next_core_id), "memHierarchy.Cache")
		l2.addParams(l2_params)
		l2.addParams(l1_prefetch_params)

		ariel_cache_link = sst.Link("ariel_cache_link_" + str(next_core_id))
		ariel_cache_link.connect( (ariel, "cache_link_" + str(next_core_id), ring_latency), (l1, "high_network_0", ring_latency) )

		l2_core_link = sst.Link("l2cache_" + str(next_core_id) + "_link")
       		l2_core_link.connect((l1, "low_network_0", ring_latency), (l2, "high_network_0", ring_latency))				

		l2_ring_link = sst.Link("l2_ring_link_" + str(next_core_id))
       		l2_ring_link.connect((l2, "cache", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency))

		next_network_id = next_network_id + 1
		next_core_id = next_core_id + 1

	for next_inactive_core in range(cores_per_group - active_cores_per_group):
		print "Creating inactive core: " + str(next_inactive_core) + " in group " + str(next_group)

		l1 = sst.Component("l1cache_" + str(next_core_id), "memHierarchy.Cache")
		l1.addParams(l1_params)
		l1.addParams(l1_prefetch_params)

		l2 = sst.Component("l2cache_" + str(next_core_id), "memHierarchy.Cache")
		l2.addParams(l2_params)
		l2.addParams(l2_prefetch_params)

		ariel_cache_link = sst.Link("ariel_cache_link_" + str(next_core_id))
		ariel_cache_link.connect( (ariel, "cache_link_" + str(next_core_id), ring_latency), (l1, "high_network_0", ring_latency) )

		l2_core_link = sst.Link("l2cache_" + str(next_core_id) + "_link")
       		l2_core_link.connect((l1, "low_network_0", ring_latency), (l2, "high_network_0", ring_latency))				

		l2_ring_link = sst.Link("l2_ring_link_" + str(next_core_id))
       		l2_ring_link.connect((l2, "cache", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency))

		next_network_id = next_network_id + 1
		next_core_id = next_core_id + 1

	for next_l3_cache_block in range(l3cache_blocks_per_group):
		print "Creating L3 cache block: " + str(next_l3_cache_block) + " in group: " + str(next_group)

		l3cache = sst.Component("l3cache" + str((next_group * l3cache_blocks_per_group) + next_l3_cache_block), "memHierarchy.Cache")
		l3cache.addParams(l3_params)

		l3cache.addParams({
			"slice_id" : str((next_group * l3cache_blocks_per_group) + next_l3_cache_block)
                })

		l3_ring_link = sst.Link("l3_ring_link_" + str((next_group * l3cache_blocks_per_group) + next_l3_cache_block))
		l3_ring_link.connect( (l3cache, "directory", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency) )		

		next_network_id = next_network_id + 1

	for next_mem_ctrl in range(memory_controllers_per_group):	
		local_size = memory_capacity / (groups * memory_controllers_per_group)

		mem = sst.Component("memory_" + str(next_memory_ctrl_id), "memHierarchy.MemController")
		mem.addParams(mem_params)

		dc = sst.Component("dc_" + str(next_memory_ctrl_id), "memHierarchy.DirectoryController")
		dc.addParams({
			"memNIC.addr_range_start" : next_memory_ctrl_id * mem_interleave_size,
			"memNIC.addr_range_end" : (memory_capacity * 1024 * 1024) - (groups * memory_controllers_per_group * mem_interleave_size) + (next_memory_ctrl_id * mem_interleave_size)
			})
		dc.addParams(dc_params)

		memLink = sst.Link("mem_link_" + str(next_memory_ctrl_id))
		memLink.connect((mem, "direct_link", ring_latency), (dc, "memory", ring_latency))

		netLink = sst.Link("dc_link_" + str(next_memory_ctrl_id))
		netLink.connect((dc, "network", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency))

		next_network_id = next_network_id + 1
		next_memory_ctrl_id = next_memory_ctrl_id + 1

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(4)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})

sst.setStatisticOutput("sst.statOutputCSV")
sst.setStatisticOutputOptions( {
		"filepath"  : "./stats-snb-ariel.csv",
		"separator" : ", "
	} )

print "Completed configuring the SST Sandy Bridge model"
