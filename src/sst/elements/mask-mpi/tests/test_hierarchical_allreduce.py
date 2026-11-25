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

parser = argparse.ArgumentParser()
parser.add_argument("--hierarchy")
parser.add_argument("--allreduce")
parser.add_argument("--intra")
parser.add_argument("--inter")
parser.add_argument("--bcast")
parser.add_argument("--nodes", type=int, default=2)
parser.add_argument("--cores", type=int, default=4)
parser.add_argument("--sparse-comm", action="store_true")
args = parser.parse_args()

PlatformDefinition.loadPlatformFile("platform_file_mask_mpi_test")
PlatformDefinition.setCurrentPlatform("platform_mask_mpi_test")
platform = PlatformDefinition.getCurrentPlatform()

params = {
    "verbose" : "0",
    "app1.name" : "ringallreduce",
    "app1.exe_library_name" : "ringallreduce",
    "app1.dependencies" : ["sumi", ],
    "app1.libraries" : ["computelibrary:ComputeLibrary",
                        "mask_mpi:MpiApi",],
    "app1.smp_optimize" : "true",
}
if args.sparse_comm:
    params["app1.argv"] = "--sparse-comm"
if args.hierarchy is not None:
    params["app1.collective.allreduce.hierarchy"] = args.hierarchy
if args.allreduce is not None:
    params["app1.collective.allreduce"] = args.allreduce
for key, alg in (("intra", args.intra), ("inter", args.inter),
                 ("broadcast", args.bcast)):
    if alg is not None:
        params["app1.collective.allreduce.{}".format(key)] = alg
platform.addParamSet("operating_system", params)

topo = topoSingle()
topo.link_latency = "20ns"
topo.num_ports = 32

ep = HgJob(0, args.nodes, numCores=args.cores)

system = System()
system.setTopology(topo)
system.allocateNodes(ep, "random", 42)
system.build()
