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

import os
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

    os_params = {
        "verbose" : "0",
        "app1.name" : "allreduce",
        "app1.exe_library_name" : "allreduce",
        "app1.dependencies" : ["sumi", ],
        "app1.libraries" : ["computelibrary:ComputeLibrary",
                            "mask_mpi:MpiApi",],
    }
    # ALG feeds the Python parameter path; SUMI_ALLREDUCE_ALG remains the
    # environment fallback when the parameter is unset.
    _alg = os.environ.get("ALG", "")
    if _alg:
        _alg_param = os.environ.get("ALG_PARAM", "collective.allreduce")
        os_params["app1." + _alg_param] = _alg
    platform.addParamSet("operating_system", os_params)

    _nranks = int(os.environ.get("NRANKS", "8"))
    topo = topoSingle()
    topo.link_latency = "20ns"
    topo.num_ports = max(32, _nranks)

    ep = HgJob(0, _nranks)

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"random",42)

    system.build()
