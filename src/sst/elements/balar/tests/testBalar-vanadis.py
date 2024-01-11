# Automatically generated SST Python input
# Run script: sst testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'

# This script will run the vanadisHandshake binary that perform a
# vectoradd progam similar to a native CUDA program launch with
# vanadis as the host CPU and balar+GPGPUSim as the CUDA device

import os
import sst
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser
import argparse
from utils import *
# from mhlib import componentlist

# Arguments
parser = argparse.ArgumentParser()
parser.add_argument(
    "-c", "--config", help="specify configuration file", required=True)
parser.add_argument("-v", "--verbose",
                    help="increase verbosity of output", action="store_true")
parser.add_argument("-s", "--statfile",
                    help="statistics file", default="./stats.out")
parser.add_argument("-l", "--statlevel",
                    help="statistics level", type=int, default=16)
parser.add_argument(
    "-x", "--binary", help="specify input cuda binary", default="")
parser.add_argument("-a", "--arguments",
                    help="colon sep binary arguments", default="")


args = parser.parse_args()

verbose = args.verbose
cfgFile = args.config
statFile = args.statfile
statLevel = args.statlevel
binaryFile = args.binary
binaryArgs = args.arguments

# Build Configuration Information
config = Config(cfgFile, verbose=verbose)

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_NIC = 0
DEBUG_LEVEL = 10

debug_params = {"debug": 1, "debug_level": 10}

# On network: Core, L1, MMIO device, memory
# Logical communication: Core->L1->memory
#                        Core->MMIO
#                        MMIO->memory
core_group = 0
mmio_group = 1
dma_group = 2
cache_group = 3
memory_group = 4

core_dst = [cache_group, mmio_group]
cache_src = [core_group, mmio_group, dma_group]
cache_dst = [memory_group]
mmio_src = [core_group]
mmio_dst = [dma_group, cache_group]
dma_src = [mmio_group]
dma_dst = [cache_group]
memory_src = [cache_group]

# Constans shared across components
network_bw = "25GB/s"
clock = "2GHz"
mmio_addr = 0x80100000
balar_mmio_size = 1024
dma_mmio_addr = mmio_addr + balar_mmio_size
dma_mmio_size = 512

mmio = sst.Component("balar", "balar.balarMMIO")
mmio.addParams({
    "verbose": 20,
    "clock": clock,
    "base_addr": mmio_addr,
    "mmio_size": balar_mmio_size,
    "dma_addr": dma_mmio_addr,
    "cpu_no_cache": False,
})
mmio.addParams(config.getGPUConfig())

balar_mmio_iface = mmio.setSubComponent("mmio_iface", "memHierarchy.standardInterface")
balar_mem_iface = mmio.setSubComponent("mem_iface", "memHierarchy.standardInterface")
# balar_mmio_iface.addParams(debug_params)

# mmio_nic = balar_mmio_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
# mmio_nic.addParams({"group": mmio_group,
#                     "network_bw": network_bw,
#                     "sources": mmio_src,
#                     "destinations": mmio_dst,
#                     })

# DMA Engine
dma = sst.Component("dmaEngine", "balar.dmaEngine")
dma.addParams({
    "verbose": 20,
    "clock": clock,
    "mmio_addr": dma_mmio_addr,
    "mmio_size": dma_mmio_size,
})
dma_mmio_if = dma.setSubComponent("mmio_iface", "memHierarchy.standardInterface")
dma_mem_if = dma.setSubComponent("mem_iface", "memHierarchy.standardInterface")
# dma_nic = dma_mmio_if.setSubComponent("memlink", "memHierarchy.MemNIC")
# dma_nic.addParams({"group": dma_group,
#                    "network_bw": network_bw,
#                    "sources": dma_src,
#                    "destinations": dma_dst
#                    })

# GPU Memory hierarchy
#          mmio/GPU
#           |
#           L1
#           |
#           L2
#           |
#           mem port
#
# GPU Memory hierarchy configuration
print("Configuring GPU Network-on-Chip...")

