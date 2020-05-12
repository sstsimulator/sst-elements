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


class topoHyperX(Topology):
        

    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency","host_link_latency","bundleEndpoints","_num_dims","_dim_size","_dim_width"])
        self._defineRequiredParams(["shape", "width", "local_ports"])
        self._defineOptionalParams(["algorithm"])
        self._setCallbackOnWrite("shape",self._shape_callback)
        self._setCallbackOnWrite("width",self._shape_callback)

    def _shape_callback(self,variable_name,value):
        if variable_name == "shape":
            if not self.width:
                return
            shape = value
            width = self.width
        else:
            if not self.shape:
                return
            shape = self.shape
            width = value
            
        # Get the size in dimension
        self._dim_size = [int(x) for x in shape.split('x')]

        # Get the number of links in each dimension and check to
        # make sure the number of dimensions matches the shape
        self._dim_width = [int(x) for x in width.split('x')]

        self._num_dims = len(self._dim_size)

        if len(self._dim_size) != len(self._dim_width):
            print("topoHyperX:  Incompatible number of dimensions set for shape and width."%getName())
            exit(1)
        
    def getName(self):
        return "HyperX"

    def getNumNodes(self):
        if not self._dim_size or not self.local_ports:
            print("topoHyperX: calling getNumNodes before shape, width and local_ports were set.")
            exit(1)

        num_routers = 1;
        for x in self._dim_size:
            num_routers = num_routers * x

        return num_routers * int(self.local_ports)


    def setShape(self,shape,width,local_ports):
        this.shape = shape
        this.width = width
        this.local_ports = local_ports

        
    def _formatShape(self, arr):
        return 'x'.join([str(x) for x in arr])

    def _idToLoc(self,rtr_id):
        foo = list()
        for i in range(self._num_dims-1, 0, -1):
            div = 1
            for j in range(0, i):
                div = div * self._dim_size[j]
            value = (rtr_id // div)
            foo.append(value)
            rtr_id = rtr_id - (value * div)
        foo.append(rtr_id)
        foo.reverse()
        return foo


    def getRouterNameForId(self,rtr_id):
        return self.getRouterNameForLocation(self._idToLoc(rtr_id))
        
    def getRouterNameForLocation(self,location):
        return "%srtr.%s"%(self._prefix,self._formatShape(location))
    
    def findRouterByLocation(self,location):
        return sst.findComponentByName(self.getRouterNameForLocation(location))
        
    
    def build(self, endpoint):
        if self.host_link_latency is None:
            self.host_link_latency = self.link_latency

        # get some local variables from the parameters
        local_ports = int(self.local_ports)
        num_dims = len(self._dim_size)
        
        
        # Calculate number of routers and endpoints
        num_routers = 1
        for x in self._dim_size:
            num_routers = num_routers * x

        num_peers = num_routers * local_ports

        radix = local_ports
        for x in range(num_dims):
            radix += (self._dim_width[x] * (self._dim_size[x]-1))
        
        links = dict()
        def getLink(name1, name2, num):
            # Sort name1 and name2 so order doesn't matter
            if str(name1) < str(name2):
                name = "link.%s:%s:%d"%(name1, name2, num)
            else:
                name = "link.%s:%s:%d"%(name2, name1, num)
            if name not in links:
                links[name] = sst.Link(name)
            #print("Getting link with name: %s"%name)
            return links[name]

        # loop through the routers to hook up links
        for i in range(num_routers):
            # set up 'mydims'
            mydims = self._idToLoc(i)
            mylocstr = self._formatShape(mydims)

            #print("Creating router %s (%d)"%(mylocstr,i))

            rtr = self._instanceRouter(radix,i)

            topology = rtr.setSubComponent(self._router_template.getTopologySlotName(),"merlin.hyperx")
            self._applyStatisticsSettings(topology)
            topology.addParams(self._params)

            port = 0
            # Connect to all routers that only differ in one location index
            for dim in range(num_dims):
                theirdims = mydims[:]

                # We have links to every other router in each dimension
                for router in range(self._dim_size[dim]):
                    if router != mydims[dim]: # no link to ourselves
                        theirdims[dim] = router
                        theirlocstr = self._formatShape(theirdims)
                        # Hook up "width" number of links for this dimension
                        for num in range(self._dim_width[dim]):
                            rtr.addLink(getLink(mylocstr, theirlocstr, num), "port%d"%port, self.link_latency)
                            #print("Wired up port %d"%port)
                            port = port + 1


            for n in range(local_ports):
                nodeID = local_ports * i + n
                (ep, port_name) = endpoint.build(nodeID, {})
                if ep:
                    nicLink = sst.Link("nic.%d:%d"%(i, n))
                    if self.bundleEndpoints:
                       nicLink.setNoCut()
                    nicLink.connect( (ep, port_name, self.host_link_latency), (rtr, "port%d"%port, self.host_link_latency) )
                port = port+1


