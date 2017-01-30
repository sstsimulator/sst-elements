
import math
import sys

def getParams( ranks, flopsPerRank ):

	print "ranks={0} flopsPerRank={1}".format(ranks,flopsPerRank)

	nx=100
	ny=100
	nz=100
	#pex=32
	pex=384

	while ranks / pex > 2 * pex:
		pex *= 2

	pey = ranks / pex

	kba=10
	fpc=6

	motifArgs = "nx=" + str(nx) + " ny=" + str(ny) + " nz=" + str(nz)
	motifArgs += " pex=" + str(pex) + " pey=" + str(pey)
	motifArgs += " kba="+ str(kba) + " nodeflops=" + '%f' % flopsPerRank
	motifArgs += " fields_per_cell=" + str(fpc)
	print motifArgs

	return motifArgs
