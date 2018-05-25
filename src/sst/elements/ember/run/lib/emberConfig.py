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

import sys, pprint, copy

import componentConfig 
import loadUtils
import hermesConfig

pp = pprint.PrettyPrinter(indent=4)

def getOptions():
	return ['emberVerbose=','emberVerboseNode=','emberMotifLogFile=',
			'emberMotifLogNode=','emberRankmapper='] \
					+ hermesConfig.getOptions()

def parseOptions(opts):
	level = 0
	nodeList = [] 
	motifLogFile = None
	motifLogFileNodeList = []
	rankmapper = None

	if opts:	
		for  o,a in opts:
			if o in('--emberVerbose'):
				level = int(a)
			elif o in('--emberVerboseNode'):
				nodeList += [ int(a) ]
			elif o in('--emberMotifLogFile'):
				motifLogFile =a 
			elif o in('--emberMotifLogNode'):
				motifLogFileNodeList += [int(a)] 
			elif o in('--emberRankMapper'):
				rankmapper = a 
	
	return level, nodeList, motifLogFile, motifLogFileNodeList, rankmapper


className = 'EmberConfig' 

class EmberConfig(componentConfig.ComponentConfig):
	def __init__( self, emberParams, hermesParams, jobInfo, opts = None ):

		#print className + '()'

		self.params = emberParams 
		self.params.update(hermesParams)
		self.jobInfo = jobInfo

		self.debugLevel, \
		self.debugNodeList, \
		self.motifLogFile, \
		self.motifLogNodeList, \
		self.params['ranksmapper'] = parseOptions(opts) 

		self.hermesLevel = hermesConfig.parseOptions( opts )

	def getParams( self, nodeNum ):
		#print className + '::getParams()'

		extra = copy.deepcopy(self.params)

		if 0 == len( self.debugNodeList ) or nodeNum in self.debugNodeList: 
			if self.debugLevel:	
				extra['verbose'] = self.debugLevel 
			if self.hermesLevel:	
				extra['hermesParams.ctrlMsg.verboseLevel'] = self.hermesLevel

		if self.motifLogFile and nodeNum in self.motifLogNodeList:
			extra['motifLog' ] = self.motifLogFile + '-' + str(nodeNum)

		extra['jobId'] = self.jobInfo.jobId() 
		extra['hermesParams.netId'] = nodeNum
		extra['hermesParams.netMapId'] = \
			loadUtils.calcNetMapId( nodeNum, self.jobInfo.getNidlist() )
		extra['hermesParams.netMapSize'] = \
			loadUtils.calcNetMapSize( self.jobInfo.getNidlist() ) 
	
		extra.update( loadUtils.getMotifParams( self.jobInfo.genWorkFlow( nodeNum ) ) )

		return extra

	def getNidList(self):
		return self.jobInfo.getNidlist()

	def getName( self, nodeNum ): 
		return "ember.EmberEngine"

	def getNumRanks( self ):
		return self.jobInfo.ranksPerNode()

	def getDetailed( self, nodeId ):
		return self.jobInfo.getDetailed( nodeId ) 
