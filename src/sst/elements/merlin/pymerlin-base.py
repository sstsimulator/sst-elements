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

import sst
import random
import copy


"""Base class for all the "templates" that will be using in the library.
It overloads __setattr__ and __getattr__ to accomplish the following:

1 - Allows you to set params as if they were variables in the class.
You can define required and optional parameters.  There is a check
that can be called to make sure all required parameters have been set.
These parameters are set using _defineRequiredParams and
_defineOptionalParams.  You can still set parameters using addParam
and addParams.

2 - It will fail if you accidently write to a variable that isn't
explicity declared.  This is to keep users from mispelling a
parameter, only to have no error message.  A consequence of this is
that you have to declare all your parameter names using
_declareClassVariables.

"""
class TemplateBase(object):
    def __init__(self):
        object.__setattr__(self,"_params",dict())
        object.__setattr__(self,"_req_params",set())
        object.__setattr__(self,"_opt_params",set())
        object.__setattr__(self,"_vars",set(["_params","_req_params","_opt_params","_vars"]))

    def _defineRequiredParams(self,l):
        self._req_params.update(l)

    def _defineOptionalParams(self,l):
        self._opt_params.update(l)

    def _declareClassVariables(self, l):
        for x in l:
            self._vars.update([x])
            object.__setattr__(self,x,None)

    def __setattr__(self,key,value):
        if key in self._req_params or key in self._opt_params:
            if value is None:
                self._params.pop(key,None)
            else:
                self._params[key] = value
        elif key in self._vars:
            object.__setattr__(self,key,value)
            #self.__dict__[key] = value
        else:
            raise AttributeError("%r has no attribute %r"%(self.__class__.__name__,key))

    def __getattr__(self,key):
        if key in self._req_params or key in self._opt_params:
            try:
                x = self._params[key]
                return self._params[key]
            except KeyError:
                #raise AttributeError("%r has not yet been set in %r"%(key,self.__class__.__name__))
                return None
        else:
            raise AttributeError("%r has no attribute %r"%(self.__class__.__name__,key))

    def addParams(self,p):
        for x in p:
            #if x in self._req_params or x in self._opt_params:
            #    self._params[x] = p[x]
            #else:
            #    raise KeyError("%r is not a defined parameter for %r"%(x,self.__class__.__name__))
            self.addParam(x,p[x])

    def addParam(self, key, value):
        if key in self._req_params or key in self._opt_params:
            if value is None:
                self._params.pop(key,None)
            else:
                self._params[key] = value
        else:
            raise KeyError("%r is not a defined parameter for %r"%(key,self.__class__.__name__))

    def checkRequiredParams(self):
        ret = []
        for x in self._req_params:
            if x not in self._params:
                ret.append(x)
        return ret

    def clone(self):
        return copy.deepcopy(self)

    def _createPrefixedParams(self,name):
        self._vars.add(name)
        object.__setattr__(self,name,TemplateBase())
        return self.__dict__[name]

    def combineParams(self,defaultParams,userParams):
        returnParams = defaultParams.copy()
        returnParams.update(userParams)
        return returnParams

# Classes implementing SimplenNetwork interface

