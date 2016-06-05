
import componentConfig 
import pprint
import copy

pp = pprint.PrettyPrinter(indent=4)

class NicConfig(componentConfig.ComponentConfig):
	def __init__( self, params ):
		self.params = params 

	def getParams( self, nodeNum, ranksPerNode ):
		extra = copy.copy(self.params)
		extra["nid"] =  nodeNum	
		extra["num_vNics"] = ranksPerNode	
	
		return extra

	def getName( self, nodeNum ): 
		return "firefly.nic"
