#!/usr/bin/env python
#
# Copyright 2009-2015 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2015, Sandia Corporation
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

class DetailedModel:
    def getName(self):
        pass
    def build(self,nodeID,numCores):
        pass 
    def getThreadLink(self,core):
        pass
    def getNicLink(self):
        pass
