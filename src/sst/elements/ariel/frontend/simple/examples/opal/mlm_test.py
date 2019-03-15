import sst
import os


import sys

#exe = str(sys.argv[1])
#num_banks = int(sys.argv[2])
#num_rows = int(sys.argv[3])
#opts = map((lambda x: "-" + x), sys.argv[4].split("-"))

#if len(sys.argv) > 5:
#	max_outstanding = sys.argv[5]
#else:
#	max_outstanding = num_banks

sst_root = os.getenv( "SST_ROOT" )

next_core_id = 0
next_network_id = 0
next_memory_ctrl_id = 0
next_l3_cache_id = 0

Executable = sst_root + "/sst-elements/src/sst/elements/ariel/frontend/simple/examples/opal/opal_test"
clock = "2GHz"
memory_clock = "200MHz"
coherence_protocol = "MESI"

cores_per_group = 8*2 #2 cores and 2 MMUs
memory_controllers_per_group = 1
groups = 1

os.environ['OMP_NUM_THREADS'] = str(cores_per_group * groups/2)

l3cache_blocks_per_group = 2
l3cache_block_size = "2MB"

l3_cache_per_core  = int(l3cache_blocks_per_group / cores_per_group)
l3_cache_remainder = l3cache_blocks_per_group - (l3_cache_per_core * cores_per_group)

ring_latency = "300ps" # 2.66 GHz time period plus slack for ringstop latency
ring_bandwidth = "96GiB/s" # 2.66GHz clock, moves 64-bytes per cycle, plus overhead = 36B/c
ring_flit_size = "8B"

memory_network_bandwidth = "96GiB/s"

mem_interleave_size = 64 #64	# Do 64B cache-line level interleaving
memory_capacity = 16384 	# Size of memory in MBs
page_size = 4                   # In KB 
num_pages = memory_capacity * 1024 / page_size

streamN = 1000000

l1_prefetch_params = {
	"prefetcher": "cassini.PalaPrefetcher",
      "prefetcher.reach": 4,
	"prefetcher.detect_range" : 1
}

l2_prefetch_params = {
       	"prefetcher": "cassini.PalaPrefetcher",
       	"prefetcher.reach": 16,
	"prefetcher.detect_range" : 1
}

ringstop_params = {
        "torus:shape" : groups * (cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group),
        "output_latency" : "25ps",
        "xbar_bw" : ring_bandwidth,
        "input_buf_size" : "2KiB",
        "input_latency" : "25ps",
        "num_ports" : "3",
        "torus:local_ports" : "1",
        "flit_size" : ring_flit_size,
       	"output_buf_size" : "2KiB",
       	"link_bw" : ring_bandwidth,
       	"torus:width" : "1",
        "topology" : "merlin.torus"
}


# Adding MMU units
#'''
mmu = sst.Component("mmu0", "Samba")
mmu.addParams({
        "os_page_size": 4,
	"perfect": 0,
        "corecount": groups * cores_per_group/2,
        "sizes_L1": 3,
        "page_size1_L1": 4,
        "page_size2_L1": 2048,
        "page_size3_L1": 1024*1024,
        "assoc1_L1": 4,
        "size1_L1": 32,
        "assoc2_L1": 4,
        "size2_L1": 32,
        "assoc3_L1": 4,
        "size3_L1": 4,
        "sizes_L2": 4,
        "page_size1_L2": 4,
        "page_size2_L2": 2048,
        "page_size3_L2": 1024*1024,
        "assoc1_L2": 12,
        "size1_L2": 1536,#1536,
        "assoc2_L2": 32, #12,
        "size2_L2": 32, #1536,
        "assoc3_L2": 4,
        "size3_L2": 16,
        "clock": clock,
        "levels": 2,
        "max_width_L1": 3,
        "max_outstanding_L1": 2,
	"max_outstanding_PTWC": 2,
        "latency_L1": 4,
        "parallel_mode_L1": 1,
        "max_outstanding_L2": 2,
        "max_width_L2": 4,
        "latency_L2": 10,
        "parallel_mode_L2": 0,
	"self_connected" : 0,
	"page_walk_latency": 200,
	"size1_PTWC": 32, # this just indicates the number entries of the page table walk cache level 1 (PTEs)
	"assoc1_PTWC": 4, # this just indicates the associtativit the page table walk cache level 1 (PTEs)
	"size2_PTWC": 32, # this just indicates the number entries of the page table walk cache level 2 (PMDs)
	"assoc2_PTWC": 4, # this just indicates the associtativit the page table walk cache level 2 (PMDs)
	"size3_PTWC": 32, # this just indicates the number entries of the page table walk cache level 3 (PUDs)
	"assoc3_PTWC": 4, # this just indicates the associtativit the page table walk cache level 3 (PUDs)
	"size4_PTWC": 32, # this just indicates the number entries of the page table walk cache level 4 (PGD)
	"assoc4_PTWC": 4, # this just indicates the associtativit the page table walk cache level 4 (PGD)
	"latency_PTWC": 10, # This is the latency of checking the page table walk cache
	"emulate_faults": 1,
});
#'''



