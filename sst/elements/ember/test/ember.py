
import sst
from sst.merlin import *
 
import sys,getopt

iterations = 1;
msgSize = 0;
motif = "PingPong"
shape = "2"

def main():
    global iterations
    global msgSize
    global motif
    global shape
    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["msgSize=","iter=","motif=","shape="])
    except getopt.GetopError as err:
        print str(err)
        sys.exit(2)
    for o, a in opts:
        if o in ("--iter"):
            iterations = a
        elif o in ("--msgSize"):
            msgSize = a
        elif o in ("--motif"):
            motif = a
        elif o in ("--shape"):
            shape = a
        else:
            assert False, "unhandle option" 

main()

motif = "ember.Ember" + motif + "Generator"

def calcNumRanks( shape ):
    tmp = shape.split( 'x' )  
    num = 1
    for d in tmp:
        num = num * int(d)
    return num 

def calcNumDim( shape ):
    return len( shape.split( 'x' ) ) 

def calcWidth( shape ):
    tmp = len( shape.split( 'x' ) ) - 1
    retval = "1"
    count = 0
    while ( count < tmp ):
        retval += "x1" 
        count  += 1
    return retval 

numRanks = calcNumRanks( shape )
numDim = calcNumDim( shape )
width = calcWidth( shape )

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = "560Mhz"
sst.merlin._params["xbar_bw"] = "560Mhz"
sst.merlin._params["input_latency"] = "25ns"
sst.merlin._params["output_latency"] = "25ns"
sst.merlin._params["input_buf_size"] = 128
sst.merlin._params["output_buf_size"] = 128

sst.merlin._params["num_dims"] = numDim 
sst.merlin._params["torus:shape"] = shape 
sst.merlin._params["torus:width"] = width 
sst.merlin._params["torus:local_ports"] = 1

nicParams = ({ 
		"hermesParams.nicParams.debug" : 0,
		"hermesParams.nicParams.verboseLevel": 2,
		"hermesParams.nicParams.module" : "merlin.linkcontrol",
		"hermesParams.nicParams.topology" : "merlin.torus",
		"hermesParams.nicParams.num_vcs" : 2,
		"hermesParams.nicParams.link_bw" : "560Mhz",
		"hermesParams.nicParams.buffer_size" : 128,
		"hermesParams.nicParams.txBusDelay_ns" : 150,
		"hermesParams.nicParams.rxBusDelay_ns" : 150,
		"hermesParams.nicParams.rxMatchDelay_ns" : 100,
		"hermesParams.nicParams.txDelay_ns" : 100,
	})

driverParams = ({
		"debug" : 0,
		"verbose" : 2,
		"bufLen" : 8,
		"hermesModule" : "firefly.hades",
		"msgapi" : "firefly.hades",
		"printStats" : 1,
		"generator" : motif,
		"generatorParams.messagesize" : msgSize,
		"generatorParams.iterations" : iterations,
		"hermesParams.numRanks"  : numRanks,
		"hermesParams.nidListFile" : "nidlist.txt",
		"hermesParams.nicModule" : "firefly.nic",
		"hermesParams.policy" : "adjacent",
		"hermesParams.nodeParams.numCores" : 1,
		"hermesParams.nodeParams.coreNum" : 0,
		"hermesParams.functionSM.defaultDebug" : 0,
		"hermesParams.functionSM.defaultVerbose" : 1,
		"hermesParams.longMsgProtocol.shortMsgLength" : 40000,
		"hermesParams.longMsgProtocol.debug" : 0,
		"hermesParams.longMsgProtocol.verboseLevel" : 1,
		"hermesParams.longMsgProtocol.matchDelay_ps" : 0,
		"hermesParams.longMsgProtocol.memcpyDelay_ps" : 200,
		"hermesParams.longMsgProtocol.txDelay_ns" : 300,
		"hermesParams.longMsgProtocol.rxDelay_ns" : 300,
		"hermesParams.longMsgProtocol.regRegionBaseDelay_ps" : 10000000,
		"hermesParams.longMsgProtocol.regRegionPerByteDelay_ps" : 28,
		"hermesParams.longMsgProtocol.regRegionXoverLength" : 4096,
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

