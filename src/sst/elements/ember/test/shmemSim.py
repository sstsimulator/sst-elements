

def getWorkFlow( defaults ):

    workFlow = []
    motif = dict.copy( defaults )
    motif['cmd'] = "ShmemCollect32 nelems=1000 printResults=0"
    motif['api'] = "HadesSHMEM"
    workFlow.append( motif )

	# numNodes = 0 implies use all nodes on network
    numNodes = 0 
    numCores = 1 

    return workFlow, numNodes, numCores 

def getNetwork():

	#platform = 'chamaPSM'
	#platform = 'chamaOpenIB'
	#platform = 'bgq'
	platform = 'default'

	#topo = ''
	#shape = ''
	topo = 'torus'
	shape = '2x2x2'

	return platform, topo, shape 

def getDetailedModel():
    return "","",[]
