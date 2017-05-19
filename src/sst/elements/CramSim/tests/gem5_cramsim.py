# Copyright (c) 2015-2016 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Curtis Dunham

import sst
import sys
import os

lat="1 ns"
buslat="2 ns"
clockRate = "1GHz"


def getenv(name):
    res = ""
    try:
        res = os.environ[name]
    except KeyError:
        pass
    return res

def debug(d):
    try:
        r = int(getenv(d))
    except ValueError:
        return 0
    return r


# Define SST Program Options:
sst.setProgramOption("timebase", "1ps")
#sst.setProgramOption("stopAtCycle", "0 ns")


baseCacheParams = ({
    "debug" :debug("DEBUG"),
    "debug_level" : 6,
    "coherence_protocol" : "MSI",
    "replacement_policy" : "LRU",
    "cache_line_size" : 64,
    "cache_frequency" : clockRate
    })

l1CacheParams = ({
    "debug" : debug("DEBUG"),
    "debug_level" : 6,
    "L1" : 1,
    "cache_size" : "64 KB",
    "associativity" : 4,
    "access_latency_cycles" : 2,
    "low_network_links" : 1
    })

l2CacheParams = ({
    "debug" : debug("DEBUG"),
    "debug_level" : 6,
    "L1" : 0,
    "cache_size" : "256 KB",
    "associativity" : 8,
    "access_latency_cycles" : 8,
    "high_network_links" : 1,
    "mshr_num_entries" : 4096,
    "low_network_links" : 1
    })

#debug_start=184009085000
pwd=os.getcwd()
kernel=pwd + "/linux-arm-gem5/vmlinux"
device_tree=pwd+"/linux-arm-gem5/dt/armv7_gem5_v1_4cpu.dtb"
machine="VExpress_GEM5_V1"
memsize=512

cmd = "../../gem5/configs/example/fs.py --num-cpus 4 --kernel=%s --dtb-filename=%s --machine-type=%s --cpu-type=timing --external-memory-system=sst" %(kernel,device_tree,machine)




GEM5 = sst.Component("system", "gem5.gem5")
GEM5.addParams({
    "comp_debug" : 0,
    "gem5DebugFlags" : debug("M5_DEBUG"),
    "frequency" : clockRate,
    "cmd" : cmd,
	"mem_size" : ("%dMiB"%memsize),
    "addr_range_start" : 2 * (1024 ** 3),
    "addr_range_end" : 2 * (1024 ** 3) + memsize * (1024 ** 2)
    })

#GEM5.addParams({
#    "comp_debug" : debug("GEM5_DEBUG"),
#    "gem5DebugFlags" : debug("M5_DEBUG"),
#    "frequency" : clockRate,
#    "cmd" : "configs/example/fs.py --num-cpus 4 --disk-image=linaro-minimal-aarch64.img --root-device=/dev/sda2 --kernel=vmlinux.aarch64.20140821 --dtb-filename=vexpress.aarch64.20140821.dtb --mem-size=256MB --machine-type=VExpress_EMM64 --cpu-type=timing --external-memory-system=sst"
#    })

#GEM5.addParams({
#    "comp_debug" : debug("GEM5_DEBUG"),
#    "gem5DebugFlags" : debug("M5_DEBUG"),
#    "frequency" : clockRate,
#	"cmd" : "configs/example/se.py -c a.out"
#    })



bus = sst.Component("membus", "memHierarchy.Bus")
bus.addParams({
    "bus_frequency": "2GHz",
    "debug" : 0,
    "debug_level" : 8
    })

def buildL1(name, m5, connector):
    cache = sst.Component(name, "memHierarchy.Cache")
    cache.addParams(baseCacheParams)
    cache.addParams(l1CacheParams)
    link = sst.Link("cpu_%s_link"%name)
    link.connect((m5, connector, lat), (cache, "high_network_0", lat))
    return cache

SysBusConn = buildL1("gem5SystemBus", GEM5, "system.external_memory.port")
bus_port = 0
link = sst.Link("sysbus_bus_link")
link.connect((SysBusConn, "low_network_0", buslat), (bus, "high_network_%u" % bus_port, buslat))

