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


clock = "4000MHz"
memory_clock = "4000MHz"
coherence_protocol = "MSI"


memory_controllers_per_group = 1
groups = 8
memory_capacity = 16384     # Size of memory in MBs


cpu_params = {
    "verbose" : 0,
    "printStats" : 1,
}

l1cache_params = {
    "clock" : clock,
    "coherence_protocol": coherence_protocol,
    "cache_frequency": clock,
    "replacement_policy": "lru",
    "cache_size": "32KB",
    "maxRequestDelay" : "1000000",
    "associativity": 8,
    "cache_line_size": 64,
    "access_latency_cycles": 1,
    "L1": 1,
    "debug": "0"
}

bus_params = {
    "bus_frequency" : memory_clock 
}

l2cache_params = {
    "clock" : clock,
    "coherence_protocol": coherence_protocol,
    "cache_frequency": clock,
    "replacement_policy": "lru",
    "cache_size": "32KB",
    "maxRequestDelay" : "1000000",
    "associativity": 8,
    "cache_line_size": 64,
    "access_latency_cycles": 1,
    "LL": 1,
    "debug": "0" 
}

memory_params = {
    "coherence_protocol" : coherence_protocol,
    "backend.access_time" : "1ps",
    "rangeStart" : 0,
    "backend.mem_size" : memory_capacity / (groups * memory_controllers_per_group),
    "clock" : memory_clock,
}

params = {
    "numThreads" :      1,
    "cpu_params" :      cpu_params,
    "l1_params" :       l1cache_params,
    "bus_params" :      bus_params,
    "l2_params" :       l2cache_params,
    "nic_cpu_params" :  cpu_params,
    "nic_l1_params" :   l1cache_params,
    "memory_params" :   memory_params,
}
