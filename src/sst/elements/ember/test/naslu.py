
import math
import sys

def getParams( ranks, flopsPerRank ):

	#PARAMETER 0 = arg.iterations (Sets the number of ping pong operations to perform) [1]
	#PARAMETER 1 = arg.pex (Sets the processor array size in X-dimension, 0 means auto-calculate) [0]
	#PARAMETER 2 = arg.pey (Sets the processor array size in Y-dimension, 0 means auto-calculate) [0]
	#PARAMETER 3 = arg.nx (Sets the problem size in the X-dimension) [50]
	#PARAMETER 4 = arg.ny (Sets the problem size in the Y-dimension) [50]
	#PARAMETER 5 = arg.nz (Sets the problem size in the Z-dimension) [50]
	#PARAMETER 6 = arg.nzblock (Sets the Z-dimensional block size (Nz % Nzblock == 0, default is 1)) [1]
	#PARAMETER 7 = arg.computetime (Sets the compute time per KBA-data block in nanoseconds) [1000]

	pex = pey = int(math.sqrt(ranks))

	iterations = 1
	nx = ny = nz = 50
	nzblock = 1
	computetime = 1000

	motifArgs = " iterations=" + str(iterations)
	motifArgs += " pex=" + str(pex) + " pey=" + str(pey)
	motifArgs += " nx=" + str(nx) + " ny=" + str(ny) + " nz=" + str(nz)
	motifArgs += " nzblock=" + str(nzblock)
	motifArgs += " computetime=" + str(computetime)
	#print motifArgs

	return motifArgs
