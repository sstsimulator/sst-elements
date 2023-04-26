#!/usr/bin/env python

# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# of the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: MIT License

# Authors: Kartik Lakhotia

import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *


if __name__=="__main__":
    ### Configuration
    specified_d=8
    specified_algo='UGAL'  
    specified_k=3


    ### Setup the topology
    topo                        = topoPolarStar(d=specified_d)
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

