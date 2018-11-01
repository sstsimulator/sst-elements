def calcNetMapId( nodeId, nidList ):

    if nidList == 'Null':
        return -1

    pos = 0
    a = nidList.split(',')

    for b in a:
        c = b.split('-')
    
        start = int(c[0])
        stop = start

        if 2 == len(c):
            stop = int(c[1])

        if nodeId >= start and nodeId <= stop:
            return pos + (nodeId - start) 

        pos = pos + ((stop - start) + 1)

    return -1

def calcNetMapSize( nidList ):

    if nidList == 'Null':
        return 0 

    pos = 0
    a = nidList.split(',')

    for b in a:
        c = b.split('-')
    
        xx = 1 
        if 2 == len(c):
            xx = int(c[1]) - int(c[0]) + 1

        pos += xx

    return pos

def calcMaxNode( nidList ):

    if nidList == 'Null':
        return 0 
	
    max = 0
    a = nidList.split(',')

    for b in a:
        c = b.split('-')
    
        tmp = int(c[0]) 
        if 2 == len(c):
           tmp  = int(c[1])

        if tmp > max:
            max = tmp

    return max + 1 

