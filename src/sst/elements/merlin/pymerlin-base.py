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
        object.__setattr__(self,"_enabled_stats",list())
        object.__setattr__(self,"_stat_load_level",None)
        object.__setattr__(self,"_callbacks",dict())
        object.__setattr__(self,"_vars",set(["_params","_req_params","_opt_params","_enabled_stats","_stat_load_level","_vars","_callbacks"]))

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

        if key in self._callbacks:
            self._callbacks[key](key,value)

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

    def _setCallbackOnWrite(self,variable,func):
        self._callbacks[variable] = func

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

    # Functions to enable statistics
    def enableAllStatistics(self,stat_params,apply_to_children=False):
        self._enabled_stats.append((["--ALL--"],stat_params,apply_to_children))

    def enableStatistics(self,stats,stat_params,apply_to_children=False):
        self._enabled_stats.append((stats,stat_params,apply_to_children))

    def setStatisticLoadLevel(self,level,apply_to_children=False):
        self._stat_load_level = (level,apply_to_children)


    def _applyStatisticsSettings(self,component):
        for stat in self._enabled_stats:
            if stat[0][0] == "--ALL--":
                component.enableAllStatistics(stat[1],stat[2])
            else:
                component.enableStatistics(stat[0],stat[1],stat[2])

        if self._stat_load_level:
            component.setStatisticLoadLevel(self._stat_load_level[0],self._stat_load_level[1])


# Classes implementing SimplenNetwork interface

class Topology(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareClassVariables(["endPointLinks","built","_router_template","_prefix"])
        self.endPointLinks = []
        self.built = False
        self._prefix = ""
    def setNetworkName(self,name):
        if name:
            self._prefix = "%s."%name
    def getNetworkName(self):
        if self._prefix:
            return self._prefix[1:]
        return ""
    def getName(self):
        return "NoName"
    def build(self, endpoint):
        pass
    def getEndPointLinks(self):
        pass
    def setRouterTemplate(self,template):
        self._router_template = template
    def getNumNodes(self):
        pass
    def getRouterNameForId(self,rtr_id):
        return "%srtr.%d"%(self._prefix,rtr_id)
    def findRouterById(self,rtr_id):
        return sst.findComponentByName(self.getRouterNameForId(rtr_id))
    def _instanceRouter(self,radix,rtr_id):
        return self._router_template.instanceRouter(self.getRouterNameForId(rtr_id), radix, rtr_id)

class NetworkInterface(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)

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
        self._applyStatisticsSettings(sub)
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
        self._applyStatisticsSettings(sub)
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
        self._declareClassVariables(["statInterval","network_interface","job_id","_nid_map","size"])
        self.job_id = job_id
        self.size = size
        self.statInterval = "0"
        self._nid_map = None

    def getName(self):
        return "BaseJobClass"

    def getSize(self):
        return self.size



class RouterTemplate(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
    def instanceRouter(self, name, radix, rtr_id):
        pass
    def getTopologySlotName(self):
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
        self._applyStatisticsSettings(rtr)
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

    # Functions used to allocated endpoints to jobs
    _allocate_functions = {}

    @classmethod
    def addAllocationFunction(cls,name,func):
        cls._allocate_functions[name] = func


    def __init__(self):
        self._topology = None
        #self.num_rails = 0
        #self.multirail_mode = None
        self._allocation_block_size = 1
        self._available_nodes = None
        self._num_nodes = None
        self._endpoints = None

    def setTopology(self, topo):
        self.topology = topo

    # Build the system
    def build(self):
        system_ep = SystemEndpoint(self)
        self._topology.build(system_ep)

    #def setNumRails(self,num_rails):
    #    self.num_rails = num_rails

    #def setMultiRailMode(self,mode):
    #    self.multirail_mode = mode

    def setTopology(self,topology, allocation_block_size = 1):
        self._topology = topology
        self._num_nodes = topology.getNumNodes()
        self._allocation_block_size = allocation_block_size
        self._available_nodes = [i for i in range(self._num_nodes // allocation_block_size)]
        self._endpoints = [None for i in range(self._num_nodes)]

    def allocateNodes(self,job,method,*args):
        # First, get the allocated list and the new available list
        size = job.getSize()
        # Calulate the number of allocation units needed.  This just
        # does a celing divide (note, only works in python, in C the
        # result is truncated instead of floored)
        num_units = -(-size // self._allocation_block_size)

        allocated, available = self._allocate_functions[method](self._available_nodes,num_units,*args)
        self._available_nodes = available

        #  Record what type of endpoint is at each node, taking into
        #  account the allocation block size
        nid_map = [0 for i in range(size)]

        # Loop through and create a flattened map
        for i in range(num_units):
            for j in range(self._allocation_block_size):
                nid_map[i*self._allocation_block_size+j] = allocated[i] * self._allocation_block_size + j

        for i in nid_map:
            self._endpoints[i] = job

        job._nid_map = nid_map



# Built-in allocation functions for System
def _allocate_random(available_nodes, num_nodes, seed = None):

    if seed is not None:
        random.seed(seed)

    random.shuffle(available_nodes)

    nid_list = available_nodes[0:num_nodes]
    available_nodes = available_nodes[num_nodes:]

    return nid_list, available_nodes


def _allocate_linear(available_nodes, num_nodes):

    available_nodes.sort()

    nid_list = available_nodes[0:num_nodes]
    available_nodes = available_nodes[num_nodes:]

    return nid_list, available_nodes


def _allocate_random_linear(available_nodes, num_nodes, seed = None):

    if seed is not None:
        random.seed(seed)

    random.shuffle(available_nodes)

    nid_list = available_nodes[0:num_nodes]
    available_nodes = available_nodes[num_nodes:]

    nid_list.sort()

    return nid_list, available_nodes

def _allocate_interval(available_nodes, num_nodes, start_index, interval, count = 1):
    # Start with a sorted nid space
    available_nids = len(available_nodes)
    available_nodes.sort()


    nid_list = []
    index = start_index
    left = num_nodes

    try:
        while left > 0:
            for i in range(0,count):
                nid_list.append(available_nodes.pop(index))
                left -= 1
                # No need to increment index bacause we popped the last value

            index += (interval - count)
    except IndexError as e:
        print("ERROR: call to _allocate_interval caused index to exceed nid array bounds")
        print("start_index = %d, interval = %d, count = %d, available_nids at start of function = %d"%(start_index,interval,count,available_nids))
        raise

    return nid_list, available_nodes

def _allocate_indexed(available_nodes, num_nodes, nids):
    if len(nids) != num_nodes:
        print("ERROR: in call to _allocate_indexed length of index list does not equal number of endpoints to allocate")
        sst.exit()

    nid_list = []

    available_nodes.sort()

    count = 0
    try:
        # Get the indexed nids
        for i in nids:
            nid_list.append(available_nodes[i])


    except IndexError as e:
        print("ERROR: call to _allocate_indexed caused index to exceed nid array bounds, check index list")
        raise

    n = nids[:]
    n.sort(reverse=True)
    for i in n:
        available_nodes.pop(i)

    return nid_list, available_nodes

System.addAllocationFunction("random", _allocate_random)
System.addAllocationFunction("linear", _allocate_linear)
System.addAllocationFunction("random-linear",_allocate_random_linear)
System.addAllocationFunction("interval",_allocate_interval)
System.addAllocationFunction("indexed",_allocate_indexed)
