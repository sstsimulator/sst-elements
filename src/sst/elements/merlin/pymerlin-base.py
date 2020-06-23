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

# importlib didn't exist until 2.7, so if we're running on 2.6, then
# import statement will fail.
try:
    from importlib import import_module
except ImportError:
    # We must be using 2.6, use the old module import code.
    def import_module(filename):
        return __import__( filename, fromlist=[''] )

class PlatformDefinition:

    _platforms = dict()
    _current_platform = None

    def __init__(self, platform_name):
        self.name = platform_name
        self._param_sets = dict()
        self._instance_types = dict()

    @classmethod
    def registerPlatformDefinition(cls,definition):
        cls._platforms[definition.name] = definition

    @classmethod
    def setCurrentPlatform(cls,platform_name):
        cls._current_platform = cls._platforms[platform_name]

    @classmethod
    def getCurrentPlatform(cls):
        return cls._current_platform

    @classmethod
    def loadPlatformFile(cls,name):
        import_module(name)

    @classmethod
    def getClassType(cls,key):
        return cls._current_platform.getPlatformDefinedType(key)

    @classmethod
    def getPlatformDefinedClassInstance(cls,key):
        if not cls._current_platform:return None
        type_name = cls._current_platform.getClassType(key)
        if not type_name: return None
        
        module_name, class_name = type_name.rsplit(".", 1)
        return getattr(import_module(module_name), class_name)()




    # Compose a new PlatformDefinition from existing ones.  The
    # definititions used must already be registered.  The function
    # takes a list of tuples, where each tuple is a
    # PlatformDefinition, paramset pair.
    @classmethod
    def compose(cls,platform_name, parts):
        platdef = PlatformDefinition(platform_name)
        for (pd_name, ps_name) in parts:
            pd = PlatformDefinition._platforms[pd_name]
            if ps_name == "ALL":
                ps_list = pd.getParamSetNames()
            else:
                ps_list = [ ps_name ]

            for name in ps_list:
                ps = pd.getParamSet(name)
                platdef.addParamSet(name,ps)

        PlatformDefinition.registerPlatformDefinition(platdef)
        return platdef

    # Adds a new parameter set or updates the set if it already exists
    def addParamSet(self,set_name, params):
        if set_name in self._param_sets:
            self._param_sets[set_name].update(params)
        else:
            self._param_sets[set_name] = copy.copy(params)

    def getParamSetNames(self):
        return self._param_sets.keys()

    def getParamSet(self,name):
        return self._param_sets[name]

    def addClassType(self,key,value):
        self._instance_types[key] = value

    def getClassType(self,key):
        try:
            return self._instance_types[key]
        except KeyError:
            return None
        
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
class _member_info(object):
    def __init__(self):
        self.value = None
        self.dictionaries = list()
        self.locked = False
        self.call_back = None



