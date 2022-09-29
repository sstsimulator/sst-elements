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
import sys
from sst.merlin.base import *
from sst.merlin.topology import *
from sst.hg import *

if __name__ == "__main__":

    PlatformDefinition.loadPlatformFile("hg_platform")
    PlatformDefinition.setCurrentPlatform("hg-platform")

    num_jobs = 1
    iterations = 1
    
    # Parse the arguments.  Valid arguments are:
    # --jobs: Set number of halo jobs
    #      format: --jobs=X [default=1]
    # --iterations: Set number of halo iterations
    #      format: --iterations=X [default=2]
    # --param: load parameters in platform definition:
    #          format: --param=param_set:key=value
    args = copy.copy(sys.argv)
    # ignore name of program
    args.pop(0)
    print(args)
    param_sets = dict()
    for arg in args:
        if arg.startswith("--jobs="):
            num_jobs = int(arg[7:])
        elif arg.startswith("--iterations="):
            iterations = int(arg[13:])
        elif arg.startswith("--param="):
            arg = arg[8:]
            pset, kv = arg.split(":",1)
            key, value = kv.split("=",1)
            if not pset in param_sets:
                param_sets[pset] = dict()
            param_sets[pset][key] = value;
        else:
            print("Unknown command line argument: " + arg)
            sst.exit()
            
    print(param_sets)

    print("num_jobs = " + str(num_jobs))
    print("iterations = " + str(iterations))
    print("")

    # create suffix for the various files
    suffix = "_jobs-%d_iterations-%d_"%(num_jobs,iterations)

    print(suffix)

    pd = PlatformDefinition.getCurrentPlatform()

    for key in param_sets:
        #print(key)
        #print(param_sets[key])
        pd.addParamSet(key,param_sets[key])

    #print(pd.getParamSet("topology"))


    ### Setup the topology
    topo = topoSingle()
    topo.num_ports = 2

    system = System()
    system.setTopology(topo)

    total_nodes = system.topology.getNumNodes()
    
    # Create a job 
    job_params = {
        "eventsToSend" : 50,    # Required parameter, error if not provided
        "eventSize" : 32        # Optional parameter, defaults to 16 if not provided
    }
    hg_job = HgJob(0,2,{},job_params)

    # Allocate job to system

    system.allocateNodes(hg_job,"linear")

    system.build()