# GPU Xbar group
l1g_group = 1
l2g_group = 2

gpu_router_ports = config.gpu_cores + config.gpu_l2_parts

GPUrouter = sst.Component("gpu_xbar", "shogun.ShogunXBar")
GPUrouter.addParams(config.getGPUXBarParams())
GPUrouter.addParams({
    "verbose": 1,
    "port_count": gpu_router_ports,
})

# Connect Cores & l1caches to the router
for next_core_id in range(config.gpu_cores):
    print("Configuring GPU core %d..." % next_core_id)

    gpuPort = "requestGPUCacheLink%d" % next_core_id

    l1g = sst.Component("l1gcache_%d" % (next_core_id), "memHierarchy.Cache")
    l1g.addParams(config.getGPUL1Params())
    l1g.addParams(debug_params)

    l1g_gpulink = l1g.setSubComponent("cpulink", "memHierarchy.MemLink")
    l1g_memlink = l1g.setSubComponent("memlink", "memHierarchy.MemNIC")
    l1g_memlink.addParams({"group": l1g_group})
    l1g_linkctrl = l1g_memlink.setSubComponent(
        "linkcontrol", "shogun.ShogunNIC")

    connect("gpu_cache_link_%d" % next_core_id,
            mmio, gpuPort,
            l1g_gpulink, "port",
            config.default_link_latency).setNoCut()

    connect("l1gcache_%d_link" % next_core_id,
            l1g_linkctrl, "port",
            GPUrouter, "port%d" % next_core_id,
            config.default_link_latency)

# Connect GPU L2 caches to the routers
num_L2s_per_stack = config.gpu_l2_parts // config.hbmStacks
sub_mems = config.gpu_memory_controllers
total_mems = config.hbmStacks * sub_mems

interleaving = 256
next_mem = 0
cacheStartAddr = 0x00
memStartAddr = 0x00

if (config.gpu_l2_parts % total_mems) != 0:
    print("FAIL Number of L2s (%d) must be a multiple of the total number memory controllers (%d)." % (
        config.gpu_l2_parts, total_mems))
    raise SystemExit