opal = sst.Component("opal0", "Opal")

opal.addParams({
  "num_cores": groups * cores_per_group/2,
  "latency" : 100,
});


# ariel cpu
ariel = sst.Component("a0", "ariel.ariel")
arielParams = {
    "verbose" : 1,
    "clock" : clock,
    "maxcorequeue" : 1024,
    "maxissuepercycle" : 3,
    "maxtranscore": 16,
    "pipetimeout" : 0,
    "corecount" : groups * cores_per_group/2,
    "memmgr.memorylevels" : 1,
    "memmgr.pagecount0" : num_pages,
    "memmgr.pagesize0" : page_size * 1024,
    "memmgr.defaultlevel" : 0,
    "arielmode" : 0,
    "appargcount" : 4,
    "apparg0": "-s",
    "apparg1": "small",
    "apparg2": "-t",
    "apparg3": "2",
    "max_insts" : 100000000,
    "opal_enabled": 1,
    "executable" : sst_root + "/sst-elements/src/sst/elements/ariel/frontend/simple/examples/opal/opal_test",
}

# ARGUMENTS
#arielParams["appargcount"] = 2 + len(opts)
#arielParams["apparg0"] = "-b" + str(num_banks)
#arielParams["apparg1"] = "-r" + str(num_rows)
#for i in range(0, len(opts)):
#	arielParams["apparg" + str(i + 2)] = opts[i]

ariel.addParams(arielParams)

l1_params = {
        "cache_frequency": clock,
        "cache_size": "32KiB",
        "associativity": 8,
        "access_latency_cycles": 4,
       	"L1": 1,
        "verbose": 30,
        # Default params
        # "cache_line_size": 64,
	# "coherence_protocol": coherence_protocol,
        # "replacement_policy": "lru",
        "maxRequestDelay" : "1000000",
}

l1_dummy_params = {
        "cache_frequency": clock,
        "cache_size": "1KiB",
        "associativity": 1,
        "access_latency_cycles": 1,
        "L1": 1,
        "verbose": 30,
        # Default params
        # "cache_line_size": 64,
        # "coherence_protocol": coherence_protocol,
        # "replacement_policy": "lru",
        "maxRequestDelay" : "1000000",
}



l2_params = {
        "cache_frequency": clock,
        "cache_size": "256KiB",
        "associativity": 8,
        "access_latency_cycles": 6,
        "mshr_num_entries" : 16,
	"network_bw": ring_bandwidth,
        # Default params
        #"cache_line_size": 64,
        #"coherence_protocol": coherence_protocol,
        #"replacement_policy": "lru",
}

l2_dummy_params = {
        "cache_frequency": clock,
        "cache_size": "1KiB",
        "associativity": 1,
        "access_latency_cycles": 1,
        "mshr_num_entries" : 16,
        "network_bw": ring_bandwidth,
        # Default params
        #"cache_line_size": 64,
        #"coherence_protocol": coherence_protocol,
        #"replacement_policy": "lru",
}

