
import sys,getopt
import defaultParams
import defaultSim
import chamaOpenIBParams
import chamaPSMParams
import bgqParams

import sst
from sst.merlin import *

import loadInfo
from loadInfo import *

import networkConfig 
from networkConfig import *

import random 

debug    = 0
emberVerbose = 0

jobid = 0
loadFile = '' 
workList = []
workFlow = []
numCores = 1
numNodes = 0

platform = 'default'

netFlitSize = '' 
netBW = '' 
netPktSize = '' 
netTopo = ''
netShape = ''
netArb = ''

rndmPlacement = False
bgPercentage = int(0)
bgMean = 1000
bgStddev = 300
bgMsgSize = 1000

motifDefaults = { 
	'cmd' : "",
	'printStats' : 0, 
	'api': "HadesMP",
	'spyplotmode': 0 
}

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["topo=", "shape=",
		"debug=","platform=","numNodes=",
		"numCores=","loadFile=","cmdLine=","printStats=","randomPlacement=",
		"emberVerbose=","netBW=","netPktSize=","netFlitSize=","netPktSize",
		"netArb=",		
		"bgPercentage=","bgMean=","bgStddev=","bgMsgSize="])

except getopt.GetopError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--shape"):
        netShape = a
    elif o in ("--platform"):
        platform = a
    elif o in ("--numCores"):
        numCores = a
    elif o in ("--numNodes"):
        numNodes = a
    elif o in ("--debug"):
        debug = a
    elif o in ("--loadFile"):
        loadFile = a
    elif o in ("--cmdLine"):
    	motif = dict.copy(motifDefaults)
    	motif['cmd'] = a 
    	workFlow.append( motif )
    elif o in ("--topo"):
        netTopo = a
    elif o in ("--printStats"):
        motifDefaults['printStats'] = a
    elif o in ("--emberVerbose"):
        emberVerbose = a
    elif o in ("--netBW"):
        netBW = a
    elif o in ("--netFlitSize"):
        netFlitSize = a
    elif o in ("--netPktSize"):
        netPktSize = a
    elif o in ("--netArb"):
        netArb = a
    elif o in ("--randomPlacement"):
        if a == "True":
            rndmPlacement = True
    elif o in ("--bgPercentage"):
        bgPercentage = int(a)
    elif o in ("--bgMean"):
        bgMean = int(a) 
    elif o in ("--bgStddev"):
        bgStddev = int(a) 
    elif o in ("--bgMsgSize"):
        bgMsgSize = int(a) 
    else:
        assert False, "unhandle option" 

if 1 == len(sys.argv):
	workFlow, numNodes, numCores = defaultSim.getWorkFlow( motifDefaults )
	platform, netTopo, netShape = defaultSim.getNetwork( )

workList.append( [jobid, workFlow] )
jobid += 1

print "platform: {0}".format( platform )

platNetConfig = {}
if platform == "default":
    nicParams = defaultParams.nicParams
    networkParams = defaultParams.networkParams
    hermesParams = defaultParams.hermesParams
    emberParams = defaultParams.emberParams 

elif platform == "chamaPSM":
    nicParams = chamaPSMParams.nicParams
    networkParams = chamaPSMParams.networkParams
    hermesParams = chamaPSMParams.hermesParams
    emberParams = chamaPSMParams.emberParams 
    platNetConfig = chamaPSMParams.netConfig

elif platform == "chamaOpenIB":
    nicParams = chamaOpenIBParams.nicParams
    networkParams = chamaOpenIBParams.networkParams
    hermesParams = chamaOpenIBParams.hermesParams
    emberParams = chamaOpenIBParams.emberParams 
    platNetConfig = chamaOpenIBParams.netConfig

elif platform == "bgq":
    nicParams = bgqParams.nicParams
    networkParams = bgqParams.networkParams
    hermesParams = bgqParams.hermesParams
    emberParams = bgqParams.emberParams 
    platNetConfig = bgqParams.netConfig

if netBW:
	networkParams['link_bw'] = netBW

if netFlitSize:
	networkParams['flitSize'] = netFlitSize

if netPktSize:
	networkParams['packetSize'] = netPktSize

if "" == netTopo:
	if platNetConfig['topology']:
		netTopo = platNetConfig['topology']
	else:
		sys.exit("What topo? [torus|fattree|dragonfly]")

if "" == netShape:
	if platNetConfig['shape']:
		netShape = platNetConfig['shape']
	else:
		sys.exit("Error: " + netTopo + " needs shape")

if "torus" == netTopo:

	topoInfo = TorusInfo(netShape)
	topo = topoTorus()

elif "fattree" == netTopo:

	topoInfo = FattreeInfo(netShape)
	topo = topoFatTree()

elif "dragonfly" == netTopo:
		
	topoInfo = DragonFlyInfo(netShape)
	topo = topoDragonFly()

