#!/usr/bin/env python
#
# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *

from sst.ember import *

if __name__ == "__main__":

    PlatformDefinition.setCurrentPlatform("firefly-defaults")

    ### Setup the topology
    #topo = topoDragonFly()
    #topo.hosts_per_router = 2
    #topo.routers_per_group = 4
    #topo.intergroup_links = 2
    #topo.num_groups = 2
    #topo.link_latency = "20ns"
    #topo.algorithm = ["minimal","adaptive-local"]
    #topo.algorithm = "ugal"

    topo = topoSingle()
    topo.link_latency = "20ns"
    topo.num_ports = 32
    
    # Set up the routers
    router = hr_router()
    router.link_bw = "12GB/s"
    router.flit_size = "8B"
    router.xbar_bw = "50GB/s"
    router.input_latency = "40ns"
    router.output_latency = "40ns"
    router.input_buf_size = "16kB"
    router.output_buf_size = "16kB"
    router.num_vns = 1
    router.xbar_arb = "merlin.xbar_arb_lru"

    topo.router = router
    topo.link_latency = "20ns"
    
    ### set up the endpoint
    networkif = ReorderLinkControl()
    networkif.link_bw = "12GB/s"
    networkif.input_buf_size = "16kB"
    networkif.output_buf_size = "16kB"

    # Set up VN remapping
    #networkif.vn_remap = [0]
    
    ep = EmberMPIJob(0,3)
    #ep = EmberMPIJob(0,topo.getNumNodes())
    ep.network_interface = networkif
    ep.addMotif("Init")
    ep.addMotif("TriCount vertices_filename=vertices4.txt debug_level=0")
    ep.addMotif("Fini")
    ep.nic.nic2host_lat= "100ns"
        
    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()

    # sst.setStatisticLoadLevel(9)

    # sst.setStatisticOutput("sst.statOutputCSV");
    # sst.setStatisticOutputOptions({
    #     "filepath" : "stats.csv",
    #     "separator" : ", "
    # })

