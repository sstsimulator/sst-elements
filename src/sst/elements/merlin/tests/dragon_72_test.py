#!/usr/bin/env python
#
# Copyright 2009-2021 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2021, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
from sst.merlin import *

if __name__ == "__main__":

    topo = topoDragonFly()
    endPoint = TestEndPoint()


    sst.merlin._params["dragonfly:hosts_per_router"] = "2"
    sst.merlin._params["dragonfly:routers_per_group"] = "4"
    sst.merlin._params["dragonfly:intergroup_links"] = "1"
    sst.merlin._params["dragonfly:num_groups"] = "9"
    sst.merlin._params["dragonfly:algorithm"] = "minimal"
    #sst.merlin._params["dragonfly:algorithm"] = "adaptive-local"
    #sst.merlin._params["dragonfly:adaptive_threshold"] = "2.0"

    #glm = [0, 15, 1, 14, 2, 13, 3, 12, 4, 11, 5, 10, 6, 9, 7, 8]
    #topo.setGlobalLinkMap(glm)
    #topo.setRoutingModeRelative()


    sst.merlin._params["link_bw"] = "4GB/s"
    #sst.merlin._params["link_bw:host"] = "2GB/s"
    #sst.merlin._params["link_bw:group"] = "1GB/s"
    #sst.merlin._params["link_bw:global"] = "1GB/s"
    sst.merlin._params["link_lat"] = "20ns"
    sst.merlin._params["flit_size"] = "8B"
    sst.merlin._params["xbar_bw"] = "4GB/s"
    sst.merlin._params["input_latency"] = "20ns"
    sst.merlin._params["output_latency"] = "20ns"
    sst.merlin._params["input_buf_size"] = "4kB"
    sst.merlin._params["output_buf_size"] = "4kB"

    #sst.merlin._params["checkerboard"] = "1"
    sst.merlin._params["xbar_arb"] = "merlin.xbar_arb_lru"

    topo.prepParams()
    endPoint.prepParams()
    topo.setEndPoint(endPoint)
    topo.build()

    #sst.setStatisticLoadLevel(9)

    #sst.setStatisticOutput("sst.statOutputCSV");
    #sst.setStatisticOutputOptions({
    #    "filepath" : "stats.csv",
    #    "separator" : ", "
    #})

    #endPoint.enableAllStatistics("0ns")

    #sst.enableAllStatisticsForComponentType("merlin.hr_router", {"type":"sst.AccumulatorStatistic","rate":"0ns"})
