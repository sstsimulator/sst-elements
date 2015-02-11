
import sys,getopt
import defaultParams
import chamaOpenIBParams
import chamaPSMParams
import bgqParams

import sst
from sst.merlin import *

import loadInfo
from loadInfo import *

import networkConfig 
from networkConfig import *

from random import sample 
from random import shuffle 
from sets import Set 

loadFile = ""
workList = []
numCores = 1
debug    = 0
topology = ""
shape    = ""
loading  = 0
radix    = 0
emberVerbose = 0
numNodes = 0
platform = "default"

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

workFlow = []
if 1 == len(sys.argv) :
    motif = dict.copy(motifDefaults)
    motif['cmd'] = "Init"
    workFlow.append( motif )

    motif = dict.copy(motifDefaults)
    motif['cmd'] = "Sweep3D nx=30 ny=30 nz=30 computetime=140 pex=4 pey=16 pez=0 kba=10"
    workFlow.append( motif )

    motif = dict.copy(motifDefaults)
    motif['cmd'] = "Fini"
    workFlow.append( motif )

    topology = "torus"
    shape    = "4x4x4"
    platform = "chamaPSM"

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["topo=", "shape=",
					"radix=","loading=","debug=","platform=","numNodes=",
					"numCores=","loadFile=","cmdLine=","printStats=","randomPlacement=",
					"emberVerbose=","netBW=","netPktSize=","netFlitSize=",
                    "bgPercentage=","bgMean=","bgStddev=","bgMsgSize="])

except getopt.GetopError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--shape"):
        shape = a
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

workList.append( workFlow )

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
elif platform == "chamaOpenIB":
    nicParams = chamaOpenIBParams.nicParams
    networkParams = chamaOpenIBParams.networkParams
    hermesParams = chamaOpenIBParams.hermesParams
    emberParams = chamaOpenIBParams.emberParams 
elif platform == "bgq":
    nicParams = bgqParams.nicParams
    networkParams = bgqParams.networkParams
    hermesParams = bgqParams.hermesParams
    emberParams = bgqParams.emberParams 
else:
	sys.exit("Must specify platform configuration")


if "" == topology:
	sys.exit("What topo? [torus|fattree]")

if "torus" == topology:
	if "" == shape:
		sys.exit("What torus shape? (e.x. 4, 2x2, 4x4x8)")
	topoInfo = TorusInfo(shape)
	topo = topoTorus()
	print "network: topology=torus shape={0}".format(shape)

elif "fattree" == topology:
	if "" == shape: # use shape if defined, otherwise use radix as legacy mode
		if 0 == radix: 
			sys.exit("Must either specify shape or radix/loading.")
		if 0 == loading:
			sys.exit("Must either specify shape or radix/loading.")
	topoInfo = FattreeInfo(radix,loading,shape)
	topo = topoFatTree()
	print "network: topology=fattree radix={0} loading={1}".format(radix,loading)
else:
	sys.exit("how did we get here")

if int(numNodes) == 0:
    numNodes = int(topoInfo.getNumNodes())

if int(numNodes) > int(topoInfo.getNumNodes()):
    sys.exit("need more nodes")

print "numRanks={0} numNics={1}".format(numNodes, topoInfo.getNumNodes() )

emptyNids = []

if rndmPlacement:
	print "random placement"
	nidList=""
	nids = sample( xrange(int(topoInfo.getNumNodes())-1), int(numNodes))
	nids.sort()

	allNids = []
	for num in range ( 0, int( topoInfo.getNumNodes()) ): 
		allNids.append( num ) 

	emptyNids = list( Set(allNids).difference( Set(nids) ) )

	while nids:
		nidList += str(nids.pop(0)) 
		if nids:
			nidList +=","
    
	for x in workList[0]:
		x['cmd'] = "-nidList=" + nidList + " " + x['cmd']

shuffle( emptyNids )

XXX = []

if bgPercentage > 0:
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
        motif['cmd'] = nidList + " TrafficGen mean="+str(bgMean)+ " stddev=" + \
                    str(bgStddev) + " messageSize="+str(bgMsgSize)
        workFlow.append( motif )

        motif = dict.copy(motifDefaults)
        motif['cmd'] = nidList + " Fini"
        workFlow.append( motif )

        workList.append( workFlow )
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

sst.merlin._params["link_lat"] = "40ns"
sst.merlin._params["link_bw"] = networkParams['link_bw']   
sst.merlin._params["xbar_bw"] = networkParams['link_bw'] 
sst.merlin._params["flit_size"] = networkParams['flitSize'] 
sst.merlin._params["input_latency"] = "50ns"
sst.merlin._params["output_latency"] = "50ns"
sst.merlin._params["input_buf_size"] = networkParams['buffer_size'] 
sst.merlin._params["output_buf_size"] = networkParams['buffer_size'] 

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
