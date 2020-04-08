import random, sys

def generate( args ):
    args = args.split(',')

    totalNodes = int(args[0])    
    start = int(args[1])
    length = int(args[2])

    random.seed( 0xf00dbeef )
    nids = random.sample( list(range( totalNodes )), totalNodes)

    tmp = nids[start:start+length]
    nidList = ','.join( str(x) for x in tmp)

    return nidList 
