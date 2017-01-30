import math
import sys

def getParams( ranks, flopsPerRank ):
	messagesize = 128
	iterations = 10
	print 'Random: messagesize{0} iterations={1}'.format( messagesize, iterations )
	motifArgs='messagesize=' + str(messagesize) + ' iterations=' + str(iterations)

	return motifArgs
