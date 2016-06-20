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

# numNodes = -1 implies use all nodes on network

numNodes = -1 
ranksPerNode = 1 

platform = 'defaultParams'

topo = 'torus'
shape = '2'

detailedNodes = [0]

detailedModel = "sandyBridgeModel" 
detailedModelParams = "sandyBridgeModelParams" 

arguments = 'messagesize=100000 printRank=-1' 

xxx = "Ring computeTime=10998000 " + arguments 
yyy = "DetailedRing " + arguments 

def genWorkFlow( defaults, nodeNum = None ):

	#print 'genWorkFlow()'

	workFlow = []
	motif = dict.copy( defaults )
	motif['cmd'] = "Init"
	workFlow.append( motif )

	motif = dict.copy( defaults )
	if nodeNum in detailedNodes:
		motif['cmd'] = yyy 
	else:
		motif['cmd'] = xxx
	workFlow.append( motif )

	motif = dict.copy( defaults )
	motif['cmd'] = "Fini"
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

def getDetailedModel():
    return detailedModel,detailedModelParams,detailedNodes
