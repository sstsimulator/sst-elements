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
from sst.hg import *
from sst.firefly import *

class HgJob(Job):
    def __init__(self, job_id, num_nodes, apis, params, numCores = 1, nicsPerNode = 1):
        Job.__init__(self,job_id,num_nodes * nicsPerNode)
        self._declareClassVariables(["_numCores","_nicsPerNode","nic","_loopBackDict","os","_apis","_params","_numNodes"])

        # Not needed - the subclasses will set the nic to the correct type:
        #x = self._createPrefixedParams("nic")
        #x._declareParams("nic",_basic_nic_defaults.keys())
        ##x._subscribeToPlatformParamSet("nic")

        # Not sure what to do with this yet
        #x = self._createPrefixedParams("ember")
        #self._declareParamsWithUserPrefix("mercury","mercury",["verbose"])
        #x._subscribeToPlatformParamSet("ember")

        # Not clear what to do with this yet. Don't think we need it
        # since it's already captured throuch ctrl params??
        #x = self.ember._createPrefixedParams("firefly")

        self._numCores = numCores
        self._nicsPerNode = nicsPerNode
        self._apis = apis
        self._loopBackDict = dict()
        self._params = params
        self._numNodes = num_nodes

        # Instance the OS layer and lock it (make it read only)
        self.os = FireflyHades()
        self._lockVariable("os")

        # set default nic configuration
        self.nic = BasicNicConfiguration()

    def getName(self):
        return "HgJob"


    def build(self, nodeID, extraKeys):
        print("building nodeID=%d" % nodeID)
        if self._check_first_build():
            sst.addGlobalParams("loopback_params_%s"%self._instance_name,
                            { "numCores" : self._numCores,
                              "nicsPerNode" : self._nicsPerNode })

            sst.addGlobalParam("params_%s"%self._instance_name, 'jobId', self.job_id)

        nic, slot_name = self.nic.build(nodeID,self._numCores // self._nicsPerNode)

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
        #my_id_name = str( (nodeID // self._nicsPerNode) * self._nicsPerNode)
        my_id_name = str(nodeID)

        # need to make this work
        loopBackName = "loopBack" + my_id_name
        if nodeID % self._nicsPerNode == 0:
            loopBack = sst.Component(loopBackName, "firefly.loopBack")
            loopBack.addParam( "numCores", self._numCores )
            loopBack.addParam( "nicsPerNode", self._nicsPerNode )
            loopBack.addGlobalParamSet("loopback_params_%s"%self._instance_name);
            self._loopBackDict[loopBackName] = loopBack
        else:
            loopBack = self._loopBackDict[loopBackName]


        #for x in range(self._numNodes):
        # Instance the HgNode
        ep = sst.Component("hgnode" + my_id_name, "hg.node")
        ep.addParams(self._params)
        self._applyStatisticsSettings(ep)

        ep.addGlobalParamSet("params_%s"%self._instance_name )

        # Create the links to the OS layer
        linkname = "nic" + my_id_name + "_link";
        print("creating link %s" % linkname)
        nicLink = sst.Link(linkname)
        nicLink.setNoCut()
        print("connecting link %s" % linkname)
        #nicLink.connect( (ep,'nic','1ns' ),(nic,'core0','1ns'))
        nic.addLink(nicLink,'core0','1ns')

        linkName = "node" + str(nodeID) + "nic" + str(0) + "core" + str(0) + "loopback"
        print("loopName: %s" % linkName)
        loopLink = sst.Link( linkName );
        loopLink.setNoCut()
        loopBack.addLink(loopLink,'nic0core0','1ns')

        # Create the OS layer
        self.os.build(ep,nicLink,loopLink,self.size,self._nicsPerNode,self.job_id,nodeID,logical_id,0)

        return retval

#class BasicNicConfiguration(TemplateBase):

#    def __init__(self):
#        TemplateBase.__init__(self)
#        #self._declareClassVariables("_nic")

#        # Set up all the parameters:
#        self._declareParams("main", [
#            "verboseLevel", "verboseMask",
#            "nic2host_lat",
#            "numVNs", "getHdrVN", "getRespLargeVN",
#            "getRespSmallVN", "getRespSize",
#            "rxMatchDelay_ns", "txDelay_ns",
#            "hostReadDelay_ns",
#            "tracedPkt", "tracedNode",
#            "maxSendMachineQsize", "maxRecvMachineQSize",
#            "numSendMachines", "numRecvNicUnits",
#            "messageSendAlignment", "nicAllocationPolicy",
#            "packetOverhead", "packetSize",
#            "maxActiveRecvStreams", "maxPendingRecvPkts",
#            "dmaBW_GBs", "dmaContentionMult",
#            ])

#        self._declareParams("main",["useSimpleMemoryModel"])
#        self.useSimpleMemoryModel = 0
#        self._lockVariable("useSimpleMemoryModel")
#        # Set up default parameters
#        self._subscribeToPlatformParamSet("nic")


#    def build(self,nID,num_vNics=1):
#        if self._check_first_build():
#            sst.addGlobalParams("params_%s"%self._instance_name,self._getGroupParams("main"))
#            sst.addGlobalParam("params_%s"%self._instance_name,"num_vNics",num_vNics)

#        nic = sst.Component("nic" + str(nID), "firefly.nic")
#        self._applyStatisticsSettings(nic)
#        nic.addGlobalParamSet("params_%s"%self._instance_name)
#        nic.addParam("nid",nID)
#        #nic.addParam("num_vNics",num_vNics)
#        return nic, "rtrLink"

#    def getVirtNicPortName(self, index):
#        if not self._nic:
#            print("Connecting Virtual Nic to firefly.nic before it has been built")
#            sys.exit(1)
#        return 'core'+str(x)
