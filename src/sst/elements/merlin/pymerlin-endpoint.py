#!/usr/bin/env python
#
# Copyright 2009-2016 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2016, Sandia Corporation
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys
import sst
from sst.merlin.core import *


class TestEndPoint(EndPoint):
    def __init__(self):
        EndPoint.__init__(self)
        #self.enableAllStats = False;
        #self.statInterval = "0"
        #self.nicKeys = ["topology", "num_peers", "num_messages", "link_bw", "checkerboard"]
        self.epKeys.extend(["link_bw", "link_lat"])
        #self.epOptKeys.extend(["checkerboard", "num_messages"])

    def getName(self):
        return "TestEndPoint"

    def build(self, nID, extraKeys):
        nic = sst.Component("testNic.%d"%nID, "merlin.test_nic")
        nic.addParams(self.params)
        nic.addParams(extraKeys)
        nic.addParam("id", nID)
        if self.enableAllStats:
            nic.enableAllStatistics({"type":"sst.AccumulatorStatistic","rate":self.statInterval})
        return (nic, "rtr", self.params["link_lat"])
        #print "Created Endpoint with id: %d, and params: %s %s\n"%(nID, _params.subset(self.nicKeys), _params.subset(extraKeys))

    def enableAllStatistics(self,interval):
        self.enableAllStats = True;
        self.statInterval = interval;



class OfferedLoadEndPoint(EndPoint):
    def __init__(self):
        EndPoint.__init__(self)
        #self.enableAllStats = False;
        #self.statInterval = "0"
        self.epKeys.extend(["offered_load", "num_peers", "link_bw", "message_size", "buffer_size", "pattern"])
        #self.epOptKeys.extend(["linkcontrol"])

    def getName(self):
        return "Offered Load End Point"

    def prepParams(self):
        pass
        
    def build(self, nID, extraKeys):
        nic = sst.Component("offered_load.%d"%nID, "merlin.offered_load")
        nic.addParams(self.params)
        nic.addParams(extraKeys)
        nic.addParams("id",nID)        
        if self.enableAllStats:
            nic.enableAllStatistics({"type":"sst.AccumulatorStatistic", "rate":self.statInterval})
        return (nic, "rtr", self.params["link_lat"])
        #print "Created Endpoint with id: %d, and params: %s %s\n"%(nID, _params.subset(self.nicKeys), _params.subset(extraKeys))

    def enableAllStatistics(self,interval):
        self.enableAllStats = True;
        self.statInterval = interval;



        
