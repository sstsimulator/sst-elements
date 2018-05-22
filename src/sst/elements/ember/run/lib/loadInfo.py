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

import pprint

from loadUtils import *
from emberEP import *

pp = pprint.PrettyPrinter(indent=4)

class LoadInfo:

	def __init__(self, nicConfig, numNics, nullEmber = None ):
		self.nicConfig = nicConfig
		self.numNics = numNics
		self.map = []
		self.nullEmber = nullEmber 

	def addEmberConfig( self, emberConfig ):
		self.map.append( emberConfig )

	def inRange( self, nid, start, end ):
		if nid >= start:
			if nid <= end:
				return True
		return False

	def setNode(self,nodeId):

		#print "setNode() nodeId={0}".format( nodeId )
		for ep in self.map:
			x = ep.getNidList().split(',')
			for y in x:	
				tmp = y.split('-')

				if 1 == len(tmp):
					if nodeId == int( tmp[0] ):
						return EmberEP( self.nicConfig, ep  )  
				else:
					if self.inRange( nodeId, int(tmp[0]), int(tmp[1]) ):
						return EmberEP( self.nicConfig, ep  )  

		if self.nullEmber:
			return EmberEP( self.nicConfig, self.nullEmber )
		else:
			sys.exit("FATAL: node {0} doesn't have a config".format(nodeId))
