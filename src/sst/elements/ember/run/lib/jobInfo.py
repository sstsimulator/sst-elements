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

import copy,pprint,sys

import detailedModel

pp = pprint.PrettyPrinter(indent=4)

motifDefaults = {
    'cmd' : "",
    'api': "HadesMP",
    'spyplotmode': 0
}

def getOptions():
	return [ "cmdLine=","numNodes=", "numRanksPerNode=","randomLoad"]


def parseOptions(opts):

    random = False
    numNodes = -1
    ranksPerNode = 1
    motifs = []

    for o,a in opts:
        if o in ('--randomLoad'):
			random = True
        elif o in ('--numNodes'):
            numNodes = int( a )
        elif o in ('--numRanksPerNode'):
            ranksPerNode = int( a )
        elif o in ('--cmdLine'):
            motif = {}
            motif['cmd'] = a
            motifs += [ motif ]

    if 0 == len(motifs):
        sys.exit('FATAL: must specify --cmdLine')

    return numNodes, ranksPerNode, motifs, random

class JobInfoBase:
	def __init__(self, jobId, numNodes, ranksPerNode ): 
		self._jobId = jobId 
		self._numNodes = numNodes 
		self._ranksPerNode = ranksPerNode 
		self._nidlist = None
		self._detailedModel = None 
		self._randomLoad = False

	def genWorkFlow( self, nodeNum ):
		pass

	def getNidlist(self):
		return self._nidlist

	def setNidList(self,nidlist):
		self._nidlist = nidlist

	def ranksPerNode(self):
		return self._ranksPerNode

	def getNumNodes(self):
		return self._numNodes

	def jobId(self):
		return self._jobId

	def printWork(self):
		pass

	def getDetailed(self,nodeId):
		#print 'JobLoad::getDetailded()',nodeId
		if self._detailedModel:
			model,params,nodes = self._detailedModel
			if nodeId in nodes:
				return detailedModel.getModel( model, params )
			else:
				return None
		else:
			return None

	def setDetailed(self,detailed):
		self._detailedModel = detailed

	def setRandom(self):
		self._randomLoad = True

	def getRandom(self):
		return self._randomLoad

	def printInfo(self):
		print 'JobInfo: jobId={0} numNodes={1} numRanksPerNode={2}'.\
			format( self.jobId(), self.getNumNodes(), self.ranksPerNode() )		
		print 'JobInfo: nidList="{0}"'.format( self.getNidlist() ) 
		if self._detailedModel:
			print 'JobInfo: detailed model {0}'.format(self._detailedModel)
		self.printWork()

class JobInfoCmd(JobInfoBase):
	def __init__(self, jobId, numNodes, ranksPerNode, motifs, defaults = motifDefaults ): 
		JobInfoBase.__init__( self, jobId, numNodes, ranksPerNode )
		self._motifs = []

		for motif in motifs: 
			tmp = copy.deepcopy( defaults )
 			tmp.update( motif )
			self._motifs += [ tmp ]	

	def printWork(self):
		for cmd in self._motifs:
			print 'JobInfo:    cmdLine: "{0}"'.format(cmd['cmd'])
	
	def genWorkFlow( self, nodeNum ):
		return self._motifs

class JobInfo(JobInfoBase):

	def __init__(self, jobId, numNodes, ranksPerNode, workflow, defaults = motifDefaults ): 
		JobInfoBase.__init__( self, jobId, numNodes, ranksPerNode )
		self._genWorkFlow = workflow
		self._motifDefaults =  defaults

	def printWork(self):
		motifs = self._genWorkFlow( self._motifDefaults, 0 )
		print 'JobInfo: showing work for node 0 only '
		for cmd in motifs:
			print 'JobInfo:    cmdLine: "{0}"'.format(cmd['cmd'])

	def genWorkFlow( self, nodeNum ):
		return self._genWorkFlow( self._motifDefaults, nodeNum )


