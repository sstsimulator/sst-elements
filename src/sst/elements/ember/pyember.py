#!/usr/bin/env python
#
# Copyright 2009-2020 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2020, NTESS
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
    "nic2host_lat" : "150ns",
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
    "hostReadDelay_ns" : 200,
    "packetSize" : "2048B",
    "packetOverhead" : 0,
    
    "numVNs" : 1, # total number of VN used
    "getHdrVN" : 0, # VN used for sending a get request
    "getRespSmallVN" : 0, # VN used for sending a get response <= getRespSize
    "getRespLargeVN" : 0, # VN used for sending a get response > getRespSize
    "getRespSize" : 15000
}
    
defaults.addParamSet("ember.basic_nic",_basic_nic_defaults)

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
    
    #'rxMemcpyModParams.range.0': '0-:344ps',
    #'txMemcpyModParams.range.0': '0-:344ps',
    #'txSetupModParams.range.0': '0-:130ns',
    #'txMemcpyMod': 'firefly.LatencyMod',
    #'txSetupMod': 'firefly.LatencyMod',
    #'rxSetupMod': 'firefly.LatencyMod',
    #'rxMemcpyMod': 'firefly.LatencyMod',
    
    'regRegionPerPageDelay_ns': 100,
    'verboseLevel': 0,
    'txMemcpyModParams.op': 'Mult',
    'sendAckDelay_ns': 0,
    'shortMsgLength': 12000,
    'regRegionXoverLength': 4096,
    
    'rendezvousVN' : 0, # VN used to send a match header that requires a get by the target
    'ackVN' : 0,  # VN used to send an ACK back to originator after target does a get
    
    #'pqs.verboseMask': -1,
    #'pqs.verboseLevel': 0,
    #'rxMemcpyModParams.range.0': '0-:344ps',
    #'txMemcpyModParams.range.0': '0-:344ps',
    #'txSetupModParams.range.0': '0-:130ns',
    #'txMemcpyModParams.op': 'Mult',
    #'rxSetupModParams.range.0': '0-:100ns'
}

defaults.addParamSet("ember.ctrl",_ctrl_defaults)

class BasicNicConfiguration(TemplateBase):

    def __init__(self):
        TemplateBase.__init__(self)
        #self._declareClassVariables("_nic")
        self._declareParams("main",_basic_nic_defaults.keys())
        # Set up default parameters
        self._subscribeToPlatformParamSet("ember.basic_nic")


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
        funcsm = self._createPrefixedParams("functionSM")
        funcsm._declareParams("main",_functionsm_defaults.keys(),"functionSM.")
        funcsm._subscribeToPlatformParamSet("ember.functionsm")
        ctrl = self._createPrefixedParams("ctrl")
        ctrl._declareParams("ctrl",_ctrl_defaults.keys())
        ctrl._subscribeToPlatformParamSet("ember.ctrl")


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

        # prefixed by "hermesParams.ctrlMsg."
        ctrl_params = {
            'txMemcpyMod': 'firefly.LatencyMod',
            'rxMemcpyMod': 'firefly.LatencyMod',
            'txSetupMod': 'firefly.LatencyMod',
            'rxSetupMod': 'firefly.LatencyMod',

            'rxMemcpyModParams.range.0': '0-:344ps',
            'txMemcpyModParams.range.0': '0-:344ps',
            'txSetupModParams.range.0': '0-:130ns',
            'txMemcpyModParams.op': 'Mult',
            'rxSetupModParams.range.0': '0-:100ns',
            'pqs.verboseMask': -1,
            'pqs.verboseLevel': 0,

            'nicsPerNode': nicsPerNode,
        }


        proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
        process = proto.setSubComponent( "process", "firefly.ctrlMsg" )


        proto.addParams(self._getGroupParams("ctrl"))
        proto.addParams(ctrl_params)
        process.addParams(self._getGroupParams("ctrl"))
        process.addParams(ctrl_params)

        #nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))
        virtNic.addLink(nicLink,'nic','1ns')

        #loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic'+str(nodeID%self._nicsPerNode)+'core'+str(x),'1ns'))
        process.addLink(loopLink,'loop','1ns')




