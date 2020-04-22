#!/usr/bin/env python
#
# Copyright 2009-2020 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2020, NTESS
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

class TestJob(Job):
    def __init__(self,job_id,size):
        Job.__init__(self,job_id,size)
        self._defineRequiredParams(["num_peers"])
        self.num_peers = size
        self._defineOptionalParams(["num_messages","message_size","send_untimed_bcast"])

    def getName(self):
        return "TestJob"

    def build(self, nID, extraKeys):
        nic = sst.Component("testNic.%d"%nID, "merlin.test_nic")
        self._applyStatisticsSettings(nic)
        nic.addParams(self._params)
        nic.addParams(extraKeys)
        # Get the logical node id
        id = self._nid_map.index(nID)
        nic.addParam("id", id)

        #  Add the linkcontrol
        networkif, port_name = self.network_interface.build(nic,"networkIF",0,self.job_id,self.size,id,True)

        return (networkif,port_name)


class OfferedLoadJob(Job):
    def __init__(self,job_id,size):
        Job.__init__(self,job_id,size)
        self._defineRequiredParams(["offered_load","pattern","num_peers","message_size","link_bw"])
        self.num_peers = size
        self._defineOptionalParams(["warmup_time","collect_time","drain_time"])

    def getName(self):
        return "Offered Load Job"

    def build(self, nID, extraKeys):
        nic = sst.Component("offered_load.%d"%nID, "merlin.offered_load")
        self._applyStatisticsSettings(nic)
        nic.addParams(self._params)
        nic.addParams(extraKeys)
        id = self._nid_map.index(nID)
        nic.addParam("id", id)

        return (networkif, portname)


