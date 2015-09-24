

def getWorkFlow( defaults ):
    workFlow = []
    motif = dict.copy( defaults )
    motif['cmd'] = "Init"
    workFlow.append( motif )

    motif = dict.copy( defaults )
    #motif['cmd'] = "Sweep3D nx=30 ny=30 nz=30 computetime=140 pex=4 pey=16 pez=0 kba=10"     
    #motif['cmd'] = "PingPong blockingSend=0 blockingRecv=0 messageSize=1000"
    #motif['cmd'] = "PingPong messageSize=800000"
    motif['cmd'] = "3DAMR blockfile=/home/mjleven/plot.0400.binary pex=8 pey=16 nx=8 ny=8 nx=8 filetype=binary "
    workFlow.append( motif )

    motif = dict.copy( defaults )
    motif['cmd'] = "Fini"
    #workFlow.append( motif )

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
	shape = '4x4x8'
	#shape = '2'

	return platform, topo, shape 
