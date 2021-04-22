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

class TargetGenerator(TemplateBase):
    def __init__(self):
        TemplateBase.__init__(self)
        self._declareParams("params",[])

    # Functions used if instancing as AnonymousSubcomponent
    def getTypeName(self):
        pass

    # If the child puts their parameters in the "params" group, then
    # they won't have to implement this function themselves
    def addAsAnonymous(self, comp, type_key, prefix = ""):
        comp.addParam(type_key,self.getTypeName())
        params = self._getGroupParams("params");
        for (key,value) in params.items():
            comp.addParam(prefix + key, value)
        
    # Function used if instancing as UserSubComponent
    # If the child puts their parameters in the "params" group, then
    # they won't have to implement this function themselves
    def build(self, parent, slot, slot_num = 0):
        subcomp = parent.setSubComponent(slot,self.getTypeName(), slot_num)
        subcomp.addParams(self._getGroupParams("params"))
        return subcomp


class UniformTarget(TargetGenerator):
    def __init__(self):
        TargetGenerator.__init__(self)

    def getTypeName(self):
        return "merlin.targetgen.uniform"


class BitComplementTarget(TargetGenerator):
    def __init__(self):
        TargetGenerator.__init__(self)

    def getTypeName(self):
        return "merlin.targetgen.bit_complement"


class ShiftTarget(TargetGenerator):
    def __init__(self):
        TargetGenerator.__init__(self)
        self._declareParams("params",["shift"])

    def getTypeName(self):
        return "merlin.targetgen.shift"
