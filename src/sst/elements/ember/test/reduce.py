import math
import sys

def getParams( ranks, flopsPerRank ):
	count = 1
	iterations = 1
	compute = 1
	root = 0
	print 'Reduce: count{0} iterations={1} compute={2} root={3}'.format( count, iterations, compute, root )
	motifArgs='bytes=' + str(count) + ' iterations=' + str(iterations) + ' compute=' + str(compute) + ' root=' + str(root)

	return motifArgs