l3_params = {
      	"access_latency_cycles" : "12",
      	"cache_frequency" : clock,
      	"associativity" : "16",
      	"cache_size" : l3cache_block_size,
      	"mshr_num_entries" : "4096",
      	"network_bw": ring_bandwidth,
      	# Distributed caches
        "num_cache_slices" : str(groups * l3cache_blocks_per_group),
      	"slice_allocation_policy" : "rr",
        # Default params
      	# "replacement_policy" : "lru",
      	# "cache_line_size" : "64",
      	# "coherence_protocol" : coherence_protocol,
}

#mem_params = {
#	"backend.mem_size" : str(memory_capacity / (groups * memory_controllers_per_group)) + "MiB",
#        "clock" : memory_clock,
#	"network_bw": ring_bandwidth,
#        "max_requests_per_cycle" : 1,
#        "do_not_back" : 1,
#}



mem_params = {
    	"backend.mem_size" : str(memory_capacity / (groups * memory_controllers_per_group)) + "MiB",
    	"backing" : "none",
    	"backend" : "memHierarchy.Messier",
    	"backendConvertor.backend" : "memHierarchy.Messier",
    	"backend.clock" : clock,
	"clock" : clock,
}


#messier_inst = sst.Component("NVMmemory", "Messier")
#messier_inst.addParams(messier_params)

messier_params = {
      "clock" : clock,
      "tCL" : 30,
      "tRCD" : 300,
      "tCL_W" : 1000,
      "write_buffer_size" : 32,
      "flush_th" : 90,
      "num_banks" : 16,
      "max_outstanding" : 16,
      "max_writes" : "4",
      "max_current_weight" : 32*50,
      "read_weight" : "5",
      "write_weight" : "5",
      "cacheline_interleaving" : 0, #bank interleaving
}

dc_params = {
        "memNIC.interleave_size": str(mem_interleave_size) + "B",
        "memNIC.interleave_step": str((groups * memory_controllers_per_group) * (mem_interleave_size)) + "B",
        "entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
        "clock": memory_clock,
        "memNIC.network_bw": ring_bandwidth,
        # Default params
	# "coherence_protocol": coherence_protocol,
}

router_map = {}

print "Configuring Ring Network-on-Chip..."

for next_ring_stop in range((cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) * groups):
	ring_rtr = sst.Component("rtr." + str(next_ring_stop), "merlin.hr_router")
        ring_rtr.addParams(ringstop_params)
        ring_rtr.addParams({
               	"id" : next_ring_stop
               	})
        router_map["rtr." + str(next_ring_stop)] = ring_rtr

