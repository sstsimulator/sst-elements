#!/usr/bin/env python
#
# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

from email.mime import base
import sst
from sst.merlin.base import *
# from src.sst.elements.merlin.pymerlin_base import *
from sst.merlin.endpoint import * 
# from src.sst.elements.merlin.pymerlin_endpoint import *
from sst.merlin.interface import *
# from src.sst.elements.merlin.interfaces.pymerlin_interface import *
from sst.merlin.topology import *
# from src.sst.elements.merlin.topology.pymerlin_topo_dragonfly import *

from sst.ember import *
# from src.sst.elements.ember.pyember import *

if __name__ == "__main__":

    PlatformDefinition.setCurrentPlatform("firefly-defaults")

    ### Setup the topology
    topo = topoDragonFly()
    topo.hosts_per_router = 2
    topo.routers_per_group = 4
    topo.intergroup_links = 2
    topo.num_groups = 4
    topo.algorithm = ["minimal","adaptive-local"]
    
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
    networkif = ReorderLinkControl()
    networkif.link_bw = "4GB/s"
    networkif.input_buf_size = "1kB"
    networkif.output_buf_size = "1kB"
    
    ep = EmberMPIJob(0,topo.getNumNodes())
    ep.network_interface = networkif
    ep.addMotif("Foo")
    ep.nic.nic2host_lat= "100ns"
        
    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()
