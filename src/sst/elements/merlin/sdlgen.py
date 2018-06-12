#!/usr/bin/env python
#
# Copyright 2009-2018 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2018, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys


class Params(dict):
    def __missing__(self, key):
        print "Please enter %s:"% key
        val = raw_input()
        self[key] = val
        return val
    def emit(self, outfile, key, sdlkey = None):
        if sdlkey:
            outfile.write("    <%s> %s </%s>\n" % (sdlkey, self[key], sdlkey))
        else:
            outfile.write("    <%s> %s </%s>\n" % (key, self[key], key))

params = Params()


class Topo:
    def getName(self):
        return "NoName"
    def formatParams(self, out):
        pass
    def formatSDL(self, out, endPoint):
        pass
        

class topoSimple:
    def getName(self):
        return "Simple"
    def formatParams(self, out):
        params["topology"] = "merlin.singlerouter"
        params["topoNICParams"] = dict()
        params["num_vcs"] = 1
        params["router_radix"] = int(params["router_radix"])
        params["peers"] = params["router_radix"]

        out.write("  <rtr_params>\n")
        out.write("    <debug> 0 </debug>\n")
        params.emit(out, "router_radix", "num_ports")
        params.emit(out, "num_vcs")
        foo = params["link_lat"]  # It makes more sense to ask here
        params.emit(out, "link_bw")
        params.emit(out, "xbar_bw")
        params.emit(out, "topology")
        params.emit(out, "peers", "singlerouter:peers")
        out.write("  </rtr_params>\n")

    def formatSDL(self, out, endPoint):
        out.write("  <component name=router type=merlin.hr_router>\n")
        out.write("    <params include=rtr_params>\n")
        out.write("      <id> 0 </id>\n")
        out.write("    </params>\n")
        for l in xrange(params["peers"]):
            out.write("    <link name=link:%d port=port%d latency=%s />\n"%(l, l, params["link_lat"]))
        out.write("  </component>\n")
        out.write("\n\n")
        for l in xrange(params["peers"]):
            endPoint.formatComp(out, "nic:%d"%l, l, "link:%d"%l, dict())
            out.write("\n")

        


