
import sst
from sst.merlin import *

sst.merlin._params["link_lat"] = "10ns"
sst.merlin._params["link_bw"] = "1Ghz"
sst.merlin._params["xbar_bw"] = "1Ghz"

sst.merlin._params["num_dims"] = 3
sst.merlin._params["torus:shape"] = "2x2x2"
sst.merlin._params["torus:width"] = "1x1x1"
sst.merlin._params["torus:local_ports"] = 1 

nicParams = ({ 
		"hermesParams.nicParams.debug" : 0,
		"hermesParams.nicParams.verboseLevel": 2,
		"hermesParams.nicParams.module" : "merlin.linkcontrol",
		"hermesParams.nicParams.topology" : "merlin.torus",
		"hermesParams.nicParams.num_vcs" : 2,
		"hermesParams.nicParams.link_bw" : "1Ghz"
	})

driverParams = ({
		"debug" : 1,
		"verbose" : 2,
		"bufLen" : 8,
		"hermesModule" : "firefly.hades",
		"msgapi" : "firefly.hades",
		"printStats" : 1,
		"generator" : "ember.EmberPingPongGenerator",
		"hermesParams.debug" : 0,
		"hermesParams.verboseLevel" : 1,
		"hermesParams.numRanks"  : 8,
		"hermesParams.nidListFile" : "nidlist.txt",
		"hermesParams.nicModule" : "firefly.nic",
		"hermesParams.policy" : "adjacent",
		"hermesParams.nodeParams.numCores" : 1,
		"hermesParams.nodeParams.coreNum" : 0
	})

class EmberEP(EndPoint):
	def getName(self):
		return "EmberEP"
	def prepParams(self):
		pass
	def build(self, nodeID, link, extraKeys):
		ep = sst.Component("emberEP_" + str(nodeID), "ember.EmberEngine")
		ep.addParams(nicParams)
		ep.addParams(driverParams)
		ep.addParam("hermesParams.nicParams.nid", nodeID)
		ep.addLink(link, "rtr", "10ns")

topo = topoTorus()
topo.prepParams()
endPoint = EmberEP()
endPoint.prepParams()
topo.build(endPoint)