class Topology(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareClassVariables(["endPointLinks","built","_router_template"])
        self.endPointLinks = []
        self.built = False
    def getName(self):
        return "NoName"
    #def addParams(self,p):
    #    self._params.update(p)
    #def addParam(self, key, value):
    #    self._params[key] = value
    def build(self, network_name, endpoint = None):
        pass
    def getEndPointLinks(self):
        pass
    def setRouterTemplate(self,template):
        self._router_template = template
    def getNumNodes(self):
        pass

class NetworkInterface(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)

    #def addParams(self,p):
    #    self._params.update(p)

    #def addParam(self, key, value):
    #    self._params[key] = value

    # returns subcomp, port_name
    def build(self,comp,slot,slot_num,link,job_id,job_size,logical_nid,use_nid_remap=False):
        return None


class LinkControl(NetworkInterface):
    def __init__(self):
        NetworkInterface.__init__(self)
        self._defineRequiredParams(["link_bw","input_buf_size","output_buf_size"])
        self._defineOptionalParams(["vn_remap","checkerboard","checkerboard_alg"])

    # returns subcomp, port_name
    def build(self,comp,slot,slot_num,job_id,job_size,logical_nid,use_nid_remap = False):
        sub = comp.setSubComponent(slot,"merlin.linkcontrol",slot_num)
        #sub.addLink(link,"rtr_port",link_latency)
        sub.addParams(self._params)
        sub.addParam("job_id",job_id)
        sub.addParam("job_size",job_id)
        sub.addParam("use_nid_remap",use_nid_remap)
        sub.addParam("logical_nid",logical_nid)
        return sub,"rtr_port"


class ReorderLinkControl(NetworkInterface):
    def __init__(self):
        NetworkInterface.__init__(self)
        self._declareClassVariables(["network_interface"])

    def setNetworkInterface(self,interface):
        self.network_interface = interface

    def build(self,comp,slot,slot_num,job_id,job_size,nid,use_nid_map = False):
        sub = comp.setSubComponent(slot,"merlin.reorderlinkcontrol",slot_num)
        sub.addParams(self._params)
        return self.network_interface.build(sub,"networkIF",0,job_id,job_size,nid,use_nid_map)




# Base class that is used to build endpoints
class Buildable(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)

    # build() returns an sst.SubComponent and port name
    def build(self, nID, extraKeys):
        return None

# Classes that define endpoints
class Job(Buildable):
    def __init__(self,job_id,size):
        Buildable.__init__(self)
        self._declareClassVariables(["enableAllStats","statInterval","network_interface","job_id","_nid_map","size"])
        self.job_id = job_id
        self.size = size
        self.enableAllStats = False
        self.statInterval = "0"
        self._nid_map = None

    def getName(self):
        return "BaseJobClass"

    def enableAllStatistics(self,interval):
        self.enableAllStats = True;
        self.statInterval = interval;



class TestJob(Job):
    def __init__(self,job_id,size):
        Job.__init__(self,job_id,size)
        self._defineRequiredParams(["num_peers"])
        self.num_peers = size
        self._defineOptionalParams(["num_messages","message_size","send_untimed_bcast"])

    def getName(self):
        return "TestJob"

    def build(self, nID, extraKeys):
        nic = sst.Component("testNic.%d"%nID, "merlin.test_nic")
        nic.addParams(self._params)
        nic.addParams(extraKeys)
        # Get the logical node id
        id = self._nid_map.index(nID)
        nic.addParam("id", id)

        #  Add the linkcontrol
        networkif, port_name = self.network_interface.build(nic,"networkIF",0,self.job_id,self.size,id,True)
        if self.enableAllStats:
            nic.enableAllStatistics({"type":"sst.AccumulatorStatistic","rate":self.statInterval})
            networkif.enableAllStatistics({"type":"sst.AccumulatorStatistic","rate":self.statInterval})

        return (networkif,port_name)





class RouterTemplate(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
    def instanceRouter(self, name, rtr_id, radix):
        pass
    def getTopologySlotName(self):
        pass
    def build(self, nID, extraKeys):
        pass
    def getPortConnectionInfo(port_num):
        pass

class hr_router(RouterTemplate):
    def __init__(self):
        RouterTemplate.__init__(self)
        self._defineRequiredParams(["link_bw","flit_size","xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size"])
        self._defineOptionalParams(["xbar_arb","network_inspectors","oql_track_port","oql_track_remote","num_vns","vn_remap","vn_remap_shm"])

    def instanceRouter(self, name, radix, rtr_id):
        rtr = sst.Component(name, "merlin.hr_router")
        rtr.addParams(self._params)
        rtr.addParam("num_ports",radix)
        rtr.addParam("id",rtr_id)
        return rtr
    
    def getTopologySlotName(self):
        return "topology"

    def enableQOS(self,qos_settings):
        self._params["portcontrol:output_arb"] = "merlin.arb.output.qos.multi"
        self._params["portcontrol:arbitration:qos_settings"] = qos_settings


class SystemEndpoint(Buildable):
    def __init__(self,system):
        Buildable.__init__(self)
        self._declareClassVariables(["_system"])
        self._system = system

    # build() returns an sst.Component and port name
    def build(self, nID, extraKeys):
        # Just get the proper job object for this nID and call build
        if self._system._endpoints[nID]:
            return self._system._endpoints[nID].build(nID, extraKeys)
        else:
            return (None, None)


class System:
    def __init__(self):
        self._topology = None
        #self.num_rails = 0
        #self.multirail_mode = None
        self._available_nodes = None
        self._num_nodes = None
        self._endpoints = None
        self._allocate_functions = {
            "random" : self._allocate_random,
            "linear" : self._allocate_linear}

    def setTopology(self, topo):
        self.topology = topo

    # Build the system
    def build(self):
        system_ep = SystemEndpoint(self)
        self._topology.build("network",system_ep)

    #def setNumRails(self,num_rails):
    #    self.num_rails = num_rails

    #def setMultiRailMode(self,mode):
    #    self.multirail_mode = mode

    def setTopology(self,topology):
        self._topology = topology
        self._num_nodes = topology.getNumNodes()
        self._available_nodes = [i for i in range(self._num_nodes)]
        self._endpoints = [None for i in range(self._num_nodes)]

    def allocateNodes(self,job,num_nodes,method,*args):
        # First, get the allocated list and the new available list
        allocated, available = self._allocate_functions[method](self._available_nodes,num_nodes,*args)
        self._available_nodes = available
        #  Record what type of endpoint is at each node
        job._nid_map = allocated
        for i in allocated:
            self._endpoints[i] = job


    # Built-in allocation functions
    def _allocate_random(self, available_nodes, num_nodes, seed = None):

        if seed is not None:
            random.seed(seed)

        random.shuffle(available_nodes)

        nid_list = available_nodes[0:num_nodes]
        available_nodes = available_nodes[num_nodes:]

        return nid_list, available_nodes


    def _allocate_linear(self, available_nodes, num_nodes):

        available_nodes.sort()

        nid_list = available_nodes[0:num_nodes]
        available_nodes = available_nodes[num_nodes:]

        return nid_list, available_nodes


