#!/usr/bin/env python
#
# Copyright 2009-2016 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2016, Sandia Corporation
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

clock = "2660MHz"
memory_clock = "200MHz"
coherence_protocol = "MESI"

cores_per_group = 1 
#cores_per_group = 2
memory_controllers_per_group = 1
#groups = 8 
groups = 1

#l3cache_blocks_per_group = 5
l3cache_blocks_per_group = 1 
l3cache_block_size = "1MB"

ring_latency = "50ps"
ring_bandwidth = "85GB/s"
ring_flit_size = "72B"

memory_network_bandwidth = "85GB/s"

mem_interleave_size = 4096  # Do 4K page level interleaving
memory_capacity = 16384     # Size of memory in MBs


num_routers = groups * (cores_per_group + memory_controllers_per_group + l3cache_blocks_per_group) + 2 

l1_prefetch_params = {
}

l2_prefetch_params = {
    "prefetcher": "cassini.StridePrefetcher",
    "reach": 16,
    "detect_range" : 1
}

rtr_params = {
        "torus:shape" : str(num_routers), 
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
        "debug": 0
}

l2_params = {
    "clock" : clock,
        "coherence_protocol": coherence_protocol,
        "cache_frequency": clock,
        "replacement_policy": "lru",
        "cache_size": "256KB",
        "associativity": 8,
        "cache_line_size": 64,
        "access_latency_cycles": 8,
        "low_network_links": 1,
        "high_network_links": 1,
        "mshr_num_entries" : 16,
        "L1": 0,
        "debug": 10,
    #"bottom_network" : "cache",
    #"top_network" : ""
}

l3_params = {
    "debug" : "0",
        "access_latency_cycles" : "6",
        "cache_frequency" : "2GHz",
        "replacement_policy" : "lru",
        "coherence_protocol" : "MSI",
        "associativity" : "4",
        "cache_line_size" : "64",
        "debug_level" : "10",
        "debug" : "10",
        "L1" : "0",
        "cache_size" : "128 KB",
        "mshr_num_entries" : "4096",
        #"top_network" : "cache",
        #"bottom_network" : "directory",
        "num_cache_slices" : str(groups * l3cache_blocks_per_group),
        "slice_allocation_policy" : "rr"
}

mem_params = {
    "coherence_protocol" : coherence_protocol,
    "backend.access_time" : "30ns",
    "rangeStart" : 0,
    "backend.mem_size" : memory_capacity / (groups * memory_controllers_per_group),
    "clock" : memory_clock,
}

dc_params = {
    "coherence_protocol": coherence_protocol,
        "network_bw": memory_network_bandwidth,
        "interleave_size": str(mem_interleave_size/1024)+'KiB',
        "interleave_step": str( (groups * memory_controllers_per_group) * (mem_interleave_size / 1024) ) +'KiB',
        "entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
        "clock": memory_clock,
        "debug": 10,
}

#######


cpu_params = {
    "verbose" : 0,
    "printStats" : 1,
}


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
