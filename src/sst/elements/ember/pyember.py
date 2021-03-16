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

import sys
import sst
from sst.merlin.base import *


defaults = PlatformDefinition("ember-defaults")
PlatformDefinition.registerPlatformDefinition(defaults)


_basic_nic_defaults = {
    "maxRecvMachineQsize" : 100,
    "maxSendMachineQsize" : 100,
    "nic2host_lat" : "150ns",
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
    "hostReadDelay_ns" : 200,
    "packetSize" : "2048B",
    "packetOverhead" : 0,
    
    #"numVNs" : 1, # total number of VN used
    #"getHdrVN" : 0, # VN used for sending a get request
    #"getRespSmallVN" : 0, # VN used for sending a get response <= getRespSize
    #"getRespLargeVN" : 0, # VN used for sending a get response > getRespSize
    #"getRespSize" : 15000
}
    
defaults.addParamSet("nic",_basic_nic_defaults)

_embermain_defaults = {
    'verboseMask': -1,
    'verboseLevel': 0,
}

defaults.addParamSet("ember",_embermain_defaults)


_functionsm_defaults = {
    'verboseLevel': 0,
    'defaultReturnLatency': 30000,
    'defaultEnterLatency': 30000,
    'defaultModule': 'firefly',
    'smallCollectiveVN' : 0, # VN used for collectives <= smallCollectiveSize
    'smallCollectiveSize' : 8,
}

defaults.addParamSet("ember.functionsm",_functionsm_defaults)

_ctrl_defaults = {
    'sendStateDelay_ps' : 0,
    'recvStateDelay_ps' : 0,
    'waitallStateDelay_ps' : 0,
    'waitanyStateDelay_ps' : 0,
    'matchDelay_ns': 150,
    'regRegionBaseDelay_ns': 3000,
    
    'txMemcpyMod': 'firefly.LatencyMod',
    'txSetupMod': 'firefly.LatencyMod',
    'rxSetupMod': 'firefly.LatencyMod',
    'rxMemcpyMod': 'firefly.LatencyMod',
    
    'regRegionPerPageDelay_ns': 100,
    'verboseLevel': 0,
    'sendAckDelay_ns': 0,
    'shortMsgLength': 12000,
    'regRegionXoverLength': 4096,
    
    'rendezvousVN' : 0, # VN used to send a match header that requires a get by the target
    'ackVN' : 0,  # VN used to send an ACK back to originator after target does a get
    
    'pqs.verboseMask': -1,
    'pqs.verboseLevel': 0,
    'rxMemcpyModParams.base': '344ps',
    'txMemcpyModParams.base': '344ps',
    'txSetupModParams.base': '130ns',
    'rxSetupModParams.base': '100ns',
    'txMemcpyModParams.op': 'Mult'
}

defaults.addParamSet("ember.ctrl",_ctrl_defaults)

class BasicNicConfiguration(TemplateBase):

    def __init__(self):
        TemplateBase.__init__(self)
        #self._declareClassVariables("_nic")

        # Set up all the parameters:
        self._declareParams("main", [
            "verboseLevel", "verboseMask",
            "nic2host_lat",
            "numVNs", "getHdrVN", "getRespLargeVN",
            "getRespSmallVN", "getRespSize",
            "reMatchDelay_ns", "txDelay_ns",
            "hostReadDelay_ns",
            "tracedPkt", "tracedNode",
            "maxSendMachineQsize", "maxRecvMachineQSize",
            "numSendMachines", "numRecvNicUnits",
            "messageSendAlignment", "nicAllocationPolicy",
            "packetOverhead", "packetSize",
            "maxActiveRecvStreams", "maxPendingRecvPkts",
            "dmaBW_GBs", "dmaContentionMult",
            ])

        self._declareParams("main",["useSimpleMemoryModel"])
        self.useSimpleMemoryModel = 0
        self._lockVariable("useSimpleMemoryModel")
        # Set up default parameters
        self._subscribeToPlatformParamSet("nic")


    def build(self,nID,num_vNics):
        nic = sst.Component("nic" + str(nID), "firefly.nic")
        self._applyStatisticsSettings(nic)
        #nic.addParams(self.combineParams(self.nic_defaults,self._getGroupParams("main")))
        nic.addParams(self._getGroupParams("main"))
        nic.addParam("nid",nID)
        nic.addParam("num_vNics",num_vNics)
        return nic, "rtrLink"

    def getVirtNicPortName(self, index):
        if not self._nic:
            print("Connecting Virtual Nic to firefly.nic before it has been built")
            sys.exit(1)
        return 'core'+str(x)

