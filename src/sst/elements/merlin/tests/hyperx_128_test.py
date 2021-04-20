#!/usr/bin/env python
#
# Copyright 2009-2021 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2021, NTESS
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

if __name__ == "__main__":


    ### Setup the topology
    topo = topoHyperX()
    topo.shape = "4x4"
    topo.width = "2x2"
    topo.local_ports = 8
    topo.algorithm = ["DOR","MIN-A"]
    
    # Set up the routers
    router = hr_router()
    router.link_bw = "4GB/s"
    router.flit_size = "8B"
    router.xbar_bw = "6GB/s"
    router.input_latency = "20ns"
    router.output_latency = "20ns"
    router.input_buf_size = "4kB"
    router.output_buf_size = "4kB"
    router.num_vns = 2
    router.xbar_arb = "merlin.xbar_arb_lru"

    topo.router = router
    topo.link_latency = "20ns"
    
    ### set up the endpoint
    networkif = LinkControl()
    networkif.link_bw = "4GB/s"
    networkif.input_buf_size = "1kB"
    networkif.output_buf_size = "1kB"

    networkif2 = LinkControl()
    networkif2.link_bw = "4GB/s"
    networkif2.input_buf_size = "1kB"
    networkif2.output_buf_size = "1kB"

    # Set up VN remapping
    networkif.vn_remap = [0]
    networkif2.vn_remap = [1]
    
    ep = TestJob(0,topo.getNumNodes() // 2)
    ep.network_interface = networkif
    #ep.num_messages = 10
    #ep.message_size = "8B"
    #ep.send_untimed_bcast = False
        
    ep2 = TestJob(1,topo.getNumNodes() // 2)
    ep2.network_interface = networkif2
    #ep.num_messages = 10
    #ep.message_size = "8B"
    #ep.send_untimed_bcast = False
        
    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")
    system.allocateNodes(ep2,"linear")

    system.build()
    

    sst.setStatisticLoadLevel(9)

    sst.setStatisticOutput("sst.statOutputCSV");
    sst.setStatisticOutputOptions({
        "filepath" : "stats.csv",
        "separator" : ", "
    })

