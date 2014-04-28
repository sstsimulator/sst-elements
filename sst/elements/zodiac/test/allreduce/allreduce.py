
import sst
from sst.merlin import *
 
import sys,getopt

iterations = 1;
msgSize = 0;
shape = "2"
num_vNics = 1

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
		"debug" : 0,
		"verboseLevel": 2,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin.torus",
		"num_vcs" : 2,
		"link_bw" : "560Mhz",
		"buffer_size" : 128,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 100,
        "num_vNics" : num_vNics,
	})

driverParams = ({
		"debug" : 1,
		"verbose" : 1,
		"bufLen" : 8,
		"hermesModule" : "firefly.hades",
		"msgapi" : "firefly.hades",
		"trace" : "npe-" + str(numRanks) + "/allred-" + str(numRanks) +".stf",
		"sharedTrace" : "allred-128.stf",
		"printStats" : 1,
		"buffersize" : 140,
		"hermesParams.debug" : 0,
		"hermesParams.verboseLevel" : 1,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : 0,
		"hermesParams.nicParams.debugLevel" : 1 ,
		"hermesParams.functionSM.defaultDebug" : 0,
		"hermesParams.functionSM.defaultVerbose" : 1,
		"hermesParams.ctrlMsg.debug" : 0,
		"hermesParams.ctrlMsg.verboseLevel" : 1,
		"hermesParams.longMsgProtocol.shortMsgLength" : 4000,
		"hermesParams.longMsgProtocol.debug" : 0,
		"hermesParams.longMsgProtocol.verboseLevel" : 1,
		"hermesParams.longMsgProtocol.matchDelay_ps" : 1,
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
		num_vNics = int(nicParams["num_vNics"])
		nic = sst.Component("nic" + str(nodeID), "firefly.nic")
		nic.addParams(nicParams)
		nic.addParam("nid", nodeID)
		nic.addLink(link, "rtr", "10ns")

		loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
		loopBack.addParam("numCores", num_vNics)

		for x in xrange(num_vNics ):
			ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_TraceReader", "zodiac.ZodiacSiriusTraceReader")
			ep.addParams(driverParams)
			nidList = "0:" + str(numRanks);
			ep.addParam("hermesParams.nidListString", nidList )
			nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )
			loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) + "_Link"  )
			ep.addLink(nicLink, "nic", "150ns")
			nic.addLink(nicLink, "core" + str(x), "150ns")
            
			ep.addLink(loopLink, "loop", "1ns")
			loopBack.addLink(loopLink, "core" + str(x), "1ns")


topo = topoTorus()
topo.prepParams()
endPoint = EmberEP()
endPoint.prepParams()

topo.setEndPoint(endPoint)
topo.build()

