import sys

class TopoInfo:
	def getNumNodes(self):
		pass
	def getNetworkParams(self):
		pass

class TorusInfo(TopoInfo):
	def __init__( self, shape ):
		self.params = {}
		self.params["num_dims"] = self.calcNumDim(shape)
		self.params["torus:shape"] = shape
		self.params["torus:width"] = self.calcWidth(shape)
		self.params["torus:local_ports"] = 1
		self.numNodes = self.calcNumNodes( shape )

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

	def calcWidth(self,shape):
		tmp = len( shape.split( 'x' ) ) - 1
		retval = "1"
		count = 0
		while ( count < tmp ):
			retval += "x1"
			count  += 1
		return retval


class FattreeInfo(TopoInfo):
	def __init__( self, radix, loading, shape  ):
		self.params = {}
                my_shape = shape
		if ( "" == my_shape ):
                        my_radix = int(radix)
			my_shape = "%s,%d:%d,%d:%d"%(loading,my_radix/2,my_radix/2,my_radix/2,my_radix)
#                        self.params["router_radix"] = int(radix) 
#                        self.params["fattree:hosts_per_edge_rtr"] = int(loading) 
#                        self.numNodes = int(radix) * int(radix)/2 * int(loading)
		self.numNodes = self.calcNumNodes(my_shape)
		self.params["fattree:shape"] = my_shape
                
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
