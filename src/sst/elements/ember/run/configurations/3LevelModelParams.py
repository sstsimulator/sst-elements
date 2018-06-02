#!/usr/bin/env python
#
# Copyright 2009-2018 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2018, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.


clock = "2600MHz"
memory_clock = "200MHz"
coherence_protocol = "MESI"
maxmemreqpending = 16


memory_controllers_per_group = 1
groups = 8
memory_capacity = 16384     # Size of memory in MBs


cpu_params = {
	"max_reqs_cycle": 2,
	"maxmemreqpending" : maxmemreqpending,
	"max_reorder_lookups": 168,
	"clock" : clock, 
	"verbose" : 1,
	"printStats" : 1,
	"clock" : clock
}

l1cache_params = {
    "clock" : clock,
    "debug": 1, 
    "debug_level": 0, 
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
    "debug": 0, 

    "prefetcher": "cassini.StridePrefetcher",
    "reach": 16,
    "detect_range" : 1
}
    #"prefetcher": "cassini.StridePrefetcher",
    #"reach": 16,
    #"detect_range" : 1 

bus_params = {
    "bus_frequency" : clock 
}

l2cache_params = {
    "debug": 1, 
    "debug_level": 0, 
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
    "mshr_num_entries" : 32,
    "debug": 10,
}

l3cache_params = {
    "debug_level": 0, 
    "debug" : "1",

    "access_latency_cycles" : 21,
    "cache_frequency" : clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : coherence_protocol,
    "associativity" : "4",
    "cache_line_size" : "64",
    "LL" : "1",
    #"cache_size" : "2048 KB",
    "cache_size" : "16384 KB",
    "mshr_num_entries" : "128",
    "num_cache_slices" : 1,
    "slice_allocation_policy" : "rr",

}

memory_params = {
    "coherence_protocol" : coherence_protocol,
    "backend.mem_size" : memory_capacity / (groups * memory_controllers_per_group),
    "backend.access_time" : "30 ns",
    "rangeStart" : 0,
    "clock" : memory_clock,
}

params = {
    "numThreads" :      1,
    "cpu_params" :      cpu_params,
    "l1_params" :       l1cache_params,
    "bus_params" :      bus_params,
    "l2_params" :       l2cache_params,
    "l3_params" :       l3cache_params,
    "nic_cpu_params" :  cpu_params,
    "nic_l1_params" :   l1cache_params,
    "memory_params" :   memory_params,
}
