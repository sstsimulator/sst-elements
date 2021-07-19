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
import re
from collections import deque

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
class LockedWriteError(Exception):
    pass

class _member_info(object):
    def __init__(self,name):
        self.fullname = name
        self.value = None
        self.dictionaries = list()
        self.locked = False
        self.call_back = None

class _member_formatted_info(object):
    def __init__(self):
        self.dictionaries = list()
        self.call_back = None



class _AttributeManager(object):
    def __init__(self):
        object.__setattr__(self,"_in_dict",set(["_in_dict","_vars","_format_vars","_name","_passthrough_target"]))
        object.__setattr__(self,"_vars",dict())
        object.__setattr__(self,"_format_vars",dict())
        object.__setattr__(self,"_name",None)
        object.__setattr__(self,"_passthrough_target",None)

    def _addDirectAttribute(self,name,value=None):
        self._in_dict.add(name)
        object.__setattr__(self,name,value)

    def _addVariable(self,var,dictionary=None,prefix=None):
        if not var in self._vars:
            if self._name:
                self._vars[var] = _member_info(self._name + "." + var)
            else:
                self._vars[var] = _member_info(var)
            #raise AttributeError("%r: %s was already declared as a variable or parameter"%(self.__class__.__name__,var))

        myvar = self._vars[var]
        if dictionary is not None:
            myvar.dictionaries.append(( dictionary, prefix) )

    def _addFormattedVariable(self,var,dictionary=None,prefix=None,callback=None):
        if "%d" in var:
            # Switch out the %d with \d+ (the regex version)
            var = var.replace("%d","\d+")

        if not var in self._format_vars:
            self._format_vars[var] = _member_formatted_info()

        myvar = self._format_vars[var]

        if dictionary is not None:
            myvar.dictionaries.append(( dictionary, prefix ) )

        if callback is not None:
            myvar.call_back = callback


    def __getErrorReportClass(self):
        if "_parent" in self.__dict__:
            return self._parent.__class__.__name__
        else:
            return self.__class__.__name__

    def __getErrorReportName(self,var_name):
        if self._name:
            return self._name + "." + var_name
        else:
            return var_name


    # Function that will be called when a class variable is accessed
    # to write
    def __setattr__(self,key,value):

        if key in self._in_dict:
            return object.__setattr__(self,key,value)

        var = None
        callback = None
        if key in self._vars:
            var = self._vars[key]
            if var.locked:
                raise LockedWriteError("attribute %s of class %r has been marked read only"%(key,self.__class__.__name__))

        else:

            # Check to see if there is a formatted var that matches
            for match in self._format_vars:
                if re.match(match,key):
                    obj = self._format_vars[match]

                    # Create a "real" variable for this and copy over
                    # the dictionaries list
                    if key in self._vars:
                        # This shouldn't happen.  After it's called
                        # once it should use the normal path
                        print("ERROR: logic error in __settattr__ for class _AttributeManager")
                        sst.exit()

                    self._vars[key] = _member_info(self._name + "." + key)
                    var = self._vars[key]
                    var.dictionaries = copy.copy(obj.dictionaries)
                    callback = obj.call_back
                    break

        # Process the variable
        if not var:
            # Variable not found, see if there is a passthrough
            # If there's a passthrough target, send this write to it
            if self._passthrough_target:
                return self._passthrough_target.__setattr__(key,value)
            else:
                raise KeyError("%r has no attribute %r"%(self.__getErrorReportClass(),self.__getErrorReportName(key)))

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

        # If there's a callback, call it.  It can either be a
        # callback for a formatted variable (callback will not be
        # None, or it could be a regular callback
        if callback:
            callback(var.fullname,value)
        elif var.call_back:
            var.call_back(var.fullname,value)


    # Function will be called when a variable not in __dict__ is read
    def __getattr__(self,key):
        if key in self._vars:
            return self._vars[key].value
        else:
            if self._passthrough_target:
                self._passthrough_target.__getattr__(key,value)
            else:
                raise KeyError("%r has no attribute %r"%(self.__getErrorReportClass(),self.__getErrorReportName(key)))

    def clone(self):
        return copy.deepcopy(self)


class _SubAttributeManager(_AttributeManager):
    def __init__(self,parent,name=None):
        _AttributeManager.__init__(self)
        self._addDirectAttribute("_parent",parent)
        self._name = name


