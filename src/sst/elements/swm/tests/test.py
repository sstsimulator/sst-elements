
import sst
from sst.merlin import *
 
import sys,getopt

iterations = 1;
msgSize = 0;
#shape = "16x16x16" # 4K
#shape = "16x16x8" # 2K
shape = "8x8"
#shape = "9x9x9"
#shape = "3x3x3"
#shape = "4x4x4"
#shape = "5x5x5"
#shape = "6x6x6"
#shape = "8x8x4"
#shape = "8"
#shape = "4"
num_vNics = 1

netPktSizeBytes="2048B"
netFlitSize="8B"

def main():
    global iterations
    global msgSize
    global shape
    global num_vNics
    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["msgSize=","iter=","shape=","numCores="])
    except getopt.GetopError as err:
        print (str(err))
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

#print (numNodes)
#print (numRanks)

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = "4GB/s"
sst.merlin._params["xbar_bw"] = "4GB/s"
sst.merlin._params["input_latency"] = "50ns"
sst.merlin._params["output_latency"] = "50ns"
sst.merlin._params["input_buf_size"] = "14KB"
sst.merlin._params["output_buf_size"] = "14KB"
sst.merlin._params["flit_size"] = netFlitSize

sst.merlin._params["num_dims"] = numDim 
sst.merlin._params["torus:shape"] = shape 
sst.merlin._params["torus:width"] = width 
sst.merlin._params["torus:local_ports"] = 1

nicParams = ({ 
		"debug" : 0,
		"verboseLevel": 0,
		"module" : "merlin.linkcontrol",
		"topology" : "merlin.torus",
		"link_bw" : sst.merlin._params["link_bw"], 
		"input_buf_size" : sst.merlin._params["input_buf_size"],
		"output_buf_size" : sst.merlin._params["output_buf_size"],
		"packetSize" : netPktSizeBytes,
		"rxMatchDelay_ns" : 100,
		"txDelay_ns" : 50,
        "nic2host_lat" : "150ns",
        "num_vNics" : num_vNics,
        "useSimpleMemoryModel" : 0,
	})

driverParams = ({
		"debug" : 0,
		"verboseLevel" : 0,
		"verboseMask" : -1,
		"convert.verboseLevel" : 0,
		"workload.verboseLevel" : 0,
		"bufLen" : 8,
		"hermesModule" : "firefly.hades",
		"os.module" : "firefly.hades",
		"trace" : "npe-" + str(numRanks) + "/allred-" + str(numRanks) +".stf",
		"sharedTrace" : "allred-128.stf",
		"printStats" : 1,
		"buffersize" : 140,
		"os.name" : "hermesParams",
		"hermesParams.debug" : 0,
		"hermesParams.verboseLevel" : 0,
		"hermesParams.nicModule" : "firefly.VirtNic",
		"hermesParams.nicParams.debug" : 0,
		"hermesParams.nicParams.debugLevel" : 1 ,
        "hermesParams.functionSM.defaultEnterLatency" : 30000,
        "hermesParams.functionSM.defaultReturnLatency" : 30000,
        "hermesParams.functionSM.defaultDebug" : debug,
        "hermesParams.functionSM.defaultVerbose" : 2,
        "hermesParams.ctrlMsg.debug" : debug,
        "hermesParams.ctrlMsg.verboseLevel" : 0,
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

        #"hermesParams.ctrlMsg.txNicDelay_ns" : 0,
        #"hermesParams.ctrlMsg.rxNicDelay_ns" : 0,
        #"hermesParams.ctrlMsg.sendReqFiniDelay_ns" : 0,
        "hermesParams.ctrlMsg.sendAckDelay_ns" : 0,
        "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 3000,
        "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 100,
        "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
        "hermesParams.loadMap.0.start" : 0,
        "hermesParams.loadMap.0.len" : 2,

        'hermesParams.ctrlMsg.pqs.verboseLevel' : 0, 
        'hermesParams.ctrlMsg.pqs.verboseMask' : -1,
#        'hermesParams.ctrlMsg.pqs.maxUnexpectedMsg' : 500, 

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

		rtrLink = nic.setSubComponent( "rtrLink", "merlin.linkcontrol" )
		rtrLink.addParams( nicParams )

		retval = (rtrLink, "rtr_port", sst.merlin._params["link_lat"] )

		loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
		loopBack.addParam("numCores", num_vNics)

		for x in range(num_vNics ):
		#	ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_TraceReader", "zodiac.ZodiacSiriusTraceReader")
			ep = sst.Component("nic" + str(nodeID) + "core" + str(x) + "_TraceReader", "sstSwm.Swm")
			ep.addParams(driverParams)
			ep.addParam("nid", nodeID)
			#ep.addParam("path", "incast/incast.json")
			#ep.addParam("workload", "incast")
			ep.addParam("path", "lammps/lammps_workload.json")
			ep.addParam("workload", "lammps")
			#ep.addParam("path", "milc/milc_skeleton.json")
			#ep.addParam("workload", "milc")
			#ep.addParam("path", "nekbone/workload1.json")
			#ep.addParam("workload", "nekbone")
			#ep.addParam("path", "many_to_many/many_to_many_workload1.json")
			#ep.addParam("workload", "many_to_many")
			os = ep.setSubComponent( "OS", "firefly.hades" )
			for key, value in driverParams.items():
				if key.startswith("hermesParams."):
					key = key[key.find('.')+1:]
					os.addParam( key,value)

			virtNic = os.setSubComponent( "virtNic", "firefly.VirtNic" )
			proto = os.setSubComponent( "proto", "firefly.CtrlMsgProto" )
			process = proto.setSubComponent( "process", "firefly.ctrlMsg" )
			process.addParam( "pqs.rank",nodeID)

			os.addParam('netId', nodeID )
			os.addParam('netMapSize', numRanks )
			os.addParam('netMapName', "NetMap" )

			prefix = "hermesParams.ctrlMsg."
			for key, value in driverParams.items():
				if key.startswith(prefix):
					key = key[len(prefix):]
					proto.addParam( key,value)
					process.addParam( key,value)
            
			nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) + "_Link"  )

			loopLink = sst.Link( "loop" + str(nodeID) + "nic0core" + str(x) + "_Link"  )

			nicLink.connect( (virtNic,'nic','1ns' ),(nic,'core'+str(x),'150ns'))
            
			loopLink.connect( (process,'loop','1ns' ),(loopBack,'nic0core'+str(x),'1ns'))

		return retval


topo = topoTorus()
topo.prepParams()
endPoint = EmberEP()
endPoint.prepParams()

topo.setEndPoint(endPoint)
topo.build()

