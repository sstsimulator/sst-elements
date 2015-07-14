#!/usr/bin/env python
#
# Copyright 2009-2015 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2015, Sandia Corporation
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
import sys
import math
#from sst.merlin import *

if __name__ == "__main__":

    class Params(dict):
        def __missing__(self, key):
            print "Please enter %s: "%key
            val = raw_input()
            self[key] = val
            return val
        def subset(self, keys, optKeys = []):
            ret = dict((k, self[k]) for k in keys)
            ret.update(dict((k, self[k]) for k in keys and self))
            return ret
        # Needed to avoid asking for input when a key isn't present
    #    def optional_subset(self, keys):
    #        return 
        
    _params = Params()
    debug = 0

    class Topo:
        def __init__(self):
            self.topoKeys = []
            self.topoOptKeys = []
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

    class topoDragonFly(Topo):
        def __init__(self):
            Topo.__init__(self)
            self.topoKeys = ["topology", "xbar_arb", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw", "dragonfly:hosts_per_router", "dragonfly:routers_per_group", "dragonfly:intergroup_per_router", "dragonfly:num_groups","input_latency","output_latency","input_buf_size","output_buf_size"]
            self.topoOptKeys = ["xbar_arb"]
        def getName(self):
            return "Dragonfly"

        def prepParams(self):
            if "xbar_arb" not in _params:
                _params["xbar_arb"] = "merlin.xbar_arb_lru"
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

            # Define SST core options
            sst.setProgramOption("run-mode", "both")

            # Add scheduler component
            scheduler = sst.Component("myScheduler", "scheduler.schedComponent")
            scheduler.addParams({
                "traceName" : "/home/fkaplan/scheduler/simulations/test_scheduler_Atlas_short.sim",
                "machine" : "mesh[2,2,18]",
                "coresPerNode" : "2",
                "scheduler" : "easy",
                "allocator" : "bestfit",
                "timeperdistance" : ".001865[.1569,0.0129]",
                "dMatrixFile" : "none",
                "detailedNetworkSim" : "ON"
            })

            # Select the Dragonfly global link arrangement
            #glb_link_arr = "default"
            glb_link_arr = "absolute"
            #glb_link_arr = "relative"
            #glb_link_arr = "circulant-based"

            router_num = 0
            nic_num = 0

            if glb_link_arr == "default":
                # GROUPS
                for g in xrange(_params["dragonfly:num_groups"]):
                    tgt_grp = 0

                    # GROUP ROUTERS
                    for r in xrange(_params["dragonfly:routers_per_group"]):
                        rtr = sst.Component("rtr:G%dR%d"%(g, r), "merlin.hr_router")
                        rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
                        rtr.addParam("id", router_num)

                        port = 0
                        # Connect hosts to the routers
                        for p in xrange(_params["dragonfly:hosts_per_router"]):
                            link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                            rtr.addLink(link, "port%d"%port, _params["link_lat"])
                            self._getEndPoint(nic_num).build(nic_num, link, {})
                            scheduler.addLink(getLink("link:schedToNode%d"%(nic_num)), "nodeLink%d"%(nic_num), "0ns")
                            nic_num = nic_num + 1
                            print "hosts per router"
                            print "group:%s, router:%s, port:%s" %(g,r,port)
                            port = port + 1
                        # Connect routers within the same group
                        for p in xrange(_params["dragonfly:routers_per_group"]):
                            if p != r:
                                src = min(p,r)
                                dst = max(p,r)
                                rtr.addLink(getLink("link:g%dr%dr%d"%(g, src, dst)), "port%d"%port, _params["link_lat"])
                                print "routers per group"
                                print "group:%s, router:%s, port:%s" %(g,r,port)
                                port = port + 1
                        #Connect routers to the other groups
                        for p in xrange(_params["dragonfly:intergroup_per_router"]):
                            if (tgt_grp%_params["dragonfly:num_groups"]) == g:
                                tgt_grp = tgt_grp +1

                            src_g = min(g, tgt_grp%_params["dragonfly:num_groups"])
                            dst_g = max(g, tgt_grp%_params["dragonfly:num_groups"])

                            rtr.addLink(getLink("link:g%dg%d:%d"%(src_g, dst_g, tgt_grp/_params["dragonfly:num_groups"])), "port%d"%port, _params["link_lat"])
                            print "inter group per router"
                            print "group:%s, router:%s, port:%s" %(g,r,port)
                            port = port +1
                            tgt_grp = tgt_grp +1

                        router_num = router_num +1

            elif glb_link_arr == "absolute":

                h = _params["dragonfly:intergroup_per_router"]
                # GROUPS
                for g in xrange(_params["dragonfly:num_groups"]):
                    tgt_grp = 0
                    vport = 0 # virtual port of the virtual switch. k = {0,...,(num_groups-2)} in the paper.

                    # GROUP ROUTERS
                    for r in xrange(_params["dragonfly:routers_per_group"]):
                        rtr = sst.Component("rtr:G%dR%d"%(g, r), "merlin.hr_router")
                        rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
                        rtr.addParam("id", router_num)

                        port = 0
                        # Connect hosts to the routers
                        for p in xrange(_params["dragonfly:hosts_per_router"]):
                            link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                            rtr.addLink(link, "port%d"%port, _params["link_lat"])
                            self._getEndPoint(nic_num).build(nic_num, link, {})
                            scheduler.addLink(getLink("link:schedToNode%d"%(nic_num)), "nodeLink%d"%(nic_num), "0ns")
                            nic_num = nic_num + 1
                            #print "hosts per router"
                            #print "group:%s, router:%s, port:%s" %(g,r,port)
                            port = port + 1
                        # Connect routers within the same group
                        for p in xrange(_params["dragonfly:routers_per_group"]):
                            if p != r:
                                src = min(p,r)
                                dst = max(p,r)
                                rtr.addLink(getLink("link:g%dr%dr%d"%(g, src, dst)), "port%d"%port, _params["link_lat"])
                                #print "routers per group"
                                #print "group:%s, router:%s, port:%s" %(g,r,port)
                                port = port + 1
                        #Connect routers to the other groups
                        for p in xrange(_params["dragonfly:intergroup_per_router"]):
                            # k < i
                            if vport < g:
                                tgt_grp = vport
                                tgt_vport = g - 1
                                #tgt_vport = int(math.floor((g-1)/h))* h
                            # k >= i
                            else:           
                                tgt_grp = vport + 1
                                tgt_vport = g
                                #tgt_vport = int(math.floor(g/h))* h + 1

                            src_g = min(g, tgt_grp)
                            dst_g = max(g, tgt_grp)

                            src_p = min(vport, tgt_vport)
                            dst_p = max(vport, tgt_vport)

                            rtr.addLink(getLink("link:g%dg%d:vp%dvp%d"%(src_g, dst_g, src_p, dst_p)), "port%d"%port, _params["link_lat"])
                            #print "inter group per router"
                            #print "group:%s, router:%s, port:%s" %(g,r,port)
                            #print "p(%d)g(%d)->p(%d)g(%d)" %(vport, g, tgt_vport, tgt_grp)
                            #print "link name:g%dg%d:vp%dvp%d"%(src_g, dst_g, src_p, dst_p)
                            port = port +1
                            vport = vport +1

                        router_num = router_num +1

            elif glb_link_arr == "relative":
                # GROUPS
                for g in xrange(_params["dragonfly:num_groups"]):
                    tgt_grp = 0
                    vport = 0 # virtual port of the virtual switch. k = {0,...,(num_groups-2)} in the paper.

                    # GROUP ROUTERS
                    for r in xrange(_params["dragonfly:routers_per_group"]):
                        rtr = sst.Component("rtr:G%dR%d"%(g, r), "merlin.hr_router")
                        rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
                        rtr.addParam("id", router_num)

                        port = 0
                        # Connect hosts to the routers
                        for p in xrange(_params["dragonfly:hosts_per_router"]):
                            link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                            rtr.addLink(link, "port%d"%port, _params["link_lat"])
                            self._getEndPoint(nic_num).build(nic_num, link, {})
                            scheduler.addLink(getLink("link:schedToNode%d"%(nic_num)), "nodeLink%d"%(nic_num), "0ns")
                            nic_num = nic_num + 1
                            #print "hosts per router"
                            #print "group:%s, router:%s, port:%s" %(g,r,port)
                            port = port + 1
                        # Connect routers within the same group
                        for p in xrange(_params["dragonfly:routers_per_group"]):
                            if p != r:
                                src = min(p,r)
                                dst = max(p,r)
                                rtr.addLink(getLink("link:g%dr%dr%d"%(g, src, dst)), "port%d"%port, _params["link_lat"])
                                #print "routers per group"
                                #print "group:%s, router:%s, port:%s" %(g,r,port)
                                port = port + 1
                        #Connect routers to the other groups
                        for p in xrange(_params["dragonfly:intergroup_per_router"]):
                            tgt_grp = (g + vport + 1)%_params["dragonfly:num_groups"]
                            tgt_vport = _params["dragonfly:num_groups"] - 2 - vport

                            src_g = min(g, tgt_grp)
                            dst_g = max(g, tgt_grp)

                            src_p = min(vport, tgt_vport)
                            dst_p = max(vport, tgt_vport)

                            rtr.addLink(getLink("link:g%dg%d:vp%dvp%d"%(src_g, dst_g, src_p, dst_p)), "port%d"%port, _params["link_lat"])
                            #print "inter group per router"
                            #print "group:%s, router:%s, port:%s" %(g,r,port)
                            #print "p(%d)g(%d)->p(%d)g(%d)" %(vport, g, tgt_vport, tgt_grp)
                            #print "link name:g%dg%d:vp%dvp%d"%(src_g, dst_g, src_p, dst_p)
                            port = port +1
                            vport = vport +1

                        router_num = router_num +1


            elif glb_link_arr == "circulant-based":
                # This arrangement assumes h = intergroup_per_router is even
                if _params["dragonfly:intergroup_per_router"]%2 != 0:
                    print "Error! Intergroup_per_router must be even in %s arrangement." %(glb_link_arr)
                    sys.exit(1)

                # GROUPS
                for g in xrange(_params["dragonfly:num_groups"]):
                    tgt_grp = 0
                    vport = 0 # virtual port of the virtual switch. k = {0,...,(num_groups-2)} in the paper.

                    # GROUP ROUTERS
                    for r in xrange(_params["dragonfly:routers_per_group"]):
                        rtr = sst.Component("rtr:G%dR%d"%(g, r), "merlin.hr_router")
                        rtr.addParams(_params.subset(self.topoKeys, self.topoOptKeys))
                        rtr.addParam("id", router_num)

                        port = 0
                        # Connect hosts to the routers
                        for p in xrange(_params["dragonfly:hosts_per_router"]):
                            link = sst.Link("link:g%dr%dh%d"%(g, r, p))
                            rtr.addLink(link, "port%d"%port, _params["link_lat"])
                            self._getEndPoint(nic_num).build(nic_num, link, {})
                            scheduler.addLink(getLink("link:schedToNode%d"%(nic_num)), "nodeLink%d"%(nic_num), "0ns")
                            nic_num = nic_num + 1
                            #print "hosts per router"
                            #print "group:%s, router:%s, port:%s" %(g,r,port)
                            port = port + 1
                        # Connect routers within the same group
                        for p in xrange(_params["dragonfly:routers_per_group"]):
                            if p != r:
                                src = min(p,r)
                                dst = max(p,r)
                                rtr.addLink(getLink("link:g%dr%dr%d"%(g, src, dst)), "port%d"%port, _params["link_lat"])
                                #print "routers per group"
                                #print "group:%s, router:%s, port:%s" %(g,r,port)
                                port = port + 1
                        #Connect routers to the other groups
                        for p in xrange(_params["dragonfly:intergroup_per_router"]):
                            # virtual port number is even
                            if vport%2 == 0:
                                tgt_grp = (g + vport/2 + 1)%_params["dragonfly:num_groups"]
                                tgt_vport = vport + 1
                            # virtual port number is odd
                            else:           
                                tgt_grp = (g - int(math.floor(vport/2)) - 1)%_params["dragonfly:num_groups"]
                                tgt_vport = vport - 1

                            src_g = min(g, tgt_grp)
                            dst_g = max(g, tgt_grp)

                            src_p = min(vport, tgt_vport)
                            dst_p = max(vport, tgt_vport)

                            rtr.addLink(getLink("link:g%dg%d:vp%dvp%d"%(src_g, dst_g, src_p, dst_p)), "port%d"%port, _params["link_lat"])
                            #print "inter group per router"
                            print "group:%s, router:%s, port:%s" %(g,r,port)
                            print "p(%d)g(%d)->p(%d)g(%d)" %(vport, g, tgt_vport, tgt_grp)
                            print "link name:g%dg%d:vp%dvp%d"%(src_g, dst_g, src_p, dst_p)
                            port = port +1
                            vport = vport +1

                        router_num = router_num +1

            else:
                print "Error: Unknown dragonfly global link arrangement! (absolute | relative | circulant-based)"
                sys.exit(1)



    ############################################################################

    class EndPoint:
        def __init__(self):
            self.epKeys = []
            self.epOptKeys = []
            self.enableAllStats = False
            self.statInterval = "0"
        def getName(self):
            print "Not implemented"
            sys.exit(1)
        def prepParams(self):
            pass
        def build(self, nID, link, extraKeys):
            pass    

    class NicComponentEndPoint(EndPoint):
        def __init__(self):
            self.enableAllStats = False;
            self.statInterval = "0"
            self.nicKeys = ["topology", "num_peers", "link_bw", "checkerboard"]

        def getName(self):
            return "Nic Component Endpoint"

        def prepParams(self):
            if "checkerboard" not in _params:
                _params["checkerboard"] = "1"

        def build(self, nID, link, extraKeys):
            nic = sst.Component("routeTest.%d"%nID, "scheduler.nicComponent")
            nic.addParams(_params.subset(self.nicKeys))
            nic.addParams(_params.subset(extraKeys))
            nic.addParam("id", nID)
            nic.addLink(link, "rtr", _params["link_lat"])

            # Add node component and connect to NIC
            node = sst.Component("n%d"%nID, "scheduler.nodeComponent")
            node.addParam("nodeNum", nID)
            niclink = sst.Link("link:nic%dToNode%d"%(nID, nID))
            nic.addLink(niclink, "node", "0ns")
            node.addLink(niclink, "NicLink", "0ns")
            
            # Connect node to scheduler
            schedlink = sst.Link("link:schedToNode%d"%(nID))
            node.addLink(schedlink, "Scheduler", "0ns")     

            if self.enableAllStats:
                nic.enableAllStatistics({"type":"sst.AccumulatorStatistic","rate":self.statInterval})

        #print "Created Endpoint with id: %d, and params: %s %s\n"%(nID, _params.subset(self.nicKeys), _params.subset(extraKeys))
        def enableAllStatistics(self,interval):
            self.enableAllStats = True;
            self.statInterval = interval;
    

    topo = topoDragonFly()
    endPoint = NicComponentEndPoint()

    _params["checkerboard"] = "1"    
    _params["xbar_arb"] = "merlin.xbar_arb_lru"
    _params["topology"] = "merlin.dragonfly"
    _params["debug"] = debug
    _params["num_vns"] = 1

    _params["router_radix"] = 7
    _params["num_ports"] = int(_params["router_radix"])
    _params["dragonfly:hosts_per_router"] = 2
    _params["dragonfly:routers_per_group"] = 4
    _params["dragonfly:intergroup_per_router"] = 2
    _params["dragonfly:num_groups"] = 9
    _params["flit_size"] = "4B"
    _params["link_bw"] = "4GB/s"
    _params["xbar_bw"] = "4GB/s"
    _params["input_latency"] = "20ns"
    _params["output_latency"] = "20ns"
    _params["link_lat"] = "20ns"
    _params["input_buf_size"] = "1kB"
    _params["output_buf_size"] = "1kB"
    _params["dragonfly:algorithm"] = "minimal"




    topo.prepParams()
    endPoint.prepParams()
    topo.setEndPoint(endPoint)
    topo.build()

