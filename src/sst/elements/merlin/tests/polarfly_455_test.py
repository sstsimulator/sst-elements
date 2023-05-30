#!/usr/bin/env python

# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause

# Authors: Kartik Lakhotia

import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *

import sys


if __name__ == "__main__":


    ### Configuration
    specified_q=9
    specified_k=5
    specified_algo='UGAL_PF'

    if ('sympy.polys.galoistools' in sys.modules):
        ### Setup the topology
        topo                        = topoPolarFly(q=specified_q)
        topo.algorithm              = specified_algo
        topo.hosts_per_router       = specified_k


        # Set up the routers
        router                      = hr_router()
        router.link_bw              = "4GB/s"
        router.flit_size            = "8B"
        router.xbar_bw              = "6GB/s"
        router.input_latency        = "20ns"
        router.output_latency       = "20ns"
        router.input_buf_size       = "4kB"
        router.output_buf_size      = "4kB"
        router.num_vns              = 1
        router.xbar_arb             = "merlin.xbar_arb_lru"
        router.oql_track_port       = True

        topo.router                 = router
        topo.link_latency           = "20ns"


        ### set up the endpoint
        networkif                   = LinkControl()
        networkif.link_bw           = "4GB/s"
        networkif.input_buf_size    = "1kB"
        networkif.output_buf_size   = "1kB"


        # Set up VN remapping
        networkif.vn_remap = [0]


        #jobId, # endpoints
        ep                          = TestJob(0, topo.getNumNodes()) 
        ep.network_interface        = networkif  

        
        system                      = System()
        system.setTopology(topo)
        system.allocateNodes(ep,"linear")
        system.build() 
        
