
import sys,pprint,random
pp = pprint.PrettyPrinter(indent=4)



def getCommand( key, cmd ):

	params = {}
	x = cmd.split(' ')
	params[ key + '.name' ] = 'ember.' + x[0] + 'Motif' 
	x.pop(0)

	for arg in x:
		argName, value = arg.split('=')
		params[ key + '.arg.' + argName ] = value 
	return params 

def getMotifParams( workflow ):
	print "getMotifParams()"
	params = {}
	motif_count = 0
	for motif in workflow:
		#pp.pprint(motif)	
		curName = 'motif' + str(motif_count)
		params[curName+'.api'] = motif['api'] 

		params.update( getCommand(curName, motif['cmd']) )

		params[curName+'.spyplotmode'] = motif['spyplotmode'] 
		motif_count += 1

	params['motif_count'] = motif_count

	return params 

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



def genRandom( networkSize, appSize ):

    print "EMBER: random placement", networkSize, appSize

    random.seed( 0xf00dbeef )

    nids = random.sample( xrange(networkSize) , appSize )
    pp.pprint(nids)
    #nids.sort()

    nidList = ','.join(str(x) for x in nids)
    pp.pprint(nidList)

    return nidList

def genLinear( numNodes ):
	return '0-' + str(numNodes-1);

def genNidList( networkSize, appSize, random = False ):
	if networkSize < appSize:
		sys.exit("FATAL: numAppNodes={1} is > numNeworkNodes={0} ".format(networkSize,appSize))

	if random:
		return genRandom( networkSize, appSize )
	else:
		return genLinear( appSize )

