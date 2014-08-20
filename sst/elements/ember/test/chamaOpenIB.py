
import sys,getopt

import sst
from sst.merlin import *

import loadInfo
from loadInfo import *

import networkConfig 
from networkConfig import *

loadFile = ""
cmdLine  = ""
numCores = 1
debug    = 0
topology = "torus"
shape    = "2"
loading  = 0
radix    = 0
printStats = 0
emberVerbose = 0
emberBufferSize = 1000

netBW = "3.3GB/s"
netPktSize="2048B"
netFlitSize="8B"
netBufSize="8KB"

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["topo=", "shape=",
					"radix=","loading=","debug=",
					"numCores=","loadFile=","cmdLine=","printStats=",
					"emberVerbose=","netBW=","netPktSize=","netFlitSize="])

except getopt.GetopError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--shape"):
        shape = a
    elif o in ("--numCores"):
        numCores = a
    elif o in ("--debug"):
        debug = a
    elif o in ("--loadFile"):
        loadFile = a
    elif o in ("--cmdLine"):
        cmdLine = a
    elif o in ("--topo"):
        topology = a
    elif o in ("--radix"):
        radix = a
    elif o in ("--loading"):
        loading = a
    elif o in ("--printStats"):
        printStats = a
    elif o in ("--emberVerbose"):
        emberVerbose = a
    elif o in ("--netBW"):
        netBW = a
    elif o in ("--netFlitSize"):
        netFlitSize = a
    elif o in ("--netPktSize"):
        netPktSize = a
    else:
        assert False, "unhandle option" 


if "" == topology:
	sys.exit("What topo? [torus|fattree]")

if "torus" == topology:
	if "" == shape:
		sys.exit("What torus shape? (e.x. 4, 2x2, 4x4x8")
	topoInfo = TorusInfo(shape)
	topo = topoTorus()
	print "network: topology=torus shape={0}".format(shape)

elif "fattree" == topology:
	if 0 == radix: 
		sys.exit("What radix?")
	if 0 == loading:
		sys.exit("What loading?")

	topoInfo = FattreeInfo(radix,loading)
	topo = topoFatTree()
else:
	sys.exit("how did we get here")

print "network: BW={0} pktSize={1} flitSize={2}".format(netBW,netPktSize,netFlitSize)

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = netBW 
sst.merlin._params["xbar_bw"] = netBW 
sst.merlin._params["flit_size"] = netFlitSize
sst.merlin._params["input_latency"] = "50ns"
sst.merlin._params["output_latency"] = "50ns"
sst.merlin._params["input_buf_size"] = netBufSize 
sst.merlin._params["output_buf_size"] = netBufSize 

sst.merlin._params.update( topoInfo.getNetworkParams() )

_nicParams = { 
		"debug" : debug,
		"verboseLevel": 1,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin." + topology,
		"packetSize" : netPktSize,
		"link_bw" : netBW,
		"buffer_size" : netBufSize,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 50,
	}

_emberParams = {
		"hermesModule" : "firefly.hades",
		"msgapi" : "firefly.hades",
		"verbose" : emberVerbose,
		"printStats" : printStats,
		"buffersize" : emberBufferSize,
	}

_hermesParams = {
		"hermesParams.debug" : debug,
		"hermesParams.verboseLevel" : 2,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : debug,
		"hermesParams.nicParams.debugLevel" : 1 ,
		"hermesParams.policy" : "adjacent",
		"hermesParams.functionSM.defaultEnterLatency" : 30000,
		"hermesParams.functionSM.defaultReturnLatency" : 30000,
		"hermesParams.functionSM.defaultDebug" : debug,
		"hermesParams.functionSM.defaultVerbose" : 2,
		"hermesParams.ctrlMsg.debug" : debug,
		"hermesParams.ctrlMsg.verboseLevel" : 2,
		"hermesParams.ctrlMsg.shortMsgLength" : 12000,
		"hermesParams.ctrlMsg.matchDelay_ns" : 150,
		"hermesParams.ctrlMsg.memcpyBaseDelay_ns" : 80,
		"hermesParams.ctrlMsg.memcpyPer64BytesDelay_ns" : 25,
		"hermesParams.ctrlMsg.txDelay_ns" : 1000,
		"hermesParams.ctrlMsg.rxDelay_ns" : 1600,
		"hermesParams.ctrlMsg.txNicDelay_ns" : 0,
		"hermesParams.ctrlMsg.rxNicDelay_ns" : 0,
		"hermesParams.ctrlMsg.sendReqFiniDelay_ns" : 700,
		"hermesParams.ctrlMsg.sendAckDelay_ns" : 700,
		"hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 3400,
		"hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 250,
		"hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
		"hermesParams.loadMap.0.start" : 0,
		"hermesParams.loadMap.0.len" : 2,
	}


epParams = {} 
epParams.update(_emberParams)
epParams.update(_hermesParams)

loadInfo = LoadInfo( _nicParams, epParams, topoInfo.getNumNodes(), numCores )

if len(loadFile) > 0:
	if len(cmdLine) > 0:
		sys.exit("Error: can't specify both loadFile and cmdLine");

	loadInfo.initFile(loadFile)
else:
	if len(cmdLine) > 0:
		if len(loadFile) > 0:
			sys.exit("Error: can't specify both loadFile and cmdLine");

		loadInfo.initCmd(cmdLine)
	else:
		sys.exit("Error: need a loadFile or cmdLine")

topo.prepParams()

topo.setEndPointFunc( loadInfo.setNode )
topo.build()
