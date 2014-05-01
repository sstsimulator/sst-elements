
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
topology = ""
shape    = ""
loading  = 0
radix    = 0
printStats = 0

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["topo=", "shape=",
					"radix=","loading=","debug=",
					"numCores=","loadFile=","cmdLine=","printStats="])
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
    else:
        assert False, "unhandle option" 


if "" == topology:
	sys.exit("What topo? [torus|fattree]")

if "torus" == topology:
	if "" == shape:
		sys.exit("What torus shape? (e.x. 4, 2x2, 4x4x8")
	topoInfo = TorusInfo(shape)
	topo = topoTorus()

elif "fattree" == topology:
	if 0 == radix: 
		sys.exit("What radix?")
	if 0 == loading:
		sys.exit("What loading?")

	topoInfo = FattreeInfo(radix,loading)
	topo = topoFatTree()
else:
	sys.exit("how did we get here")

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = "560Mhz"
sst.merlin._params["xbar_bw"] = "560Mhz"
sst.merlin._params["input_latency"] = "25ns"
sst.merlin._params["output_latency"] = "25ns"
sst.merlin._params["input_buf_size"] = 128
sst.merlin._params["output_buf_size"] = 128

sst.merlin._params.update( topoInfo.getNetworkParams() )

_nicParams = { 
		"debug" : debug,
		"verboseLevel": 1,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin." + topology,
		"num_vcs" : 2,
		"link_bw" : "560Mhz",
		"buffer_size" : 128,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 100,
	}

_emberParams = {
		"hermesModule" : "firefly.hades",
		"msgapi" : "firefly.hades",
		"debug" : debug,
		"verbose" : 1,
		"printStats" : printStats,
		"buffersize" : 8192,
	}

_hermesParams = {
		"hermesParams.debug" : debug,
		"hermesParams.verboseLevel" : 2,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : debug,
		"hermesParams.nicParams.debugLevel" : 1 ,
		"hermesParams.policy" : "adjacent",
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
