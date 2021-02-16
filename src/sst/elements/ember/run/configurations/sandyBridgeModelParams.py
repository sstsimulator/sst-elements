#!/usr/bin/env python
#
# Copyright 2009-2021 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2021, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

debug = 0
debug_level = 0
#debug_addr = 0x40280
#debug_addr = 0x100000
debug_addr = 0x100a00

clock = "2660MHz"
memory_clock = "200MHz"
#memory_clock = "800MHz"
coherence_protocol = "MESI"

cores_per_group = 2
memory_controllers_per_group = 1
groups = 4

l3cache_blocks_per_group = 2
l3cache_block_size = "2MB"


l3cache_blocks_per_group = 2
l3cache_block_size = "2MB"

l3_cache_per_core  = int(l3cache_blocks_per_group / cores_per_group)
l3_cache_remainder = l3cache_blocks_per_group - (l3_cache_per_core * cores_per_group)

# Intel actually has four rings -> request, response, ack, data
ring_latency = "300ps" # 2.66 GHz time period plus slack for ringstop latency
ring_bandwidth = "96GB/s" # 2.66GHz clock, moves 64-bytes per cycle, plus overhead = 36B/c
ring_flit_size = "8B"

memory_network_bandwidth = "96GB/s"

mem_interleave_size = 64    # Do 4K page level interleaving
memory_capacity = 16384     # Size of memory in MBs

num_routers = groups * (cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) + 2
#num_routers = groups * (cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group)

cpu_params = {
	"max_reqs_cycle": 2,
    "maxloadmemreqpending" : 8,
    "maxstorememreqpending" : 8,
	"max_reorder_lookups": 168,
    "verbose" : 1,
    "printStats" : 1,
	"clock" : clock
}

l1_prefetch_params = {
    "prefetcher": "cassini.StridePrefetcher",
    "reach": 4,
    "detect_range" : 1
}

l2_prefetch_params = {
    "prefetcher": "cassini.StridePrefetcher",
    "reach": 16,
    "detect_range" : 1
}

rtr_params = {
        "debug" : "0",
        "torus:shape" : str(num_routers),
        "output_latency" : "25ps",
        "xbar_bw" : ring_bandwidth,
        "input_buf_size" : "2KB",
        "input_latency" : "25ps",
        "num_ports" : "3",
        "torus:local_ports" : "1",
        "flit_size" : ring_flit_size,
        "output_buf_size" : "2KB",
        "link_bw" : ring_bandwidth,
        "torus:width" : "1",
        "topology" : "merlin.torus"
}

l1_params = {
		"debug_addr" : debug_addr,
    	"debug" : debug,
        "debug_level" : debug_level,
    	"clock" : clock,
    	"coherence_protocol": coherence_protocol,
        "cache_frequency": clock,
        "replacement_policy": "lru",
        "cache_size": "32KB",
        "maxRequestDelay" : "1000000",
        "associativity": 8,
        "cache_line_size": 64,
        "low_network_links": 1,
        "access_latency_cycles": 4,
        "L1": 1,
}

l2_params = {
		"debug_addr" : debug_addr,
    	"debug" : debug,
        "debug_level" : debug_level,
    	"clock" : clock,
        "coherence_protocol": coherence_protocol,
        "cache_frequency": clock,
        "replacement_policy": "lru",
        "cache_size": "256KB",
        "associativity": 8,
        "cache_line_size": 64,
        "access_latency_cycles": 6,
        "low_network_links": 1,
        "high_network_links": 1,
        "mshr_num_entries" : 16,
		"network_bw": ring_bandwidth,
}

l3_params = {
		"debug_addr" : debug_addr,
    	"debug" : debug,
        "debug_level" : debug_level,
        "access_latency_cycles" : "12",
        "cache_frequency" : clock,
        "replacement_policy" : "lru",
        "coherence_protocol" : coherence_protocol,
        "associativity" : "16",
        "cache_line_size" : "64",
        "cache_size" : l3cache_block_size,
        "mshr_num_entries" : "4096",
		"network_bw": ring_bandwidth,
        "num_cache_slices" : str(groups * l3cache_blocks_per_group),
        "slice_allocation_policy" : "rr",
}

mem_params = {
		"debug_addr" : debug_addr,
    	"debug" : debug,
        "debug_level" : debug_level,
    "coherence_protocol" : coherence_protocol,
    "clock" : memory_clock,
   	"request_width" : "32",

    "backend.mem_size" : memory_capacity / (groups * memory_controllers_per_group),

	#"backend": 'memHierarchy.dramsim',
	#"backend.device_ini" : "ini/DDR3_micron_32M_8B_x4_sg125.ini",
   	#"backend.system_ini" : "ini/system.ini",

	# simple memory
    "backend.access_time" : "45ns",
}

dc_params = {
		"debug_addr" : debug_addr,
    	"debug" : debug,
        "debug_level" : debug_level,
    	"debug" : "0",
    	"coherence_protocol": coherence_protocol,
        "network_bw": ring_bandwidth,
        "interleave_size": str(mem_interleave_size) + "B",
        "interleave_step": str((groups * memory_controllers_per_group) * (mem_interleave_size))+ "B",
        "entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
        "clock": memory_clock,
}

#######



params = {
    "numThreads" :      1,
    "cpu_params" :      cpu_params,
    "num_routers" :     num_routers,
    "l1_params" :       l1_params,
    "l2_params" :       l2_params,
    "l3_params" :       l3_params,
    "nic_cpu_params" :  cpu_params,
    "nic_l1_params" :   l1_params,
    "memory_params" :   mem_params,
    "rtr_params" :      rtr_params,
    "ring_latency" :    ring_latency,
    "groups" :          groups,
    "cores_per_group" : cores_per_group,
    "l1_prefetch_params" : l1_prefetch_params,
    "l2_prefetch_params" : l2_prefetch_params,
    "l3cache_blocks_per_group" : l3cache_blocks_per_group,
    "memory_controllers_per_group" : memory_controllers_per_group,
    "memory_capacity" : memory_capacity,
    "mem_interleave_size" : mem_interleave_size,
    "mem_params" : mem_params,
    "dc_params" : dc_params,
}