class topoTorus(Topo):
    def getName(self):
        return "Torus"
    def formatParams(self, out):
        params["topology"] = "merlin.torus"
        params["topoNICParams"] = dict()



        nd = int(params["num_dims"])
        params["num_dims"] = nd # converts it to an int
        if "dimsize" not in params or "dimwidth" not in params:
            params["peers"] = 1
            params["router_radix"] = 0
            dims = []
            dimwidths = []
            for x in xrange(nd):
                print "Dim %d size:" % x
                ds = int(raw_input())
                dims.append(ds)
                print "Dim %d width (# of links in this dimension):" % x
                dw = int(raw_input())
                dimwidths.append(dw)
                params["peers"] = params["peers"] * ds
                params["router_radix"] = params["router_radix"] + (2 * dw)
            params["dimsize"] = dims
            params["dimwidth"] = dimwidths
            params["torus:local_ports"] = int(params["torus:local_ports"])
            params["router_radix"] = params["router_radix"] + params["torus:local_ports"]
            params["peers"] = params["peers"] * params["torus:local_ports"]

        params["torus:shape"] = self.formatShape()
        params["torus:width"] = self.formatWidth()

        foo = params["link_lat"]  # It makes more sense to ask here
        params["num_vcs"] = 2
        out.write("  <rtr_params>\n")
        out.write("    <debug> 0 </debug>\n")
        params.emit(out, "router_radix", "num_ports")
        params.emit(out, "num_vcs")
        params.emit(out, "link_bw")
        params.emit(out, "xbar_bw")
        params.emit(out, "topology")
        params.emit(out, "torus:shape")
        params.emit(out, "torus:width")
        params.emit(out, "torus:local_ports")
        out.write("  </rtr_params>\n")


    def formatShape(self):
        shape = params["dimsize"][0]
        for x in xrange(1, params["num_dims"]):
            shape = "%sx%d" % (shape, params["dimsize"][x])
        return shape

    def formatWidth(self):
        shape = params["dimwidth"][0]
        for x in xrange(1, params["num_dims"]):
            shape = "%sx%d" % (shape, params["dimwidth"][x])
        return shape

    def formatSDL(self, out, endPoint):
        def idToLoc(rtr_id):
            foo = list()
            for i in xrange(params["num_dims"]-1, 0, -1):
                div = 1
                for j in range(0, i):
                    div = div * params["dimsize"][j]
                value = (rtr_id / div)
                foo.append(value)
                rtr_id = rtr_id - (value * div)
            foo.append(rtr_id)
            foo.reverse()
            return foo

        def formatLoc(dims):
            return "x".join([str(x) for x in dims])
            

        num_routers = params["peers"] / params["torus:local_ports"]

        for i in xrange(num_routers):
            # set up 'mydims'
            mydims = idToLoc(i)
            mylocstr = formatLoc(mydims)

            out.write("  <component name=rtr.%s type=merlin.hr_router>\n" % mylocstr)
            out.write("    <params include=rtr_params>\n")
            out.write("      <id> %d </id>\n" % i)
            out.write("    </params>\n")

            port = 0
            for dim in xrange(params["num_dims"]):
                theirdims = mydims[:]

                # Positive direction
                theirdims[dim] = (mydims[dim] +1 ) % params["dimsize"][dim]
                theirlocstr = formatLoc(theirdims)
                for num in xrange(params["dimwidth"][dim]):
                    out.write("    <link name=link.%s:%s:%d port=port%d latency=%s />\n" % (mylocstr, theirlocstr, num, port, params["link_lat"]))
                    port = port+1

                # Negative direction
                theirdims[dim] = ((mydims[dim] -1) +params["dimsize"][dim]) % params["dimsize"][dim]
                theirlocstr = formatLoc(theirdims)
                for num in xrange(params["dimwidth"][dim]):
                    out.write("    <link name=link.%s:%s:%d port=port%d latency=%s />\n" % (theirlocstr, mylocstr, num, port, params["link_lat"]))
                    port = port+1

            for n in xrange(params["torus:local_ports"]):
                out.write("    <link name=nic.%d:%d port=port%d latency=%s />\n" % (i, n, port, params["link_lat"]))
                port = port+1
            out.write("  </component>\n\n")


            for n in xrange(params["torus:local_ports"]):
                nodeID = int(params["torus:local_ports"]) * i + n
                linkName = "nic.%d:%d"%(i, n)
                endPoint.formatComp(out, "nic.%s-%d"%(mylocstr, n), nodeID, linkName, dict())