class SimpleMemoryNicConfiguration(BasicNicConfiguration):
    def __init__(self):
        BasicNicConfiguration.__init__(self)
        self._unlockVariable("useSimpleMemoryModel")
        self.addParam("useSimpleMemoryModel",1)
        self._lockVariable("useSimpleMemoryModel")
        # Define the extra params
        self._declareParamsWithUserPrefix(
            "main", # dictionary params will end up in
            "simpleMemoryModel", # prefix seen by end user
            [ "verboseLevel",
              "verboseMask","useBusBridge","useHostCache","useDetailedModel",
              "printConfig",
              "memReadLat_ns", "memWriteLat_ns", "memNumSlots",
              "nicNumLoadSlots", "nicNumStoreSlots",
              "hostNumLoadSlots", "hostNumStoreSlots",
              "busBandwidth_Gbs", "busNumLinks", "busLatency",
              "DLL_bytes", "TLP_overhead",
              "hostCacheUnitSize", "hostCacheNumMSHR", "hostCacheLineSize",
              "widgetSlots", "tlbPageSize", "tlbSize", "tlbMissLat_ns",
              "numTlbSlots", "nicToHostMTU",
              "numWalkers=32",
              ],
            "simpleMemoryModel."
        )

        self._declareParamsWithUserPrefix(
            "main",
            "shmem",
            ["nicCmdLatency", "hostCmdLatency"],
            "shmem."
        )

        self._declareParams(
            "main",
            ["shmemRxDelay_ns", "numShmemCmdSlots", "shmemSendAlignment",
             "FAM_memSize","FAM_backed"]
        )

        # This will be called in the parent as well, but we need to
        # pick up any parameters that just got defined
        self._subscribeToPlatformParamSet("nic")

class FireflyOS(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)

    def build(self,engine,nicLink,loopLink,size,nicsPerNode,job_id,pid,lid,coreId):
        pass

