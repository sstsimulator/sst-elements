
import math
import sys

def getParams( ranks, flopsPerRank ):

	print "Halo3D ranks={0} flopsPerRank={1}".format(ranks,flopsPerRank)

	#dim = 100
	dim = 100 
	nx=dim
	ny=dim
	nz=dim

	pex=1
	pey=1
	pez=1

	count=0
	if (ranks == 48*48*48):
		pex = pey = pez = 48
	while pex * pey * pez < ranks:
		if count == 0: 
			pex *= 2
		if count == 1: 
			pey *= 2
		if count == 2: 
			pez *= 2
		count += 1
		
		if count == 3:
			count = 0


	#PARAMETER 4 = arg.nx (Sets the problem size in X-dimension) [100]
	#PARAMETER 5 = arg.ny (Sets the problem size in Y-dimension) [100]
	#PARAMETER 6 = arg.nz (Sets the problem size in Z-dimension) [100]
	motifArgs = "nx=" + str(nx) + " ny=" + str(ny) + " nz=" + str(nz)

	#PARAMETER 7 = arg.pex (Sets the processors in X-dimension (0=auto)) [0]
	#PARAMETER 8 = arg.pey (Sets the processors in Y-dimension (0=auto)) [0]
	#PARAMETER 9 = arg.pez (Sets the processors in Z-dimension (0=auto)) [0]
	motifArgs += " pex=" + str(pex) + " pey=" + str(pey) + " pez=" + str(pez)

	#PARAMETER 3 = arg.peflops (Sets the FLOP/s rate of the processor (used to calculate compute time if not supplied, default is 10000000000 FLOP/s)) [10000000000]
	motifArgs += " peflops="+ '%.0f' % flopsPerRank 
	
	#PARAMETER 10 = arg.copytime (Sets the time spent copying data between messages) [5]
	#PARAMETER 11 = arg.doreduce (How often to do reduces, 1 = each iteration) [1]

	#PARAMETER 12 = arg.fields_per_cell (Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1) [1]
	motifArgs += " fields_per_cell=16"

	#PARAMETER 13 = arg.field_chunk (Specify how many variables are being computed per cell (this is one of the dimensions in message size. Default is 1) [1]
	#PARAMETER 14 = arg.datatype_width (Specify the size of a single variable, single grid point, typically 8 for double, 4 for float, default is 8 (double). This scales message size to ensure
	#print motifArgs

	return motifArgs
