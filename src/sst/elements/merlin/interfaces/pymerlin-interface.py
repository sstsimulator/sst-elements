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


class LinkControl(NetworkInterface):
    def __init__(self):
        NetworkInterface.__init__(self)
        self._declareParams("params",["link_bw","input_buf_size","output_buf_size","vn_remap"])
        self._subscribeToPlatformParamSet("network_interface")

    # returns subcomp, port_name
    def build(self,comp,slot,slot_num,job_id,job_size,logical_nid,use_nid_remap = False):
        if self._check_first_build():
            set_name = "params_%s"%self._instance_name
            sst.addGlobalParams(set_name, self._getGroupParams("params"))
            sst.addGlobalParam(set_name,"job_id",job_id)
            sst.addGlobalParam(set_name,"job_size",job_size)
            sst.addGlobalParam(set_name,"use_nid_remap",use_nid_remap)


        sub = comp.setSubComponent(slot,"merlin.linkcontrol",slot_num)
        self._applyStatisticsSettings(sub)
        sub.addGlobalParamSet("params_%s"%self._instance_name)
        sub.addParam("logical_nid",logical_nid)
        return sub,"rtr_port"


class ReorderLinkControl(NetworkInterface):
    def __init__(self):
        NetworkInterface.__init__(self)
        self._declareClassVariables(["network_interface"])
        self._setCallbackOnWrite("network_interface",self._network_interface_callback)

        self.network_interface = PlatformDefinition.getPlatformDefinedClassInstance("reorderlinkcontrol_network_interface")
        if not self.network_interface:
            self.network_interface = LinkControl()
            # This is just a default, can be overwritten
            self._unlockVariable("network_interface")

    def _network_interface_callback(self, variable_name, value):
        if not value: return
        self._lockVariable("network_interface")
        self._setPassthroughTarget(value)

    def setNetworkInterface(self,interface):
        self.network_interface = interface

    def build(self,comp,slot,slot_num,job_id,job_size,nid,use_nid_map = False):
        sub = comp.setSubComponent(slot,"merlin.reorderlinkcontrol",slot_num)
        #self._applyStatisticsSettings(sub)
        #sub.addParams(self._params)
        return self.network_interface.build(sub,"networkIF",0,job_id,job_size,nid,use_nid_map)

    # Functions to enable statistics
    def enableAllStatistics(self,stat_params,apply_to_children=False):
        # no stats of our own, simply pass to network interface
        if self.network_interface:
            self.network_interface.enableAllStatistics(stat_params,apply_to_children)

    def enableStatistics(self,stats,stat_params,apply_to_children=False):
        # no stats of our own, simply pass to network interface
        if self.network_interface:
            self.network_interface.enableStatistics(stats,stat_params,apply_to_children)

    def setStatisticLoadLevel(self,level,apply_to_children=False):
        # no stats of our own, simply pass to network interface
        if self.network_interface:
            self.network_intrface.setStatisticLoadLevel(level,apply_to_children)