class FireflyHades(FireflyOS):

    def __init__(self):
        FireflyOS.__init__(self)
        #self._declareClassVariables(["_final_hades_params","_final_ctrl_params"])
        self._declareParams("main",["verboseLevel","verboseMask"])
        #self._subscribeToPlatformParamSet("ember")
        
        ### functionSM variables ###
        self._declareParamsWithUserPrefix(
            "main",  # dictionary params will end up in
            "functionsm", # prefix as seen by the end user
            [ 'verboseLevel', 'defaultReturnLatency', 'defaultEnterLatency', 'defaultModule',
              'smallCollectiveVN', 'smallCollectiveSize'],
            "functionSM." # prefix needed in the dictionary so things get passed correctly to elements
        )

        # Subscribe to ember.functionsm platform param set.  Need to
        # prefix the keys with functionsm so that the end use doesn't
        # have to put it on each key entry
        self._subscribeToPlatformParamSetAndPrefix("ember.functionsm","functionsm")

        ### ctrl variables ###
        self._declareParamsWithUserPrefix(
            "ctrl", # dictionary params will end up in
            "ctrl", # user visible prefix
            [ 'sendStateDelay_ps', 'recvStateDelay_ps', 'waitallStateDelay_ps', 'waitanyStateDelay_ps', 'matchDelay_ns',
              'regRegionBaseDelay_ns',
              'regRegionPerPageDelay_ns', 'verboseLevel', 'sendAckDelay_ns', 'shortMsgLength',
              'regRegionXoverLength',
              'rendezvousVN', 'ackVN' ]
        )

        self._declareParamsWithUserPrefix(
            "ctrl", # dictionary params will end up in
            "ctrl", # user visible prefix
            [ 'pqs.verboseMask', 'pqs.verboseLevel' ],
            "pqs." # prefix needed in the dictionary so things get passed correctly to elements
        )

        # Initial the variables for the modules
        #'txMemcpyMod', 'txSetupMod', 'rxSetupMod', 'rxMemcpyMod',
        subs = [
            "rxMemcpyMod",
            "txMemcpyMod",
            "txSetupMod",
            "rxSetupMod",
            "txFiniMod",
            "rxFiniMod",
            "rxPostMod"
        ]

        for var in subs:
            self._declareParamsWithUserPrefix(
                "ctrl", # dictionary params will end up in
                "ctrl", # user visible prefix
                [ var ],
            )

            self._declareParamsWithUserPrefix(
                "ctrl", # dictionary params will end up in
                "ctrl." + var + "Params", # user visible prefix
                [ "op", "base" ],
                var + "Params."
            )

            self._declareFormattedParamsWithUserPrefix(
                "ctrl", # dictionary params will end up in
                "ctrl." + var + "Params", # user visible prefix
                [ 'range.%d' ],
                var + "Params.range."
            )
            
        # Subscribe to ember.ctrl platform param set.  Need to
        # prefix the keys with ctrl so that the end use doesn't
        # have to put it on each key entry
        self._subscribeToPlatformParamSetAndPrefix("ember.ctrl","ctrl")
        

    def build(self,engine,nicLink,loopLink,size,nicsPerNode,job_id,pid,lid,coreId):

        os = engine.setSubComponent( "OS", "firefly.hades" )

        os.addParams(self._getGroupParams("main"))
        os.addParam( 'numNodes', size )
        os.addParam( 'netMapName', 'Ember' + str(job_id) )
        os.addParam( 'netId', pid )
        os.addParam( 'netMapId', lid )
        os.addParam( 'netMapSize', size )
        os.addParam( 'coreId', coreId )

        virtNic = os.setSubComponent( "virtNic", "firefly.VirtNic" )
        # For now, just use default parameters
        #for key, value in self.driverParams.items():
        #    if key.startswith(self.driverParams['os.name']+'.virtNic'):
        #        key = key[key.rfind('.')+1:]
        #        virtNic.addParam( key,value)


        proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
        process = proto.setSubComponent( "process", "firefly.ctrlMsg" )


        proto.addParams(self._getGroupParams("ctrl"))
        process.addParams(self._getGroupParams("ctrl"))

        # Should figure out how to put this into the main param group
        # object
        proto.addParam("nicsPerNode", nicsPerNode)
        process.addParam("nicsPerNode", nicsPerNode)

        
        #nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))
        virtNic.addLink(nicLink,'nic','1ns')

        #loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic'+str(nodeID%self._nicsPerNode)+'core'+str(x),'1ns'))
        process.addLink(loopLink,'loop','1ns')




