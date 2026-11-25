#!/usr/bin/env python3
#
# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import argparse
import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.hg import *

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--alg")
    args = parser.parse_args()

    PlatformDefinition.loadPlatformFile("platform_file_mask_mpi_test")
    PlatformDefinition.setCurrentPlatform("platform_mask_mpi_test")
    platform = PlatformDefinition.getCurrentPlatform()

    params = {
        "verbose" : "0",
        "app1.name" : "alltoall",
        "app1.exe_library_name" : "alltoall",
        "app1.dependencies" : ["sumi", ],
        "app1.libraries" : ["computelibrary:ComputeLibrary",
                            "mask_mpi:MpiApi",],
    }
    if args.alg is not None:
        params["app1.collective.alltoall"] = args.alg
    platform.addParamSet("operating_system", params)

    topo = topoSingle()
    topo.link_latency = "20ns"
    topo.num_ports = 32

    ep = HgJob(0,8)

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()
