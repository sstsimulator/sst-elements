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
from sst.firefly import *

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
        if self._check_first_build():
            sst.addGlobalParams("loopback_params_%s"%self._instance_name,
                            { "numCores" : self._numCores,
                              "nicsPerNode" : self._nicsPerNode })

            sst.addGlobalParams("params_%s"%self._instance_name, self._getGroupParams("ember"))
            sst.addGlobalParam("params_%s"%self._instance_name, 'jobId', self.job_id)
            sst.addGlobalParams("params_%s"%self._instance_name, self._motifs);
            sst.addGlobalParams("params_%s"%self._instance_name, self._apis);

        nic, slot_name = self.nic.build(nodeID,self._numCores // self._nicsPerNode)

        #print( nodeID, "nic", self._getGroupParams("nic") )
        #print( nodeID, "ember", self._getGroupParams("ember") )
        # not needed: nic.addParams( self._getGroupParams("nic") ).  This is done already in the nic.build call

        # Build NetworkInterface
        logical_id = self._nid_map[nodeID]
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
            #loopBack.addParam( "numCores", self._numCores )
            #loopBack.addParam( "nicsPerNode", self._nicsPerNode )
            loopBack.addGlobalParamSet("loopback_params_%s"%self._instance_name);
            self._loopBackDict[loopBackName] = loopBack
        else:
            loopBack = self._loopBackDict[loopBackName]


        for x in range(self._numCores // self._nicsPerNode):
            # Instance the EmberEngine
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_EmberEP", "ember.EmberEngine")
            self._applyStatisticsSettings(ep)

            ep.addGlobalParamSet("params_%s"%self._instance_name )

            # Add the params to the EmberEngine
            #ep.addParam('jobId',self.job_id)

            # Add the parameters defining the motifs

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
            self.os.build(ep,nicLink,loopLink,self.size,self._nicsPerNode,self.job_id,nodeID,logical_id,x)

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
