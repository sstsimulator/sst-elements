
import sys

import componentConfig 
import pprint
import copy
from loadUtils import *

pp = pprint.PrettyPrinter(indent=4)

className = 'EmberConfig' 
class EmberConfig(componentConfig.ComponentConfig):
	def __init__( self, emberParams, hermesParams, jobInfo ):

		print className + '()'

		self.params = emberParams 
		self.params.update(hermesParams)
		self.jobInfo = jobInfo

	def getParams( self, nodeNum ):
		print className + '::getParams()'
		extra = copy.copy(self.params)

		extra['jobId'] = self.jobInfo.jobId() 
		extra['hermesParams.netId'] = nodeNum
		extra['hermesParams.netMapId'] = calcNetMapId( nodeNum, self.jobInfo.nidlist() )
		extra['hermesParams.netMapSize'] = calcNetMapSize( self.jobInfo.nidlist() ) 
	
		tmp = getMotifParams( self.jobInfo.genWorkFlow( nodeNum ) )

		extra.update( tmp )

		return extra

	def getNidList(self):
		return self.jobInfo.nidlist()

	def getName( self, nodeNum ): 
		return "ember.EmberEngine"

	def getNumRanks( self ):
		return self.jobInfo.ranksPerNode()

	def getDetailed( self, nodeId ):
        #if detailed:
        #    detailed = self.detailedModel.build( nodeID, self.ranksPerNode )
		return None

      #      for id in self.statNodes:
      #          if nodeID == id:
      #              print "printStats for node {0}".format(id)
      #              ep.addParams( {'motif1.printStats': 1} )
