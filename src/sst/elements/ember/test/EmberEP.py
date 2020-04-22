
import sst
from sst.merlin import *

from loadUtils import *

class EmberEP( EndPoint ):
    def __init__( self, jobId, driverParams, nicParams, motifs, nicsPerNode, numCores, ranksPerNode, statNodes, nidMap, numNodes, motifLogNodes, detailedModel ): # added motifLogNodes here

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
        self.nicsPerNode = nicsPerNode 
        self.loopBackDict = dict();

    def getName( self ):
        return "EmberEP"

    def prepParams( self ):
        pass

    def build( self, nodeID, extraKeys ):

        
        # See if this endpoints is not an ember endpoint
        if self.motifs["motif0.name"].startswith("<"):
            # This is a non ember endpoint. Find the element name.
            # The name will also include the slot to put the
            # linkcontrol into.  So the format is:
            # lib.element:slot_name
            element = self.motifs["motif0.name"]
            element = element[1:-1]
            
            # parse out the slot
            element,slot = element.split(":",1)

            ep = sst.Component( "nic" + str(nodeID), element)
            sc = ep.setSubComponent(slot, self.nicParams["module"])
            retval = (sc, "rtr_port", sst.merlin._params["link_lat"] )
            
            # Add paramters to the component
            ep.addParams(self.motifs["params"])

            ep.addParam("num_peers",self.numNids)
            
            # Add parameters to the linkcontrol
            sc.addParam("link_bw",sst.merlin._params["link_bw"]);
            sc.addParam("input_buf_size",sst.merlin._params["input_buf_size"]);
            sc.addParam("output_buf_size",sst.merlin._params["output_buf_size"]);
            sc.addParam("logical_nid",self.nidMap[nodeID])
            sc.addParam("logical_peers",self.numNids)
            sc.addParam("nid_map_name","nid_map_jobid_%d"%self.driverParams["jobId"])
            return retval

        
        
        # Regular ember motif processing
        nicComponentName = "firefly.nic"
        if 'nicComponent' in self.nicParams:
            nicComponentName = self.nicParams['nicComponent']

        nic = sst.Component( "nic" + str(nodeID), nicComponentName )
        rtrLink = nic.setSubComponent( "rtrLink", self.nicParams["module"] )
        #rtrLink.addParams( self.nicParams )
        if "link_bw" in self.nicParams:
            rtrLink.addParam("link_bw",self.nicParams["link_bw"])
        if "input_buf_size" in self.nicParams:
            rtrLink.addParam("input_buf_size",self.nicParams["input_buf_size"])
        if "output_buf_size" in self.nicParams:
            rtrLink.addParam("output_buf_size",self.nicParams["output_buf_size"])

        nic.addParams( self.nicParams )
        nic.addParams( extraKeys)
        nic.addParam( "nid", nodeID )
        retval = (rtrLink, "rtr_port", sst.merlin._params["link_lat"] )
        
        built = False 
        if self.detailedModel:
            #print (nodeID,  "use detailed")
            built = self.detailedModel.build( nodeID, self.numCores )

        memory = None
        if built:
            if self.nicParams["useSimpleMemoryModel"] == 0 :
                #print (nodeID,  "addLink  detailed")

                nicDetailedRead = nic.setSubComponent(  "nicDetailedRead", self.nicParams['detailedCompute.name']  )
                nicDetailedRead.addLink( self.detailedModel.getNicReadLink(), "detailed0", "1ps" )

                nicDetailedWrite = nic.setSubComponent(  "nicDetailedWrite", self.nicParams['detailedCompute.name']  )
                nicDetailedWrite.addLink( self.detailedModel.getNicWriteLink(), "detailed0", "1ps" )
            else:
                nicDetailedInterface = nic.setSubComponent(  "detailedInterface", self.nicParams['simpleMemoryModel.detailedModel.name']  )
                nicDetailedInterface.addParam("id",nodeID);
                memIF=nicDetailedInterface.setSubComponent("memInterface", "memHierarchy.memInterface")
                memIF.addParam("port","detailed")
                memIF.addLink( self.detailedModel.getNicLink( ), "detailed", "1ps" )

            memory = sst.Component("memory" + str(nodeID), "thornhill.MemoryHeap")
            memory.addParam( "nid", nodeID )
            #memory.addParam( "verboseLevel", 1 )

        loopBackName = "loopBack" + str(nodeID//self.nicsPerNode)        
        if nodeID % self.nicsPerNode == 0:
            loopBack = sst.Component(loopBackName, "firefly.loopBack")
            loopBack.addParam( "numCores", self.numCores )
            loopBack.addParam( "nicsPerNode", self.nicsPerNode )
            self.loopBackDict[loopBackName] = loopBack 
        else:
            loopBack = self.loopBackDict[loopBackName]


        # Create a motifLog only for one core of the desired node(s)
        logCreatedforFirstCore = False
        # end

        for x in range(self.numCores//self.nicsPerNode):
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_EmberEP", "ember.EmberEngine")

            os = ep.setSubComponent( "OS", "firefly.hades" )
            for key, value in list(self.driverParams.items()):
                if key.startswith("hermesParams."):
                    key = key[key.find('.')+1:] 
                    #print (key, value)
                    os.addParam( key,value)

            virtNic = os.setSubComponent( "virtNic", "firefly.VirtNic" )
            for key, value in list(self.driverParams.items()):
                if key.startswith(self.driverParams['os.name']+'.virtNic'):
                    key = key[key.rfind('.')+1:]
                    virtNic.addParam( key,value)

            proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
            process = proto.setSubComponent( "process", "firefly.ctrlMsg" )

            prefix = "hermesParams.ctrlMsg."
            for key, value in list(self.driverParams.items()):
                if key.startswith(prefix):
                    key = key[len(prefix):] 
                    #print (key, value)
                    proto.addParam( key,value)
                    process.addParam( key,value)

            ep.addParams(self.motifs)
            if built:
                links = self.detailedModel.getThreadLinks( x )
                cpuNum = 0
                for link in links: 
                    dc = os.setSubComponent( "detailedCompute", self.driverParams["hermesParams.detailedCompute.name"] )
                    dc.addLink(link,"detailed"+str(cpuNum),"1ps")
                    cpuNum = cpuNum + 1

            # Create a motif log only for the desired list of nodes (endpoints)
            # Delete the 'motifLog' parameter from the param list of other endpoints
            if 'motifLog' in self.driverParams:
                if self.driverParams['motifLog'] != '':
                    if (self.motifLogNodes):
                        for id in self.motifLogNodes:
                            if nodeID == int(id) and logCreatedforFirstCore == False:
                                #print (str(nodeID) + " " + str(self.driverParams['jobId']) + " " + str(self.motifLogNodes))
                                #print ("Create motifLog for node {0}".format(id))
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
                    print ("printStats for node {0}".format(id))
                    ep.addParams( {'motif1.printStats': 1} )

            os.addParams( {'netMapName': 'Ember' + str(self.driverParams['jobId']) } )
            os.addParams( {'netId': nodeID } )
            os.addParams( {'netMapId': self.nidMap[ nodeID ] } )
            os.addParams( {'netMapSize': self.numNids } )
            os.addParams( {'coreId': x } )

            nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )
            nicLink.setNoCut()

            linkName = "loop" + str(nodeID//self.nicsPerNode) + "nic"+ str(nodeID%self.nicsPerNode)+"core" + str(x) + "_Link" 
            loopLink = sst.Link( linkName ); 
            loopLink.setNoCut() 

            #ep.addLink(nicLink, "nic", self.nicParams["nic2host_lat"] )
            #nic.addLink(nicLink, "core" + str(x), self.nicParams["nic2host_lat"] )
            nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))

            loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic'+str(nodeID%self.nicsPerNode)+'core'+str(x),'1ns'))

            if built:
                memoryLink = sst.Link( "memory" + str(nodeID) + "core" + str(x) + "_Link"  )
                memoryLink.setNoCut()

                ml = os.setSubComponent( "memoryHeap", "thornhill.MemoryHeapLink" )
                memoryLink.connect( (memory,"detailed" + str(x), "0 ns" ),(ml,"memoryHeap", "0 ps"))

        return retval
