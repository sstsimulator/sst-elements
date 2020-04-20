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
from sst.merlin.base import *


class topoFatTree(Topology):
    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency","host_link_latency","bundleEndpoints","_ups","_downs","_routers_per_level","_groups_per_level","_start_ids"])
        self._defineRequiredParams(["shape"])
        self._defineOptionalParams(["routing_alg","adaptive_threshold"])        


    def getName(self):
        return "Fat Tree"



    def getNumNodes(self):
        levels = self.shape.split(":")
        total_hosts = 1
        
        for l in levels:
            links = l.split(",")
            total_hosts = total_hosts * int(links[0])

        return total_hosts

    
    def build(self, network_name, endpoint):

        if not self.host_link_latency:
            self.host_link_latency = self.link_latency
        
        # Process the shape
        ups = []
        downs = []
        routers_per_level = []
        groups_per_level = []
        start_ids = []

        #Recursive function to build levels
        def fattree_rb(self, level, group, links):
            id = start_ids[level] + group * (routers_per_level[level]//groups_per_level[level])


            host_links = []
            if level == 0:
                # create all the nodes
                for i in range(downs[0]):
                    node_id = id * downs[0] + i
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
                rtr = self._router_template.instanceRouter("rtr_l0_g%d_r0"%(group), ups[0] + downs[0], rtr_id)
                
                topology = rtr.setSubComponent(self._router_template.getTopologySlotName(),"merlin.fattree")
                topology.addParams(self._params)
                # Add links
                for l in range(len(host_links)):
                    rtr.addLink(host_links[l],"port%d"%l, self.link_latency)
                for l in range(len(links)):
                    rtr.addLink(links[l],"port%d"%(l+downs[0]), self.link_latency)
                return

            rtrs_in_group = routers_per_level[level] // groups_per_level[level]
            # Create the down links for the routers
            rtr_links = [ [] for index in range(rtrs_in_group) ]
            for i in range(rtrs_in_group):
                for j in range(downs[level]):
                    rtr_links[i].append(sst.Link("link_l%d_g%d_r%d_p%d"%(level,group,i,j)));

            # Now create group links to pass to lower level groups from router down links
            group_links = [ [] for index in range(downs[level]) ]
            for i in range(downs[level]):
                for j in range(rtrs_in_group):
                    group_links[i].append(rtr_links[j][i])

            for i in range(downs[level]):
                fattree_rb(self,level-1,group*downs[level]+i,group_links[i])

            # Create the routers in this level.
            # Start by adding up links to rtr_links
            for i in range(len(links)):
                rtr_links[i % rtrs_in_group].append(links[i])

            for i in range(rtrs_in_group):
                rtr_id = id + i
                rtr = self._router_template.instanceRouter("rtr_l%d_g%d_r%d"%(level,group,i), ups[level] + downs[level], rtr_id)

                topology = rtr.setSubComponent(self.router_template.getTopologySlotName(),"merlin.fattree")
                topology.addParams(self._params)
                # Add links
                for l in range(len(rtr_links[i])):
                    rtr.addLink(rtr_links[i][l],"port%d"%l, self.link_latency)
        #  End recursive function
        

        levels = self.shape.split(":")

        for l in levels:
            links = l.split(",")

            downs.append(int(links[0]))
            if len(links) > 1:
                ups.append(int(links[1]))

        total_hosts = 1
        for i in downs:
            total_hosts *= i


        routers_per_level = [0] * len(downs)
        routers_per_level[0] = total_hosts // downs[0]
        for i in range(1,len(downs)):
            routers_per_level[i] = routers_per_level[i-1] * ups[i-1] // downs[i]

        start_ids = [0] * len(downs)
        for i in range(1,len(downs)):
            start_ids[i] = start_ids[i-1] + routers_per_level[i-1]

        groups_per_level = [1] * len(downs);
        if ups: # if ups is empty, then this is a single level and the following line will fail
            groups_per_level[0] = total_hosts // downs[0]

        for i in range(1,len(downs)-1):
            groups_per_level[i] = groups_per_level[i-1] // downs[i]


        level = len(ups)
        if ups: # True for all cases except for single level
            #  Create the router links
            rtrs_in_group = routers_per_level[level] // groups_per_level[level]

            # Create the down links for the routers
            rtr_links = [ [] for index in range(rtrs_in_group) ]
            for i in range(rtrs_in_group):
                for j in range(downs[level]):
                    rtr_links[i].append(sst.Link("link_l%d_g0_r%d_p%d"%(level,i,j)));

            # Now create group links to pass to lower level groups from router down links
            group_links = [ [] for index in range(downs[level]) ]
            for i in range(downs[level]):
                for j in range(rtrs_in_group):
                    group_links[i].append(rtr_links[j][i])


            for i in range(downs[len(ups)]):
                fattree_rb(self,level-1,i,group_links[i])

            # Create the routers in this level
            radix = downs[level]
            for i in range(routers_per_level[level]):
                rtr_id = start_ids[len(ups)] + i
                rtr = self._router_template.instanceRouter("rtr_l%d_g0_r%d"%(len(ups),i),radix,rtr_id);

                topology = rtr.setSubComponent(self._router_template.getTopologySlotName(),"merlin.fattree",0)
                topology.addParams(self._params)

                for l in range(len(rtr_links[i])):
                    rtr.addLink(rtr_links[i][l], "port%d"%l, self.link_latency)

        else: # Single level case
            # create all the nodes
            for i in range(downs[0]):
                node_id = i
#                print("Instancing node " + str(node_id))
        rtr_id = 0
#        print("Instancing router " + str(rtr_id))


