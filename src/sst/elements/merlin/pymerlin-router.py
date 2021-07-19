#!/usr/bin/env python
#
# Copyright 2009-2021 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2021, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sst
from sst.merlin.base import *


class hr_router(RouterTemplate):
    def __init__(self):
        RouterTemplate.__init__(self)
        self._declareParams("params",["link_bw","flit_size","xbar_bw","input_latency","output_latency","input_buf_size","output_buf_size",
                                      "xbar_arb","network_inspectors","oql_track_port","oql_track_remote","num_vns","vn_remap","vn_remap_shm"])

        self._declareParams("params",["qos_settings"],"portcontrol.arbitration.")
        self._declareParams("params",["output_arb"],"portcontrol.")

        self._setCallbackOnWrite("qos_settings",self._qos_callback)
        self._subscribeToPlatformParamSet("router")


    def _qos_callback(self,variable_name,value):
        self._lockVariable(variable_name)
        if not self.output_arb: self.output_arb = "merlin.arb.output.qos.multi"
        
    def instanceRouter(self, name, radix, rtr_id):
        rtr = sst.Component(name, "merlin.hr_router")
        self._applyStatisticsSettings(rtr)
        rtr.addParams(self._getGroupParams("params"))
        rtr.addParam("num_ports",radix)
        rtr.addParam("id",rtr_id)
        return rtr
    
    def getTopologySlotName(self):
        return "topology"


class SystemEndpoint(Buildable):
    def __init__(self,system):
        Buildable.__init__(self)
        self._declareClassVariables(["_system"])
        self._system = system

    # build() returns an sst.Component and port name
    def build(self, nID, extraKeys):
        # Just get the proper job object for this nID and call build
        if self._system._endpoints[nID]:
            return self._system._endpoints[nID].build(nID, extraKeys)
        else:
            return (None, None)
