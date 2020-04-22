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

# Class to do most of the work for both mesh and torus
class _topoMeshBase(Topology):
    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency","host_link_latency","bundleEndpoints"])
        self._defineRequiredParams(["shape", "width", "local_ports"])
        #self._defineOptionalParams([])

    def _processShape(self):
        if self.shape and self.width:
            # Get the size in dimension
            dim_size = [int(x) for x in self.shape.split('x')]

            # Get the number of links in each dimension and check to
            # make sure the number of dimensions matches the shape
            dim_width = [int(x) for x in self.width.split('x')]

            if len(dim_size) != len(dim_width):
                print("topoHyperX:  Incompatible number of dimensions set for shape and width.")
                exit(1)

            return dim_size, dim_width

        else:
            return None, None

    def _getTopologyName():
        pass
        
    def _includeWrapLinks():
        pass

    def getNumNodes(self):
        dim_size, dim_shape = self._processShape()

        if not dim_size or not self.local_ports:
            print("%s: calling getNumNodes before shape, width and local_ports was set."%self.getName())
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
            radix = radix + (dim_width[x] * 2)
            
        
        links = dict()
        def getLink(leftName, rightName, num):
            name = "link.%s:%s:%d"%(leftName, rightName, num)
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]

        
        for i in range(num_routers):
            # set up 'mydims'
            mydims = idToLoc(i)
            mylocstr = self._formatShape(mydims)

            rtr = self._router_template.instanceRouter("rtr.%s"%mylocstr,radix,i)
            
            topology = rtr.setSubComponent(self._router_template.getTopologySlotName(),self._getTopologyName())
            self._applyStatisticsSettings(topology)
            topology.addParams(self._params)

            port = 0
            for dim in range(num_dims):
                theirdims = mydims[:]

                # Positive direction
                if mydims[dim]+1 < dim_size[dim] or self._includeWrapLinks():
                    theirdims[dim] = (mydims[dim] +1 ) % dim_size[dim]
                    theirlocstr = self._formatShape(theirdims)
                    for num in range(dim_width[dim]):
                        rtr.addLink(getLink(mylocstr, theirlocstr, num), "port%d"%port, self.link_latency)
                        port = port+1
                else:
                    port += dim_width[dim]

                # Negative direction
                if mydims[dim] > 0 or self._includeWrapLinks():
                    theirdims[dim] = ((mydims[dim] -1) + dim_size[dim]) % dim_size[dim]
                    theirlocstr = self._formatShape(theirdims)
                    for num in range(dim_width[dim]):
                        rtr.addLink(getLink(theirlocstr, mylocstr, num), "port%d"%port, self.link_latency)
                        port = port+1
                else:
                    port += dim_width[dim]

            for n in range(local_ports):
                nodeID = local_ports * i + n
                (ep, port_name) = endpoint.build(nodeID, {})
                if ep:
                    nicLink = sst.Link("nic.%d:%d"%(i, n))
                    if self.bundleEndpoints:
                       nicLink.setNoCut()
                    nicLink.connect( (ep, port_name, self.host_link_latency), (rtr, "port%d"%port, self.host_link_latency) )
                port = port+1



class topoMesh(_topoMeshBase):

    def __init__(self):
        _topoMeshBase.__init__(self)

    def getName(self):
        return "Mesh"

    def _getTopologyName(self):
        return "merlin.mesh"

    def _includeWrapLinks(self):
        return False


class topoTorus(_topoMeshBase):

    def __init__(self):
        _topoMeshBase.__init__(self)

    def getName(self):
        return "Torus"

    def _getTopologyName(self):
        return "merlin.torus"

    def _includeWrapLinks(self):
        return True


class topoSingle(Topology):

    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency","bundleEndpoints"])
        self._defineRequiredParams(["num_ports"])

    def getName(self):
        return "Single Router"

    def getNumNodes(self):
        return self.num_ports

    def build(self, network_name, endpoint):
        rtr = self._router_template.instanceRouter("router",self.num_ports,0)

        topo = rtr.setSubComponent(self._router_template.getTopologySlotName(),"merlin.singlerouter",0)
        self._applyStatisticsSettings(topo)
        topo.addParams(self._params)
        
        for l in range(self.num_ports):
            (ep, portname) = endpoint.build(l, {})
            if ep:
                link = sst.Link("link:%d"%l)
                if self.bundleEndpoints:
                    link.setNoCut()
                link.connect( (ep, portname, self.link_latency), (rtr, "port%d"%l, self.link_latency) )

