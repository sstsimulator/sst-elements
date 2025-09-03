#!/usr/bin/env python
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# of the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
from sst.merlin.base import *

class LinkControl(NetworkInterface):
    def __init__(self):
        NetworkInterface.__init__(self)
        self._declareParams("params",["link_bw","input_buf_size","output_buf_size","vn_remap"])
        self._subscribeToPlatformParamSet("network_interface")

    # returns subcomp, port_name
    def build(self,comp,slot,slot_num,job_id,job_size,logical_nid,use_nid_remap = False, link=None):
        if self._check_first_build():
            set_name = "params_%s"%self._instance_name
            sst.addGlobalParams(set_name, self._getGroupParams("params"))
            sst.addGlobalParam(set_name,"job_id",job_id)
            sst.addGlobalParam(set_name,"job_size",job_size)
            sst.addGlobalParam(set_name,"use_nid_remap",use_nid_remap)


        sub = comp.setSubComponent(slot,"merlin.linkcontrol",slot_num)
        self._applyStatisticsSettings(sub)
        sub.addGlobalParamSet("params_%s"%self._instance_name)
        sub.addParam("logical_nid",logical_nid)

        if link:
            sub.addLink(link, "rtr_port");
            return True
        else:
            return sub,"rtr_port"


class ReorderLinkControl(NetworkInterface):
    def __init__(self):
        NetworkInterface.__init__(self)
        self._declareClassVariables(["network_interface"])
        self._setCallbackOnWrite("network_interface",self._network_interface_callback)

        self.network_interface = PlatformDefinition.getPlatformDefinedClassInstance("reorderlinkcontrol_network_interface")
        if not self.network_interface:
            self.network_interface = LinkControl()
            # This is just a default, can be overwritten
            self._unlockVariable("network_interface")

    def _network_interface_callback(self, variable_name, value):
        if not value: return
        self._lockVariable("network_interface")
        self._setPassthroughTarget(value)

    def setNetworkInterface(self,interface):
        self.network_interface = interface

    def build(self,comp,slot,slot_num,job_id,job_size,nid,use_nid_map = False, link = None):
        sub = comp.setSubComponent(slot,"merlin.reorderlinkcontrol",slot_num)
        #self._applyStatisticsSettings(sub)
        #sub.addParams(self._params)

        return NetworkInterface._instanceNetworkInterfaceBackCompat(
            self.network_interface,sub,"networkIF",0,job_id,job_size,nid,use_nid_map,link)


    # Functions to enable statistics
    def enableAllStatistics(self,stat_params,apply_to_children=False):
        # no stats of our own, simply pass to network interface
        if self.network_interface:
            self.network_interface.enableAllStatistics(stat_params,apply_to_children)

    def enableStatistics(self,stats,stat_params,apply_to_children=False):
        # no stats of our own, simply pass to network interface
        if self.network_interface:
            self.network_interface.enableStatistics(stats,stat_params,apply_to_children)

    def setStatisticLoadLevel(self,level,apply_to_children=False):
        # no stats of our own, simply pass to network interface
        if self.network_interface:
            self.network_interface.setStatisticLoadLevel(level,apply_to_children)


## ==============================================
# Python classes for EndpointNIC plugins

class NICplugin(TemplateBase):
    # Registry of all plugin classes
    _PLUGIN_REGISTRY = {}
    
    # These should be overridden in subclasses
    PluginName = None
    PluginFullName = None
    
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareParams("params",["PluginName", "PluginFullName"])

    def __init_subclass__(cls, **kwargs):
        """Automatically register subclasses when they are defined"""
        super().__init_subclass__(**kwargs)
        # Only register if PluginName is defined (not None)
        if cls.PluginName is not None:
            if cls.PluginFullName is None:
                raise ValueError(f"Plugin class {cls.__name__} must define both PluginName and PluginFullName")
            cls._PLUGIN_REGISTRY[cls.PluginName] = cls
    
    @classmethod
    def get_plugin_class(cls, name):
        """Get a plugin class by name"""
        return cls._PLUGIN_REGISTRY.get(name)
    
    @classmethod
    def list_plugins(cls):
        """Return list of registered plugin names"""
        return list(cls._PLUGIN_REGISTRY.keys())

    def build(self, endpointNIC_sstcomp, endpointID, plugin_index, **kwargs):
        assert(self.PluginFullName is not None), "PluginFullName must be set before building the plugin."
        assert(self.PluginName is not None), "PluginFullName must be set before building the plugin."
        thisPlugin = endpointNIC_sstcomp.setSubComponent(self.PluginName, self.PluginFullName, plugin_index)
        thisPlugin.addParam("EP_id", endpointID)

        return thisPlugin

