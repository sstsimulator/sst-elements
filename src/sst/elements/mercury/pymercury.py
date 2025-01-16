#!/usr/bin/env python
#
# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# of the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys
import sst
from sst.merlin.base import *
from sst.merlin import *

class HgJob(Job):
    def __init__(self, job_id, numNodes, numCores = 1, nicsPerNode = 1):
        Job.__init__(self,job_id,numNodes * nicsPerNode)
        self._declareClassVariables(["_numCores","_nicsPerNode","node","nic","os","_params","_numNodes"])
        self._numCores = numCores
        self._nicsPerNode = nicsPerNode
        self._numNodes = numNodes
        self.node = HgNode()
        self.os = HgOS()
        self.nic = HgNIC()

    def getName(self):
        return "HgJob"


    def build(self, nodeID, extraKeys):
        logical_id = self._nid_map[nodeID]
        node = self.node.build(nodeID,logical_id,self._numNodes * self._numCores)
        os = self.os.build(node,"os_slot") 
        nic = self.nic.build(node,"nic_slot")

        # Build NetworkInterface
        networkif, port_name = self.network_interface.build(node,"link_control_slot",0,self.job_id,self.size,logical_id,True)

        return (networkif, port_name)

class HgNode(TemplateBase):

    def __init__(self):
        TemplateBase.__init__(self)
        self._declareParams("params",["verbose",])
        self._subscribeToPlatformParamSet("node")

    def build(self,nid,lid,nranks):
        if self._check_first_build():
            sst.addGlobalParams("params_%s"%self._instance_name, self._getGroupParams("params"))
        node = sst.Component("node" + str(nid), "hg.node")
        node.addGlobalParamSet("params_%s"%self._instance_name)
        node.addParam("nodeID", nid)
        node.addParam("logicalID", lid)
        node.addParam("nranks", nranks)
        return node

class HgNIC(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareParams("params",["verbose",])
        self._subscribeToPlatformParamSet("nic")

    def build(self,comp,slot):
        if self._check_first_build():
            sst.addGlobalParams("params_%s"%self._instance_name, self._getGroupParams("params"))
        nic = comp.setSubComponent(slot,"hg.nic")
        nic.addGlobalParamSet("params_%s"%self._instance_name)
        return nic

class HgOS(TemplateBase):

    def __init__(self):
        TemplateBase.__init__(self)
        self._declareParams("params",["verbose",])
        self._declareParamsWithUserPrefix("params","app1",["name","exe","apis","verbose"],"app1.")
        self._subscribeToPlatformParamSet("operating_system")        

    def build(self,comp,slot):
        if self._check_first_build():
            sst.addGlobalParams("params_%s"%self._instance_name, self._getGroupParams("params"))
        sub = comp.setSubComponent(slot,"hg.operating_system")
        sub.addGlobalParamSet("params_%s"%self._instance_name)
        return sub 
