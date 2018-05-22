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

import componentConfig 
import myprint
import copy

def getOptions():
	return ['netPktSize=','nicVerboseLevel=','nicVerboseMask=']

class NicConfig(componentConfig.ComponentConfig):
	def __init__( self, params, opts, getNicParams = None ):
		self.params = params 
		self.getNicParams = getNicParams
		for o,a in opts:
			if o in ('--netPktSize'):
				self.params['packetSize'] = a
			elif o in ('--nicVerboseLevel'):
				self.params['verboseLevel'] = int( a )
			elif o in ('--nicVerboseMask'):
				self.params['verboseMask'] = int( a )

		#myprint.printParams( 'NicConfig:', self.params )

	def getParams( self, nodeNum, ranksPerNode ):
		extra = copy.deepcopy(self.params)
		extra["nid"] =  nodeNum	
		extra["num_vNics"] = ranksPerNode	

		if self.getNicParams:
			extra.update(self.getNicParams( nodeNum ) )
	
		return extra

	def getName( self, nodeNum ): 
		return "firefly.nic"
