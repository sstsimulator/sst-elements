#!/usr/bin/env python
#
# Copyright 2009-2016 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2016, Sandia Corporation
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys
import sst
from sst.merlin.core import *

class topoDragonFly2(Topology):

    def __init__(self):
        Topology.__init__(self)
        self.topoKeys = ["flit_size", "link_bw", "xbar_bw", "dragonfly:hosts_per_router", "dragonfly:routers_per_group", "dragonfly:num_groups","dragonfly:intergroup_links","input_latency","output_latency","input_buf_size","output_buf_size"]

        self.hosts_per_router = 0
        self.routers_per_group = 0
        self.intergroup_links = 0
        self.num_groups = 0
     
        self.global_link_map = None
        self.global_routes = "absolute"


    def getTopologyName(self):
        return "Dragonfly2"


    def setShape(self,hosts_per_router, router_per_group, intergroup_links, num_groups):
        self.params["dragonfly:hosts_per_router"] = hosts_per_router
        self.params["dragonfly:routers_per_group"] = routers_per_group
        self.params["dragonfly:intergroup_links"] = intergroup_links
        self.params["dragonfly:num_groups"] = num_groups

        
    def setHostsPerRouter(self,hosts_per_router):
        self.params["dragonfly:hosts_per_router"] = hosts_per_router

        
    def setRoutersPerGroup(self,routers_per_group):
        self.params["dragonfly:routers_per_group"] = routers_per_group

        
    def setInterGroupLinks(self,intergroup_links):
        self.params["dragonfly:intergroup_links"] = intergroup_links


    def setGroups(num_groups):
        self.params["dragonfly:num_groups"] = num_groups


    def setGlobalLinkMap(self, glm):
        self.global_link_map = glm

        
    def setRoutingModeAbsolute(self):
        self.global_routes = "absolute"

        
    def setRoutingModeRelative(self):
        self.global_routes = "relative"


    def build(self, network_name, endpoint):

        self.params.verifyParamsExist(self.topoKeys)
        endpoint.verifyParams()
        
        self.params["topology"] = "merlin.dragonfly2"

        self.hosts_per_router = int(self.params["dragonfly:hosts_per_router"])
        self.routers_per_group = int(self.params["dragonfly:routers_per_group"])
        self.intergroup_links = int(self.params["dragonfly:intergroup_links"])
        self.num_groups = int(self.params["dragonfly:num_groups"])

        #self.params["debug"] = debug
        self.params["num_peers"] = self.hosts_per_router * self.routers_per_group * self.num_groups
        self.params["dragonfly:global_route_mode"] = self.global_routes


        self.total_intergroup_links = (self.num_groups - 1) * self.intergroup_links
        self.params["dragonfly:intergroup_per_router"] = (self.total_intergroup_links + self.routers_per_group - 1 ) / self.routers_per_group
        self.empty_ports = self.params["dragonfly:intergroup_per_router"] * self.routers_per_group - self.total_intergroup_links
        
        #self.params["router_radix"] = self.routers_per_group - 1 + self.hosts_per_router + self.params["dragonfly:intergroup_per_router"]
        self.params["num_ports"] = self.routers_per_group - 1 + self.hosts_per_router + self.params["dragonfly:intergroup_per_router"]

        if self.num_groups > 2:
            #foo = self.params["dragonfly:algorithm"]
            self.topoKeys.append("dragonfly:algorithm")

        links = dict()

        #####################
        def getLink(name):
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]
        #####################

        rpg = int(self.params["dragonfly:routers_per_group"])
        ng = self.num_groups - 1 # don't count my group
        igpr = int(self.params["dragonfly:intergroup_per_router"])
            
        if self.global_link_map is None:
            # Need to define global link map

            self.global_link_map = [-1 for i in range(igpr * rpg)]
            
            # Links will be mapped in linear order, but we will
            # potentially skip one port per router, depending on the
            # parameters.  The variable self.empty_ports tells us how
            # many routers will have one global port empty.
            count = 0
            start_skip = rpg - self.empty_ports
            for i in xrange(0,rpg):
                # Determine if we skip last port for this router
                end = igpr
                if i >= start_skip:
                    end = end - 1
                for j in xrange(0,end):
                    self.global_link_map[i*igpr+j] = count;
                    count = count + 1

        # End set global link map with default
            

        # g is group number
        # r is router number with group
        # p is port number relative to start of global ports

        def getGlobalLink(g, r, p):
            # Look into global link map to get the dest group and link
            # number to that group
            raw_dest = self.global_link_map[r * igpr + p];
            if raw_dest == -1:
                return None
            
            # Turn raw_dest into dest_grp and link_num
            link_num = raw_dest / ng;
            dest_grp = raw_dest - link_num * ng

            if ( self.global_routes == "absolute" ):
                # Compute dest group ignoring my own group id, for a
                # dest_grp >= g, we need to add 1 to get the right group
                if dest_grp >= g:
                    dest_grp = dest_grp + 1
            elif ( self.global_routes == "relative"):
                # For relative, add current group to dest_grp + 1 and
                # do modulo of num_groups to get actual group
                dest_grp = (dest_grp + g + 1) % (ng+1)
            #else:
                # should never happen
                
            src = min(dest_grp, g)
            dest = max(dest_grp, g)

            #getLink("link:g%dg%dr%d"%(g, src, dst)), "port%d"%port, self.params["link_lat"])
            return getLink("%s:global_link:g%dg%dr%d"%(network_name,src,dest,link_num))

        #########################    

        
        router_num = 0
        nic_num = 0
        # GROUPS
        for g in xrange(self.num_groups):

            # GROUP ROUTERS
            for r in xrange(self.routers_per_group):
                rtr = sst.Component("%s:rtr:G%dR%d"%(network_name, g, r), "merlin.hr_router")
                #rtr.addParams(self.params.subset(self.topoKeys, self.topoOptKeys))
                rtr.addParams(self.params)
                rtr.addParam("id", router_num)
                if router_num == 0:
                    # Need to send in the global_port_map
                    #map_str = str(self.global_link_map).strip('[]')
                    #rtr.addParam("dragonfly:global_link_map",map_str)
                    rtr.addParam("dragonfly:global_link_map",self.global_link_map)
                    
                port = 0
                for p in xrange(self.hosts_per_router):
                    ep = endpoint.build(nic_num, {"num_peers":self.params["num_peers"]})
                    if ep:
                        link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                        link.connect(ep, (rtr, "port%d"%port, self.params["link_lat"]) )
                    nic_num = nic_num + 1
                    port = port + 1

                for p in xrange(self.routers_per_group):
                    if p != r:
                        src = min(p,r)
                        dst = max(p,r)
                        rtr.addLink(getLink("link:g%dr%dr%d"%(g, src, dst)), "port%d"%port, self.params["link_lat"])
                        port = port + 1
                
                for p in xrange(igpr):
                    link = getGlobalLink(g,r,p)
                    if link is not None:
                        rtr.addLink(link,"port%d"%port, self.params["link_lat"])
                    port = port +1

                router_num = router_num +1