for next_ring_stop in range((cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) * groups):
	print next_ring_stop
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

	for next_active_core in range(cores_per_group):
		for next_l3_cache_block in range(l3_cache_per_core):
			print "Creating L3 cache block " + str(next_l3_cache_id) + "..."
			
			l3cache = sst.Component("l3cache_" + str(next_l3_cache_id), "memHierarchy.Cache")
			l3cache.addParams(l3_params)

			l3cache.addParams({
				"slice_id" : str(next_l3_cache_id)
       	    })

			l3_ring_link = sst.Link("l3_" + str(next_l3_cache_id) + "_link")
			l3_ring_link.connect( (l3cache, "directory", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency) )		

			next_l3_cache_id = next_l3_cache_id + 1
			next_network_id = next_network_id + 1
		
		print "Creating Core " + str(next_active_core) + " in Group " + str(next_group)

		l1 = sst.Component("l1cache_" + str(next_core_id), "memHierarchy.Cache")

		if next_core_id < cores_per_group*groups/2:
			l1.addParams(l1_params)
		else:
			l1.addParams(l1_dummy_params)

		#l1.addParams(l1_prefetch_params)

		l2 = sst.Component("l2cache_" + str(next_core_id), "memHierarchy.Cache")


		if next_core_id < cores_per_group*groups/2:
			l2.addParams(l2_params)
		else:
			l2.addParams(l2_dummy_params)

		#l2.addParams(l2_prefetch_params)


		'''
                arielL1Link = sst.Link("cpu_cache_link_" + str(next_core_id))
                arielL1Link.connect((ariel, "cache_link_%d"%next_core_id, ring_latency), (l1, "high_network_0", ring_latency))
		arielL1Link.setNoCut()
		'''

		#'''
		arielMMULink = sst.Link("cpu_mmu_link_" + str(next_core_id))
                MMUCacheLink = sst.Link("mmu_cache_link_" + str(next_core_id))
		PTWMemLink = sst.Link("ptw_mem_link_" + str(next_core_id))
		PTWOpalLink = sst.Link("ptw_opal_" + str(next_core_id))
		ArielOpalLink = sst.Link("ariel_opal_" + str(next_core_id))



		if next_core_id < cores_per_group*groups/2:
                	arielMMULink.connect((ariel, "cache_link_%d"%next_core_id, ring_latency), (mmu, "cpu_to_mmu%d"%next_core_id, ring_latency))
                	ArielOpalLink.connect((ariel, "opal_link_%d"%next_core_id, ring_latency), (opal, "requestLink%d"%(next_core_id + cores_per_group*groups/2), ring_latency))
                	MMUCacheLink.connect((mmu, "mmu_to_cache%d"%next_core_id, ring_latency), (l1, "high_network_0", ring_latency))
			PTWOpalLink.connect( (mmu, "ptw_to_opal%d"%next_core_id, "50ps"), (opal, "requestLink%d"%next_core_id, "50ps") )
			arielMMULink.setNoCut()
			PTWOpalLink.setNoCut()
			MMUCacheLink.setNoCut()
		else:
			PTWMemLink.connect((mmu, "ptw_to_mem%d"%(next_core_id-cores_per_group*groups/2), ring_latency), (l1, "high_network_0", ring_latency))
		#'''



		l2_core_link = sst.Link("l2cache_" + str(next_core_id) + "_link")
       		l2_core_link.connect((l1, "low_network_0", ring_latency), (l2, "high_network_0", ring_latency))				
		l2_core_link.setNoCut()

		l2_ring_link = sst.Link("l2_ring_link_" + str(next_core_id))
       		l2_ring_link.connect((l2, "cache", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency))

		next_network_id = next_network_id + 1
		next_core_id = next_core_id + 1

	for next_l3_cache_block in range(l3_cache_remainder):
		print "Creating L3 cache block: " + str(next_l3_cache_id) + "..."

		l3cache = sst.Component("l3cache_" + str(next_l3_cache_id), "memHierarchy.Cache")
		l3cache.addParams(l3_params)

		l3cache.addParams({
			"slice_id" : str(next_l3_cache_id)
     		})

		l3_ring_link = sst.Link("l3_" + str(next_l3_cache_id) + "_link")
		l3_ring_link.connect( (l3cache, "directory", ring_latency), (router_map["rtr." + str(next_network_id)], "port2", ring_latency) )		

		next_l3_cache_id = next_l3_cache_id + 1
		next_network_id = next_network_id + 1

	for next_mem_ctrl in range(memory_controllers_per_group):	
		local_size = memory_capacity / (groups * memory_controllers_per_group)

		mem = sst.Component("memory_" + str(next_memory_ctrl_id), "memHierarchy.MemController")
		mem.addParams(mem_params)

		messier_inst = sst.Component("NVMmemory_" + str(next_memory_ctrl_id), "Messier")
		messier_inst.addParams(messier_params)

		link_nvm_bus_link = sst.Link("link_nvm_bus_link" + str(next_memory_ctrl_id))
		link_nvm_bus_link.connect( (messier_inst, "bus", "50ps"), (mem, "cube_link", "50ps") )


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







# ===============================================================================

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(16)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})

sst.setStatisticOutput("sst.statOutputCSV")

#if len(sys.argv) > 5:
output_file = "test.csv"
#else:
#	output_file = "STATS/"+str(exe) + "-b" + str(num_banks) + "-r" + str(num_rows) + "".join(opts) + ".csv"

sst.setStatisticOutputOptions( {
	"filepath"  : output_file,
	"separator" : ", "
} )

print "Completed configuring the SST Sandy Bridge model"
