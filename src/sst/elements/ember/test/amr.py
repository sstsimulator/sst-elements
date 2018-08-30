
import math
import sys

def getParams( ranks, flopsPerRank, ts ):

	dim = 8 
	nx=dim
	ny=dim
	nz=dim

	pex = 2
	while pex * pex < ranks:
		pex = pex * 2

	pey=ranks/pex

	if pex * pey != ranks:
		pey = pey / 2

	ts='{0:04d}'.format(int(ts))
	#blockfile='/home/tgroves/sst/sst/elements/ember/test/plot.' +ts +'.binary'
	blockfile='/home/tgroves/sst/sst/elements/ember/test/amr-files/amr-' + str(ranks) + '-corner-spheres/plot.' + ts + '.binary'
	filetype='binary'

	motifArgs = "nx=" + str(nx) + " ny=" + str(ny) + " nz=" + str(nz)
	motifArgs += " pex=" + str(pex) + " pey=" + str(pey)
	motifArgs += " blockfile="+ blockfile
	motifArgs += " filetype=" + filetype 
	#motifArgs += " verbose=2" 
	print motifArgs

	return motifArgs