else:
	sys.exit("how did we get here")

if netArb:
	print "network: topology={0} shape={1} arbitration={2}".format(netTopo,netShape,netArb)
else:
	print "network: topology={0} shape={1}".format(netTopo,netShape)

if int(numNodes) == 0:
    numNodes = int(topoInfo.getNumNodes())

if int(numNodes) > int(topoInfo.getNumNodes()):
    sys.exit("need more nodes want " + str(numNodes) + ", have " + str(topoInfo.getNumNodes()))
 
print "numRanks={0} numNics={1}".format(numNodes, topoInfo.getNumNodes() )

emptyNids = []

if rndmPlacement:
	print "random placement"
	random.seed( 0xf00dbeef )
	nidList=""
	nids = random.sample( xrange( int(topoInfo.getNumNodes())), int(numNodes) )
	nids.sort()

	allNids = []
	for num in range ( 0, int( topoInfo.getNumNodes()) ): 
		allNids.append( num ) 

	emptyNids = list( set(allNids).difference( set(nids) ) )

	while nids:
		nidList += str(nids.pop(0)) 
		if nids:
			nidList +=","
    
	tmp = workList[0]
	tmp = tmp[1]

	for x in tmp:
		x['cmd'] = "-nidList=" + nidList + " " + x['cmd']

	random.shuffle( emptyNids )

XXX = []

if rndmPlacement and bgPercentage > 0:
    if bgPercentage > 100:
        sys.exit( "fatal: bgPercentage " + str(bgPercentage) );
    count = 0 
    bgPercentage = float(bgPercentage) / 100.0
    avail = int( topoInfo.getNumNodes() * bgPercentage ) 
    bgMotifs, r = divmod( avail - int(numNodes), 2 )

    print "netAlloced {0}%, bg motifs {1}, mean {2} ns, stddev {3} ns, msgsize {4} bytes".\
                    format(int(bgPercentage*100),bgMotifs,bgMean,bgStddev,bgMsgSize)
    while ( count < bgMotifs ) :
        workFlow = []
        nidList = "-nidList=" + str(emptyNids[ count * 2 ] ) + "," + str(emptyNids[ count * 2 + 1])
        motif = dict.copy(motifDefaults)
        motif['cmd'] = nidList + " Init"
        workFlow.append( motif )

        motif = dict.copy(motifDefaults)
        x,y = divmod( count , 60 )
        motif['cmd'] = nidList + " TrafficGen mean="+str(bgMean)+ " stddev=" + \
                    str(bgStddev) + " messageSize="+str(bgMsgSize) + " startDelay=" + str( y * 500 )
        workFlow.append( motif )

        motif = dict.copy(motifDefaults)
        motif['cmd'] = nidList + " Fini"
        workFlow.append( motif )

        workList.append( [ jobid, workFlow ] )
        jobid += 1
        count += 1

nicParams['debug'] = debug
nicParams['verboseLevel'] = 1 
hermesParams['hermesParams.debug'] = debug
hermesParams['hermesParams.nicParams.debug'] = debug
hermesParams['hermesParams.functionSM.defaultDebug'] = debug
hermesParams['hermesParams.ctrlMsg.debug'] = debug
emberParams['verbose'] = emberVerbose

print "network: BW={0} pktSize={1} flitSize={2}".format(
        networkParams['link_bw'], networkParams['packetSize'], networkParams['flitSize'])

sst.merlin._params["link_lat"] = networkParams['link_lat']
sst.merlin._params["link_bw"] = networkParams['link_bw']   
sst.merlin._params["xbar_bw"] = networkParams['link_bw'] 
sst.merlin._params["flit_size"] = networkParams['flitSize'] 
sst.merlin._params["input_latency"] = networkParams['input_latency'] 
sst.merlin._params["output_latency"] = networkParams['output_latency'] 
sst.merlin._params["input_buf_size"] = networkParams['buffer_size'] 
sst.merlin._params["output_buf_size"] = networkParams['buffer_size'] 

if netArb:
	sst.merlin._params["xbar_arb"] = "merlin." + netArb 

sst.merlin._params.update( topoInfo.getNetworkParams() )

epParams = {} 
epParams.update(emberParams)
epParams.update(hermesParams)

loadInfo = LoadInfo( nicParams, epParams, numNodes, numCores )

if len(loadFile) > 0:
	if len(workList) > 0:
		sys.exit("Error: can't specify both loadFile and cmdLine");

	loadInfo.initFile( motifDefaults, loadFile)
else:
	if len(workList) > 0:
		if len(loadFile) > 0:
			sys.exit("Error: can't specify both loadFile and cmdLine");

		loadInfo.initWork( workList )
	else:
		sys.exit("Error: need a loadFile or cmdLine")

topo.prepParams()

topo.setEndPointFunc( loadInfo.setNode )
topo.build()
