
import sys,getopt,random 

from nicConfig import NicConfig
from emberConfig import EmberConfig
from jobInfo import JobInfo
from jobInfo import JobInfoX
from loadInfo2 import *
from networkConfig import *
import merlin

debug    = 0
emberVerbose = 0
embermotifLog = ''
emberrankmapper = ''

statNodeList = []
loadFile = '' 

platform = 'default'

netFlitSize = '' 
netBW = '' 
netPktSize = '' 
netTopo = ''
netShape = ''
netInspect = ''
rtrArb = ''

jobInfo = None 

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

try:
    opts, args = getopt.getopt(sys.argv[1:], "", 
		["topo=", 
		"shape=",
		"netBW=",
		"netPktSize=",
		"netFlitSize=",
		"rtrArb=",

		"simConfig=",
		"platParams=",
		"debug=",
		"platform=",

		"numNodes=",
		"numRanksPerNode=",
		"cmdLine=",

		"loadFile=",
		"randomPlacement=",
		"emberVerbose=",
		"embermotifLog=",
		"rankmapper=",
		"bgPercentage=",
		"bgMean=",
		"bgStddev=",
		"bgMsgSize=",
		"netInspect=",
        "detailedNameModel=",
		"detailedModelParams=",
		"detailedModelNodes="])

except getopt.GetoptError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--shape"):
        netShape = a
    elif o in ("--platform"):
        platform = a

    elif o in ("--numNodes"):
		if not jobInfo:
			jobInfo = JobInfoX(0)
		jobInfo.setNumNodes( int(a) )
    elif o in ("--numRanksPerNode"):
		if not jobInfo:
			jobInfo = JobInfoX(0)
		jobInfo.setNumRanksPerNode( int(a) )
    elif o in ("--cmdLine"):
		if not jobInfo:
			jobInfo = JobInfoX(0)
		jobInfo.addMotif( a )

    elif o in ("--debug"):
        debug = a
    elif o in ("--loadFile"):
        loadFile = a
    elif o in ("--topo"):
        netTopo = a
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
	simConfig = 'defaultSim2'

workConfig = None

if simConfig:
	try:
		workConfig = __import__( simConfig, fromlist=[''] )
	except:
		sys.exit('Failed: could not import sim configuration `{0}`'.format(simConfig) )

	numNodes = workConfig.getNumNodes()
	numRanksPerNode = workConfig.getRanksPerNode() 
	platform, netTopo, netShape = workConfig.getNetwork( )
	detailedModelName, detailedModelParams, detailedModelNodes = workConfig.getDetailedModel()

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

platConfig = None
if not platParams:
	platParams = platform + 'Params'
try:
	platConfig = __import__( platParams, fromlist=[''] )
except:
	sys.exit('Failed: could not import `{0}`'.format(platParams) )


nicParams = platConfig.nicParams

networkParams = platConfig.networkParams
platNetConfig = platConfig.netConfig

hermesParams = platConfig.hermesParams
emberParams = platConfig.emberParams 


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

 
print "EMBER: numNodes={0}".format( topoInfo.getNumNodes() )

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

merlin.setParams( networkParams, rtrArb, topoInfo.getNetworkParams() )

nicConfig = NicConfig( nicParams )

try:
	nullConfig = __import__( 'nullSim', fromlist=[''] )
except:
	sys.exit('Failed: could not import sim configuration `{0}`'.format('nullSim') )


nullEmber = JobInfo( -1, 1, nullConfig.getRanksPerNode(), nullConfig.genWorkFlow )
nullEmber.setNidList( 'Null' )

nullEmber = EmberConfig( emberParams, hermesParams, nullEmber )

loadInfo = LoadInfo( nicConfig, topoInfo.getNumNodes(), nullEmber )

if not jobInfo:
	jobId = 0 
	jobInfo = JobInfo( jobId, workConfig.getNumNodes(), 
			workConfig.getRanksPerNode(), workConfig.genWorkFlow )

jobInfo.printMe()

nidList = genNidList( topoInfo.getNumNodes(), jobInfo.getNumNodes(), False )
jobInfo.setNidList( nidList )

loadInfo.addEmberConfig( EmberConfig( emberParams, hermesParams, jobInfo ) )

topo.prepParams()

topo.setEndPointFunc( loadInfo.setNode )
topo.build()