class TemplateBase(_AttributeManager):
    _instance_number = 0


    def __init__(self):
        _AttributeManager.__init__(self)
        self._addDirectAttribute("_groups",dict())
        self._addDirectAttribute("_enabled_stats",list())
        self._addDirectAttribute("_stat_load_level",None)

        #self._addDirectAttribute("_instance_name","%s_%d"%(type(self).__name__,TemplateBase._instance_number))
        self._declareClassVariables(["_instance_name", "_first_build"])
        self._instance_name = "%s_%d"%(type(self).__name__,TemplateBase._instance_number)
        self._lockVariable("_instance_name")
        TemplateBase._instance_number = TemplateBase._instance_number + 1

        self._first_build = True
        self._lockVariable("_first_build")


    def _check_first_build(self):
        ret = self._first_build
        if self._first_build:
            self._unlockVariable("_first_build")
            self._first_build = False
            self._lockVariable("_first_build")
        return ret


    def _setPassthroughTarget(self,target):
        self._passthrough_target = target

    def addParams(self,p):
        for x in p:
            #if x in self._req_params or x in self._opt_params:
            #    self._params[x] = p[x]
            #else:
            #    raise KeyError("%r is not a defined parameter for %r"%(x,self.__class__.__name__))
            self.addParam(x,p[x])

    def addParam(self, key, value):

        # Provide a convenience feature of recursing into other
        # Attribute Managers.  See if the first item is another
        # TemplateBase
        if "." in key:
            tokens = key.split(".",1)
            try:
                if tokens[0] in self._vars:
                    return self._vars[tokens[0]].value.addParam(tokens[1],value)
            except AttributeError as e:
                # This means there was no addParams call, so not a
                # valid name
                pass

        sub, var_name = self.__parseSubAttributeName(key)

        try:
            return sub.__setattr__(var_name,value)
        except LockedWriteError:
            raise LockedWriteError("parameter %s of class %r has been marked read only"%(key,self.__class__.__name__))
        except KeyError:
            pass


    def getParam(self,key):
        sub, var = self.__parseSubAttribute(key);
        return sub.__getattr__(var)


    # Function to lock a variable (make it read-only)
    def _lockVariable(self,variable):
        (_, var) = self.__parseSubAttribute(variable)
        var.locked = True

    # Function to unlock a variable (make it writable)
    def _unlockVariable(self,variable):
        (_, var) = self.__parseSubAttribute(variable)
        var.locked = False

    def _isVariableLocked(self,variable):
        return self._vars[variable].locked

    def _areVariablesLocked(self,variables):
        ret = True
        for var in variables:
            (_, v) = self.__parseSubAttribute(var)
            ret = ret and v.locked
        return ret

    # Set a callback to be called on write to a variable
    def _setCallbackOnWrite(self,variable,func):
        (_, var) = self.__parseSubAttribute(variable)
        var.call_back = func



    # Declare variables used by the class, but not passed as
    # parameters to any Components/SubCompnoents.  Variable will be
    # initialized to None.
    def _declareClassVariables(self, vlist):
        for var in vlist:
            self._addVariable(var)



    def __createSubItems(self,param):
        curr = self

        fullname = ""
        remaining = param
        while remaining:
            if not "." in remaining:
                return curr, remaining

            (sub_name, remaining) = remaining.split(".",1)
            fullname = fullname + sub_name
            if not sub_name in curr._in_dict:
                # Not created yet
                curr._addDirectAttribute(sub_name,_SubAttributeManager(self,fullname))

            curr = curr.__dict__[sub_name]

            fullname = fullname + "."


    # Declare parameters (variables) of the class.  The group will
    # collect params together for easy passing as parameters to
    # Components/SubComponents
    def _declareParams(self,group,plist,prefix=None):
        if not group in self._groups:
            self._groups[group] = dict()

        group_dict = self._groups[group]

        for v in plist:
            sub, var = self.__createSubItems(v)
            sub._addVariable(var,group_dict,prefix)


    def _declareParamsWithUserPrefix(self,group,user_prefix,plist,prefix=None):
        # Mostly we'll defer to _declareParams
        new_plist = [user_prefix + "." + sub for sub in plist]
        self._declareParams(group,new_plist,prefix)


    def _declareFormattedParams(self,group,plist,prefix=None,callback=None):
        if not group in self._groups:
            self._groups[group] = dict()

        group_dict = self._groups[group]

        for v in plist:
            sub, var = self.__createSubItems(v)
            sub._addFormattedVariable(var,group_dict,prefix,callback)

    def _declareFormattedParamsWithUserPrefix(self,group,user_prefix,plist,prefix=None,callback=None):
        # Mostly we'll defer to _declareParams
        new_plist = [user_prefix + "." + sub for sub in plist]
        self._declareFormattedParams(group,new_plist,prefix,callback)



    # Get the dictionary of parameters for a group
    def _getGroupParams(self,group):
        return self._groups[group]


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
                    self.addParam(key,value)
                except LockedWriteError as err:
                    print("WARNING: attribute %s cannot be set in %r as it is marked read-only"%(key,self.__class__.__name__))
                except AttributeError as err:
                    print("WARNING: attribute %s not found in class %r"%(key,self.__class__.__name__))
                except KeyError:
                    # not a valid variable for this class, just ignore
                    pass


    def _subscribeToPlatformParamSetAndPrefix(self,set_name,user_prefix):
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
                    self.addParam(user_prefix + "." + key,value)
                except LockedWriteError as err:
                    print("WARNING: attribute %s cannot be set in %r as it is marked read-only"%(key,self.__class__.__name__))
                except AttributeError as err:
                    print("WARNING: attribute %s not found in class %r"%(key,self.__class__.__name__))
                except KeyError:
                    # not a valid variable for this class, just ignore
                    pass

    def __parseSubAttribute(self,attribute):
        if "." not in attribute:
            try:
                return (self, self._vars[attribute])
            except KeyError:
                raise AttributeError("%r has no param %r"%(self.__class__.__name__,attribute))

        tokens = attribute.split(".")
        var = tokens.pop(-1)
        sub = self
        for level in tokens:
            if level in sub.__dict__:
                sub = sub.__dict__[level]
            else:
                raise AttributeError("%r has no param %r (%r not found)"%(self.__class__.__name__,attribute,level))

        if var not in sub._vars:
            raise AttributeError("%r has no param %r"%(self.__class__.__name__,attribute))

        return (sub, sub._vars[var])

    def __parseSubAttributeName(self,attribute):
        if "." not in attribute:
            return (self, attribute)

        tokens = attribute.split(".")
        var = tokens.pop(-1)
        sub = self
        for level in tokens:
            if level in sub.__dict__:
                sub = sub.__dict__[level]
            else:
                raise AttributeError("%r has no param %r (%r not found)"%(self.__class__.__name__,attribute,level))

        return (sub, var)

