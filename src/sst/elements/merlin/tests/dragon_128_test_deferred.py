#!/usr/bin/env python
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
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

    # This file creates two dragonfly networks of different sizes and
    # instances the endpoints using deferred build.

    # Network 1
    ### Setup the topology
    topo = topoDragonFly()
    topo.hosts_per_router = 4
    topo.routers_per_group = 8
    topo.intergroup_links = 4
    topo.num_groups = 5
    topo.algorithm = "ugal"
    topo.network_name = "net1"

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

    networkif_ro = ReorderLinkControl()
    networkif_ro.network_interface = networkif

    ep = TestJob(0,topo.getNumNodes())
    ep.network_interface = networkif_ro
    #ep.num_messages = 10
    #ep.message_size = "8B"
    #ep.send_untimed_bcast = False

    system = System()
    system.setTopology(topo)

    system.deferred_build()

    # Need to push the name prefix to match the network name
    sst.pushNamePrefix(topo.network_name)
    ep.createLinearNidMap()
    for x in range(topo.getNumNodes()):
        ep.build(x, {}, system.getLinkForEndpoint(x))
    sst.popNamePrefix()

    ##### Done with first network

    # Newtork 2
    ### Setup the topology
    topo = topoDragonFly()
    topo.hosts_per_router = 4
    topo.routers_per_group = 8
    topo.intergroup_links = 4
    topo.num_groups = 4
    topo.algorithm = "ugal"
    topo.network_name = "net2"

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

    networkif_ro = ReorderLinkControl()
    networkif_ro.network_interface = networkif

    ep = TestJob(0,topo.getNumNodes())
    ep.network_interface = networkif_ro
    #ep.num_messages = 10
    #ep.message_size = "8B"
    #ep.send_untimed_bcast = False

    system = System()
    system.setTopology(topo)

    system.deferred_build()

    # Need to push the name prefix to match the network name
    sst.pushNamePrefix(topo.network_name)
    ep.createLinearNidMap()
    for x in range(topo.getNumNodes()):
        ep.build(x, {}, system.getLinkForEndpoint(x))
    sst.popNamePrefix()


    # sst.setStatisticLoadLevel(9)

    # sst.setStatisticOutput("sst.statOutputCSV");
    # sst.setStatisticOutputOptions({
    #     "filepath" : "stats.csv",
    #     "separator" : ", "
    # })