class topoHyperX(Topology):
    def __init__(self):
        Topology.__init__(self)
        self.topoKeys = ["topology", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw", "hyperx:shape", "hyperx:width", "hyperx:local_ports","input_latency","output_latency","input_buf_size","output_buf_size"]
        self.topoOptKeys = ["xbar_arb"]
        self.shape = None
        self.width = None
        self.hosts_per_router = None
        self.link_latency = None

        self._dims = None
        
    def getName(self):
        return "HyperX"

    
    def getNetworkSize(self):
        if self.shape is None or self.width is None or self.hosts_per_router is None:
            print "ERROR: Trying to get network size before fully defining network shape parameters (shape, width, hosts_per_router)"
            sys.exit(1)
        if len(self.shape) != len(self.width):
            print "ERROR: shape and width arrays are not of the same length"
            sys.exit(1)
        peers = 1
        for x in self.shape:
            peers = peers * x
        peers = peers * self.hosts_per_router
        return peers


    def formatShape(self, arr):
        return 'x'.join([str(x) for x in arr])

    
    # def prepParams(self):
    #     radix = 0
    #     if not "hyperx:shape" in _params:
    #         for x in xrange(self.nd):
    #             print "Dim %d size:"%x
    #             ds = int(raw_input())
    #             self.dims.append(ds);
    #         _params["hyperx:shape"] = self.formatShape(self.dims)
    #     else:
    #         self.dims = [int(x) for x in _params["hyperx:shape"].split('x')]
    #     if not "hyperx:width" in _params:
    #         for x in xrange(self.nd):
    #             print "Dim %d width (# of links in this dimension):" % x
    #             dw = int(raw_input())
    #             self.dimwidths.append(dw)
    #         _params["hyperx:width"] = self.formatShape(self.dimwidths)
    #     else:
    #         self.dimwidths = [int(x) for x in _params["hyperx:width"].split('x')]

    #     local_ports = int(_params["hyperx:local_ports"])
    #     radix = local_ports
    #     for x in xrange(self.nd):
    #        radix += (self.dimwidths[x] * (self.dims[x]-1))

    #     for x in self.dims:
    #         peers = peers * x
    #     peers = peers * local_ports


    def build(self, network_name, endpoint):

        # Calculate the number of endpoints.  This will also check to
        # make sure all the shape parameters were set
        self.params["num_peers"] = self.getNetworkSize()

        # Get the number of dimensions
        self._dims = len(self.shape)

        # Calculate router radix and set the parameter
        radix = self.hosts_per_router
        for x in xrange(self._dims):
            radix += (self.width[x] * (self.shape[x]-1))        
        self.params["num_ports"] = radix

        self.params["topology"] = "merlin.hyperx"
        #self.params["debug"] = debug
        self.params["hyperx:local_ports"] = self.hosts_per_router
        self.params["hyperx:shape"] = self.formatShape(self.shape)
        self.params["hyperx:width"] = self.formatShape(self.width)
        self.params["hyperx:local_ports"] = self.hosts_per_router
        
        num_routers = self.params["num_peers"] / self.hosts_per_router
        
        
        def idToLoc(rtr_id):
            foo = list()
            for i in xrange(self._dims-1, 0, -1):
                div = 1
                for j in range(0, i):
                    div = div * self.shape[j]
                value = (rtr_id / div)
                foo.append(value)
                rtr_id = rtr_id - (value * div)
            foo.append(rtr_id)
            foo.reverse()
            return foo

        links = dict()        
        def getLink(name1, name2, num):
            # Sort name1 and name2 so order doesn't matter
            if str(name1) < str(name2):
                name = "link.%s:%s:%d"%(name1, name2, num)
            else:
                name = "link.%s:%s:%d"%(name2, name1, num)
            if name not in links:
                links[name] = sst.Link(name)
            #print "Getting link with name: %s"%name
            return links[name]

        # loop through the routers to hook up links
        for i in xrange(num_routers):
            # set up 'mydims'
            mydims = idToLoc(i)
            mylocstr = self.formatShape(mydims)

            #print "Creating router %s (%d)"%(mylocstr,i)
            
            rtr = sst.Component("rtr.%s"%mylocstr, "merlin.hr_router")
            #rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
            rtr.addParams(self.params)
            rtr.addParam("id", i)

            port = 0
            # Connect to all routers that only differ in one location index
            for dim in xrange(self._dims):
                theirdims = mydims[:]

                # We have links to every other router in each dimension
                for router in xrange(self.shape[dim]):
                    if router != mydims[dim]: # no link to ourselves
                        theirdims[dim] = router
                        theirlocstr = self.formatShape(theirdims)
                        # Hook up "width" number of links for this dimension
                        for num in xrange(self.width[dim]):
                            rtr.addLink(getLink(mylocstr, theirlocstr, num), "port%d"%port, self.link_latency)
                            #print "Wired up port %d"%port
                            port = port + 1
                    

            for n in xrange(self.hosts_per_router):
                nodeID = int(self.hosts_per_router) * i + n
                ep = endpoint.build(nodeID, {"num_peers":self.params["num_peers"]})
                if ep:
                    nicLink = sst.Link("nic.%d:%d"%(i, n))
                    #if self.bundleEndpoints:
                       #nicLink.setNoCut()
                    nicLink.connect(ep, (rtr, "port%d"%port, self.link_latency))
                port = port+1



                
"""    
class topoSimple(Topology):
    def __init__(self, name):
        Topology.__init__(self, name)
        self.topoKeys.extend(["topology", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size"])
        self.topoOptKeys.extend(["xbar_arb"])
    def getName(self):
        return "Simple"
    def prepParams(self):
#        if "xbar_arb" not in _params:
#            _params["xbar_arb"] = "merlin.xbar_arb_lru"
        _params["topology"] = "merlin.singlerouter"
        _params["debug"] = debug
        _params["num_ports"] = int(_params["router_radix"])
        _params["num_peers"] = int(_params["router_radix"])
        _params["num_vns"] = 1

    def build(self):
        rtr = sst.Component("router", "merlin.hr_router")
        _params["topology"] = "merlin.singlerouter"
        _params["debug"] = debug
#        rtr.addParams(_params.subset(self.rtrKeys))
        rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
        rtr.addParam("id", 0)

        for l in xrange(_params["num_ports"]):
            ep = self._getEndPoint(l).build(l, {})
            if ep:
                link = sst.Link("link:%d"%l)
                link.connect(ep, (rtr, "port%d"%l, _params["link_lat"]) )
            

class topoTorus(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.topoKeys.extend(["topology", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw", "torus:shape", "torus:width", "torus:local_ports","input_latency","output_latency","input_buf_size","output_buf_size"])
        self.topoOptKeys.extend(["xbar_arb"])
    def getName(self):
        return "Torus"
    def prepParams(self):
#        if "xbar_arb" not in _params:
#            _params["xbar_arb"] = "merlin.xbar_arb_lru"
        self.nd = int(_params["num_dims"])
        peers = 1
        radix = 0
        self.dims = []
        self.dimwidths = []
        if not "torus:shape" in _params:
            for x in xrange(self.nd):
                print "Dim %d size:"%x
                ds = int(raw_input())
                self.dims.append(ds);
            _params["torus:shape"] = self.formatShape(self.dims)
        else:
            self.dims = [int(x) for x in _params["torus:shape"].split('x')]
        if not "torus:width" in _params:
            for x in xrange(self.nd):
                print "Dim %d width (# of links in this dimension):" % x
                dw = int(raw_input())
                self.dimwidths.append(dw)
            _params["torus:width"] = self.formatShape(self.dimwidths)
        else:
            self.dimwidths = [int(x) for x in _params["torus:width"].split('x')]

        local_ports = int(_params["torus:local_ports"])
        radix = local_ports + 2 * sum(self.dimwidths)

        for x in self.dims:
            peers = peers * x
        peers = peers * local_ports

        _params["num_peers"] = peers
        _params["num_dims"] = self.nd
        _params["topology"] = _params["topology"] = "merlin.torus"
        _params["debug"] = debug
        _params["num_ports"] = _params["router_radix"] = radix
        _params["num_vns"] = 2
        _params["torus:local_ports"] = local_ports

    def formatShape(self, arr):
        return 'x'.join([str(x) for x in arr])

    def build(self):
        def idToLoc(rtr_id):
            foo = list()
            for i in xrange(self.nd-1, 0, -1):
                div = 1
                for j in range(0, i):
                    div = div * self.dims[j]
                value = (rtr_id / div)
                foo.append(value)
                rtr_id = rtr_id - (value * div)
            foo.append(rtr_id)
            foo.reverse()
            return foo


        num_routers = _params["num_peers"] / _params["torus:local_ports"]
        links = dict()
        def getLink(leftName, rightName, num):
            name = "link.%s:%s:%d"%(leftName, rightName, num)
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]

        for i in xrange(num_routers):
            # set up 'mydims'
            mydims = idToLoc(i)
            mylocstr = self.formatShape(mydims)

            rtr = sst.Component("rtr.%s"%mylocstr, "merlin.hr_router")
            rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
            rtr.addParam("id", i)

            port = 0
            for dim in xrange(self.nd):
                theirdims = mydims[:]

                # Positive direction
                theirdims[dim] = (mydims[dim] +1 ) % self.dims[dim]
                theirlocstr = self.formatShape(theirdims)
                for num in xrange(self.dimwidths[dim]):
                    rtr.addLink(getLink(mylocstr, theirlocstr, num), "port%d"%port, _params["link_lat"])
                    port = port+1

                # Negative direction
                theirdims[dim] = ((mydims[dim] -1) +self.dims[dim]) % self.dims[dim]
                theirlocstr = self.formatShape(theirdims)
                for num in xrange(self.dimwidths[dim]):
                    rtr.addLink(getLink(theirlocstr, mylocstr, num), "port%d"%port, _params["link_lat"])
                    port = port+1

            for n in xrange(_params["torus:local_ports"]):
                nodeID = int(_params["torus:local_ports"]) * i + n
                ep = self._getEndPoint(nodeID).build(nodeID, {})
                if ep:
                    nicLink = sst.Link("nic.%d:%d"%(i, n))
                    nicLink.connect(ep, (rtr, "port%d"%port, _params["link_lat"]))
                port = port+1



class topoMesh(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.topoKeys = ["topology", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw", "mesh:shape", "mesh:width", "mesh:local_ports","input_latency","output_latency","input_buf_size","output_buf_size"]
        self.topoOptKeys = ["xbar_arb"]
    def getName(self):
        return "Mesh"
    def prepParams(self):
#        if "xbar_arb" not in _params:
#            _params["xbar_arb"] = "merlin.xbar_arb_lru"
        self.nd = int(_params["num_dims"])
        peers = 1
        radix = 0
        self.dims = []
        self.dimwidths = []
        if not "mesh:shape" in _params:
            for x in xrange(self.nd):
                print "Dim %d size:"%x
                ds = int(raw_input())
                self.dims.append(ds);
            _params["mesh:shape"] = self.formatShape(self.dims)
        else:
            self.dims = [int(x) for x in _params["mesh:shape"].split('x')]
        if not "mesh:width" in _params:
            for x in xrange(self.nd):
                print "Dim %d width (# of links in this dimension):" % x
                dw = int(raw_input())
                self.dimwidths.append(dw)
            _params["mesh:width"] = self.formatShape(self.dimwidths)
        else:
            self.dimwidths = [int(x) for x in _params["mesh:width"].split('x')]

        local_ports = int(_params["mesh:local_ports"])
        radix = local_ports + 2 * sum(self.dimwidths)

        for x in self.dims:
            peers = peers * x
        peers = peers * local_ports

        _params["num_peers"] = peers
        _params["num_dims"] = self.nd
        _params["topology"] = _params["topology"] = "merlin.mesh"
        _params["debug"] = debug
        _params["num_ports"] = _params["router_radix"] = radix
        _params["num_vns"] = 2
        _params["mesh:local_ports"] = local_ports

    def formatShape(self, arr):
        return 'x'.join([str(x) for x in arr])

    def build(self):
        def idToLoc(rtr_id):
            foo = list()
            for i in xrange(self.nd-1, 0, -1):
                div = 1
                for j in range(0, i):
                    div = div * self.dims[j]
                value = (rtr_id / div)
                foo.append(value)
                rtr_id = rtr_id - (value * div)
            foo.append(rtr_id)
            foo.reverse()
            return foo


        num_routers = _params["num_peers"] / _params["mesh:local_ports"]
        links = dict()
        def getLink(leftName, rightName, num):
            name = "link.%s:%s:%d"%(leftName, rightName, num)
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]

        for i in xrange(num_routers):
            # set up 'mydims'
            mydims = idToLoc(i)
            mylocstr = self.formatShape(mydims)

            rtr = sst.Component("rtr.%s"%mylocstr, "merlin.hr_router")
            rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
            rtr.addParam("id", i)

            port = 0
            for dim in xrange(self.nd):
                theirdims = mydims[:]

                # Positive direction
                if mydims[dim]+1 < self.dims[dim]:
                    theirdims[dim] = (mydims[dim] +1 ) % self.dims[dim]
                    theirlocstr = self.formatShape(theirdims)
                    for num in xrange(self.dimwidths[dim]):
                        rtr.addLink(getLink(mylocstr, theirlocstr, num), "port%d"%port, _params["link_lat"])
                        port = port+1
                else:
                    port += self.dimwidths[dim]

                # Negative direction
                if mydims[dim] > 0:
                    theirdims[dim] = ((mydims[dim] -1) +self.dims[dim]) % self.dims[dim]
                    theirlocstr = self.formatShape(theirdims)
                    for num in xrange(self.dimwidths[dim]):
                        rtr.addLink(getLink(theirlocstr, mylocstr, num), "port%d"%port, _params["link_lat"])
                        port = port+1
                else:
                    port += self.dimwidths[dim]

            for n in xrange(_params["mesh:local_ports"]):
                nodeID = int(_params["mesh:local_ports"]) * i + n
                ep = self._getEndPoint(nodeID).build(nodeID, {})
                if ep:
                    nicLink = sst.Link("nic.%d:%d"%(i, n))
                    nicLink.connect(ep, (rtr, "port%d"%port, _params["link_lat"]))
                port = port+1





class topoFatTree(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.topoKeys = ["topology", "debug", "flit_size", "link_bw", "xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size", "fattree:shape"]
        self.topoOptKeys = ["xbar_arb", "fattree:routing_alg", "fattree:adaptive_threshold"]
        self.nicKeys = ["link_bw"]
        self.ups = []
        self.downs = []
        self.routers_per_level = []
        self.groups_per_level = []
        self.start_ids = []


    def getName(self):
        return "Fat Tree"

    def prepParams(self):
#        if "xbar_arb" in _params:
#            self.rtrKeys.append("xbar_arb")
            #            _params["xbar_arb"] = "merlin.xbar_arb_lru"
#        if "fattree:routing_alg" in _params:
#            self.rtrKeys.append("fattree:routing_alg")
#        if "fattree:adaptive_threshold" in _params:
#            self.rtrKeys.append("fattree:adaptive_threshold")
            
        self.shape = _params["fattree:shape"]
        
        levels = self.shape.split(":")

        for l in levels:
            links = l.split(",")

            self.downs.append(int(links[0]))
            if len(links) > 1:
                self.ups.append(int(links[1]))

        self.total_hosts = 1
        for i in self.downs:
            self.total_hosts *= i

#        print "Total hosts: " + str(self.total_hosts)

        self.routers_per_level = [0] * len(self.downs)
        self.routers_per_level[0] = self.total_hosts / self.downs[0]
        for i in xrange(1,len(self.downs)):
            self.routers_per_level[i] = self.routers_per_level[i-1] * self.ups[i-1] / self.downs[i]

        self.start_ids = [0] * len(self.downs)
        for i in xrange(1,len(self.downs)):
            self.start_ids[i] = self.start_ids[i-1] + self.routers_per_level[i-1]

        self.groups_per_level = [1] * len(self.downs);
        if self.ups: # if ups is empty, then this is a single level and the following line will fail
            self.groups_per_level[0] = self.total_hosts / self.downs[0]

        for i in xrange(1,len(self.downs)-1):
            self.groups_per_level[i] = self.groups_per_level[i-1] / self.downs[i] 

        _params["debug"] = debug
        _params["topology"] = "merlin.fattree"
        _params["num_peers"] = self.total_hosts
        

    def fattree_rb(self, level, group, links):
#        print "routers_per_level: %d, groups_per_level: %d, start_ids: %d"%(self.routers_per_level[level],self.groups_per_level[level],self.start_ids[level])
        id = self.start_ids[level] + group * (self.routers_per_level[level]/self.groups_per_level[level])
    #    print "start id: " + str(id)


        host_links = []
        if level == 0:
            # create all the nodes
            for i in xrange(self.downs[0]):
                node_id = id * self.downs[0] + i
                #print "group: %d, id: %d, node_id: %d"%(group, id, node_id)
                ep = self._getEndPoint(node_id).build(node_id, {})
                if ep:
                    hlink = sst.Link("hostlink_%d"%node_id)
                    ep[0].addLink(hlink, ep[1], ep[2])
                    host_links.append(hlink)
                #print "Instancing node " + str(node_id)
                #self._getEndPoint(node_id).build(node_id, hlink, _params.subset(self.nicKeys, []))
                #self._getEndPoint(node_id).build(node_id, hlink, _params.subset(self.nicKeys, []))

            # Create the edge router
            rtr_id = id
#            print "Instancing router " + str(rtr_id)
            rtr = sst.Component("rtr_l0_g%d_r0"%(group), "merlin.hr_router")
            # Add parameters
            rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
#            rtr.addParams(_params.optional_subset(self.optRtrKeys))
            rtr.addParam("id",rtr_id)
            rtr.addParam("num_ports",self.ups[0] + self.downs[0])
            # Add links
            for l in xrange(len(host_links)):
                rtr.addLink(host_links[l],"port%d"%l, _params["link_lat"])
            for l in xrange(len(links)):
                rtr.addLink(links[l],"port%d"%(l+self.downs[0]), _params["link_lat"])
            return
    
        rtrs_in_group = self.routers_per_level[level] / self.groups_per_level[level]
        # Create the down links for the routers
        rtr_links = [ [] for index in range(rtrs_in_group) ]
        for i in xrange(rtrs_in_group):
            for j in xrange(self.downs[level]):
#                print "Creating link: link_l%d_g%d_r%d_p%d"%(level,group,i,j);
                rtr_links[i].append(sst.Link("link_l%d_g%d_r%d_p%d"%(level,group,i,j)));

        # Now create group links to pass to lower level groups from router down links
        group_links = [ [] for index in range(self.downs[level]) ]
        for i in xrange(self.downs[level]):
            for j in xrange(rtrs_in_group):
                group_links[i].append(rtr_links[j][i])
            
        for i in xrange(self.downs[level]):
            self.fattree_rb(level-1,group*self.downs[level]+i,group_links[i])

        # Create the routers in this level.
        # Start by adding up links to rtr_links
        for i in xrange(len(links)):
            rtr_links[i % rtrs_in_group].append(links[i])
            
        for i in xrange(rtrs_in_group):
            rtr_id = id + i
#            print "Instancing router " + str(rtr_id)
            rtr = sst.Component("rtr_l%d_g%d_r%d"%(level,group,i), "merlin.hr_router")
            # Add parameters
            rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
#            rtr.addParams(_params.optional_subset(self.optRtrKeys))
            rtr.addParam("id",rtr_id)
            rtr.addParam("num_ports",self.ups[level] + self.downs[level])
            # Add links
            for l in xrange(len(rtr_links[i])):
                rtr.addLink(rtr_links[i][l],"port%d"%l, _params["link_lat"])
    
    def build(self):
#        print "build()"
        level = len(self.ups)
        if self.ups: # True for all cases except for single level
            #  Create the router links
            rtrs_in_group = self.routers_per_level[level] / self.groups_per_level[level]

            # Create the down links for the routers
            rtr_links = [ [] for index in range(rtrs_in_group) ]
            for i in xrange(rtrs_in_group):
                for j in xrange(self.downs[level]):
#                    print "Creating link: link_l%d_g0_r%d_p%d"%(level,i,j);
                    rtr_links[i].append(sst.Link("link_l%d_g0_r%d_p%d"%(level,i,j)));

            # Now create group links to pass to lower level groups from router down links
            group_links = [ [] for index in range(self.downs[level]) ]
            for i in xrange(self.downs[level]):
                for j in xrange(rtrs_in_group):
                    group_links[i].append(rtr_links[j][i])


            for i in xrange(self.downs[len(self.ups)]):
                self.fattree_rb(level-1,i,group_links[i])

            # Create the routers in this level
            radix = self.downs[level]
            for i in xrange(self.routers_per_level[level]):
                rtr_id = self.start_ids[len(self.ups)] + i
#                print "Instancing router " + str(rtr_id)
                rtr = sst.Component("rtr_l%d_g0_r%d"%(len(self.ups),i),"merlin.hr_router")
                rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
#                rtr.addParams(_params.optional_subset(self.optRtrKeys))
                rtr.addParam("id", rtr_id)
                rtr.addParam("num_ports",radix)

                for l in xrange(len(rtr_links[i])):
                    rtr.addLink(rtr_links[i][l], "port%d"%l, _params["link_lat"])

        else: # Single level case
            # create all the nodes
            for i in xrange(self.downs[0]):
                node_id = i
#                print "Instancing node " + str(node_id)
        rtr_id = 0
#        print "Instancing router " + str(rtr_id)




class topoDragonFly(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.topoKeys = ["topology", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw", "dragonfly:hosts_per_router", "dragonfly:routers_per_group", "dragonfly:intergroup_per_router", "dragonfly:num_groups","input_latency","output_latency","input_buf_size","output_buf_size"]
        self.topoOptKeys = ["xbar_arb","link_bw:host","link_bw:group","link_bw:global","input_latency:host","input_latency:group","input_latency:global","output_latency:host","output_latency:group","output_latency:global","input_buf_size:host","input_buf_size:group","input_buf_size:global","output_buf_size:host","output_buf_size:group","output_buf_size:global",]
    def getName(self):
        return "Dragonfly"

    def prepParams(self):
#        if "xbar_arb" not in _params:
#            _params["xbar_arb"] = "merlin.xbar_arb_lru"
        _params["topology"] = "merlin.dragonfly"
        _params["debug"] = debug
        _params["num_vns"] = 1
        _params["router_radix"] = int(_params["router_radix"])
        _params["num_ports"] = int(_params["router_radix"])
        _params["dragonfly:hosts_per_router"] = int(_params["dragonfly:hosts_per_router"])
        _params["dragonfly:routers_per_group"] = int(_params["dragonfly:routers_per_group"])
        _params["dragonfly:intergroup_per_router"] = int(_params["dragonfly:intergroup_per_router"])
        _params["dragonfly:num_groups"] = int(_params["dragonfly:num_groups"])
        _params["num_peers"] = _params["dragonfly:hosts_per_router"] * _params["dragonfly:routers_per_group"] * _params["dragonfly:num_groups"]

        if (_params["dragonfly:routers_per_group"]-1 + _params["dragonfly:hosts_per_router"] + _params["dragonfly:intergroup_per_router"]) > _params["router_radix"]:
            print "ERROR: # of ports per router is only %d\n" % _params["router_radix"]
            sys.exit(1)
        if _params["dragonfly:num_groups"] > 2:
            foo = _params["dragonfly:algorithm"]
            self.topoKeys.append("dragonfly:algorithm")


    def build(self):
        links = dict()
        def getLink(name):
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]

        router_num = 0
        nic_num = 0
        # GROUPS
        for g in xrange(_params["dragonfly:num_groups"]):
            tgt_grp = 0

            # GROUP ROUTERS
            for r in xrange(_params["dragonfly:routers_per_group"]):
                rtr = sst.Component("rtr:G%dR%d"%(g, r), "merlin.hr_router")
                rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
                rtr.addParam("id", router_num)

                port = 0
                for p in xrange(_params["dragonfly:hosts_per_router"]):
                    ep = self._getEndPoint(nic_num).build(nic_num, {})
                    if ep:
                        link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                        link.connect(ep, (rtr, "port%d"%port, _params["link_lat"]))
                    nic_num = nic_num + 1
                    port = port + 1

                for p in xrange(_params["dragonfly:routers_per_group"]):
                    if p != r:
                        src = min(p,r)
                        dst = max(p,r)
                        rtr.addLink(getLink("link:g%dr%dr%d"%(g, src, dst)), "port%d"%port, _params["link_lat"])
                        port = port + 1
                
                for p in xrange(_params["dragonfly:intergroup_per_router"]):
                    if (tgt_grp%_params["dragonfly:num_groups"]) == g:
                        tgt_grp = tgt_grp +1

                    src_g = min(g, tgt_grp%_params["dragonfly:num_groups"])
                    dst_g = max(g, tgt_grp%_params["dragonfly:num_groups"])

                    rtr.addLink(getLink("link:g%dg%d:%d"%(src_g, dst_g, tgt_grp/_params["dragonfly:num_groups"])), "port%d"%port, _params["link_lat"])
                    port = port +1
                    tgt_grp = tgt_grp +1

                router_num = router_num +1
"""
