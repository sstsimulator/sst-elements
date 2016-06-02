
import sys,getopt

import sst
from sst.merlin import *

import loadInfo
from loadInfo import *

import networkConfig 
from networkConfig import *

import random 

debug    = 0
emberVerbose = 0
embermotifLog = ''
emberrankmapper = ''

statNodeList = []
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
netInspect = ''
rtrArb = ''

rndmPlacement = False
#rndmPlacement = True
bgPercentage = int(0)
bgMean = 1000
bgStddev = 300
bgMsgSize = 1000

detailedModelName = ""
detailedModelNodes = [] 
detailedModelParams = ""
simConfig = ""
platParams = ""

motifDefaults = { 
	'cmd' : "",
	'printStats' : 0, 
	'api': "HadesMP",
	'spyplotmode': 0 
}

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["topo=", "shape=",
		"simConfig=","platParams=",",debug=","platform=","numNodes=",
		"numCores=","loadFile=","cmdLine=","printStats=","randomPlacement=",
		"emberVerbose=","netBW=","netPktSize=","netFlitSize=",
		"rtrArb=","embermotifLog=",	"rankmapper=",
		"bgPercentage=","bgMean=","bgStddev=","bgMsgSize=","netInspect=",
        "detailedNameModel=","detailedModelParams=","detailedModelNodes="])

except getopt.GetoptError as err:
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
    elif o in ("--embermotifLog"):
        embermotifLog = a
    elif o in ("--rankmapper"):
        emberrankmapper = a
    elif o in ("--netBW"):
        netBW = a
    elif o in ("--netFlitSize"):
        netFlitSize = a
    elif o in ("--netPktSize"):
        netPktSize = a
    elif o in ("--netInspect"):
        netInspect = a
    elif o in ("--rtrArb"):
        rtrArb = a
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
    elif o in ("--detailedModelName"):
        detailedModelName = a
    elif o in ("--detailedModelNodes"):
        detailedModelNodes = a
    elif o in ("--detailedModelParams"):
        detailedModelParams = a
    elif o in ("--simConfig"):
        simConfig = a
    elif o in ("--platParams"):
        platParams = a
    else:
        assert False, "unhandle option" 

if 1 == len(sys.argv):
	simConfig = 'defaultSim'

if simConfig:
	try:
		config = __import__( simConfig, fromlist=[''] )
	except:
		sys.exit('Failed: could not import sim configuration `{0}`'.format(simConfig) )

	workFlow, numNodes, numCores = config.getWorkFlow( motifDefaults )
	platform, netTopo, netShape = config.getNetwork( )
	detailedModelName, detailedModelParams, detailedModelNodes = config.getDetailedModel()

if workFlow:
	workList.append( [jobid, workFlow] )
	jobid += 1

model = None

if detailedModelName:

	try:
		modelModule = __import__( detailedModelName, fromlist=[''] )
	except:
		sys.exit('Failed: could not import detailed model `{0}`'.format(detailedModelName) )

	try:
		modelParams = __import__( detailedModelParams, fromlist=[''] )
	except:
		sys.exit('Failed: could not import detailed model params `{0}`'.format(detailedModelNameParms) )

	model = modelModule.getModel(modelParams.params,detailedModelNodes)

print "EMBER: platform: {0}".format( platform )

if not platParams:
	platParams = platform + 'Params'
try:
	config = __import__( platParams, fromlist=[''] )
except:
	sys.exit('Failed: could not import `{0}`'.format(platParams) )


nicParams = config.nicParams
networkParams = config.networkParams
hermesParams = config.hermesParams
emberParams = config.emberParams 
platNetConfig = config.netConfig

if netBW:
	networkParams['link_bw'] = netBW

if netInspect:
	networkParams['network_inspectors'] = netInspect

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

elif "dragonfly2" == netTopo:

	topoInfo = DragonFly2Info(netShape)
	topo = topoDragonFly2()

else:
	sys.exit("how did we get here")

if rtrArb:
	print "EMBER: network: topology={0} shape={1} arbitration={2}".format(netTopo,netShape,rtrArb)
else:
	print "EMBER: network: topology={0} shape={1}".format(netTopo,netShape)

if int(numNodes) == 0:
    numNodes = int(topoInfo.getNumNodes())

if int(numNodes) > int(topoInfo.getNumNodes()):
    sys.exit("need more nodes want " + str(numNodes) + ", have " + str(topoInfo.getNumNodes()))
 
print "EMBER: numNodes={0} numNics={1}".format(numNodes, topoInfo.getNumNodes() )

emptyNids = []

if jobid > 0 and rndmPlacement:

	print "EMBER: random placement"

	hermesParams["hermesParams.mapType"] = 'random'

	random.seed( 0xf00dbeef )
	nidList=""
	nids = random.sample( xrange( int(topoInfo.getNumNodes())), int(numNodes) )
	#nids.sort()

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

    print "EMBER: netAlloced {0}%, bg motifs {1}, mean {2} ns, stddev {3} ns, msgsize {4} bytes".\
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

nicParams['verboseLevel'] = debug
nicParams['verboseMask'] = 1
hermesParams['hermesParams.verboseLevel'] = debug
hermesParams['hermesParams.nicParams.verboseLevel'] = debug
hermesParams['hermesParams.functionSM.verboseLevel'] = debug
hermesParams['hermesParams.ctrlMsg.verboseLevel'] = debug
emberParams['verbose'] = emberVerbose
if embermotifLog:
    emberParams['motifLog'] = embermotifLog
if emberrankmapper:
    emberParams['rankmapper'] = emberrankmapper

print "EMBER: network: BW={0} pktSize={1} flitSize={2}".format(
        networkParams['link_bw'], networkParams['packetSize'], networkParams['flitSize'])

sst.merlin._params["link_lat"] = networkParams['link_lat']
sst.merlin._params["link_bw"] = networkParams['link_bw']   
sst.merlin._params["xbar_bw"] = networkParams['link_bw'] 
sst.merlin._params["flit_size"] = networkParams['flitSize'] 
sst.merlin._params["input_latency"] = networkParams['input_latency'] 
sst.merlin._params["output_latency"] = networkParams['output_latency'] 
sst.merlin._params["input_buf_size"] = networkParams['buffer_size'] 
sst.merlin._params["output_buf_size"] = networkParams['buffer_size'] 

if "network_inspectors" in networkParams.keys():
    sst.merlin._params["network_inspectors"] = networkParams['network_inspectors']

if rtrArb:
	sst.merlin._params["xbar_arb"] = "merlin." + rtrArb 

sst.merlin._params.update( topoInfo.getNetworkParams() )

epParams = {} 
epParams.update(emberParams)
epParams.update(hermesParams)

loadInfo = LoadInfo( nicParams, epParams, numNodes, numCores, topoInfo.getNumNodes(), model )

if len(loadFile) > 0:
	if len(workList) > 0:
		sys.exit("Error: can't specify both loadFile and cmdLine");

	loadInfo.initFile( motifDefaults, loadFile, statNodeList )
else:
	if len(workList) > 0:
		if len(loadFile) > 0:
			sys.exit("Error: can't specify both loadFile and cmdLine");

		loadInfo.initWork( workList, statNodeList )
	else:
		sys.exit("Error: need a loadFile or cmdLine")

topo.prepParams()

topo.setEndPointFunc( loadInfo.setNode )
topo.build()
