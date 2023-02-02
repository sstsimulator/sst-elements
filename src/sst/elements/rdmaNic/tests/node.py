import os
import sst
import sys

import vanadisBlock as cpuBlock
import vanadisOS 
import rdmaNic
import memory 

class Endpoint():
    def __init__(self,numNodes):
        self.numNodes = numNodes

    def prepParams(self):
        pass

    def build(self, nodeId, extraKeys ):

        prefix = 'node' + str(nodeId);

        cpuBuilder = cpuBlock.Vanadis_Builder()
        memBuilder = memory.Builder()
        osBuilder = vanadisOS.Builder()
        nicBuilder = rdmaNic.Builder(self.numNodes)

        numPorts = 4
        port = 0
        memBuilder.build(nodeId, numPorts, group=2 )

        # build the Vanadis OS, it returns
        # OS cache
        osCache = osBuilder.build( nodeId, 0 )

        # connect OS L1 to Memory 
        #memBuilder.connect( "OS_L1", port, osCache, 1, dest="2" ) 
        memBuilder.connect( "OS_L1", port, osCache, group=1 ) 
        port += 1; 

        # build the Vanadis CPU block, this returns 
        # cpu, L2 cache, DTLB ITLB  
        cpu, L2, dtlb, itlb = cpuBuilder.build(nodeId,0)

        osBuilder.connectCPU( 0, cpu )
        osBuilder.connectTlb( 0, "dtlb", dtlb )
        osBuilder.connectTlb( 0, "itlb", itlb )

        # connect CPU L2 to Memory 
        #memBuilder.connect( "CPU_L2", port, L2, 1, dest="2,3" ) 
        memBuilder.connect( "CPU_L2", port, L2, group=1 ) 
        port += 1; 

        # build the Rdma NIC, this returns
        # MMIO link, DMA cache, DMA TLB
        mmioIf, dmaCache, dmaTlb, netLink = nicBuilder.build(nodeId)

        osBuilder.connectNicTlb( "nicTlb", dmaTlb )

        # connect the NIC MMIO to Memory
        #memBuilder.connect( "NIC_MMIO", port, mmioIf, 3, source="1", dest="2" ) 
        memBuilder.connect( "NIC_MMIO", port, mmioIf, group=2 ) 
        port += 1; 

        # connect the NIC DMA Cache to Memory
        #memBuilder.connect( "NIC_DMA", port, dmaCache, 1, dest="2" ) 
        memBuilder.connect( "NIC_DMA", port, dmaCache, group=1 ) 
        port += 1; 

        return netLink
