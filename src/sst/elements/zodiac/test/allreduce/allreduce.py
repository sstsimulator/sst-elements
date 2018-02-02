
import sst
from sst.merlin import *
 
import sys,getopt

iterations = 1;
msgSize = 0;
shape = "2"
num_vNics = 1

netPktSizeBytes="64B"
netFlitSize="8B"

def main():
    global iterations
    global msgSize
    global shape
    global num_vNics
    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["msgSize=","iter=","shape=","numCores="])
    except getopt.GetopError as err:
        print str(err)
        sys.exit(2)
    for o, a in opts:
        if o in ("--iter"):
            iterations = a
        elif o in ("--msgSize"):
            msgSize = a
        elif o in ("--numCores"):
            num_vNics = a
        elif o in ("--shape"):
            shape = a
        else:
            assert False, "unhandle option" 

main()


def calcNumNodes( shape ):
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

numNodes = calcNumNodes( shape )
numDim = calcNumDim( shape )
width = calcWidth( shape )
numRanks = numNodes * num_vNics

print numNodes
print numRanks

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = "4GB/s"
sst.merlin._params["xbar_bw"] = "4GB/s"
sst.merlin._params["input_latency"] = "25ns"
sst.merlin._params["output_latency"] = "25ns"
sst.merlin._params["input_buf_size"] = "1Kb"
sst.merlin._params["output_buf_size"] = "1KB"
sst.merlin._params["flit_size"] = netFlitSize

sst.merlin._params["num_dims"] = numDim 
sst.merlin._params["torus:shape"] = shape 
sst.merlin._params["torus:width"] = width 
sst.merlin._params["torus:local_ports"] = 1

nicParams = ({ 
		"debug" : 0,
		"verboseLevel": 2,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin.torus",
		"link_bw" : "4GB/s",
		"input_buf_size" : "1KB",
		"output_buf_size" : "1KB",
		"packetSize" : netPktSizeBytes,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 100,
        "num_vNics" : num_vNics,
	})

driverParams = ({
		"debug" : 1,
		"verbose" : 1,
		"bufLen" : 8,
		"hermesModule" : "firefly.hades",
		"os.module" : "firefly.hades",
		"trace" : "npe-" + str(numRanks) + "/allred-" + str(numRanks) +".stf",
		"sharedTrace" : "allred-128.stf",
		"printStats" : 1,
		"buffersize" : 140,
		"os.name" : "hermesParams",
		"hermesParams.debug" : 0,
		"hermesParams.verboseLevel" : 1,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : 0,
		"hermesParams.nicParams.debugLevel" : 1 ,
        "hermesParams.functionSM.defaultEnterLatency" : 30000,
        "hermesParams.functionSM.defaultReturnLatency" : 30000,
        "hermesParams.functionSM.defaultDebug" : debug,
        "hermesParams.functionSM.defaultVerbose" : 2,
        "hermesParams.ctrlMsg.debug" : debug,
        "hermesParams.ctrlMsg.verboseLevel" : 2,
        "hermesParams.ctrlMsg.shortMsgLength" : 12000,
        "hermesParams.ctrlMsg.matchDelay_ns" : 150,

        "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-:130ns",

        "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-:100ns",

        "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
        "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-:344ps",

        "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.LatencyMod",
        "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
        "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-:344ps",

        "hermesParams.ctrlMsg.txNicDelay_ns" : 0,
        "hermesParams.ctrlMsg.rxNicDelay_ns" : 0,
        "hermesParams.ctrlMsg.sendReqFiniDelay_ns" : 0,
        "hermesParams.ctrlMsg.sendAckDelay_ns" : 0,
        "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 3000,
        "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 100,
        "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
        "hermesParams.loadMap.0.start" : 0,
        "hermesParams.loadMap.0.len" : 2,

	})

class EmberEP(EndPoint):
	def getName(self):
		return "EmberEP"
	def prepParams(self):
		pass
	def build(self, nodeID, extraKeys):
		num_vNics = int(nicParams["num_vNics"])
		nic = sst.Component("nic" + str(nodeID), "firefly.nic")
		nic.addParams(nicParams)
		nic.addParam("nid", nodeID)

		loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
		loopBack.addParam("numCores", num_vNics)

		for x in xrange(num_vNics ):
			ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_TraceReader", "zodiac.ZodiacSiriusTraceReader")
			ep.addParams(driverParams)
			ep.addParam('hermesParams.netId', nodeID )
			ep.addParam('hermesParams.netMapSize', numRanks )
			ep.addParam('hermesParams.netMapName', "NetMap" )
            
			nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )
			loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) + "_Link"  )
			ep.addLink(nicLink, "nic", "150ns")
			nic.addLink(nicLink, "core" + str(x), "150ns")
            
			ep.addLink(loopLink, "loop", "1ns")
			loopBack.addLink(loopLink, "core" + str(x), "1ns")
		return (nic, "rtr", "10ns")


topo = topoTorus()
topo.prepParams()
endPoint = EmberEP()
endPoint.prepParams()

topo.setEndPoint(endPoint)
topo.build()