class topoFatTree(Topo):
    def getName(self):
        return "Fat Tree"
    def formatParams(self, out):
        params["topology"] = "merlin.fattree"
        params["router_radix"] = int(params["router_radix"])
        params["fattree:hosts_per_edge_rtr"] = int(params["fattree:hosts_per_edge_rtr"])
        params["fattree:loading"] = params["fattree:hosts_per_edge_rtr"]
        params["topoNICParams"] = dict()
        params["topoNICParams"]["fattree:loading"] = params["fattree:loading"]
        params["topoNICParams"]["fattree:radix"] = params["router_radix"]

        params["peers"] = params["router_radix"] * (params["router_radix"]/2) * params["fattree:hosts_per_edge_rtr"]
        params["num_vcs"] = 2

        foo = params["link_lat"]  # It makes more sense to ask here
        out.write("  <rtr_params>\n")
        out.write("    <debug> 0 </debug>\n")
        params.emit(out, "router_radix", "num_ports")
        params.emit(out, "num_vcs")
        params.emit(out, "link_bw")
        params.emit(out, "xbar_bw")
        params.emit(out, "topology")
        params.emit(out, "fattree:loading")
        out.write("  </rtr_params>\n")


    def formatSDL(self, out, endPoint):
        def addrToNum(addr):
            return (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0]
        def formatAddr(addr):
            return ".".join([str(x) for x in addr])

        myip = [10, params["router_radix"], 1, 1]
        router_num = 0

        out.write("  <!-- CORE ROUTERS -->\n")
        num_core = (params["router_radix"]/2) * (params["router_radix"]/2)
        for i in xrange(num_core):
            myip[2] = 1 + i/(params["router_radix"]/2)
            myip[3] = 1 + i%(params["router_radix"]/2)

            out.write("  <component name=core:%s type=merlin.hr_router>\n" % formatAddr(myip))
            out.write("    <params include=rtr_params>\n")
            out.write("      <id> %d </id>\n" % router_num)
            out.write("      <fattree:addr> %d </fattree:addr>\n" % addrToNum(myip))
            out.write("      <fattree:level> 3 </fattree:level>\n")
            out.write("    </params>\n")
            router_num = router_num +1

            for l in xrange(params["router_radix"]):
                out.write("    <link name=link:pod%d_core%d port=port%d latency=%s />\n"%(l, i, l, params["link_lat"]))
            out.write("  </component>\n")
            out.write("\n")

        for pod in xrange(params["router_radix"]):
            out.write("\n\n")
            out.write("  <!-- POD %d -->\n" % pod)
            myip[1] = pod
            out.write("  <!-- AGGREGATION ROUTERS -->\n")
            for r in xrange(params["router_radix"]/2):
                router = params["router_radix"]/2 + r
                myip[2] = router
                myip[3] = 1

                out.write("  <component name=agg:%s type=merlin.hr_router>\n" % formatAddr(myip))
                out.write("    <params include=rtr_params>\n")
                out.write("      <id> %d </id>\n" % router_num)
                out.write("      <fattree:addr> %d </fattree:addr>\n" % addrToNum(myip))
                out.write("      <fattree:level> 2 </fattree:level>\n")
                out.write("    </params>\n")
                router_num = router_num +1

                for l in xrange(params["router_radix"]/2):
                    out.write("    <link name=link:pod%d_aggr%d_edge%d port=port%d latency=%s />\n"%(pod, r, l, l, params["link_lat"]))
                for l in xrange(params["router_radix"]/2):
                    core = (params["router_radix"]/2) * r + l
                    out.write("    <link name=link:pod%d_core%d port=port%d latency=%s />\n"%(pod, core, l+params["router_radix"]/2, params["link_lat"]))
                out.write("  </component>\n")
                out.write("\n")


            out.write("\n")
            out.write("  <!-- EDGE ROUTERS -->\n")
            for r in xrange(params["router_radix"]/2):
                myip[2] = r
                myip[3] = 1
                out.write("  <component name=edge:%s type=merlin.hr_router>\n" % formatAddr(myip))
                out.write("    <params include=rtr_params>\n")
                out.write("      <id> %d </id>\n" % router_num)
                out.write("      <fattree:addr> %d </fattree:addr>\n" % addrToNum(myip))
                out.write("      <fattree:level> 1 </fattree:level>\n")
                out.write("    </params>\n")
                router_num = router_num +1

                for l in xrange(params["fattree:hosts_per_edge_rtr"]):
                    node_id = pod * (params["router_radix"]/2) * params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + r * params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + l
                    out.write("    <link name=link:pod%d_edge%d_node%d port=port%d latency=%s />\n"%(pod, r, node_id, l, params["link_lat"]))

                for l in xrange(params["router_radix"]/2):
                    out.write("    <link name=link:pod%d_aggr%d_edge%d port=port%d latency=%s />\n"%(pod,l, r, l+params["router_radix"]/2, params["link_lat"]))
                out.write("  </component>\n")
                out.write("\n");

                for n in range(params["fattree:hosts_per_edge_rtr"]):
                    node_id = pod * (params["router_radix"]/2) * params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + r * params["fattree:hosts_per_edge_rtr"]
                    node_id = node_id + n

                    myip[3] = n+2

                    endPoint.formatComp(out, "nic:%s"%formatAddr(myip), node_id, "link:pod%d_edge%d_node%d"%(pod, r, node_id), dict([("fattree:addr",addrToNum(myip))]))








