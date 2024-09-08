# Automatically generated SST Python input
# Run script: sst testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'

# This script will run the vanadisHandshake binary that perform a
# vectoradd progam similar to a native CUDA program launch with
# vanadis as the host CPU and balar+GPGPUSim as the CUDA device

import sst
import argparse
from utils import *

# Arguments
args =  vars(balarTestParser.parse_args())

verbosity = args["balar_verbosity"]
cfgFile = args["config"]
statFile = args["statfile"]
statLevel = args["statlevel"]
traceFile = args["trace"]
binaryFile = args["cuda_binary"]

# ===========================================================
# Begin configuring Balar and Vanadis
# ===========================================================
# Overall configuration graph
# Balar side:
#   Balar memory interface
#       BalarMMIO <--mem_iface--> BalarTLBBus <--mem_iface--> dmaEngine
#                                    |
#       BalarTLB <-------------------|
#   Balar mmio interface
#       BalarMMIO <--mmio_iface--> memory
#       dmaEngine <--mmio_iface--> memory
#
# Vanadis side:
#   VanadisOS side:
#       VanadisCore <----> VanadisOS <----> mmu
#       iTLB <----> mmu
#       dTLB <----> mmu
#   Vanadis Cache side:
#       iTLB <----> L1Icache
#       dTLB <----> coreCacheBus <----> BalarTLB
#                       |
#       L1Dcache <------|
#       L1caches <----> L2cache <----> memory
import sys

# Vanadis related builders, copied from rdmaNIC test
import vanadisBlock as cpuBlock
import vanadisOS 
import memory

# Balar builder
import balarBlock

nodeId = 0
numNodes = 1

cpuBuilder = cpuBlock.Vanadis_Builder(args)
memBuilder = memory.Builder()
osBuilder = vanadisOS.Builder(args)
balarBuilder = balarBlock.Builder(args)

# ===========================================================
# Building phase
# ===========================================================
# build the Vanadis CPU block, this returns 
# cpu, L2 cache, DTLB ITLB  
cpu, L1, l1dcache_2_cpu, L2, dtlb, coredtlbWrapper, itlb = cpuBuilder.build(nodeId,0)

# build the Vanadis OS, it returns
# OS cache
osCache = osBuilder.build(numNodes, nodeId, 0)

# Build balar
balarTlbWrapper, balarTlb, balar_mmio_iface, dma_mmio_if = balarBuilder.buildVanadisIntegration(cfgFile, verbosity)

# Build CPU memory
numPorts = 4
port = 0
memBuilder.build(nodeId, numPorts, group=2)

# ===========================================================
# Connecting phase
# ===========================================================

# Cache connection to both CPU and Balar TLB
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

# OS connection
osBuilder.connectCPU( 0, cpu )
osBuilder.connectTlb( 0, "dtlb", dtlb )
osBuilder.connectTlb( 0, "itlb", itlb )
osBuilder.connectNicTlb( "nicTlb", balarTlb )

# Memory connection
# connect OS L1 to Memory 
memBuilder.connect( "OS_L1", port, osCache, group=1) 
port += 1

# connect CPU L2 to Memory 
#memBuilder.connect( "CPU_L2", port, L2, 1, dest="2,3" ) 
memBuilder.connect( "CPU_L2", port, L2, group=1) 
port += 1

# connect the Balar MMIO to Memory 
memBuilder.connect( "Balar_MMIO", port, balar_mmio_iface, group=2) 
port += 1

# connect the DMA MMIO to Memory
memBuilder.connect( "DMA_MMIO", port, dma_mmio_if, group=3)
port += 1

# ===========================================================
# Enable statistics
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": statFile})
print("Completed configuring Balar with Vanadis")
