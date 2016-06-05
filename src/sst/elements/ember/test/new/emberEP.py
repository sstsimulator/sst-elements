import sst
from sst.merlin import *

from loadUtils import *
import copy
import pprint

pp = pprint.PrettyPrinter(indent=4)

def foo( ep, detailedModel ):
    links = detailedModel.getThreadLinks( x )
    cpuNum = 0
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
            nic.addLink( detailed.getNicLink( ), "detailed0", "1ps" )

            memory = sst.Component("memory" + str(nodeID), "thornhill.MemoryHeap")
            memory.addParam( "nid", nodeID )

        loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
        loopBack.addParam( "numCores", ranksPerNode )

        for x in xrange(ranksPerNode):
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) +
                                            "_EmberEP", emberConfig.getName(nodeID))

            ep.addParams( emberParams )
            #pp.pprint( emberParams ) 

            if detailed:
				foo( ep, detailed )

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
