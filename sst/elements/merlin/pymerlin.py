#!/usr/bin/env python
#
# Copyright 2009-2014 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2014, Sandia Corporation
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys
import sst


class Params(dict):
    def __missing__(self, key):
        print "Please enter %s: "%key
        val = raw_input()
        self[key] = val
        return val
    def subset(self, keys):
        return dict((k, self[k]) for k in keys)

_params = Params()
debug = 0


class Topo:
    def __init__(self):
        def epFunc(epID):
            return None
        self._getEndPoint = epFunc
    def getName(self):
        return "NoName"
    def prepParams(self):
        pass
    def setEndPoint(self, endPoint):
        def epFunc(epID):
            return endPoint
        self._getEndPoint = epFunc
    def setEndPointFunc(self, epFunc):
        self._getEndPoint = epFunc
    def build(self):
        pass
        

class topoSimple(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.rtrKeys = ["topology", "debug", "num_ports", "num_vcs", "link_bw", "xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size"]
    def getName(self):
        return "Simple"
    def prepParams(self):
        _params["topology"] = "merlin.singlerouter"
        _params["debug"] = debug
        _params["num_ports"] = int(_params["router_radix"])
        _params["num_peers"] = int(_params["router_radix"])
        _params["num_vcs"] = 1

    def build(self, endPoint):
        rtr = sst.Component("router", "merlin.hr_router")
        _params["topology"] = "merlin.singlerouter"
        _params["debug"] = debug
        rtr.addParams(_params.subset(self.rtrKeys))
        rtr.addParam("id", 0)

        for l in xrange(_params["num_ports"]):
            link = sst.Link("link:%d"%l)
            rtr.addLink(link, "port%d"%l, _params["link_lat"])
            endPoint.build(l, link, [])
        


class topoTorus(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.rtrKeys = ["topology", "debug", "num_ports", "num_vcs", "link_bw", "xbar_bw", "torus:shape", "torus:width", "torus:local_ports","input_latency","output_latency","input_buf_size","output_buf_size"]
    def getName(self):
        return "Torus"
    def prepParams(self):
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
        _params["num_vcs"] = 2
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
            rtr.addParams(_params.subset(self.rtrKeys))
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
                nicLink = sst.Link("nic.%d:%d"%(i, n))
                rtr.addLink(nicLink, "port%d"%port, _params["link_lat"])
                port = port+1
                nodeID = int(_params["torus:local_ports"]) * i + n
                self._getEndPoint(nodeID).build(nodeID, nicLink, [])





class topoFatTree(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.rtrKeys = ["topology", "debug", "num_ports", "num_vcs", "link_bw", "xbar_bw", "fattree:loading","input_latency","output_latency","input_buf_size","output_buf_size"]
        self.nicKeys = ["fattree:addr", "fattree:IP", "fattree:loading", "fattree:radix"]
    def getName(self):
        return "Fat Tree"
    def prepParams(self):
        _params["debug"] = debug
        _params["topology"] = "merlin.fattree"
        _params["router_radix"] = int(_params["router_radix"])
        _params["num_ports"] = _params["router_radix"]
        _params["fattree:radix"] = _params["router_radix"]
        _params["fattree:hosts_per_edge_rtr"] = int(_params["fattree:hosts_per_edge_rtr"])
        _params["fattree:loading"] = _params["fattree:hosts_per_edge_rtr"]
        _params["num_vcs"] = 2
        _params["num_peers"] = _params["router_radix"] * (_params["router_radix"]/2) * _params["fattree:hosts_per_edge_rtr"]

    def build(self):
        def addrToNum(addr):
            return (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0]
        def formatAddr(addr):
            return ".".join([str(x) for x in addr])

        myip = [10, _params["router_radix"], 1, 1]
        router_num = 0

        links = dict()
        def getLink(name):
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]

        # <!-- CORE ROUTERS -->
        num_core = (_params["router_radix"]/2) * (_params["router_radix"]/2)
        for i in xrange(num_core):
            myip[2] = 1 + i/(_params["router_radix"]/2)
            myip[3] = 1 + i%(_params["router_radix"]/2)

            rtr = sst.Component("core:%s"%formatAddr(myip), "merlin.hr_router")
            rtr.addParams(_params.subset(self.rtrKeys))
            rtr.addParams({
                "id": router_num,
                "fattree:addr": addrToNum(myip),
                "fattree:level": 3})
            router_num = router_num +1

            for l in xrange(_params["router_radix"]):
                rtr.addLink(getLink("link:pod%d_core%d"%(l, i)), "port%d"%l, _params["link_lat"])


        # PODS
        for pod in xrange(_params["router_radix"]):
            myip[1] = pod
            #  <!-- AGGREGATION ROUTERS -->
            for r in xrange(_params["router_radix"]/2):
                router = _params["router_radix"]/2 + r
                myip[2] = router
                myip[3] = 1
                rtr = sst.Component("agg:%s"%formatAddr(myip), "merlin.hr_router")
                rtr.addParams(_params.subset(self.rtrKeys))
                rtr.addParams({
                    "id": router_num,
                    "fattree:addr": addrToNum(myip),
                    "fattree:level": 2})
                router_num = router_num +1

                for l in xrange(_params["router_radix"]/2):
                    rtr.addLink(getLink("link:pod%d_aggr%d_edge%d"%(pod, r, l)), "port%d"%l, _params["link_lat"])

                for l in xrange(_params["router_radix"]/2):
                    core = (_params["router_radix"]/2) * r + l
                    rtr.addLink(getLink("link:pod%d_core%d"%(pod, core)), "port%d"%(l+_params["router_radix"]/2), _params["link_lat"])

            #  <!-- EDGE ROUTERS -->\n")
            for r in xrange(_params["router_radix"]/2):
                myip[2] = r
                myip[3] = 1
                rtr = sst.Component("edge:%s"%formatAddr(myip), "merlin.hr_router")
                rtr.addParams(_params.subset(self.rtrKeys))
                rtr.addParams({
                    "id": router_num,
                    "fattree:addr": addrToNum(myip),
                    "fattree:level": 1})
                router_num = router_num +1

                for l in xrange(_params["fattree:hosts_per_edge_rtr"]):
                    node_id = pod * (_params["router_radix"]/2) * _params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + r * _params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + l
                    rtr.addLink(getLink("link:pod%d_edge%d_node%d"%(pod, r, node_id)), "port%d"%l, _params["link_lat"])

                for l in xrange(_params["router_radix"]/2):
                    rtr.addLink(getLink("link:pod%d_aggr%d_edge%d"%(pod, l, r)), "port%d"%(l+_params["router_radix"]/2), _params["link_lat"])

                for n in range(_params["fattree:hosts_per_edge_rtr"]):
                    node_id = pod * (_params["router_radix"]/2) * _params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + r * _params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + n

                    myip[3] = n+2

                    _params["fattree:addr"] = addrToNum(myip);
                    _params["fattree:IP"] = formatAddr(myip);
                    self._getEndPoint(node_id).build(node_id, getLink("link:pod%d_edge%d_node%d"%(pod, r, node_id)), self.nicKeys)





class topoDragonFly(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.rtrKeys = ["topology", "debug", "num_ports", "num_vcs", "link_bw", "xbar_bw", "dragonfly:hosts_per_router", "dragonfly:routers_per_group", "dragonfly:intergroup_per_router", "dragonfly:num_groups","input_latency","output_latency","input_buf_size","output_buf_size"]
    def getName(self):
        return "Dragonfly"

    def prepParams(self):
        _params["topology"] = "merlin.dragonfly"
        _params["debug"] = debug
        _params["num_vcs"] = 3
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
            self.rtrKeys.append("dragonfly:algorithm")


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
                rtr.addParams(_params.subset(self.rtrKeys))
                rtr.addParam("id", router_num)

                port = 0
                for p in xrange(_params["dragonfly:hosts_per_router"]):
                    link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                    rtr.addLink(link, "port%d"%port, _params["link_lat"])
                    self._getEndPoint(nic_num).build(nic_num, link, [])
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





############################################################################

class EndPoint:
    def getName(self):
        print "Not implemented"
        sys.exit(1)
    def prepParams(self):
        pass
    def build(self, nID, link, extraKeys):
        pass
        



class TestEndPoint:
    def __init__(self):
        self.nicKeys = ["topology", "num_peers", "num_vcs", "link_bw"]

    def getName(self):
        return "Test End Point"

    def prepParams(self):
        pass

    def build(self, nID, link, extraKeys):
        nic = sst.Component("testNic.%d"%nID, "merlin.test_nic")
        nic.addParams(_params.subset(self.nicKeys))
        nic.addParams(_params.subset(extraKeys))
        nic.addParam("id", nID)
        nic.addLink(link, "rtr", _params["link_lat"])
        #print "Created Endpoint with id: %d, and params: %s %s\n"%(nID, _params.subset(self.nicKeys), _params.subset(extraKeys))


class TrafficGenEndPoint:
    def __init__(self):
        self.optionalKeys = ["delay_between_packets"]
        for genType in ["PacketDest", "PacketSize", "PacketDelay"]:
            for tag in ["pattern", "RangeMin", "RangeMax", "HotSpot:target", "HotSpot:targetProbability", "Normal:Mean", "Normal:Sigma", "Binomial:Mean", "Binomial:Sigma"]:
                self.optionalKeys.append("%s:%s"%(genType, tag))
        self.nicKeys = ["topology", "num_peers", "num_vcs", "link_bw", "packets_to_send", "packet_size", "message_rate", "PacketDest:pattern", "PacketDest:RangeMin", "PacketDest:RangeMax"]
    def getName(self):
        return "Pattern-based traffic generator"
    def prepParams(self):
        _params["PacketDest:RangeMin"] = 0
        _params["PacketDest:RangeMax"] = int(_params["num_peers"])

        if _params["PacketDest:pattern"] == "NearestNeighbor":
            self.nicKeys.append("PacketDest:NearestNeighbor:3DSize")
            if not "PacketDest:NearestNeighbor:3DSize" in _params:
                _params["PacketDest:NearestNeighbor:3DSize"] = "%s %s %s"%(_params["PacketDest:3D shape X"], _params["PacketDest:3D shape Y"], _params["PacketDest:3D shape Z"])
        elif _params["PacketDest:pattern"] == "HotSpot":
            self.nicKeys.append("PacketDest:HotSpot:target")
            self.nicKeys.append("PacketDest:HotSpot:targetProbability")
        elif _params["PacketDest:pattern"] == "Normal":
            self.nicKeys.append("PacketDest:Normal:Mean")
            self.nicKeys.append("PacketDest:Normal:Sigma")
        elif _params["PacketDest:pattern"] == "Binomial":
            self.nicKeys.append("PacketDest:Binomial:Mean")
            self.nicKeys.append("PacketDest:Binomial:Sigma")

    def build(self, nID, link, extraKeys):
        nic = sst.Component("TrafficGen.%d"%nID, "merlin.trafficgen")
        nic.addParams(_params.subset(self.nicKeys))
        nic.addParams(_params.subset(extraKeys))
        for k in self.optionalKeys:
            if k in _params:
                nic.addParam(k, _params[k])
        nic.addParam("id", nID)
        nic.addLink(link, "rtr", _params["link_lat"])


############################################################################


if __name__ == "__main__":
    topos = dict([(1,topoTorus()), (2,topoFatTree()), (3,topoDragonFly()), (4,topoSimple())])
    endpoints = dict([(1,TestEndPoint()), (2, TrafficGenEndPoint())])


    print "Merlin SDL Generator\n"


    print "Please select network topology:"
    for (x,y) in topos.iteritems():
        print "[ %d ]  %s" % (x, y.getName() )
    topo = int(raw_input())
    if topo not in topos:
        print "Bad answer.  try again."
        sys.exit(1)

    topo = topos[topo]



    print "Please select endpoint:"
    for (x,y) in endpoints.iteritems():
        print "[ %d ]  %s" % (x, y.getName() )
    ep = int(raw_input())
    if ep not in endpoints:
        print "Bad answer. try again."
        sys.exit(1)

    endPoint = endpoints[ep];

    topo.prepParams()
    endPoint.prepParams()
    topo.setEndPoint(endPoint)
    topo.build()