class EmberJob(Job):
    def __init__(self, job_id, num_nodes, apis, numCores = 1, nicsPerNode = 1):
        Job.__init__(self,job_id,num_nodes * nicsPerNode)
        self._declareClassVariables(["_motifNum","_motifs","_numCores","_nicsPerNode","nic","_loopBackDict","_logfilePrefix","_logfileNids","os","_apis"])

        # Not needed - the subclasses will set the nic to the correct type:
        #x = self._createPrefixedParams("nic")
        #x._declareParams("nic",_basic_nic_defaults.keys())
        ##x._subscribeToPlatformParamSet("nic")

        # Not sure what to do with this yet
        #x = self._createPrefixedParams("ember")
        self._declareParamsWithUserPrefix("ember","ember",["verbose"])
        #x._subscribeToPlatformParamSet("ember")

        # Not clear what to do with this yet. Don't think we need it
        # since it's already captured throuch ctrl params??
        #x = self.ember._createPrefixedParams("firefly")

        self._numCores = numCores
        self._nicsPerNode = nicsPerNode
        self._motifNum = 0
        self._motifs = dict()
        self._apis = apis 
        self._loopBackDict = dict()
        # set default nic configuration

        # Instance the OS layer and lock it (make it read only)
        self.os = FireflyHades()
        self._lockVariable("os")

    def getName(self):
        return "EmberMPIJob"

    def addMotif(self,motif):
        # Parse the motif "command line" to create the proper params
        # to represent it

        # First, separate by spaces to get the individual arguments
        arglist = motif.split()

        key = 'motif' + str(self._motifNum) + '.name'

        # Look at the name of the motif.  This can be in two formats:
        # lib.motif or motif.  If the second, it turns into
        # ember.motifMotif
        motifPrefix = "ember."
        motifSuffix = "Motif"
        tmp = arglist[0].split('.')
        if len(tmp) >= 2:
            motifPrefix = tmp[0] + '.'
            arglist[0] = tmp[1]

        self._motifs[key] = motifPrefix + arglist[0] + motifSuffix

        arglist.pop(0)
        for x in arglist:
            y = x.split('=')
            key = 'motif' + str(self._motifNum) + '.arg.' + y[0]
            self._motifs[key] = y[1]

        self._motifNum = self._motifNum + 1
        self._motifs["motif_count"] = self._motifNum


    def enableMotifLog(self,logfilePrefix, nids = None):
        self._logfilePrefix = logfilePrefix;
        self._logfileNids = nids


    def build(self, nodeID, extraKeys):

        nic, slot_name = self.nic.build(nodeID,self._numCores // self._nicsPerNode)

        #print( nodeID, "nic", self._getGroupParams("nic") )
        #print( nodeID, "ember", self._getGroupParams("ember") )
        # not needed: nic.addParams( self._getGroupParams("nic") ).  This is done already in the nic.build call

        # Build NetworkInterface
        logical_id = self._nid_map.index(nodeID)
        networkif, port_name = self.network_interface.build(nic,slot_name,0,self.job_id,self.size,logical_id,False)

        # Store return value for later
        retval = ( networkif, port_name )


        # Set up the loopback for intranode communications.  In the
        # case of multiple nics, will name things based on the lowest
        # nodeID for this node.  This will allow different jobs to use
        # different numbers of nics per node (thoough the system level
        # allocation will have to be a multiple of any of the number
        # of nics per node values used.
        my_id_name = str( (nodeID // self._nicsPerNode) * self._nicsPerNode)
        
        loopBackName = "loopBack" + my_id_name
        if nodeID % self._nicsPerNode == 0:
            loopBack = sst.Component(loopBackName, "firefly.loopBack")
            loopBack.addParam( "numCores", self._numCores )
            loopBack.addParam( "nicsPerNode", self._nicsPerNode )
            self._loopBackDict[loopBackName] = loopBack
        else:
            loopBack = self._loopBackDict[loopBackName]


        for x in range(self._numCores // self._nicsPerNode):
            # Instance the EmberEngine
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_EmberEP", "ember.EmberEngine")
            self._applyStatisticsSettings(ep)

            ep.addParams( self._getGroupParams("ember") )

            # Add the params to the EmberEngine
            ep.addParam('jobId',self.job_id)

            # Add the parameters defining the motifs
            ep.addParams(self._motifs)
            ep.addParams(self._apis)

            # Check to see if we need to enable motif logging
            if self._logfilePrefix:
                if ( self._logfileNids is None or logical_id in self._logfileNids):
                    ep.addParam("motifLog",self._logfilePrefix)


            # Create the links to the OS layer
            nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )
            nicLink.setNoCut()

            linkName = "loop" + my_id_name + "nic"+ str(nodeID%self._nicsPerNode)+"core" + str(x) + "_Link"
            loopLink = sst.Link( linkName );
            loopLink.setNoCut()

            #nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))
            nic.addLink(nicLink,'core'+str(x),'1ns')

            #loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic'+str(nodeID%self._nicsPerNode)+'core'+str(x),'1ns'))
            loopBack.addLink(loopLink,'nic'+str(nodeID%self._nicsPerNode)+'core'+str(x),'1ns')


            # Create the OS layer
            self.os.build(ep,nicLink,loopLink,self.size,self._nicsPerNode,self.job_id,nodeID,self._nid_map.index(nodeID),x)

        return retval



class EmberNullJob(EmberJob):
    def __init__(self, job_id, num_nodes, numCores = 1, nicsPerNode = 1):
        # the NULL motif does not need any API so any will statisfy the check in emberengine.cc
        apis = { 'api.0.module':'firefly.hadesMP'}
        EmberJob.__init__(self,job_id,num_nodes,apis,numCores,nicsPerNode)
        self.nic = BasicNicConfiguration()
        self._lockVariable("nic")
        self.addMotif("Null")



class EmberMPIJob(EmberJob):
    def __init__(self, job_id, num_nodes, numCores = 1, nicsPerNode = 1):
        apis = { 'api.0.module':'firefly.hadesMP'}

        EmberJob.__init__(self,job_id,num_nodes,apis,numCores,nicsPerNode)

        self.nic = BasicNicConfiguration()
        self._lockVariable("nic")


class EmberSimpleMemoryJob(EmberJob):
    def __init__(self, job_id, num_nodes, apis, numCores = 1, nicsPerNode = 1):
        EmberJob.__init__(self,job_id,num_nodes,apis,numCores,nicsPerNode)

        #x = self.nic._createPrefixedParams("simpleMemoryModel")
#        x._declareParams("nic",["verboseLevel","verboseMask","useBusBridge","useHostCache","printConfig"],"simpleMemoryModel")
        #x._declareParams("nic",simpleMemory_params,"simpleMemoryModel")

        self.nic = SimpleMemoryNicConfiguration()
        self._lockVariable("nic")


class EmberSHMEMJob(EmberSimpleMemoryJob):
    def __init__(self, job_id, num_nodes, numCores = 1, nicsPerNode = 1):

        apis = {'api.0.module':'firefly.hadesSHMEM',
                'api.1.module':'firefly.hadesMisc'}

        EmberSimpleMemoryJob.__init__(self,job_id,num_nodes,apis,numCores,nicsPerNode)

        self._declareParamsWithUserPrefix(
            "ember",
            "ember.firefly.hadesSHMEM",
            ["verboseLevel","verboseMask"],
            "firefly.hadesSHMEM."
        )


class EmberFAMComputeNodeJob(EmberSHMEMJob):
    def __init__(self, job_id, num_nodes, numCores = 1, nicsPerNode = 1):

        EmberSHMEMJob.__init__(self,job_id,num_nodes,numCores,nicsPerNode)

        self._declareParamsWithUserPrefix(
            "ember",
            "ember.firefly.hadesSHMEM.famAddrMapper",
            ["name","bytesPerNode","numNodes","start","interval","blockSize"],
            "firefly.hadesSHMEM.famAddrMapper."
        )

        self._declareParamsWithUserPrefix(
            "ember",
            "ember.firefly.hadesSHMEM.famNodeMapper",
            ["name","firstNode","nodeStride"],
            "firefly.hadesSHMEM.famNodeMapper."
        )



class EmberFAMNodeJob(EmberSimpleMemoryJob):
    def __init__(self, job_id, num_nodes, numCores = 1, nicsPerNode = 1):
        # the NULL motif does not need any API so any will statisfy the check in emberengine.cc
        apis = { 'api.0.module':'firefly.hadesMP'}
        EmberSimpleMemoryJob.__init__(self,job_id,num_nodes,apis,numCores,nicsPerNode)
        #print("foobar")
        self.addMotif("Null")
        #self.nic._declareParams("nic",["FAM_memSize","FAM_backed"])
        #x = self.nic._createPrefixedParams("simpleMemoryModel")
        #x._declareParams("nic",simpleMemory_params,"simpleMemoryModel")
