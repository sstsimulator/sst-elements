#!/usr/bin/env python
#
# Copyright 2009-2020 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2020, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interfaces import *
from sst.merlin.topology import *

if __name__ == "__main__":


    ### Setup the topology
    topo = topoDragonFly()
    topo.hosts_per_router = 4
    topo.routers_per_group = 8
    topo.intergroup_links = 4
    topo.num_groups = 4
    topo.algorithm = "minimal"
    
    # Set up the routers
    router = hr_router()
    router.link_bw = "4GB/s"
    router.flit_size = "8B"
    router.xbar_bw = "4GB/s"
    router.input_latency = "20ns"
    router.output_latency = "20ns"
    router.input_buf_size = "4kB"
    router.output_buf_size = "4kB"
    router.num_vns = 1
    router.xbar_arb = "merlin.xbar_arb_lru"

    topo.router = router
    topo.link_latency = "20ns"
    
    ### set up the endpoint
    networkif = LinkControl()
    networkif.link_bw = "4GB/s"
    networkif.input_buf_size = "1kB"
    networkif.output_buf_size = "1kB"

    ep = TestJob(0,topo.getNumNodes())
    ep.network_interface = networkif
    #ep.num_messages = 10
    #ep.message_size = "8B"
    #ep.send_untimed_bcast = False
        
    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()
    

    sst.setStatisticLoadLevel(9)

    sst.setStatisticOutput("sst.statOutputCSV");
    sst.setStatisticOutputOptions({
        "filepath" : "stats.csv",
        "separator" : ", "
    })

