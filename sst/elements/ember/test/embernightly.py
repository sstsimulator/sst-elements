
import sst
from sst.merlin import *
 
import sys,getopt

iterations = 1;
msgSize = 1024;
motif = "Halo2D"
shape = "64x64"
num_vNics = 1
debug = 0

netPktSizeBytes="64B"
netFlitSize="8B"

def main():
    global iterations
    global msgSize
    global motif
    global shape
    global num_vNics
    global debug
    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["msgSize=","iter=","motif=","shape=","debug=","numCores="])
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
        elif o in ("--numCores"):
            num_vNics = a
        elif o in ("--shape"):
            shape = a
        elif o in ("--debug"):
            debug = a
        else:
            assert False, "unhandle option" 

main()

motif = "ember." + motif + "Motif"
print "Ember communication motif is " + motif

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

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = "4GB/s"
sst.merlin._params["xbar_bw"] = "4GB/s"
sst.merlin._params["flit_size"] = netFlitSize
sst.merlin._params["input_latency"] = "25ns"
sst.merlin._params["output_latency"] = "25ns"
sst.merlin._params["input_buf_size"] = "1kB" 
sst.merlin._params["output_buf_size"] = "1kB" 

sst.merlin._params["num_dims"] = numDim 
sst.merlin._params["torus:shape"] = shape 
sst.merlin._params["torus:width"] = width 
sst.merlin._params["torus:local_ports"] = 1

nicParams = ({ 
		"debug" : debug,
		"verboseLevel": 1,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin.torus",
		"link_bw" : "4GB/s",
		"buffer_size" : "1KB",
		"packetSize" : netPktSizeBytes,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 100,
        "num_vNics" : num_vNics,
	})

driverParams = ({
		"debug" : debug,
		"verbose" : 0,
		"jobId" : 0,
		"bufLen" : 8,
		"hermesModule" : "firefly.hades",
		"msgapi" : "firefly.hades",
		"printStats" : 1,
		"motif0" : motif,
		"buffersize" : 8192,
		"motifParams0.messagesize" : msgSize,
		"motifParams0.messagesizex" : msgSize,
		"motifParams0.messagesizey" : msgSize,
		"motifParams0.iterations" : iterations,
		"hermesParams.debug" : debug,
		"hermesParams.verboseLevel" : 1,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : debug,
		"hermesParams.nicParams.debugLevel" : 1 ,
		"hermesParams.functionSM.defaultDebug" : debug,
		"hermesParams.functionSM.defaultVerbose" : 1,
		"hermesParams.ctrlMsg.debug" : debug,
		"hermesParams.ctrlMsg.verboseLevel" : 1,
		"hermesParams.ctrlMsg.shortMsgLength" : 5000,
		"hermesParams.ctrlMsg.matchDelay_ps" : 0,
		"hermesParams.ctrlMsg.memcpyDelay_ps" : 200,
		"hermesParams.ctrlMsg.txDelay_ns" : 130,
		"hermesParams.ctrlMsg.rxDelay_ns" : 130,
		"hermesParams.ctrlMsg.txNicDelay_ns" : 200,
		"hermesParams.ctrlMsg.rxNicDelay_ns" : 200,
		"hermesParams.ctrlMsg.regRegionBaseDelay_ps" : 10000000,
		"hermesParams.ctrlMsg.regRegionPerByteDelay_ps" : 28,
		"hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
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
			ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_EmberEP", "ember.EmberEngine")
			ep.addParams(driverParams)
			nidList = "0-" + str(numNodes*num_vNics-1) 
			ep.addParam("hermesParams.nidListString", nidList);
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