class SourceRoutingPlugin(NICplugin):
    PluginName = "sourceRoutingPlugin"
    PluginFullName = "merlin.sourceRoutingPlugin"
    
    def __init__(self):
        NICplugin.__init__(self)

    def build(self, endpointNIC_sstcomp, endpointID, plugin_index, **kwargs):
        thisPlugin = super().build(endpointNIC_sstcomp, endpointID, plugin_index, **kwargs)

        topology = kwargs.get("topology", None)
        assert(topology is not None), "topology must be provided to sourceRoutingPlugin."
        endpoint_router_mapping = topology.get_endpoint_router_mapping()
        
        # TODO: this is repeated for every endpoint, consider optimizing to only pass this once...
        # Convert dict to SST-compatible string format: "{key1:value1, key2:value2, ...}"
        mapping_str = "{" + ", ".join(f"{k}:{v}" for k, v in endpoint_router_mapping.items()) + "}"
        thisPlugin.addParams({"endpoint_router_mapping": mapping_str})

        routing_table = kwargs.get("routing_table", None)
        assert(routing_table is not None), "routing_table must be provided to sourceRoutingPlugin"
        rtr_ID = endpoint_router_mapping[endpointID]
        thisPlugin.addParam("routing_entry_string", serialize_routing_entry(routing_table[rtr_ID], rtr_ID))
        thisPlugin.addParam("num_routers", topology.num_routers)

        return thisPlugin # just to be consistent with the parent class build() method


## ==============================================
# Python classes for EndpointNIC

class EndpointNIC(NetworkInterface):
    # this class builds an endpoint NIC component
    # It will interact with two sides: a Job and a NetworkInterface
    # It will not modify but only utilize the build methods defined in pymerlin-endpoint.py and pymerlin-interface.py

    def __init__(self, use_reorderLinkControl=False, topo=None):
        NetworkInterface.__init__(self)
        self._declareClassVariables(['network_interface', 'plugins', 'topology', 'networkIF_fullname'])
        self._setCallbackOnWrite("network_interface",self._network_interface_callback)
        self.topology = topo # back reference to the topology object

        # store plugins with their associated kwargs
        self.plugins = []

        self.network_interface = PlatformDefinition.getPlatformDefinedClassInstance("endpointNIC_network_interface")
        if use_reorderLinkControl:
            self.networkIF_fullname = "merlin.reorderlinkcontrol"
            if not self.network_interface:
                self.network_interface = ReorderLinkControl()
        else:
            self.networkIF_fullname = "merlin.linkcontrol"
            if not self.network_interface:
                self.network_interface = LinkControl()

            self._unlockVariable("network_interface")

    def addPlugin(self, plugin, **kwargs):
        """
        Add a plugin to the EndpointNIC.
        
        Args:
            plugin: a string name of a registered plugin class
            **kwargs: Additional keyword arguments to pass to the plugin
        """
        # Handle both plugin instances and string names
        if isinstance(plugin, str):
            plugin_class = NICplugin.get_plugin_class(plugin)
            if plugin_class is None:
                valid_plugins = ', '.join(NICplugin.list_plugins())
                raise ValueError(f"Invalid plugin name '{plugin}'. Valid plugins are: {valid_plugins}")
            plugin = plugin_class()
        else:
            raise TypeError(f"Plugin must be a string name, got {type(plugin)}")
        
        self.plugins.append((plugin, kwargs))
        # Plugin-specific passing:...
        if isinstance(plugin, SourceRoutingPlugin):
            assert(self.topology is not None), "Topology must be provided to EndpointNIC when using sourceRoutingPlugin."
            kwargs['topology'] = self.topology

    def _network_interface_callback(self, variable_name, value):
        if not value: return
        self._lockVariable("network_interface")
        self._setPassthroughTarget(value)

    def setNetworkInterface(self,interface):
        """
            EndpointNIC only carries Network interface **Card** functionalities, by this method we
            set the real `network_interface` to an instance of linkcontrol or reorderedlinkcontrol
        """
        self.network_interface = interface

    # Functions to enable statistics
    def enableAllStatistics(self,stat_params,apply_to_children=False):
        if self.network_interface:
            self.network_interface.enableAllStatistics(stat_params,apply_to_children)

    def enableStatistics(self,stats,stat_params,apply_to_children=False):
        if self.network_interface:
            self.network_interface.enableStatistics(stats,stat_params,apply_to_children)

    def setStatisticLoadLevel(self,level,apply_to_children=False):
        if self.network_interface:
            self.network_interface.setStatisticLoadLevel(level,apply_to_children)

    def build(self,comp,slot,slot_num,job_id,job_size,nid,use_nid_map = False,link = None):
        
        sub = comp.setSubComponent(slot,"merlin.endpointNIC",slot_num)
        sub.addParam("EP_id", nid)
        sub.addParam("networkIF", self.networkIF_fullname)
        sub.addParam("plugin_names", [plugin.PluginName for plugin, _ in self.plugins])
        
        for plugin_index, (plugin, kwargs) in enumerate(self.plugins):
            plugin.build(sub, nid, plugin_index, **kwargs) # pass 'sub' as the endpointNIC SST subcomponent

        # pass `extraParams=None` to of the underlying linkcontrol or reorderlinkcontrol
        return NetworkInterface._instanceNetworkInterfaceBackCompat(
            self.network_interface,sub,"networkIF",0,job_id,job_size,nid,use_nid_map,link)

