#!/usr/bin/env python
#
# Copyright 2009-2015 Sandia Corporation. Under the terms
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2015, Sandia Corporation
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
shape = '2'

# what nodes have the detailed models
detailedNodes = [0]

# what nodes run the detailed motif
detailedMotifs = [0]

# what nodes use the detailed model
detailedNics = [0]

detailedModel = "sandyBridgeModel"
detailedModelParams = "sandyBridgeModelParams"
#detailedModel = "basicDetailedModel" 
#detailedModelParams = "basicDetailedModelParams" 

arguments = 'messagesize=80000 printRank=-1'

detailedMotif = "DetailedRing " + arguments
nonDetailedMotif = "Ring " + arguments

def genWorkFlow( defaults, nodeNum = None ):

	#print 'genWorkFlow()'

	workFlow = []
	motif = dict.copy( defaults )
	motif['cmd'] = "Init"
	workFlow.append( motif )

	motif = dict.copy( defaults )
	if nodeNum in detailedMotifs:
		motif['cmd'] = detailedMotif 
	else:
		motif['cmd'] = nonDetailedMotif 
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

def getPerNicParams(nodeNum):
	params = {}
	if nodeNum in detailedNics: 
		params['useDetailed'] = 'True'

	return params 

def getDetailedModel():
    return detailedModel,detailedModelParams,detailedNodes
