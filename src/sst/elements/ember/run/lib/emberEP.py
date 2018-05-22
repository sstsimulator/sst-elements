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

import sst
from sst.merlin import *

from loadUtils import *
import copy
import pprint
import myprint

pp = pprint.PrettyPrinter(indent=4)

def foo( ep, nodeID, detailedModel, rank ):
    links = detailedModel.getThreadLinks( rank )
    cpuNum = 0
    print 'EmberEP: node {0} connect {1} threads'.format(nodeID, len(links))
    for link in links: 
       ep.addLink(link,"detailed"+str(cpuNum),"1ps")
       cpuNum = cpuNum + 1

class EmberEP( EndPoint ):
    def __init__( self, nicConfig, emberConfig ): 

        self.emberConfig = emberConfig 
        self.nicConfig = nicConfig

    def build( self, nodeID, extraKeys ):

        emberConfig = self.emberConfig
        nicConfig = self.nicConfig

        ranksPerNode = emberConfig.getNumRanks()

        emberParams = emberConfig.getParams( nodeID ) 
        nicParams = nicConfig.getParams( nodeID, ranksPerNode ) 

        nic = sst.Component( "nic" + str(nodeID), nicConfig.getName( nodeID ) )
        nic.addParams( nicParams )  
        nic.addParams( extraKeys )

        memory = None;
        detailed = emberConfig.getDetailed(nodeID)

        if detailed:
            detailed.build(nodeID,ranksPerNode) 

            read, write = detailed.getNicLink( )
            print 'EmberEP: node {0} connect NIC'.format(nodeID)
            nic.addLink( read, "read", "1ps" )
            nic.addLink( write, "write", "1ps" )

            memory = sst.Component("memory" + str(nodeID), "thornhill.MemoryHeap")
            memory.addParam( "nid", nodeID )

        loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
        loopBack.addParam( "numCores", ranksPerNode )

        for x in xrange(ranksPerNode):
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) +
                                            "_EmberEP", emberConfig.getName(nodeID))

            ep.addParams( emberParams )
            #myprint.printParams( 'EmberParams:', emberParams ) 

            if detailed:
				foo( ep, nodeID, detailed, x )

            nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )
            nicLink.setNoCut()

            loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) + "_Link"  )
            loopLink.setNoCut() 

            ep.addLink(nicLink, "nic", nicParams["nic2host_lat"] )
            nic.addLink(nicLink, "core" + str(x), nicParams["nic2host_lat"] )

            ep.addLink(loopLink, "loop", "1ns")
            loopBack.addLink(loopLink, "core" + str(x), "1ns")

			# if there is a detailed memory model connect it to Ember
            if memory:
                memoryLink = sst.Link( "memory" + str(nodeID) + "core" + str(x) + "_Link"  )
                memoryLink.setNoCut()

                ep.addLink(memoryLink, "memoryHeap", "0 ps")
                memory.addLink(memoryLink, "detailed" + str(x), "0 ns")

        return (nic, "rtr", sst.merlin._params["link_lat"] )

    def getName( self ):
        return "EmberEP"

    def prepParams( self ):
        pass
