

def genWorkFlow( defaults, nodeNum = None ):

    print 'genWorkFlow()'

    workFlow = []
    motif = dict.copy( defaults )
    motif['cmd'] = "Null"
    workFlow.append( motif )

    return workFlow

def getRanksPerNode():
    return 1

def getNumNodes():
    return 1
