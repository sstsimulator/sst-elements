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

defaults = PlatformDefinition("firefly-defaults")
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

_fireflymain_defaults = {
    'verboseMask': -1,
    'verboseLevel': 0,
}

defaults.addParamSet("firefly",_fireflymain_defaults)


_functionsm_defaults = {
    'verboseLevel': 0,
    'defaultReturnLatency': 30000,
    'defaultEnterLatency': 30000,
    'defaultModule': 'firefly',
    'smallCollectiveVN' : 0, # VN used for collectives <= smallCollectiveSize
    'smallCollectiveSize' : 8,
}

defaults.addParamSet("firefly.functionsm",_functionsm_defaults)

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

defaults.addParamSet("firefly.ctrl",_ctrl_defaults)

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
            "rxMatchDelay_ns", "txDelay_ns",
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


    def build(self,nID,num_vNics=1):
        if self._check_first_build():
            sst.addGlobalParams("params_%s"%self._instance_name,self._getGroupParams("main"))
            sst.addGlobalParam("params_%s"%self._instance_name,"num_vNics",num_vNics)

        nic = sst.Component("nic" + str(nID), "firefly.nic")
        self._applyStatisticsSettings(nic)
        nic.addGlobalParamSet("params_%s"%self._instance_name)
        nic.addParam("nid",nID)
        #nic.addParam("num_vNics",num_vNics)
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
        #self._subscribeToPlatformParamSet("firefly")

        ### functionSM variables ###
        self._declareParamsWithUserPrefix(
            "main",  # dictionary params will end up in
            "functionsm", # prefix as seen by the end user
            [ 'verboseLevel', 'defaultReturnLatency', 'defaultEnterLatency', 'defaultModule',
              'smallCollectiveVN', 'smallCollectiveSize'],
            "functionSM." # prefix needed in the dictionary so things get passed correctly to elements
        )

        # Subscribe to firefly.functionsm platform param set.  Need to
        # prefix the keys with functionsm so that the end use doesn't
        # have to put it on each key entry
        self._subscribeToPlatformParamSetAndPrefix("firefly.functionsm","functionsm")

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

        # Subscribe to firefly.ctrl platform param set.  Need to
        # prefix the keys with ctrl so that the end use doesn't
        # have to put it on each key entry
        self._subscribeToPlatformParamSetAndPrefix("firefly.ctrl","ctrl")


    def build(self,engine,nicLink,loopLink,size,nicsPerNode,job_id,pid,lid,coreId):

        if self._check_first_build():
            set_name = "params_%s"%self._instance_name
            sst.addGlobalParams(set_name, self._getGroupParams("main"))
            sst.addGlobalParam(set_name, 'numNodes', size )
            sst.addGlobalParam(set_name, 'netMapName', 'Firefly' + str(job_id) )
            sst.addGlobalParam(set_name, 'netMapSize', size )

            sst.addGlobalParams("ctrl_params_%s"%self._instance_name, self._getGroupParams("ctrl"))
            sst.addGlobalParam("ctrl_params_%s"%self._instance_name, "nicsPerNode", nicsPerNode)

        os = engine.setSubComponent( "OS", "firefly.hades" )

        os.addGlobalParamSet("params_%s"%self._instance_name)
        os.addParam( 'netId', pid )
        os.addParam( 'netMapId', lid )
        os.addParam( 'coreId', coreId )

        virtNic = os.setSubComponent( "virtNic", "firefly.VirtNic" )
        # For now, just use default parameters
        #for key, value in self.driverParams.items():
        #    if key.startswith(self.driverParams['os.name']+'.virtNic'):
        #        key = key[key.rfind('.')+1:]
        #        virtNic.addParam( key,value)


        proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
        process = proto.setSubComponent( "process", "firefly.ctrlMsg" )


        proto.addGlobalParamSet("ctrl_params_%s"%self._instance_name)
        process.addGlobalParamSet("ctrl_params_%s"%self._instance_name)

        # Should figure out how to put this into the main param group
        # object
        #proto.addParam("nicsPerNode", nicsPerNode)
        #process.addParam("nicsPerNode", nicsPerNode)


        #nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))
        virtNic.addLink(nicLink,'nic','1ns')

        #loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic'+str(nodeID%self._nicsPerNode)+'core'+str(x),'1ns'))
        process.addLink(loopLink,'loop','1ns')


