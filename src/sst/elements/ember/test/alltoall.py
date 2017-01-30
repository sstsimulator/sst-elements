import math
import sys

def getParams( ranks, flopsPerRank ):
	messageSize = 1
	iterations = 1
	computetime = 1
	print 'AllToAll: messageSize{0} iterations={1} computetime={2}'.format( messageSize, iterations, computetime )
	motifArgs='bytes=' + str(messageSize) + ' iterations=' + str(iterations) + ' compute=' + str(computetime)

	return motifArgs
