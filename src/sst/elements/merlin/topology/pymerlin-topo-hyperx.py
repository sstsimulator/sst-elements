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
        print("hyperX")
        self._declareClassVariables(["link_latency","host_link_latency","bundleEndpoints"])
        self._defineRequiredParams(["shape", "width", "local_ports"])
        self._defineOptionalParams(["algorithm"])

    def getName(self):
        return "HyperX"

    def _processShape(self):
        if self.shape and self.width:
            # Get the size in dimension
            dim_size = [int(x) for x in self.shape.split('x')]

            # Get the number of links in each dimension and check to
            # make sure the number of dimensions matches the shape
            dim_width = [int(x) for x in self.width.split('x')]

            if len(dim_size) != len(dim_width):
                print("HyperX:  Incompatible number of dimensions set for shape and width.")
                exit(1)

            return dim_size, dim_width

        else:
            return None, None
            
    

    def getNumNodes(self):
        dim_size, dim_shape = self._processShape()

        if not dim_size or not self.local_ports:
            print("topoHyperX: calling getNumNodes before shape, width and local_ports was set.")
            exit(1)

        num_routers = 1;
        for x in dim_size:
            num_routers = num_routers * x

        return num_routers * int(self.local_ports)


    def setShape(self,shape,width,local_ports):
        this.shape = shape
        this.width = width
        this.local_ports = local_ports

        
    def _formatShape(self, arr):
        return 'x'.join([str(x) for x in arr])


    def build(self, network_name, endpoint):
        if self.host_link_latency is None:
            self.host_link_latency = self.link_latency

        # get some local variables from the parameters
        dim_size, dim_width = self._processShape()
        local_ports = int(self.local_ports)
        num_dims = len(dim_size)
        
        
        def idToLoc(rtr_id):
            foo = list()
            for i in range(num_dims-1, 0, -1):
                div = 1
                for j in range(0, i):
                    div = div * dim_size[j]
                value = (rtr_id // div)
                foo.append(value)
                rtr_id = rtr_id - (value * div)
            foo.append(rtr_id)
            foo.reverse()
            return foo

        # Calculate number of routers and endpoints
        num_routers = 1
        for x in dim_size:
            num_routers = num_routers * x

        num_peers = num_routers * local_ports

        radix = local_ports
        for x in range(num_dims):
            radix += (dim_width[x] * (dim_size[x]-1))
        
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
            mydims = idToLoc(i)
            mylocstr = self._formatShape(mydims)

            #print("Creating router %s (%d)"%(mylocstr,i))

            rtr = self._router_template.instanceRouter("rtr.%s"%mylocstr,radix,i)

            topology = rtr.setSubComponent(self._router_template.getTopologySlotName(),"merlin.hyperx")
            self._applyStatisticsSettings(topology)
            topology.addParams(self._params)

            port = 0
            # Connect to all routers that only differ in one location index
            for dim in range(num_dims):
                theirdims = mydims[:]

                # We have links to every other router in each dimension
                for router in range(dim_size[dim]):
                    if router != mydims[dim]: # no link to ourselves
                        theirdims[dim] = router
                        theirlocstr = self._formatShape(theirdims)
                        # Hook up "width" number of links for this dimension
                        for num in range(dim_width[dim]):
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