"""
    @classmethod
    def instanceTemplate(cls,base, name):
        def findClass(base,name,_seen):
            subs = base.__subclasses__()
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
"""

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
    def getDefaultNetworkInterface(self):
        pass

class hr_router(RouterTemplate):
    _instance_num = 0
    _default_linkcontrol = "sst.merlin.interface.LinkControl"

    def __init__(self):
        RouterTemplate.__init__(self)

        self._declareParams("params",["link_bw","flit_size","xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size",
                                      "xbar_arb","network_inspectors","oql_track_port","oql_track_remote","num_vns","vn_remap","vn_remap_shm"])

        self._declareParams("params",["qos_settings"],"portcontrol.arbitration.")
        self._declareParams("params",["output_arb", "enable_congestion_management", "cm_outstanding_threshold", "cm_incast_threshold"],"portcontrol.")

        self._setCallbackOnWrite("qos_settings",self._qos_callback)

        self._subscribeToPlatformParamSet("router")


    def getDefaultNetworkInterface(self):
        module_name, class_name = hr_router._default_linkcontrol.rsplit(".", 1)
        return getattr(import_module(module_name), class_name)()

    def _qos_callback(self,variable_name,value):
        self._lockVariable(variable_name)
        if not self.output_arb: self.output_arb = "merlin.arb.output.qos.multi"

    def instanceRouter(self, name, radix, rtr_id):
        if self._check_first_build():
            sst.addGlobalParams("%s_params"%self._instance_name, self._getGroupParams("params"))

        rtr = sst.Component(name, "merlin.hr_router")
        self._applyStatisticsSettings(rtr)
        rtr.addGlobalParamSet("%s_params"%self._instance_name)
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


class EmptyJob(Job):
    def __init__(self,job_id,size):
        Job.__init__(self,job_id,size)

    def getName(self):
        return "Empty Job"

    def build(self, nID, extraKeys):
        nic = sst.Component("empty_node.%d"%nID, "merlin.simple_patterns.empty")
        id = self._nid_map[nID]

        #  Add the linkcontrol
        networkif, port_name = self.network_interface.build(nic,"networkIF",0,self.job_id,self.size,id)
        return (networkif, port_name)


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
        # For any unallocated nodes, use EmptyJob
        if len(self._available_nodes) > 0:
            remainder = EmptyJob(-1,len(self._available_nodes))
            remainder.network_interface = self.topology.router.getDefaultNetworkInterface()
            remainder.network_interface.link_bw = "1 GB/s"
            self.allocateNodes(remainder,"linear");
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
        nid_map = dict()

        # Loop through and create a flattened map
        for i in range(num_units):
            for j in range(self.allocation_block_size):
                lid = i*self.allocation_block_size+j
                pid = allocated[i] * self.allocation_block_size + j
                nid_map[pid] = lid
                self._endpoints[pid] = job

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

def _allocate_system(a_list, b_list, method):
    #a_list precedes b_list
    #b_list must not exceed system
    #method in allocate_functions, _allocate_interval and _allocate_indexed not allowed
    available_nodes = System._available_nodes
    nid_list_queue = deque()
    for num_nodes in a_list:
        while num_nodes > len(available_nodes):
            nid_list = nid_list_queue.popleft()
            for i in nid_list:
                available_nodes.append(i)
        nid_list, available_nodes = _allocate_functions[method](available_nodes, num_nodes)
        nid_list_queue.append(nid_list)
    for num_nodes in b_list:
        while num_nodes > len(available_nodes):
            nid_list = nid_list_queue.popleft()
            for i in nid_list:
                available_nodes.append(i)
        nid_list, available_nodes = _allocate_functions[method](available_nodes, num_nodes)
        b_nid_lists.append(nid_list)
    for nid_list in nid_list_deque:
        for i in nid_list:
            available_nodes.append(i)
    return b_nid_lists, available_nodes

System.addAllocationFunction("random", _allocate_random)
System.addAllocationFunction("linear", _allocate_linear)
System.addAllocationFunction("random-linear",_allocate_random_linear)
System.addAllocationFunction("interval",_allocate_interval)
System.addAllocationFunction("indexed",_allocate_indexed)