bus_port = bus_port + 1
ioCache = buildL1("ioCache", GEM5, "system.iocache.port")
ioCache.addParams({
    "debug" : 0,
    "debug_level" : 6,
    "cache_size" : "16 KB",
    "associativity" : 4
    })
link = sst.Link("ioCache_bus_link")
link.connect((ioCache, "low_network_0", buslat), (bus, "high_network_%u" % bus_port, buslat))

def buildCPU(m5, num):
    l1iCache = buildL1("cpu%u.l1iCache" % num, m5, "system.cpu%u.icache.port" % num)
    l1dCache = buildL1("cpu%u.l1dCache" % num, m5, "system.cpu%u.dcache.port" % num)
    itlbCache = buildL1("cpu%u.itlbCache" % num, m5, "system.cpu%u.itb_walker_cache.port" % num)
    dtlbCache = buildL1("cpu%u.dtlbCache" % num, m5, "system.cpu%u.dtb_walker_cache.port" % num)
    l1dCache.addParams({
        "debug" : 0,
        "debug_level" : 10,
        "snoop_l1_invalidations" : 1
    })

    global bus_port
    link = sst.Link("cpu%u.l1iCache_bus_link" % num) ; bus_port = bus_port + 1
    link.connect((l1iCache, "low_network_0", buslat), (bus, "high_network_%u" % bus_port, buslat))
    link = sst.Link("cpu%u.l1dCache_bus_link" % num) ; bus_port = bus_port + 1
    link.connect((l1dCache, "low_network_0", buslat), (bus, "high_network_%u" % bus_port, buslat))
    link = sst.Link("cpu%u.itlbCache_bus_link" % num) ; bus_port = bus_port + 1
    link.connect((itlbCache, "low_network_0", buslat), (bus, "high_network_%u" % bus_port, buslat))
    link = sst.Link("cpu%u.dtlbCache_bus_link" % num) ; bus_port = bus_port + 1
    link.connect((dtlbCache, "low_network_0", buslat), (bus, "high_network_%u" % bus_port, buslat))

buildCPU(GEM5, 0)
buildCPU(GEM5, 1)
buildCPU(GEM5, 2)
buildCPU(GEM5, 3)

l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams(baseCacheParams)
l2cache.addParams(l2CacheParams)
l2cache.addParams({
      "network_address" : "2"
})

link = sst.Link("l2cache_bus_link")
link.connect((l2cache, "high_network_0", buslat), (bus, "low_network_0", buslat))

comp_memory0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memory0.addParams({
    "coherence_protocol" : "MSI",
	"backend" : "memHierarchy.cramsim",
    "backend.access_time" : "100 ns",
    "backend.mem_size" : ("%sMiB" % memsize),
    "clock" : "1GHz",
	"max_requests_per_cycle" : 1,
	"do_not_back" : 0
    })


comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
	  "debug" : 2,
	  "debug_level" : 6,
      "xbar_bw" : "16GB/s",
      "link_bw" : "16GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "3",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
	  "debug" : 0,
	  "debug_level" : 6,
      "coherence_protocol" : "MSI",
      "network_address" : "1",
      "entry_cache_size" : "16384",
      "network_bw" : "1GB/s",
      "addr_range_start" : 2 * (1024 ** 3),
      "addr_range_end" : 2 * (1024 ** 3) + memsize * (1024 ** 2)
})

sst.Link("link_cache_net_0").connect((l2cache, "directory", "10ns"), (comp_chiprtr, "port2", "2ns"))
sst.Link("link_dir_net_0").connect((comp_chiprtr, "port1", "2ns"), (comp_dirctrl, "network", "2ns"))
sst.Link("l2cache_io_link").connect((comp_chiprtr, "port0", "2ns"), (GEM5, "network_gem5", buslat))
sst.Link("link_dir_mem_link").connect((comp_dirctrl, "memory", "10ns"), (comp_memory0, "direct_link", "10ns"))


def setup_cramsim_params(cramsim_config_file):
	l_params = {}
	l_configFile = open(cramsim_config_file, 'r')
	for l_line in l_configFile:
		l_tokens = l_line.split(' ')
		#print l_tokens[0], ": ", l_tokens[1]
		l_params[l_tokens[0]] = l_tokens[1]

	return l_params


# Setup parameters
cramsim_params = setup_cramsim_params("../ddr4_gem5.cfg")