class EmberMPIJob(Job):
    def __init__(self, job_id, num_nodes, numCores = 1, nicsPerNode = 1):
        Job.__init__(self,job_id,num_nodes * nicsPerNode)
        self._declareClassVariables(["_motifNum","_motifs","_numCores","_nicsPerNode","nic_configuration","_loopBackDict","_logfilePrefix","_logfileNids","os"])
        self._numCores = numCores
        self._nicsPerNode = nicsPerNode
        self._motifNum = 0
        self._motifs = dict()
        self._loopBackDict = dict()
        # set default nic configuration
        self.nic_configuration = BasicNicConfiguration()

        # Set up the prefixed params
        self.os = FireflyHades()

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

        nic, slot_name = self.nic_configuration.build(nodeID,self._numCores // self._nicsPerNode)

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

            # Add the params to the EmberEngine
            ep.addParam('jobId',self.job_id)
            ep.addParam('api.0.module','firefly.hadesMP')

            # Add the parameters defining the motifs
            ep.addParams(self._motifs)

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

            """
            os = ep.setSubComponent( "OS", "firefly.hades" )

            # prefixed by "hermesParams."
            os_params = {
                #'detailedCompute.name': 'thornhill.SingleThread',
                #'loadMap.0.len': 2,
                #'nicParams.verboseLevel': 0,
                #'nicModule': 'firefly.VirtNic',
                'verboseLevel': 0,
                'functionSM.defaultReturnLatency': 30000,
                'functionSM.verboseLevel': 0,
                'functionSM.defaultEnterLatency': 30000,
                #'memoryHeapLink.name': 'thornhill.MemoryHeapLink',
                #'loadMap.0.start': 0,
            }

            os.addParams(os_params)
            os.addParams( {'numNodes': self.size} )
            os.addParams( {'netMapName': 'Ember' + str(self.job_id) } )
            os.addParams( {'netId': nodeID } )
            os.addParams( {'netMapId': self._nid_map.index(nodeID) } )
            os.addParams( {'netMapSize': self.size } )
            os.addParams( {'coreId': x } )


            virtNic = os.setSubComponent( "virtNic", "firefly.VirtNic" )
            # For now, just use default parameters
            #for key, value in self.driverParams.items():
            #    if key.startswith(self.driverParams['os.name']+'.virtNic'):
            #        key = key[key.rfind('.')+1:]
            #        virtNic.addParam( key,value)

            # prefixed by "hermesParams.ctrlMsg."
            ctrl_params = {
                'shortMsgLength': 12000,
                'verboseLevel': 0,
                'txMemcpyMod': 'firefly.LatencyMod',
                'rxMemcpyMod': 'firefly.LatencyMod',
                'matchDelay_ns': 150,
                'txSetupMod': 'firefly.LatencyMod',
                'rxSetupMod': 'firefly.LatencyMod',
                'sendAckDelay_ns': 0,
                'regRegionXoverLength': 4096,
                'regRegionPerPageDelay_ns': 100,
                'regRegionBaseDelay_ns': 3000,
                "sendStateDelay_ps" : 0,
                "recvStateDelay_ps" : 0,
                "waitallStateDelay_ps" : 0,
                "waitanyStateDelay_ps" : 0,

                'rxMemcpyModParams.range.0': '0-:344ps',
                'txMemcpyModParams.range.0': '0-:344ps',
                'txSetupModParams.range.0': '0-:130ns',
                'txMemcpyModParams.op': 'Mult',
                'rxSetupModParams.range.0': '0-:100ns',
                'pqs.verboseMask': -1,
                'pqs.verboseLevel': 0,

                'nicsPerNode': self._nicsPerNode,

            }


            proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
            process = proto.setSubComponent( "process", "firefly.ctrlMsg" )


            proto.addParams(ctrl_params)
            process.addParams(ctrl_params)

            #nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'1ns'))
            virtNic.addLink(nicLink,'nic','1ns')

            #loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic'+str(nodeID%self._nicsPerNode)+'core'+str(x),'1ns'))
            process.addLink(loopLink,'loop','1ns')
            """
        return retval



