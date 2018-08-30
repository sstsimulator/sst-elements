

def getWorkFlow( defaults ):
    workFlow = []
    motif = dict.copy( defaults )
    motif['cmd'] = "Init"
    workFlow.append( motif )

    motif = dict.copy( defaults )
    motif['cmd'] = "LQCD nx=16 ny=16 nz=16 nt=16 iterations=1 peflops=14000000000"
    #motif['cmd'] = "Sweep3D nx=30 ny=30 nz=30 computetime=140 pex=2 pey=4 pez=0 kba=10"
    workFlow.append( motif )

    motif = dict.copy( defaults )
    motif['cmd'] = "Fini"
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
	shape = '2x4x2'

	return platform, topo, shape

def getDetailedModel():
    return "","",[]
