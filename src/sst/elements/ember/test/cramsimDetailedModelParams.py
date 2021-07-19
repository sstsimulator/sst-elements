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

memory_backend = "hbm"

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
    "verbose" : 0,
    "debug" : 0,
    "debug_level" : 10,
    "clock" : "800MHz",
    "max_requests_per_cycle" : "-1", # Let CramSim self-limit instead of externally limiting
    "backing" : "none",
    "access_time" : "1ns", # PHY latency
    "max_outstanding_requests" : 8 * 128 * 100,   # Match CramSim bridge
    "request_width" : 32, # Should match CramSim -> 128b x4
    "mem_size" : str( memory_capacity // (groups * memory_controllers_per_group)) + "MiB",
}

bridge_params = {
    "verbose" : 0,
    "debug" : 0,
    "debug_level" : 10,
    "numTxnPerCycle" : 8,
    "numBytesPerTransaction" : 32,
    "strControllerClockFrequency" : "800MHz",
    "maxOutstandingReqs" : 8 * 128 * 100, # NumChannels * 64 per channel
    "readWriteRatio" : "0.667",# Required but unused since we're driving with Ariel
    "randomSeed" : 0,
    "mode" : "seq",
}

ctrl_params = {
    "verbose" : 0,
    "debug" : 0,
    "debug_level" : 10,
    "TxnConverter" : "CramSim.c_TxnConverter",
    "AddrMapper"   : "CramSim.c_AddressHasher",
    "CmdScheduler" : "CramSim.c_CmdScheduler",
    "DeviceDriver" : "CramSim.c_DeviceDriver",
    "TxnScheduler" : "CramSim.c_TxnScheduler",
    "strControllerClockFrequency" : "800MHz",
    "boolEnableQuickRes" : 0,
    ### TxnConverter ###
    "relCommandWidth" : 1,
    "boolUseReadA" : 0,
    "boolUseWriteA" : 0,
    "bankPolicy" : "OPEN",
    ### AddressHasher ###
    "strAddressMapStr" : "r:14_b:2_B:2_l:3_C:3_l:3_c:1_h:5_",
    "numBytesPerTransaction" : 32,
    ### CmdScheduler ###
    "numCmdQEntries" : 32,
    "cmdSchedulingPolicy" : "BANK",
    ### DeviceDriver ###
    "boolDualCommandBus" : 1,
    "boolMultiCycleACT" : 1,
    "numChannels" : 8,
    "numPChannelsPerChannel" : 2,
    "numRanksPerChannel" : 1,
    "numBankGroupsPerRank" : 4,
    "numBanksPerBankGroup" : 4,
    "numRowsPerBank" : 16384,
    "numColsPerBank" : 64,
    "boolUseRefresh" : 1,
    "boolUseSBRefresh" : 0,
    ##NOTE: This DRAM timing is for 850 MHZ HBMm if you change the mem freq, ensure to scale these timings as well.
    "nRC" : 40,
    "nRRD" : 3,
    "nRRD_L" : 3,
    "nRRD_S" : 2,
    "nRCD" : 12,
    "nCCD" : 2,
    "nCCD_L" : 4,
    "nCCD_L_WR" : 2,
    "nCCD_S" : 2,
    "nAL" : 0,
    "nCL" : 12,
    "nCWL" : 4,
    "nWR" : 10,
    "nWTR" : 2,
    "nWTR_L" : 6,
    "nWTR_S" : 3,
    "nRTW" : 13,
    "nEWTR" : 6,
    "nERTW" : 6,
    "nEWTW" : 6,
    "nERTR" : 6,
    "nRAS" : 28,
    "nRTP" : 3,
    "nRP" : 12,
    "nRFC" : 350,       # 2Gb=160, 4Gb=260, 8Gb=350ns
    "nREFI" : 3900,     # 3.9us for 1-8Gb
    "nFAW" : 16,
    "nBL" : 2,
    ### TxnScheduler ###
    "txnSchedulingPolicy" : "FRFCFS",
    "numTxnQEntries" : 1024,
    "boolReadFirstTxnScheduling" : 0,
    "maxPendingWriteThreshold" : 1,
    "minPendingWriteThreshold" : "0.2",
}

dimm_params = {
    "verbose" : 0,
    "debug" : 0,
    "debug_level" : 10,
    "numChannels" : 8,
    "numPChannelsPerChannel" : 2,
    "numRanksPerChannel" : 1,
    "numBankGroupsPerRank" : 4,
    "numBanksPerBankGroup" : 4,
    "boolPowerCalc" : 0,
    "strControllerClockFrequency" : "800MHz",
    "nRP" : 12,
    "nRAS" : 28,
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
    "bridge_params":    bridge_params,
    "ctrl_params":      ctrl_params,
    "dimm_params":      dimm_params,
}