class _AttributeManager(object):
    def __init__(self):
        object.__setattr__(self,"_in_dict",set(["_in_dict","_vars"]))
        object.__setattr__(self,"_vars",dict())

    def _setPassthroughTarget(self,target):
        object.__setattr__(self,"_passthrough_target",target)

    def _addDirectAttribute(self,name,value=None):
        self._in_dict.add(name)
        object.__setattr__(self,name,value)
                
    def _addVariable(self,var,dictionary=None,prefix=None):
        if not var in self._vars:
            self._vars[var] = _member_info()
            #raise AttributeError("%r: %s was already declared as a variable or parameter"%(self.__class__.__name__,var))

        myvar = self._vars[var]
        if dictionary is not None:
            myvar.dictionaries.append(( dictionary, prefix) )

        
    # Function that will be called when a class variable is accessed
    # to write
    def __setattr__(self,key,value):
        if key in self._vars:
            var = self._vars[key]
            if var.locked:
                raise AttributeError("attribute %s of class %r has been marked read only"%(key,self.__class__.__name__))            
            # Set the value
            var.value = value
            # Put the value in the specified param dictionaries with
            # their corresponding prefixes
            for (d,p) in var.dictionaries:
                # Apply the prefix for this dictionary
                mykey = key
                if p:
                    mykey = p + mykey
                if value is None:
                    # If set to None, remove from dictionary
                    d.pop(mykey,None)
                else:
                    d[mykey] = value

            # If there's a callback, call it
            if var.call_back:
                var.call_back(key,value)

        # If key resides in __dict__, just call object setattr function
        elif key in self._in_dict:
            object.__setattr__(self,key,value)

        # Not allowing writes to unknown variables
        else:
            # If there's a passthrough target, send this write to it
            if "_passthrough_target" in self.__dict__:
                self.__dict__["_passthrough_target"].__setattr__(key,value)
            else:
                raise KeyError("%r has no attribute %r"%(self.__class__.__name__,key))
        

    # Function will be called when a variable not in __dict__ is read
    def __getattr__(self,key):
        if key in self._vars:
            return self._vars[key].value
        else:
            if "_passthrough_target" in self.__dict__:
                self.__dict__["passthrough_target"].__getattr__(key,value)
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
        if key in self._vars:
            var = self._vars[key]
            if var.locked:
                raise AttributeError("parameter %s of class %r has been marked read only"%(key,self.__class__.__name__))            

            # Set the value
            var.value = value
            # Put the value in the specified param dictionaries with
            # their corresponding prefixes
            for (d,p) in var.dictionaries:
                # Apply the prefix for this dictionary
                mykey = key
                if p:
                    mykey = p + mykey
                if value is None:
                    # If set to None, remove from dictionary
                    d.pop(mykey,None)
                else:
                    d[mykey] = value

            # If theres a callback, call it
            if var.call_back:
                var.call_back(key,value)
        else:
            raise KeyError("%s is not a defined parameter for %r"%(key,self.__class__.__name__))

    def getParam(self,key):
        if key in self._vars:
            return self._vars[key].value
        else:
            raise AttributeError("%r has no param %r"%(self.__class__.__name__,key))


    # Function to lock a variable (make it read-only)
    def _lockVariable(self,variable):
        self._vars[variable].locked = True
        
    # Function to unlock a variable (make it writable)
    def _unlockVariable(self,variable):
        self._vars[variable].locked = False

    def _isVariableLocked(self,variable):
        return self._vars[variable].locked

    def _areVariablesLocked(self,variables):
        ret = True
        for var in variables:
            ret = ret and self._vars[var].locked
        return ret


    # Set a callback to be called on write to a variable
    def _setCallbackOnWrite(self,variable,func):
        self._vars[variable].call_back = func


    # Functions to subscribe to PlatformParameters.  This will apply
    # platform params from the specified set to the object.  This
    # should be the last thing called in the init function of the
    # derived class.  It must be called after all variables and params
    # are declared.
    def _subscribeToPlatformParamSet(self,set_name):
        plat = PlatformDefinition.getCurrentPlatform()
        if not plat:
            return
        try:
            params = plat.getParamSet(set_name)
        except KeyError:
            # No set with the correct name
            return
        if params:
            for key,value in params.items():
                try:
                    self.__setattr__(key,value)
                except AttributeError as err:
                    print("WARNING: attribute %s cannot be set in %r as it is marked read-only"%(key,self.__class__.__name__))
                except KeyError:
                    # not a valid variable for this class, just ignore
                    pass


    def clone(self):
        return copy.deepcopy(self)

    
class _SubAttributeManager(_AttributeManager):
    def __init__(self,parent):
        _AttributeManager.__init__(self)
        self._addDirectAttribute("_parent",parent)
        
    def _declareParams(self,group,plist,prefix=None):
        if not group in self._parent._groups:
            self._parent._groups[group] = dict()

        group_dict = self._parent._groups[group]

        for var in plist:
            self._addVariable(var,group_dict,prefix)


class TemplateBase(_AttributeManager):
    def __init__(self):
        _AttributeManager.__init__(self)
        self._addDirectAttribute("_groups",dict())
        self._addDirectAttribute("_enabled_stats",list())
        self._addDirectAttribute("_stat_load_level",None)


    # Declare variables used by the class, but not passed as
    # parameters to any Components/SubCompnoents.  Variable will be
    # initialized to None.
    def _declareClassVariables(self, vlist):
        for var in vlist:
            self._addVariable(var)

    # Declare parameters (variables) of the class.  The group will
    # collect params together for easy passing as parameters to
    # Components/SubComponents
    def _declareParams(self,group,plist,prefix=None):
        if not group in self._groups:
            self._groups[group] = dict()

        group_dict = self._groups[group]
        
        for var in plist:
            self._addVariable(var,group_dict,prefix)
            #if var in self._vars:
            #    raise AttributeError("%r: %s was already declared as a variable or parameter"%(self.__class__.__name__,var))
            #self._vars[var] = _member_info(group_dict)

    # Get the dictionary of parameters for a group
    def _getGroupParams(self,group):
        return self._groups[group]

    
    def _createPrefixedParams(self,name):
        self._addDirectAttribute(name,_SubAttributeManager(self))
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

    @classmethod
    def instanceTemplate(cls,base, name):
        def findClass(base,name,_seen):
            print("Base = %r"%base)
            subs = base.__subclasses__()
            print("  subs: %r"%subs)
            #print(subs)
            for sub in subs:
                print("    sub: %r"%sub)
                sub_name = sub.__name__
                # If this is what we're looking for, return it
                if name == sub.__name__:
                    return sub
                else:
                    # If we've already seen it, continue
                    if sub_name in _seen:
                        continue;
                    _seen.add(sub.__name__)
                    cls = findClass(sub,name,_seen)
                    if cls:
                        return cls

            return None
                    
        seen = set()
        sub = findClass(base,name,seen)
                
        return sub()
        

