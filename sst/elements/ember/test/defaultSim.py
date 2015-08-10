

def getWorkFlow( defaults ):
    workFlow = []
    motif = dict.copy( defaults )
    motif['cmd'] = "Init"
    workFlow.append( motif )

#    motif = dict.copy( defaults )
#    motif['cmd'] = "CMT1D"
#    workFlow.append( motif )

#    motif = dict.copy( defaults )
#    motif['cmd'] = "CMT2D"
#    workFlow.append( motif )

    motif = dict.copy( defaults )
    motif['cmd'] = "CMT3D elementsize=12 px=8 py=4 pz=4 threads=1 mx=5 my=5 mz=4 iterations=10000 processorfreq=3.3"
#    motif['cmd'] = "CMTCR elementsize=10 px=16 py=16 pz=16 iterations=1000"
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
	shape = '8x4x4'

	return platform, topo, shape 