for next_group_id in range(config.hbmStacks):
    next_cache = next_group_id * sub_mems

    mem_l2_bus = sst.Component(
        "mem_l2_bus_" + str(next_group_id), "memHierarchy.Bus")
    mem_l2_bus.addParams({
        "bus_frequency": "4GHz",
        "drain_bus": "1"
    })

    for sub_group_id in range(sub_mems):
        memStartAddr = 0 + (256 * next_mem)
        endAddr = memStartAddr + \
            config.gpu_memory_capacity_inB - (256 * total_mems)

        if backend == "simple":
            # Create SimpleMem
            print("Configuring Simple mem part" + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")
            mem = sst.Component("Simplehbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_mem_params(
                total_mems, memStartAddr, endAddr))
            membk = mem.setSubComponent("backend", "memHierarchy.simpleMem")
            membk.addParams(config.get_GPU_simple_mem_params())
        elif backend == "ddr":
            # Create DDR (Simple)
            print("Configuring DDR-Simple mem part" + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")
            mem = sst.Component("DDR-shbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_ddr_memctrl_params(
                total_mems, memStartAddr, endAddr))
            membk = mem.setSubComponent("backend", "memHierarchy.simpleDRAM")
            membk.addParams(config.get_GPU_simple_ddr_params(next_mem))
        elif backend == "timing":
            # Create DDR (Timing)
            print("Configuring DDR-Timing mem part" + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")
            mem = sst.Component("DDR-thbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_ddr_memctrl_params(
                total_mems, memStartAddr, endAddr))
            membk = mem.setSubComponent("backend", "memHierarchy.timingDRAM")
            membk.addParams(config.get_GPU_ddr_timing_params(next_mem))
        else:
            # Create CramSim HBM
            print("Creating HBM controller " + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")

            mem = sst.Component("GPUhbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_hbm_memctrl_cramsim_params(
                total_mems, memStartAddr, endAddr))

            cramsimBridge = sst.Component(
                "hbm_cs_bridge_" + str(next_mem), "CramSim.c_MemhBridge")
            cramsimCtrl = sst.Component(
                "hbm_cs_ctrl_" + str(next_mem), "CramSim.c_Controller")
            cramsimDimm = sst.Component(
                "hbm_cs_dimm_" + str(next_mem), "CramSim.c_Dimm")

            cramsimBridge.addParams(
                config.get_GPU_hbm_cramsim_bridge_params(next_mem))
            cramsimCtrl.addParams(
                config.get_GPU_hbm_cramsim_ctrl_params(next_mem))
            cramsimDimm.addParams(
                config.get_GPU_hbm_cramsim_dimm_params(next_mem))

            linkMemBridge = sst.Link("memctrl_cramsim_link_" + str(next_mem))
            linkMemBridge.connect((mem, "cube_link", "2ns"),
                                  (cramsimBridge, "cpuLink", "2ns"))
            linkBridgeCtrl = sst.Link("cramsim_bridge_link_" + str(next_mem))
            linkBridgeCtrl.connect(
                (cramsimBridge, "memLink", "1ns"), (cramsimCtrl, "txngenLink", "1ns"))
            linkDimmCtrl = sst.Link("cramsim_dimm_link_" + str(next_mem))
            linkDimmCtrl.connect(
                (cramsimCtrl, "memLink", "1ns"), (cramsimDimm, "ctrlLink", "1ns"))

        print(" - Capacity: " + str(config.gpu_memory_capacity_inB //
              config.hbmStacks) + " per HBM")
        print(" - Start Address: " + str(hex(memStartAddr)) +
              " End Address: " + str(hex(endAddr)))

        connect("bus_mem_link_%d" % next_mem,
                mem_l2_bus, "low_network_%d" % sub_group_id,
                mem, "direct_link",
                "50ps").setNoCut()

        next_mem = next_mem + 1

    for next_mem_id in range(num_L2s_per_stack):
        cacheStartAddr = 0 + (256 * next_cache)
        endAddr = cacheStartAddr + \
            config.gpu_memory_capacity_inB - (256 * total_mems)

        print("Creating L2 controller %d-%d (%d)..." %
              (next_group_id, next_mem_id, next_cache))
        print(" - Start Address: " + str(hex(cacheStartAddr)) +
              " End Address: " + str(hex(endAddr)))

        l2g = sst.Component("l2gcache_%d" % (next_cache), "memHierarchy.Cache")
        l2g.addParams(config.getGPUL2Params(cacheStartAddr, endAddr))
        l2g_gpulink = l2g.setSubComponent("cpulink", "memHierarchy.MemNIC")
        l2g_gpulink.addParams({"group": l2g_group})
        l2g_linkctrl = l2g_gpulink.setSubComponent(
            "linkcontrol", "shogun.ShogunNIC")
        l2g_memlink = l2g.setSubComponent("memlink", "memHierarchy.MemLink")

        connect("l2g_xbar_link_%d" % (next_cache),
                GPUrouter, "port%d" % (config.gpu_cores+(next_cache)),
                l2g_linkctrl, "port",
                config.default_link_latency).setNoCut()

        connect("l2g_mem_link_%d" % (next_cache),
                l2g_memlink, "port",
                mem_l2_bus, "high_network_%d" % (next_mem_id),
                config.default_link_latency).setNoCut()

        print("++ %d-%d (%d)..." %
              (next_cache, sub_mems, (next_cache + 1) % sub_mems))
        if (next_cache + 1) % sub_mems == 0:
            next_cache = next_cache + total_mems - (sub_mems - 1)
        else:
            next_cache = next_cache + 1


# ===========================================================
# Enable statistics
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": statFile})

print("Completed configuring the cuda-test model")


# ===========================================================
# Begin configuring vanadis core
# ===========================================================
import sys

# Copied from rdmaNIC test
import vanadisBlock as cpuBlock
import vanadisOS 
import memory

prefix = 'node'
nodeId = 0
numNodes = 1