# Classes implementing topology
class Topology(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareClassVariables(["network_name","endPointLinks","built","router","_prefix"])

        self._prefix = ""
        self._lockVariable("_prefix")
        self._setCallbackOnWrite("network_name",self._network_name_callback)

        self._setCallbackOnWrite("router",self._router_callback)
        self.router = PlatformDefinition.getPlatformDefinedClassInstance("router")

        # if no router was set in platform file, set a default (can be
        # overwritten)
        if not self.router:
            self.router = hr_router()
            self._unlockVariable("router")
            
        self.endPointLinks = []
        self.built = False

    def _network_name_callback(self, variable_name, value):
        self._lockVariable(variable_name)
        if value: 
            self._unlockVariable("_prefix")
            self._prefix = "%s."%value
            self._lockVariable("_prefix")

    def _router_callback(self, variable_name, value):
        # Lock variable if it wasn't set to None
        if value: self._lockVariable(variable_name)

        
    def getName(self):
        return "NoName"
    def build(self, endpoint):
        pass
    def getEndPointLinks(self):
        pass
    def getNumNodes(self):
        pass
    def getRouterNameForId(self,rtr_id):
        return "%srtr.%d"%(self._prefix,rtr_id)
    def findRouterById(self,rtr_id):
        return sst.findComponentByName(self.getRouterNameForId(rtr_id))
    def _instanceRouter(self,radix,rtr_id):
        return self.router.instanceRouter(self.getRouterNameForId(rtr_id), radix, rtr_id)

class NetworkInterface(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)

    # returns subcomp, port_name
    def build(self,comp,slot,slot_num,link,job_id,job_size,logical_nid,use_nid_remap=False):
        return None



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

        self.network_interface = PlatformDefinition.getPlatformDefinedClassInstance("network_interface")

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
        self._declareParams("params",["link_bw","flit_size","xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size",
                                      "xbar_arb","network_inspectors","oql_track_port","oql_track_remote","num_vns","vn_remap","vn_remap_shm"])

        self._declareParams("params",["qos_settings"],"portcontrol:arbitration:")
        self._declareParams("params",["output_arb"],"portcontrol:")

        self._setCallbackOnWrite("qos_settings",self._qos_callback)
        self._subscribeToPlatformParamSet("router")


    def _qos_callback(self,variable_name,value):
        self._lockVariable(variable_name)
        if not self.output_arb: self.output_arb = "merlin.arb.output.qos.multi"
        
    def instanceRouter(self, name, radix, rtr_id):
        rtr = sst.Component(name, "merlin.hr_router")
        self._applyStatisticsSettings(rtr)
        rtr.addParams(self._getGroupParams("params"))
        rtr.addParam("num_ports",radix)
        rtr.addParam("id",rtr_id)
        return rtr
    
    def getTopologySlotName(self):
        return "topology"


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


class System(TemplateBase):
    
    # Functions used to allocated endpoints to jobs
    _allocate_functions = {}
    
    @classmethod
    def addAllocationFunction(cls,name,func):
        cls._allocate_functions[name] = func

        
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareClassVariables(["topology","allocation_block_size","_available_nodes","_num_nodes","_endpoints"])
        self._setCallbackOnWrite("topology",self._topology_config_callback)
        self._setCallbackOnWrite("allocation_block_size",self._block_size_config_callback)
        
        self._subscribeToPlatformParamSet("system")
        self.topology = PlatformDefinition.getPlatformDefinedClassInstance("topology")
        #self.allocation_block_size = 1

    def setTopology(self, topo, allocation_block_size = 1):
        self.allocation_block_size = allocation_block_size
        self.topology = topo

    # Build the system
    def build(self):
        system_ep = SystemEndpoint(self)
        self.topology.build(system_ep)

    def _topology_config_callback(self, variable_name, value):
        if not value: return
        self._lockVariable(variable_name)

        # Set the variables
        self._num_nodes = value.getNumNodes()
        self._endpoints = [None for i in range(self._num_nodes)]

        if self._areVariablesLocked(["topology","allocation_block_size"]):
            self._available_nodes = [i for i in range(self._num_nodes // self.allocation_block_size)]

    def _block_size_config_callback(self, variable_name, value):
        self._lockVariable(variable_name)
        if self._areVariablesLocked(["topology","allocation_block_size"]):
            self._available_nodes = [i for i in range(self._num_nodes // value)]
        


    def allocateNodes(self,job,method,*args):
        # If block size wasn't set yet, set it to 1
        if not self.allocation_block_size: self.allocation_block_size = 1
        # First, get the allocated list and the new available list
        size = job.getSize()
        # Calulate the number of allocation units needed.  This just
        # does a celing divide (note, only works in python, in C the
        # result is truncated instead of floored)
        num_units = -(-size // self.allocation_block_size)
        
        allocated, available = self._allocate_functions[method](self._available_nodes,num_units,*args)
        self._available_nodes = available

        #  Record what type of endpoint is at each node, taking into
        #  account the allocation block size
        nid_map = [0 for i in range(size)]

        # Loop through and create a flattened map
        for i in range(num_units):
            for j in range(self.allocation_block_size):
                nid_map[i*self.allocation_block_size+j] = allocated[i] * self.allocation_block_size + j

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
