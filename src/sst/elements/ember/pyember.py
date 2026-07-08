#!/usr/bin/env python
#
# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
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

    def useNetworkIO(self, system, mapper="firefly.Block_NetworkIOMapper",
                     mapper_params=None, pool="default",
                     suppress_permit=False):
        # PR 6b forward-compat: pool kwarg accepts str|list[str]. v1 only
        # implements single-pool binding; list raises so v2 multi-pool
        # binding is not a breaking signature change.
        if isinstance(pool, list):
            raise NotImplementedError(
                "EmberMPIJob.useNetworkIO(): multi-pool binding "
                "(pool=list[str]) is Phase-4 / Tier-1 MPI-IO work; "
                "v1 accepts a single pool name only.")
        if not isinstance(pool, str) or not pool:
            raise ValueError(
                "EmberMPIJob.useNetworkIO(): pool must be a non-empty string")

        try:
            nid_list = system.getIoNidList(pool=pool)
        except KeyError:
            raise AssertionError(
                "EmberMPIJob.useNetworkIO(): pool %r was not allocated; "
                "call system.allocateIoNodes(count, method, pool=%r) first."
                % (pool, pool))
        if not nid_list:
            raise RuntimeError(
                "EmberMPIJob.useNetworkIO(): pool %r has no I/O nodes; "
                "call system.allocateIoNodes(count, method, pool=%r) before "
                "useNetworkIO()." % (pool, pool))

        csv = ",".join(str(n) for n in nid_list)

        existing_modules = {v for k, v in self._apis.items() if k.endswith(".module")}
        next_slot = len(existing_modules)

        if "firefly.hadesSHMEM" not in existing_modules:
            self._apis["api.%d.module" % next_slot] = "firefly.hadesSHMEM"
            next_slot += 1

        api_key = "api.%d.module" % next_slot
        self._apis[api_key] = "firefly.hadesNetworkIO"

        # EmberEngine::createApiMap scopes module params by module name
        # (firefly.hadesNetworkIO.*), NOT by api.<N>.*  See ember/emberengine.cc.
        mod_prefix = "firefly.hadesNetworkIO."
        self._apis[mod_prefix + "nodeMapper.name"] = mapper
        self._apis[mod_prefix + "nodeMapper.ioNidList"] = csv
        # PR 6b: emit the pool name as a param. The C++ side (HadesNetworkIO)
        # ignores `nodeMapper.poolName` until PR 6c wires up SharedArray
        # dispatch; emitting it now lets two-pool SDLs be validated end-to-end
        # at the Python layer.
        self._apis[mod_prefix + "nodeMapper.poolName"] = pool

        if mapper_params:
            for k, v in mapper_params.items():
                self._apis[mod_prefix + "nodeMapper." + k] = str(v)

        # PR 8 debug-misconfig hook: pools testsuite uses this to force a
        # rogue-NID or empty-permit scenario. Production code must not set it.
        if suppress_permit:
            self._apis[mod_prefix + "suppressPermitInsert"] = "1"

        return nid_list


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


class IoNodeJob(EmberNullJob):
    """Empty/idle job that occupies storage NIDs allocated via
    ``System.allocateIoNodes()``.

    Runs the NullMotif so the EmberEngine stays alive (the receiving NIC
    needs primaryComponentDoNotEndSim ref-count > 0) but does no host-side
    work.  The NIC uses ``StorageNicConfiguration`` so the SimpleSSD
    storage-model subcomponent is enabled on these (and only these) NIDs.
    Pass ``useSimpleSSD=False`` to fall back to the raw DMA path; in that
    case ``BasicNicConfiguration`` is used and no SSD is loaded.
    """

    _next_job_id = 9000

    def __init__(self, io_nid_list, numCores=1, nicsPerNode=1, job_id=None,
                 network_interface=None, useSimpleSSD=True,
                 pool_name="default", pool_size=None):
        if job_id is None:
            job_id = IoNodeJob._next_job_id
            IoNodeJob._next_job_id += 1

        EmberNullJob.__init__(self, job_id, len(io_nid_list), numCores, nicsPerNode)

        if network_interface is not None:
            self.network_interface = network_interface

        resolved_pool_size = (pool_size if pool_size is not None
                              else len(io_nid_list))

        if useSimpleSSD:
            self._unlockVariable("nic")
            self.nic = StorageNicConfiguration()
            self.nic._pool_name = pool_name
            self.nic._pool_size = resolved_pool_size
            self._lockVariable("nic")

        object.__setattr__(self, "pool_name", pool_name)
        object.__setattr__(self, "pool_size", resolved_pool_size)

    def getName(self):
        return "IoNodeJob"

    def build(self, nodeID, extraKeys):
        if getattr(self.nic, "_nid_map", None) is None:
            self._unlockVariable("nic")
            self.nic._nid_map = self._nid_map
            self._lockVariable("nic")
        return EmberNullJob.build(self, nodeID, extraKeys)


def _ioNodeJobDefaultFactory(io_nid_list, network_interface, pool_name="default"):
    # PR 6b: factory invoked once per pool by System.build(). The
    # `pool_name` kwarg replaces the previous nameless single-pool model.
    return IoNodeJob(io_nid_list, network_interface=network_interface,
                     pool_name=pool_name, pool_size=len(io_nid_list))

System.setIoNodeJobFactory(_ioNodeJobDefaultFactory)
