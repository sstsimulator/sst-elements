#!/usr/bin/env python
#
# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
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
from sst.hg import *

if __name__ == "__main__":

    PlatformDefinition.loadPlatformFile("platform_file_mask_mpi_test")
    PlatformDefinition.setCurrentPlatform("platform_mask_mpi_test")
    platform = PlatformDefinition.getCurrentPlatform()

    platform.addParamSet("operating_system", {
        "verbose" : "0",
        "app1.name" : "reduce",
        "app1.exe"  : "reduce.so",
        "app1.apis" : ["systemAPI:libsystemapi.so", "SimTransport:libsumi.so", "MpiApi:libmask_mpi.so"],
    })

    topo = topoSingle()
    topo.link_latency = "20ns"
    topo.num_ports = 32

    ep = HgJob(0,8)

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"random",42)

    system.build()