class topoDragonFly(Topo):
    def getName(self):
        return "Dragonfly"

    def formatParams(self, out):
        params["topology"] = "merlin.dragonfly"
        params["topoNICParams"] = dict()


        params["router_radix"] = int(params["router_radix"])
        params["dragonfly:hosts_per_router"] = int(params["dragonfly:hosts_per_router"])
        params["dragonfly:routers_per_group"] = int(params["dragonfly:routers_per_group"])
        params["dragonfly:intergroup_per_router"] = int(params["dragonfly:intergroup_per_router"])
        params["dragonfly:num_groups"] = int(params["dragonfly:num_groups"])
        params["peers"] = params["dragonfly:hosts_per_router"] * params["dragonfly:routers_per_group"] * params["dragonfly:num_groups"]

        if (params["dragonfly:routers_per_group"]-1 + params["dragonfly:hosts_per_router"] + params["dragonfly:intergroup_per_router"]) > params["router_radix"]:
            print "ERROR: # of ports per router is only %d\n" % params["router_radix"]
            sys.exit(1)
        if params["dragonfly:num_groups"] > 2:
            foo = params["dragonfly:algorithm"]

        foo = params["link_lat"]  # It makes more sense to ask here
        params["num_vcs"] = 3
        out.write("  <rtr_params>\n")
        out.write("    <debug> 0 </debug>\n")
        params.emit(out, "router_radix", "num_ports")
        params.emit(out, "link_bw")
        params.emit(out, "xbar_bw")
        params.emit(out, "num_vcs")
        params.emit(out, "topology")
        params.emit(out, "dragonfly:hosts_per_router")
        params.emit(out, "dragonfly:routers_per_group")
        params.emit(out, "dragonfly:intergroup_per_router")
        params.emit(out, "dragonfly:num_groups")
        out.write("  </rtr_params>\n")

    def formatSDL(self, out, endPoint):
        router_num = 0
        nic_num = 0
        for g in xrange(params["dragonfly:num_groups"]):
            out.write("  <!-- GROUP %d -->\n" % g)
            tgt_grp = 0

            for r in xrange(params["dragonfly:routers_per_group"]):
                out.write("  <!-- GROUP %d, ROUTER %d -->\n" % (g, r))

                out.write("  <component name=rtr:G%dR%d type=merlin.hr_router>\n" % (g, r))
                out.write("    <params include=rtr_params>\n")
                out.write("      <id> %d </id>\n" % router_num)
                out.write("    </params>\n")

                port = 0
                for p in xrange(params["dragonfly:hosts_per_router"]):
                    out.write("    <link name=link:g%dr%dh%d port=port%d latency=%s />\n" % (g, r, p, port, params["link_lat"]))
                    port = port + 1

                for p in xrange(params["dragonfly:routers_per_group"]):
                    if p != r:
                        src = min(p,r)
                        dst = max(p,r)
                        out.write("    <link name=link:g%dr%dr%d port=port%d latency=%s />\n" % (g, src, dst, port, params["link_lat"]))
                        port = port + 1
                
                for p in xrange(params["dragonfly:intergroup_per_router"]):
                    if (tgt_grp%params["dragonfly:num_groups"]) == g:
                        tgt_grp = tgt_grp +1

                    src_g = min(g, tgt_grp%params["dragonfly:num_groups"])
                    dst_g = max(g, tgt_grp%params["dragonfly:num_groups"])

                    out.write("    <link name=link:g%dg%d:%d port=port%d latency=%s />\n" % (src_g, dst_g, tgt_grp/params["dragonfly:num_groups"], port, params["link_lat"]))
                    port = port +1
                    tgt_grp = tgt_grp +1

                out.write("  </component>\n")
                out.write("\n")

                for h in xrange(params["dragonfly:hosts_per_router"]):
                    out.write("  <!-- GROUP %d, ROUTER %d, HOST %d -->\n" % (g, r, h))
                    endPoint.formatComp(out, "nic:G%dR%dH%d"%(g,r,h), nic_num, "link:g%dr%dh%d"%(g,r,h), dict())
                    nic_num = nic_num+1
                out.write("\n")
                router_num = router_num +1
            out.write("\n")





############################################################################

class EndPoint:
    def getName(self):
        print "Not implemented"
        sys.exit(1)
    def formatParams(self, out):
        pass
    def formatComp(self, out, name, num, linkName, extraParams):
        pass
        