# class SRLinkControl(NetworkInterface):
#     def __init__(self):
#         NetworkInterface.__init__(self)
#         self._declareClassVariables(["network_interface", "routing_table_file", "routing_table", "use_reorder"])
#         self._setCallbackOnWrite("network_interface",self._network_interface_callback)
#         self.routing_table_file = None
#         # self._routing_table = {}  # Cache parsed routing tables

#         self.network_interface = PlatformDefinition.getPlatformDefinedClassInstance("reorderlinkcontrol_network_interface")
#         if not self.network_interface:
#             # we are defaulting to LinkControl here, later on it may change
#             self.network_interface = LinkControl()
#             self._unlockVariable("network_interface")

#     def _network_interface_callback(self, variable_name, value):
#         if not value: return
#         self._lockVariable("network_interface")
#         self._setPassthroughTarget(value)

#     def setNetworkInterface(self, interface):
#         print("Warning| Not recommended to directly set the network interface on SRLinkControl (use use_ReorderLinkControl instead)," \
#             " unless a complete custom network interface is being provided.")
#         del self.network_interface
#         self.network_interface = interface

#     def setRoutingTableFile(self, filename):
#         """Set the routing table file to read from"""
#         self.routing_table_file = filename

#     def use_ReorderLinkControl(self, use_reorder):
#         """Enable/disable reordering functionality"""
#         self.use_reorder = use_reorder
#         del self.network_interface

#         if self.use_reorder == True:
#             self.network_interface = ReorderLinkControl()
#         else:
#             self.network_interface = LinkControl()
#         self._unlockVariable("network_interface")

#     def _parseRoutingTable(self, filename) -> None:
#         # the routing table file is expected to be a csv file with the following format:
#         # src_router_id, dest_router_id, path1, path2, ...
#         # path1=(hop1, hop2, ..., weight), path2=(hop1, hop2, ..., weight), ...
#         # hops are int, weights are float. Note that the hops do not include the source router, only intermediate and destination routers.

#         # as the result of this method, self.routing_table will be populated as:
#         # { src_router_id : { dest_router_id : [ (weight, [hop1, hop2, ...]), ... ] } }
        
#         # if filename in self._routing_table:
#         #     return self._routing_table[filename]
        
#         self.routing_table = {}
#         try:
#             with open(filename, 'r') as f:
#                 for line_num, line in enumerate(f, 1):
#                     line = line.strip()
#                     if not line or line.startswith('#'):
#                         continue
                    
#                     # Split by comma and clean whitespace
#                     parts = [part.strip() for part in line.split(',')]
#                     if len(parts) < 3:  # At least src, dest, and one path
#                         print(f"Warning: Invalid routing table entry at line {line_num}: insufficient columns")
#                         continue
                    
#                     try:
#                         src_router = int(parts[0])
#                         dest_router = int(parts[1])
                        
#                         # Initialize source router entry if not exists
#                         if src_router not in self.routing_table:
#                             self.routing_table[src_router] = {}
                        
#                         # Initialize destination list if not exists
#                         if dest_router not in self.routing_table[src_router]:
#                             self.routing_table[src_router][dest_router] = []
                        
#                         total_weight = 0.0
#                         path_entries = []
                        
#                         # Parse each path (from column 2 onwards)
#                         for path_str in parts[2:]:
#                             if not path_str:
#                                 print(f"Warning: Empty path entry at line {line_num}: {path_str}?")
#                                 continue
                                
#                             # Expected format: (hop1,hop2,...,weight)
#                             path_str = path_str.strip()
#                             if not path_str.startswith('(') or not path_str.endswith(')'):
#                                 raise Exception(f"Invalid path format at line {line_num}: {path_str}, expected format (hop1,hop2,...,weight)")
                                
#                             # Remove parentheses and split by comma
#                             path_content = path_str[1:-1]
#                             path_parts = [p.strip() for p in path_content.split(',')]
                            
#                             if len(path_parts) < 2:
#                                 print(f"Warning: Invalid path entry at line {line_num}: {path_str}, expected format (hop1,hop2,...,weight)")
#                                 continue
                            
