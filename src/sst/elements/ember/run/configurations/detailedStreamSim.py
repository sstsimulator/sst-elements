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

# numNodes = -1 implies use all nodes on network

import pprint
pp = pprint.PrettyPrinter(indent=4)

numNodes = -1
ranksPerNode = 1

platform = 'defaultParams'

topo = 'torus'
shape = '1'

# what nodes have the detailed models
detailedNodes = [0]

detailedModel = "sandyBridgeModel"
detailedModelParams = "sandyBridgeModelParams"
#detailedModel = "basicDetailedModel"
#detailedModelParams = "basicDetailedModelParams"
#detailedModel = "3LevelModel"
#detailedModelParams = "3LevelModelParams"

arguments = 'stream_n=1024 operandwidth=32'

detailedMotif = "DetailedStream " + arguments

def genWorkFlow( defaults, nodeNum = None ):

	workFlow = []

	motif = dict.copy( defaults )
	motif['cmd'] = detailedMotif
	workFlow.append( motif )

	return workFlow

def getNumNodes():
	return numNodes

def getRanksPerNode():
	return ranksPerNode

def getTopo():
	return topo, shape

def getPlatform():
	return platform

def getPerNicParams(nodeNum):
	return {}

def getDetailedModel():
    return detailedModel,detailedModelParams,detailedNodes
