import sys

class TopoInfo:
	def getNumNodes(self):
		pass
	def getNetworkParams(self):
		pass

class TorusInfo(TopoInfo):
	def __init__( self, shape, local_ports ):

		width = 1

		self.params = {}
		self.params["num_dims"] = self.calcNumDim(shape)
		self.params["torus:shape"] = shape
		self.params["torus:width"] = self.calcWidth(shape,width)
		self.params["torus:local_ports"] = local_ports 
		self.numNodes = self.calcNumNodes( shape ) * local_ports

	def getNetworkParams(self):
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

class HyperXInfo(TopoInfo):
    def __init__( self, shape, local_ports ):

        width = 1

        self.params = {}
        self.params["num_dims"] = self.calcNumDim(shape)
        self.params["hyperx:shape"] = shape
        self.params["hyperx:width"] = self.calcWidth(shape,width)
        self.params["hyperx:local_ports"] = local_ports
        self.numNodes = self.calcNumNodes( shape ) * local_ports

    def getNetworkParams(self):
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
		self.params = {}
		self.numNodes = self.calcNumNodes(shape)
		self.params["fattree:shape"] = shape
                
	def getNetworkParams(self):
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

class DragonFlyLegacyInfo(TopoInfo):
	def __init__( self, shape ):
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
                
	def getNetworkParams(self):
		return self.params

	def getNumNodes(self):
		return self.numNodes 

class DragonFlyInfo(TopoInfo):
	def __init__( self, shape ):
		lcl, nRtrs, glbl, nGrp = shape.split(':')
		self.params = {}
		hostsPerGroup = int(nRtrs) * int(lcl)
		self.params["dragonfly:shape"] = "" 
		self.params["dragonfly:hosts_per_router"] = lcl
		self.params["dragonfly:routers_per_group"] = nRtrs
		self.params["dragonfly:intergroup_links"] = glbl
		self.params["dragonfly:num_groups"] =  nGrp
		self.params["dragonfly:algorithm"] =  "minimal" 

		self.numNodes = int(nGrp) * hostsPerGroup
                
	def getNetworkParams(self):
		return self.params

	def getNumNodes(self):
		return self.numNodes 