cpuBuilder = cpuBlock.Vanadis_Builder()
memBuilder = memory.Builder()
osBuilder = vanadisOS.Builder()

numPorts = 4
port = 0
memBuilder.build(nodeId, numPorts, group=2)

# build the Vanadis OS, it returns
# OS cache
osCache = osBuilder.build(numNodes, nodeId, 0)

# connect OS L1 to Memory 
#memBuilder.connect( "OS_L1", port, osCache, 1, dest="2" ) 
memBuilder.connect( "OS_L1", port, osCache, group=1) 
port += 1; 

# build the Vanadis CPU block, this returns 
# cpu, L2 cache, DTLB ITLB  
cpu, L1, l1dcache_2_cpu, L2, dtlb, coredtlbWrapper, itlb = cpuBuilder.build(nodeId,0)

osBuilder.connectCPU( 0, cpu )
osBuilder.connectTlb( 0, "dtlb", dtlb )
osBuilder.connectTlb( 0, "itlb", itlb )

# connect CPU L2 to Memory 
#memBuilder.connect( "CPU_L2", port, L2, 1, dest="2,3" ) 
memBuilder.connect( "CPU_L2", port, L2, group=1) 
port += 1; 

# Balar TLB
balarTlbParams = {
    "debug_level": 0,
    "hitLatency": 10,
    "num_hardware_threads": 1,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
}

balarTlbWrapperParams = {
    "debug_level": 0,
}
balarTlbWrapper = sst.Component(prefix+".balarTlb", "mmu.tlb_wrapper")
balarTlbWrapper.addParams(balarTlbWrapperParams)
balarTlb = balarTlbWrapper.setSubComponent("tlb", "mmu.simpleTLB")
balarTlb.addParams(balarTlbParams)

# BalarBus
balarBus = sst.Component("balarBus", "memHierarchy.Bus")
balarBus.addParams({
    "bus_frequency": "4GHz",
    "drain_bus": "1"
})
# Connect the data links for Balar and dmaEngine here
connect("balarBus_balarTlb_link", balar_mem_iface, "port",
        balarBus, "high_network_0", "1ns")
connect("dmaEngine_balarBus_link", dma_mem_if, "port", 
        balarBus, "high_network_1", "1ns")
connect("balar_balarBus_link", balarBus, "low_network_0",
        balarTlbWrapper, "cpu_if", "1ns")

# Add core cache bus and connect it to coreTLB, balarTLB, and core l1dcache
coreCacheBus = sst.Component("coreCacheBus", "memHierarchy.Bus")
coreCacheBus.addParams({
    "bus_frequency": "4GHz",
    "drain_bus": "1"
})
# Connect the mem links for coreTLB and balarTLB
connect("coreTLB_coreCacheBus_link", balarTlbWrapper, "cache_if", 
        coreCacheBus, "high_network_0", "1ns")
connect("balarTLB_coreCacheBus_link", coredtlbWrapper, "cache_if",
        coreCacheBus, "high_network_1", "1ns")
connect("coreCacheBus_l1cache_link", coreCacheBus, "low_network_0",
        l1dcache_2_cpu, "port", "1ns")
# End balar connection

# Connect balar TLB to mmu
osBuilder.connectNicTlb( "nicTlb", balarTlb )

# connect the Balar MMIO to Memory
#memBuilder.connect( "NIC_MMIO", port, mmioIf, 3, source="1", dest="2" ) 
# memBuilder.connect( "Balar_MMIO", port, balar_mmio_iface, group=1, dest="4,5", source=4) 
memBuilder.connect( "Balar_MMIO", port, balar_mmio_iface, group=2) 
port += 1

# connect the DMA MMIO to Memory
#memBuilder.connect( "NIC_MMIO", port, mmioIf, 3, source="1", dest="2" ) 
# memBuilder.connect( "DMA_MMIO", port, dma_mmio_if, group=2, dest="4,5", source=1) 
memBuilder.connect( "DMA_MMIO", port, dma_mmio_if, group=3)
port += 1