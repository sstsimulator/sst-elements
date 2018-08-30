#!/usr/bin/env python
#
# Copyright 2009-2015 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2015, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
sst.setStatisticLoadLevel(3)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv","separator" : "," } )

from sst.merlin import *

sst.merlin._params["flit_size"] = "16B"
sst.merlin._params["link_bw"] = "4.0GB/s"
sst.merlin._params["xbar_bw"] = "4.0GB/s"
sst.merlin._params["input_latency"] = "0.0ns"
sst.merlin._params["output_latency"] = "0.0ns"
sst.merlin._params["input_buf_size"] = "16.0KB"
sst.merlin._params["output_buf_size"] = "16.0KB"
sst.merlin._params["link_lat"] = "5000ns"

merlintorusparams = {}
merlintorusparams["num_dims"]=1
merlintorusparams["torus:shape"]="5"
merlintorusparams["torus:width"]="1"
merlintorusparams["torus:local_ports"]=1
sst.merlin._params.update(merlintorusparams)
topo = topoTorus()
topo.prepParams()

sst.merlin._params["PacketDest:pattern"] = "Uniform"
sst.merlin._params["PacketDest:RangeMin"] = "1.0"
sst.merlin._params["PacketDest:RangeMax"] = "2.0"
sst.merlin._params["PacketSize:pattern"] = "Uniform"
sst.merlin._params["PacketSize:RangeMin"] = "1.0B"
sst.merlin._params["PacketSize:RangeMax"] = "10.0B"
# Required by pymerlin
sst.merlin._params["packet_size"] = "0KB"
sst.merlin._params["PacketDelay:pattern"] = "Uniform"
sst.merlin._params["PacketDelay:RangeMin"] = "5.0ns"
sst.merlin._params["PacketDelay:RangeMax"] = "10.0ns"
# Required by pymerlin
sst.merlin._params["message_rate"] = "1GHz"
sst.merlin._params["packets_to_send"] = 10

endPoint = TrafficGenEndPoint()
endPoint.prepParams()

topo.setEndPoint(endPoint)
topo.build()

sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic","rate":"0ns"})
