
import sst
from sst.merlin import *
 
import sys,getopt,loadInfo
from loadInfo import *

loadFile = ""
cmdLine  = ""
shape    = "4"
numCores = 1
debug    = 0

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["shape=","debug=",
									"numCores","loadFile=","cmdLine="])
except getopt.GetopError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--shape"):
        shape = a
    if o in ("--numCores"):
        numCores = a
    elif o in ("--debug"):
        debug = a
    elif o in ("--loadFile"):
        loadFile = a
    elif o in ("--cmdLine"):
        cmdLine = a
    else:
        assert False, "unhandle option" 

print "shape=", str(shape), "numCores=", str(numCores)

if len(loadFile):
	print "LoadFile=", repr(loadFile)
if len(cmdLine):
	print "CmdLine=", cmdLine 

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

_nicParams = { 
		"debug" : debug,
		"verboseLevel": 1,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin.torus",
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
		"printStats" : 1,
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

loadInfo = LoadInfo( _nicParams, epParams, numNodes, numCores )

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

topo = topoTorus()
topo.prepParams()

topo.setEndPointFunc( loadInfo.setNode )
topo.build()
