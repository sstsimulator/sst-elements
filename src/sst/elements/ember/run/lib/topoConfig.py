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

import sys

from sst.merlin import *

from switch import *

def getOptions():
	return ['topo=', 'shape=']

def parseOptions(opts):
    topo = None
    shape = None
    for o,a in opts:
        if o in('--topo'):
            topo = a
        elif o in('--shape'):
            shape = a

    if None == topo:
        sys.exit('FATAL: must specify --topo=[torus|fattree|dragonfly|dragonfly2]')
    if None == shape:
        sys.exit('FATAL: must specify --shape')

    return topo, shape

class TopoInfo:
	def getNumNodes(self):
		pass
	def getParams(self):
		pass

class TorusInfo(TopoInfo):
	def __init__( self, config ):

		if not config:
			sys.exit('FATAL: need to topology specify shape' )

		args = config.split(':')
		shape = args[0]
		width = 1
		local_ports = 1

		if len( args ) > 1:
			local_ports = int( args[1] )

		if len( args ) > 2:
			width = int( args[2] )
		
		self.params = {}
		self.params["num_dims"] = self.calcNumDim(shape)
		self.params["torus:shape"] = shape
		self.params["torus:width"] = self.calcWidth(shape,width)
		self.params["torus:local_ports"] = local_ports 
		self.numNodes = self.calcNumNodes( shape ) * local_ports

	def getParams(self):
		return self.params

	def getNumNodes(self):
		return self.numNodes

	def calcNumDim(self,shape):
		return len( shape.split( 'x' ) )

	def calcNumNodes(self,shape):
		tmp = shape.split( 'x' )
		num = 1
		for d in tmp:
			num = num * int(d)
		return num

	def calcWidth(self,shape,width):
		tmp = len( shape.split( 'x' ) ) - 1
		retval = str(width) 
		count = 0
		while ( count < tmp ):
			retval += "x" + str(width)
			count  += 1
		return retval


class FattreeInfo(TopoInfo):

	def __init__( self, shape ):

		if not shape:
			sys.exit('FATAL: need to topology specify shape' )

		self.params = {}
		self.numNodes = self.calcNumNodes(shape)
		self.params["fattree:shape"] = shape
                
	def getParams(self):
		return self.params

	def getNumNodes(self):
		return self.numNodes 

        def calcNumNodes(self, shape):
                levels = shape.split(":")

                total_hosts = 1;
                for l in levels:
                        links = l.split(",")
                        total_hosts = total_hosts * int(links[0])

                return total_hosts

class DragonFlyInfo(TopoInfo):
	def __init__( self, shape ):

		if not shape:
			sys.exit('FATAL: need to topology specify shape' )

		radix, lcl, glbl, nRtrs = shape.split(':')
		self.params = {}
		hostsPerGroup = int(nRtrs) * int(lcl)
		nGrp = int(nRtrs) * int(glbl) + 1
		self.params["router_radix"] = radix
		self.params["dragonfly:shape"] = "" 
		self.params["dragonfly:hosts_per_router"] = lcl
		self.params["dragonfly:routers_per_group"] = nRtrs
		self.params["dragonfly:intergroup_per_router"] = glbl
		self.params["dragonfly:num_groups"] =  nGrp
		self.params["dragonfly:algorithm"] =  "minimal" 

		self.numNodes = nGrp * hostsPerGroup 
                
	def getParams(self):
		return self.params

	def getNumNodes(self):
		return self.numNodes 

class DragonFly2Info(TopoInfo):
	def __init__( self, shape ):

		if not shape:
			sys.exit('FATAL: need to topology specify shape' )

		lcl, nRtrs, glbl, nGrp = shape.split(':')
		self.params = {}
		hostsPerGroup = int(nRtrs) * int(lcl)
		self.params["dragonfly:shape"] = "" 
		self.params["dragonfly:hosts_per_router"] = lcl
		self.params["dragonfly:routers_per_group"] = nRtrs
		self.params["dragonfly:intergroup_links"] = glbl
		self.params["dragonfly:num_groups"] =  nGrp
		self.params["dragonfly:algorithm"] =  "minimal" 

                print lcl
                print nRtrs
                print glbl
                print nGrp
                
		self.numNodes = int(nGrp) * hostsPerGroup
                print self.numNodes
                
	def getParams(self):
		return self.params

	def getNumNodes(self):
		return self.numNodes 

def getTopoObj( topo ):
	for case in switch(topo):
		if case('torus'):
			return topoTorus()
		if case('fattree'):
			return topoFatTree()
		if case('dragonfly'):
			return topoDrgonFly()
		if case('dragonfly2'):
			return topoDrgonFly2()
			
	sys.exit("how did we get here")

def getTopoInfo( topo, shape ):
	for case in switch(topo):
		if case('torus'):
			return TorusInfo(shape)   
		if case('fattree'):
			return FattreeInfo(shape)   
		if case('dragonfly'):
			return DragonFlyInfo(shape)   
		if case('dragonfly2'):
			return DragonFly2Info(shape)   
			
	sys.exit("how did we get here")
