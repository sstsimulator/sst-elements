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
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.topology import *
from sst.merlin.interface import *
from sst.merlin.router import *

if __name__ == "__main__":

    PlatformDefinition.loadPlatformFile("platform_file_dragon_128")
    PlatformDefinition.setCurrentPlatform("platform_dragon_128_cm")

    # Allocate the system. This will create the topology since it's
    # set up in the platform file
    system = System()

    ### set up the incast endpoint
    #ep = EmptyJob(0,16)
    ep = IncastJob(0,16)
    ep.target_nids = 0
    ep.packets_to_send = 30
    ep.packet_size = "256B"

    ep2 = TestJob(1,system.topology.getNumNodes()-16)
        
    system.allocateNodes(ep,"interval",0,8)
    system.allocateNodes(ep2,"linear")

    system.build()