#                             # Last element is always the weight
#                             try:
#                                 weight = float(path_parts[-1])
#                                 total_weight += weight
#                             except ValueError:
#                                 print(f"Warning: Invalid weight at line {line_num}: {path_parts[-1]}")
#                                 continue
                            
#                             # Path with intermediate hops, including destination
#                             try:
#                                 path = [int(hop) for hop in path_parts[:-1]]
#                             except ValueError:
#                                 print(f"Warning: Invalid path hops at line {line_num}: {path_parts[:-1]}")
#                                 continue
                            
#                             path_entries.append((weight, path))
                        
#                         # Check if weights sum to approximately 1.0 (allow small floating point errors)
#                         if abs(total_weight - 1.0) > 0.001:
#                             print(f"Warning: Weights for src {src_router} to dest {dest_router} sum to {total_weight:.3f}, expected 1.0")
                        
#                         if path_entries:
#                             self.routing_table[src_router][dest_router] = path_entries
#                         else:
#                             raise Exception(f"No valid paths found at line {line_num}: {line}")
                        
#                     except ValueError as e:
#                         print(f"Warning: Error parsing line {line_num}: {e}")
#                         continue
                        
#         except IOError as e:
#             print(f"Error reading routing table file {filename}: {e}")
#             return {}
        
#         # # Cache the parsed data
#         # self._routing_table[filename] = self.routing_table
#         # return self.routing_table

#     def _formatRoutingTableForParams(self, src_router_id) -> str:
#         """Format routing table data for a specific source router as SST parameters"""
#         if src_router_id not in self.routing_table:
#             raise ValueError(f"Source router ID {src_router_id} not found in routing table.")
        
#         src_data = self.routing_table[src_router_id]
        
#         # Create a compact string representation of the routing table for this source
#         # Format: "dest1:weight1,hop1,hop2;weight2,hop3|dest2:weight1,hop1;weight2,hop2,hop3"
#         routing_entries = []
        
#         for dest_router, paths in src_data.items():
#             path_strings = []
#             for weight, path in paths:
#                 assert weight > 0.0, "Weights must be positive"
#                 assert len(path) > 0, "Path must have at least one hop (the destination)"
#                 # Path with hops
#                 path_strings.append(f"{weight},{','.join(map(str, path))}")
            
#             dest_entry = f"{dest_router}:{';'.join(path_strings)}"
#             routing_entries.append(dest_entry)
        
#         return "|".join(routing_entries)

#     def build(self, comp, slot, slot_num, job_id, job_size, logical_nid, use_nid_remap=False, link=None):
#         """Build SRLinkControl subcomponent with routing table data"""
#         sub = comp.setSubComponent(slot, "merlin.srlinkcontrol", slot_num)

#         # find which router this nid maps to, there should be a parameter in comp for that
#         try:
#             src_router_id = int(comp.getParam("router_id"))
#         except Exception as e:
#             raise RuntimeError(f"Error: Could not get parameter 'router_id' from component parameters: {e}")
        
#         assert(self.routing_table_file is not None), "Routing table file must be set before building SRLinkControl."
#         assert(src_router_id >= 0), f"Source router ID not valid: {src_router_id}"

#         # TODO: extract after-build processings. to be called after bulid() and set**() is called
#         # Parse and add routing table data if file is provided
#         self._parseRoutingTable(self.routing_table_file)
#         routing_string = self._formatRoutingTableForParams(src_router_id)
#         sub.addParam("routing_string", routing_string)
        
#         # Add path selection algorithm parameter (can be overridden by user)
#         sub.addParam("path_selection_algorithm", "random_weighted")
        
#         # Setup the underlying network interface
#         return NetworkInterface._instanceNetworkInterfaceBackCompat(
#             self.network_interface, sub, "networkIF", 0, job_id, job_size, logical_nid, use_nid_remap, link)

#     def setPathSelectionAlgorithm(self, algorithm):
#         """Set the path selection algorithm ('random_weighted' or 'weighted_round_robin')"""
#         self.path_selection_algorithm = algorithm

#     # Functions to enable statistics
#     def enableAllStatistics(self, stat_params, apply_to_children=False):
#         # no stats of our own, simply pass to network interface
#         if self.network_interface:
#             self.network_interface.enableAllStatistics(stat_params, apply_to_children)

#     def enableStatistics(self, stats, stat_params, apply_to_children=False):
#         # no stats of our own, simply pass to network interface
#         if self.network_interface:
#             self.network_interface.enableStatistics(stats, stat_params, apply_to_children)

#     def setStatisticLoadLevel(self, level, apply_to_children=False):
#         # no stats of our own, simply pass to network interface
#         if self.network_interface:
#             self.network_interface.setStatisticLoadLevel(level, apply_to_children)
