import math
import sys

def getParams( ranks, flopsPerRank ):
	messageSize = 1024
	iterations = 1000
	computetime = 0
	print 'AllPingPong: messageSize{0} iterations={1} computetime={2}'.format( messageSize, iterations, computetime )
	motifArgs='messageSize=' + str(messageSize) + ' iterations=' + str(iterations) + ' computetime=' + str(computetime)

	return motifArgs