# address hasher
comp_addressHasher = sst.Component("AddrHash0", "CramSim.c_AddressHasher")
comp_addressHasher.addParams(cramsim_params)



# txn gen --> memHierarchy Bridge
comp_memhBridge = sst.Component("memh_bridge", "CramSim.c_MemhBridge")
comp_memhBridge.addParams(cramsim_params);

# txn unit
comp_txnUnit0 = sst.Component("TxnUnit0", "CramSim.c_TxnUnit")
comp_txnUnit0.addParams(cramsim_params)

# cmd unit
comp_cmdUnit0 = sst.Component("CmdUnit0", "CramSim.c_CmdUnit")
comp_cmdUnit0.addParams(cramsim_params)

# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(cramsim_params)

link_dir_cramsim_link = sst.Link("link_dir_cramsim_link")
link_dir_cramsim_link.connect( (comp_memory0, "cube_link", "2ns"), (comp_memhBridge, "linkCPU", "2ns") )

txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_memhBridge, "outTxnGenReqPtr", cramsim_params["clockCycle"]), (comp_txnUnit0, "inTxnGenReqPtr", cramsim_params["clockCycle"]) )

# memhBridge(=TxnGen) <- TxnUnit (Req)(Token)
txnTokenLink_0 = sst.Link("txnTokenLink_0")
txnTokenLink_0.connect( (comp_memhBridge, "inTxnUnitReqQTokenChg", cramsim_params["clockCycle"]), (comp_txnUnit0, "outTxnGenReqQTokenChg", cramsim_params["clockCycle"]) )

 # memhBridge(=TxnGen) <- TxnUnit (Res)(Txn)
txnResLink_0 = sst.Link("txnResLink_0")
txnResLink_0.connect( (comp_memhBridge, "inTxnUnitResPtr", cramsim_params["clockCycle"]), (comp_txnUnit0, "outTxnGenResPtr", cramsim_params["clockCycle"]) )

# memhBridge(=TxnGen) -> TxnUnit (Res)(Token)
txnTokenLink_1 = sst.Link("txnTokenLink_1")
txnTokenLink_1.connect( (comp_memhBridge, "outTxnGenResQTokenChg", cramsim_params["clockCycle"]), (comp_txnUnit0, "inTxnGenResQTokenChg", cramsim_params["clockCycle"]) )



# TXNUNIT / CMDUNIT LINKS
# TxnUnit -> CmdUnit (Req) (Cmd)
cmdReqLink_0 = sst.Link("cmdReqLink_0")
cmdReqLink_0.connect( (comp_txnUnit0, "outCmdUnitReqPtrPkg", cramsim_params["clockCycle"]), (comp_cmdUnit0, "inTxnUnitReqPtr", cramsim_params["clockCycle"]) )

# TxnUnit <- CmdUnit (Req) (Token)
cmdTokenLink_0 = sst.Link("cmdTokenLink_0")
cmdTokenLink_0.connect( (comp_txnUnit0, "inCmdUnitReqQTokenChg", cramsim_params["clockCycle"]), (comp_cmdUnit0, "outTxnUnitReqQTokenChg", cramsim_params["clockCycle"]) )

# TxnUnit <- CmdUnit (Res) (Cmd)
cmdResLink_0 = sst.Link("cmdResLink_0")
cmdResLink_0.connect( (comp_txnUnit0, "inCmdUnitResPtr", cramsim_params["clockCycle"]), (comp_cmdUnit0, "outTxnUnitResPtr", cramsim_params["clockCycle"]) )




# CMDUNIT / DIMM LINKS
# CmdUnit -> Dimm (Req) (Cmd)
cmdReqLink_1 = sst.Link("cmdReqLink_1")
cmdReqLink_1.connect( (comp_cmdUnit0, "outBankReqPtr", cramsim_params["clockCycle"]), (comp_dimm0, "inCmdUnitReqPtr", cramsim_params["clockCycle"]) )

# CmdUnit <- Dimm (Res) (Cmd)
cmdResLink_1 = sst.Link("cmdResLink_1")
cmdResLink_1.connect( (comp_cmdUnit0, "inBankResPtr", cramsim_params["clockCycle"]), (comp_dimm0, "outCmdUnitResPtr", cramsim_params["clockCycle"]) )