class TestEndPoint:
    def getName(self):
        return "Test End Point"

    def formatParams(self, out):
        out.write("  <nic_params>\n")
        params.emit(out, "topology")
        for (n,v) in params["topoNICParams"].iteritems():
            out.write("    <%s> %d </%s>\n" % (n, v, n))
        params.emit(out, "peers", "num_peers")
        params.emit(out, "num_vcs")
        params.emit(out, "link_bw")
        out.write("  </nic_params>\n")

    def formatComp(self, out, name, num, linkName, extraParams):
        out.write("  <component name=%s type=merlin.test_nic>\n" % name)
        out.write("    <params include=nic_params>\n")
        out.write("      <id> %d </id>\n"% num)
        for (k,v) in extraParams.iteritems():
            out.write("      <%s> %d </%s>\n"% (k,v,k))
        out.write("    </params>\n")
        out.write("    <link name=%s port=rtr latency=%s />\n" % (linkName, params["link_lat"]))
        out.write("  </component>\n\n")


class TrafficGenEndPoint:
    def getName(self):
        return "Pattern-based traffic generator"
    def formatParams(self, out):
        out.write("  <nic_params>\n")
        out.write("    <topology> %s </topology>\n" % params["topology"])
        for (n,v) in params["topoNICParams"].iteritems():
            out.write("    <%s> %d </%s>\n" % (n, v, n))
        params.emit(out, "peers", "num_peers")
        params.emit(out, "num_vcs")
        params.emit(out, "link_bw")
        params.emit(out, "num_pkts_to_send", "packets_to_send")
        params.emit(out, "packet_size")
        params.emit(out, "message_rate")
        params.emit(out, "PacketDest:pattern")
        out.write("    <PacketDest:RangeMin> 0 </PacketDest:RangeMin>\n")
        out.write("    <PacketDest:RangeMax> %d </PacketDest:RangeMax>\n" % params["peers"])

        if params["PacketDest:pattern"] == "NearestNeighbor":
            out.write("    <PacketDest:NearestNeighbor:3DSize> %s %s %s </PacketDest:NearestNeighbor:3DSize>\n" % (params["PacketDest:3D shape X"], params["PacketDest:3D shape Y"], params["PacketDest:3D shape Z"]))
        elif params["PacketDest:pattern"] == "HotSpot":
            params.emit(out, "PacketDest:HotSpot:target")
            params.emit(out, "PacketDest:HotSpot:targetProbability")
        elif params["PacketDest:pattern"] == "Normal":
            params.emit(out, "PacketDest:Normal:Mean")
            params.emit(out, "PacketDest:Normal:Sigma")
        elif params["PacketDest:pattern"] == "Binomial":
            params.emit(out, "PacketDest:Binomial:Mean")
            params.emit(out, "PacketDest:Binomial:Sigma")

        out.write("  </nic_params>\n")

    def formatComp(self, out, name, num, linkName, extraParams):
        out.write("  <component name=%s type=merlin.trafficgen>\n" % name)
        out.write("    <params include=nic_params>\n")
        out.write("      <id> %d </id>\n"% num)
        for (k,v) in extraParams.iteritems():
            out.write("      <%s> %d </%s>\n"% (k,v,k))
        out.write("    </params>\n")
        out.write("    <link name=%s port=rtr latency=%s />\n" % (linkName, params["link_lat"]))
        out.write("  </component>\n\n")




############################################################################


def generateSDL(filename, topo, endpoint):

    with open(filename, 'w') as out:
        out.write( "<?xml version=\"1.0\"?>\n")
        out.write( "<sdl version=\"2.0\" />\n")
        out.write( "\n")
        out.write( "<config>\n")
        out.write( "  run-mode=both\n")
        if "config:runtime" in params:
            out.write( "  stopAtCycle=%s\n"%params["config:runtime"])
        out.write( "</config>\n")
        out.write( "\n")
        out.write( "<param_include>\n")
        topo.formatParams(out)
        out.write( "\n")
        endpoint.formatParams(out)
        out.write( "</param_include>\n")
        out.write( "\n")
        out.write( "\n")
        out.write( "<sst>\n")
        topo.formatSDL(out, endpoint)
        out.write( "</sst>\n")
        out.write( "\n\n\n")
        out.write( "<!-- \n\tParameters\n")
        for key in params:
            out.write("%s = %s\n" % (key, params[key]))
        out.write("\n-->\n\n")






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

    print "Please enter the target output file  :"
    filename = raw_input()

    
    generateSDL(filename, topo, endPoint)

