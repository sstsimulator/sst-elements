# Copyright 2009-2021 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2021, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys,pprint,random,re
pp = pprint.PrettyPrinter(indent=4)

def getCommand( key, cmd ):

	params = {}
	cmd = re.sub( ' +', ' ',cmd)
	x = cmd.split(' ')
	params[ key + '.name' ] = 'ember.' + x[0] + 'Motif'
	x.pop(0)

	for arg in x:
		try:
			argName, value = arg.split('=')
		except:
			break
		params[ key + '.arg.' + argName ] = value
	return params

def getMotifParams( workflow ):
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
	#print 'genNidList( networkSize={0} appSize={1} )'.format(networkSize,appSize)
	if appSize == -1:
		appSize = networkSize
	if networkSize < appSize:
		sys.exit("FATAL: numAppNodes={1} is > numNeworkNodes={0} ".format(networkSize,appSize))

	if random:
		return genRandom( networkSize, appSize )
	else:
		return genLinear( appSize )


def getWorkListFromFile( filename, defaultParams ):
    stage1 = []
    for line in open(filename, 'r'):
        line = line.strip()
        if line:
            if line[:1] == '[':
                stage1.append(line)
            elif line[:1] == '#':
                continue;
            else:
                stage1[-1] += ' ' + line


    nidlist = None
    jobId = None
    cmds = []
    for item in stage1:

        tag,str = item.split(' ', 1)

        if tag == '[JOB_ID]':
            if jobId:
                yield jobId, nidlist, cmds

            jobId = int(str)
            cmds = []

        elif tag == '[NID_LIST]':
            nidlist = str
        elif tag == '[MOTIF]':
			cmds += [str]

    yield jobId, nidlist, cmds
