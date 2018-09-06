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

import sst
#from sst.merlin import *

interactive_mode = False

class Params(dict):
    def __init__(self, name):
        self.name = name
        
    def verifyParamsExist(self, keys):
        notfound = list()
        for x in keys:
            if x not in self:
                notfound.append(x)

                
        if len(notfound) is not 0:
            if interactive_mode:
                print ""
                print "Please provide missing required parameters for %s:"%self.name
                for key in notfound:
                    print "%s: "%key
                    val = raw_input()
                    self[key] = val
            else:
                print ""
                print "ERROR: Missing required paramters in %s:"%self.name
                for key in notfound:
                    print key
                print ""
                sys.exit(1);
                

class Topology:
    def __init__(self):
        self.topoKeys = []
        self.topoOptKeys = []
        self.params = Params(self.getTopologyName())
        self.endPointLinks = []
        self.built = False
    def getTopologyName(self):
        return "NoName"
#    def prepParams(self):
#        pass
    def addParams(self,p):
        self.params.update(p)
    def addParam(self, key, value):
        self.params[key] = value
    def build(self, network_name, endpoint):
        pass
    def buildWithOutEndPoint(self, network_name):
        pass
    def getEndPointLinks(self):
        pass


class EndPoint:
    def __init__(self):
        self.epKeys = []
        self.epOptKeys = []
        self.enableAllStats = False
        self.statInterval = "0"
        self.params = Params(self.getName())
    def getName(self):
        print "Not implemented"
        sys.exit(1)
    def verifyParams(self):
        self.params.verifyParamsExist(self.epKeys)
    def addParams(self,p):
        self.params.update(p)
    def addParam(self, key, value):
        self.params[key] = value
    def build(self, nID, extraKeys):
        return None


