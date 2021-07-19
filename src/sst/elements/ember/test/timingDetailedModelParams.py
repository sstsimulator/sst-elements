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


clock = "100MHz"
memory_clock = "100MHz"
coherence_protocol = "MSI"

memory_backend = "timing"

memory_controllers_per_group = 1
groups = 8
memory_capacity = 16384 * 4     # Size of memory in MBs


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
    "maxRequestDelay" : "10000",
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
    "associativity": 8,
    "cache_line_size": 64,
    "access_latency_cycles": 1,
    "debug": "0"
}

memory_params = {
    "coherence_protocol" : coherence_protocol,
    "rangeStart" : 0,
    "clock" : memory_clock,

    "addr_range_start" : 0,
    "addr_range_end" :  (memory_capacity // (groups * memory_controllers_per_group) ) * 1024*1024,
}

memory_backend_params = {
    "id" : 0,
    "clock" : "1333MHz",
    "mem_size" : str( memory_capacity // (groups * memory_controllers_per_group)) + "MiB",
    "max_requests_per_cycle": "-1",
    "addrMapper" : "memHierarchy.roundRobinAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "1KiB",
    "channels" : 8,
    "channel.transaction_Q_size" : 32,
    "channel.numRanks" : 8,
    "channel.rank.numBanks" : 16,
    "channel.rank.bank.CL" : 12,
    "channel.rank.bank.CL_WR" : 10,
    "channel.rank.bank.RCD" : 12,
    "channel.rank.bank.TRP" : 12,
    "channel.rank.bank.dataCycles" : 2,
    "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.close" : 0,
    "printconfig" : 1,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0,
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
    "memory_backend":   memory_backend,
    "memory_backend_params" :   memory_backend_params,
}
