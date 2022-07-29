#!/usr/bin/env python
#
# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
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


class topoFatTree(Topology):

    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency","host_link_latency","bundleEndpoints","_ups","_downs","_routers_per_level","_groups_per_level","_start_ids",
                                     "_total_hosts"])
        self._declareParams("main",["shape","routing_alg","adaptive_threshold"])        
        self._setCallbackOnWrite("shape",self._shape_callback)
        self._subscribeToPlatformParamSet("topology")


    def _shape_callback(self,variable_name,value):
        self._lockVariable(variable_name)
        shape = value
        
        # Process the shape
        self._ups = []
        self._downs = []
        self._routers_per_level = []
        self._groups_per_level = []
        self._start_ids = []

        levels = shape.split(":")

        for l in levels:
            links = l.split(",")

            self._downs.append(int(links[0]))
            if len(links) > 1:
                self._ups.append(int(links[1]))

        self._total_hosts = 1
        for i in self._downs:
            self._total_hosts *= i


        self._routers_per_level = [0] * len(self._downs)
        self._routers_per_level[0] = self._total_hosts // self._downs[0]
        for i in range(1,len(self._downs)):
            self._routers_per_level[i] = self._routers_per_level[i-1] * self._ups[i-1] // self._downs[i]

        self._start_ids = [0] * len(self._downs)
        for i in range(1,len(self._downs)):
            self._start_ids[i] = self._start_ids[i-1] + self._routers_per_level[i-1]

        self._groups_per_level = [1] * len(self._downs);
        if self._ups: # if ups is empty, then this is a single level and the following line will fail
            self._groups_per_level[0] = self._total_hosts // self._downs[0]

        for i in range(1,len(self._downs)-1):
            self._groups_per_level[i] = self._groups_per_level[i-1] // self._downs[i]

        

    def getName(self):
        return "Fat Tree"



    def getNumNodes(self):
        return self._total_hosts


    def getRouterNameForId(self,rtr_id):
        num_levels = len(self._start_ids)

        # Check to make sure the index is in range
        level = num_levels - 1
        if rtr_id >= (self._start_ids[level] + self._routers_per_level[level]) or rtr_id < 0:
            print("ERROR: topoFattree.getRouterNameForId: rtr_id not found: %d"%rtr_id)
            sst.exit()

        # Find the level
        for x in range(num_levels-1,0,-1):
            if rtr_id >= self._start_ids[x]:
                break
            level = level - 1

        # Find the group
        remainder = rtr_id - self._start_ids[level]
        routers_per_group = self._routers_per_level[level] // self._groups_per_level[level]
        group = remainder // routers_per_group
        router = remainder % routers_per_group
        return self.getRouterNameForLocation((level,group,router))
            
    
    def getRouterNameForLocation(self,location):
        return "%srtr_l%s_g%d_r%d"%(self._prefix,location[0],location[1],location[2])
    
    def findRouterByLocation(self,location):
        return sst.findComponentByName(self.getRouterNameForLocation(location));
    
    
    
    def build(self, endpoint):

        if not self.host_link_latency:
            self.host_link_latency = self.link_latency
        
        #Recursive function to build levels
        def fattree_rb(self, level, group, links):
            id = self._start_ids[level] + group * (self._routers_per_level[level]//self._groups_per_level[level])


            host_links = []
            if level == 0:
                # create all the nodes
                for i in range(self._downs[0]):
                    node_id = id * self._downs[0] + i
                    #print("group: %d, id: %d, node_id: %d"%(group, id, node_id))
                    (ep, port_name) = endpoint.build(node_id, {})
                    if ep:
                        hlink = sst.Link("hostlink_%d"%node_id)
                        if self.bundleEndpoints:
                           hlink.setNoCut()
                        ep.addLink(hlink, port_name, self.host_link_latency)
                        host_links.append(hlink)

                # Create the edge router
                rtr_id = id
                rtr = self._instanceRouter(self._ups[0] + self._downs[0], rtr_id)
                
                topology = rtr.setSubComponent(self.router.getTopologySlotName(),"merlin.fattree")
                self._applyStatisticsSettings(topology)
                topology.addParams(self._getGroupParams("main"))
                # Add links
                for l in range(len(host_links)):
                    rtr.addLink(host_links[l],"port%d"%l, self.link_latency)
                for l in range(len(links)):
                    rtr.addLink(links[l],"port%d"%(l+self._downs[0]), self.link_latency)
                return

            rtrs_in_group = self._routers_per_level[level] // self._groups_per_level[level]
            # Create the down links for the routers
            rtr_links = [ [] for index in range(rtrs_in_group) ]
            for i in range(rtrs_in_group):
                for j in range(self._downs[level]):
                    rtr_links[i].append(sst.Link("link_l%d_g%d_r%d_p%d"%(level,group,i,j)));

            # Now create group links to pass to lower level groups from router down links
            group_links = [ [] for index in range(self._downs[level]) ]
            for i in range(self._downs[level]):
                for j in range(rtrs_in_group):
                    group_links[i].append(rtr_links[j][i])

            for i in range(self._downs[level]):
                fattree_rb(self,level-1,group*self._downs[level]+i,group_links[i])

            # Create the routers in this level.
            # Start by adding up links to rtr_links
            for i in range(len(links)):
                rtr_links[i % rtrs_in_group].append(links[i])

            for i in range(rtrs_in_group):
                rtr_id = id + i
                rtr = self._instanceRouter(self._ups[level] + self._downs[level], rtr_id)

                topology = rtr.setSubComponent(self.router.getTopologySlotName(),"merlin.fattree")
                self._applyStatisticsSettings(topology)
                topology.addParams(self._getGroupParams("main"))
                # Add links
                for l in range(len(rtr_links[i])):
                    rtr.addLink(rtr_links[i][l],"port%d"%l, self.link_latency)
        #  End recursive function

        level = len(self._ups)
        if self._ups: # True for all cases except for single level
            #  Create the router links
            rtrs_in_group = self._routers_per_level[level] // self._groups_per_level[level]

            # Create the down links for the routers
            rtr_links = [ [] for index in range(rtrs_in_group) ]
            for i in range(rtrs_in_group):
                for j in range(self._downs[level]):
                    rtr_links[i].append(sst.Link("link_l%d_g0_r%d_p%d"%(level,i,j)));

            # Now create group links to pass to lower level groups from router down links
            group_links = [ [] for index in range(self._downs[level]) ]
            for i in range(self._downs[level]):
                for j in range(rtrs_in_group):
                    group_links[i].append(rtr_links[j][i])


            for i in range(self._downs[len(self._ups)]):
                fattree_rb(self,level-1,i,group_links[i])

            # Create the routers in this level
            radix = self._downs[level]
            for i in range(self._routers_per_level[level]):
                rtr_id = self._start_ids[len(self._ups)] + i
                rtr = self._instanceRouter(radix,rtr_id);

                topology = rtr.setSubComponent(self.router.getTopologySlotName(),"merlin.fattree",0)
                self._applyStatisticsSettings(topology)
                topology.addParams(self._getGroupParams("main"))

                for l in range(len(rtr_links[i])):
                    rtr.addLink(rtr_links[i][l], "port%d"%l, self.link_latency)

        else: # Single level case
            # create all the nodes
            for i in range(self._downs[0]):
                node_id = i
#                print("Instancing node " + str(node_id))
        rtr_id = 0
#        print("Instancing router " + str(rtr_id))


