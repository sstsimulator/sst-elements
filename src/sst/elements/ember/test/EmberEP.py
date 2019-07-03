
import sst
from sst.merlin import *

from loadUtils import *

class EmberEP( EndPoint ):
    def __init__( self, jobId, driverParams, nicParams, motifs, numCores, ranksPerNode, statNodes, nidMap, numNodes, motifLogNodes, detailedModel ): # added motifLogNodes here

        self.driverParams = driverParams
        self.nicParams = nicParams
        self.motifs = motifs
        self.numCores = numCores
        self.driverParams['jobId'] = jobId
        self.statNodes = statNodes
        self.nidMap = nidMap
        self.numNids = numNodes
        # in order to create motifLog files only for the desired nodes of a job
        self.motifLogNodes = motifLogNodes
        self.detailedModel = detailedModel

    def getName( self ):
        return "EmberEP"

    def prepParams( self ):
        pass

    def build( self, nodeID, extraKeys ):

        nicComponentName = "firefly.nic"
        if 'nicComponent' in self.nicParams:
            nicComponentName = self.nicParams['nicComponent']

        nic = sst.Component( "nic" + str(nodeID), nicComponentName )
        rtrLink = nic.setSubComponent( "rtrLink", "merlin.linkcontrol" )

        nic.addParams( self.nicParams )
        nic.addParams( extraKeys)
        nic.addParam( "nid", nodeID )
        retval = (rtrLink, "rtr", sst.merlin._params["link_lat"] )
 
        built = False 
        if self.detailedModel:
            built = self.detailedModel.build( nodeID, self.numCores )

        memory = None
        if built:
            print "detailedModel", nodeID
            nic.addLink( self.detailedModel.getNicLink( ), "detailed", "1ps" )
            memory = sst.Component("memory" + str(nodeID), "thornhill.MemoryHeap")
            memory.addParam( "nid", nodeID )
            #memory.addParam( "verboseLevel", 1 )

        loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
        loopBack.addParam( "numCores", self.numCores )


        # Create a motifLog only for one core of the desired node(s)
        logCreatedforFirstCore = False
        # end

        for x in xrange(self.numCores):
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_EmberEP", "ember.EmberEngine")

            os = ep.setSubComponent( "OS", "firefly.hades" )
            for key, value in self.driverParams.items():
                if key.startswith("hermesParams."):
                    key = key[key.find('.')+1:] 
                    #print key, value
                    os.addParam( key,value)

            virtNic = os.setSubComponent( "virtNic", "firefly.VirtNic" )

            #ml = os.setSubComponent( "memoryHeap", "thornhill.MemoryHeapLink" )
            proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
            process = proto.setSubComponent( "process", "firefly.ctrlMsg" )

            prefix = "hermesParams.ctrlMsg."
            for key, value in self.driverParams.items():
                if key.startswith(prefix):
                    key = key[len(prefix):] 
                    #print key, value
                    proto.addParam( key,value)
                    process.addParam( key,value)

            ep.addParams(self.motifs)
            if built:
                links = self.detailedModel.getThreadLinks( x )
                cpuNum = 0
                for link in links: 
                    dc = os.setSubComponent( "detailedCompute", "thornhill.SingleThread" )
                    ep.addLink(link,"detailed"+str(cpuNum),"1ps")
                    cpuNum = cpuNum + 1

            # Create a motif log only for the desired list of nodes (endpoints)
            # Delete the 'motifLog' parameter from the param list of other endpoints
            if 'motifLog' in self.driverParams:
            	if self.driverParams['motifLog'] != '':
            		if (self.motifLogNodes):
            			for id in self.motifLogNodes:
            				if nodeID == int(id) and logCreatedforFirstCore == False:
                				#print str(nodeID) + " " + str(self.driverParams['jobId']) + " " + str(self.motifLogNodes)
                				#print "Create motifLog for node {0}".format(id)
                				logCreatedforFirstCore = True
                				ep.addParams(self.driverParams)
                			else:
                				tempParams = copy.copy(self.driverParams)
                				del tempParams['motifLog']
                				ep.addParams(tempParams)
                	else:
                		tempParams = copy.copy(self.driverParams)
                		del tempParams['motifLog']
                		ep.addParams(tempParams)
                else:
                	ep.addParams(self.driverParams)      				
            else:
            	ep.addParams(self.driverParams)
           	# end          


            # Original version before motifLog
            #ep.addParams(self.driverParams)

            for id in self.statNodes:
                if nodeID == id:
                    print "printStats for node {0}".format(id)
                    ep.addParams( {'motif1.printStats': 1} )

            os.addParams( {'netMapName': 'Ember' + str(self.driverParams['jobId']) } )
            os.addParams( {'netId': nodeID } )
            os.addParams( {'netMapId': self.nidMap[ nodeID ] } )
            os.addParams( {'netMapSize': self.numNids } )
            os.addParams( {'coreId': x } )

            nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )
            nicLink.setNoCut()

            loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) + "_Link"  )
            loopLink.setNoCut() 

            #ep.addLink(nicLink, "nic", self.nicParams["nic2host_lat"] )
            #nic.addLink(nicLink, "core" + str(x), self.nicParams["nic2host_lat"] )
            nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))

            loopLink.connect( (process,'loop','1ns' ),(loopBack,'core'+str(x),'1ns'))

            if built:
                memoryLink = sst.Link( "memory" + str(nodeID) + "core" + str(x) + "_Link"  )
                memoryLink.setNoCut()

                ep.addLink(memoryLink, "memoryHeap", "0 ps")
                memory.addLink(memoryLink, "detailed" + str(x), "0 ns")

        return retval
