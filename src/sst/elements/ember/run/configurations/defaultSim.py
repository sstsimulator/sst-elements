#!/usr/bin/env python
#
# Copyright 2009-2018 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2018, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

# numNodes = 0 implies use all nodes on network
numNodes = 2 
ranksPerNode = 1 

#platform = 'chamaPSM'
#platform = 'chamaOpenIB'
#platform = 'bgq'
platform = 'defaultParams'

#topo = ''
#shape = ''
topo = 'torus'
shape = '4'

detailedNodes = []

detailedModel = "" 
detailedModelParams = "" 
xxx = "Sweep3D nx=30 ny=30 nz=30 computetime=140 pex=4 pey=16 pez=0 kba=10"     
xxx = "Ring n=100"

def genWorkFlow( defaults, nodeNum = None ):

	#print 'genWorkFlow()'

	workFlow = []
	motif = dict.copy( defaults )
	motif['cmd'] = "Init"
	workFlow.append( motif )

	motif = dict.copy( defaults )
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
