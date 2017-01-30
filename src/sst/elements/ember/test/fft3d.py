
import math
import sys

#chama
#nsPerElement = 5.9
#cost = nsPerElement * x * ((y * z)/numPe)
flopsPerElement=125
fudge = [ 2.4, 2.5, 1.50, 2.0, 3.17, 1.60 ]

# bgq
#nsPerElement = 50
#nsPerElement = 1
#fudge = [ 1.51, 1.33, 1.44, 1.22, 1.32, 1.29 ]

def getParams( ranks, flopsPerRank ):
	if int(ranks) >= 100000:
		size=332*6
	else:
		size=128*2

	nsPerElement = (flopsPerElement / flopsPerRank) * (1000*1000*1000)

	print "FFT3D: flopsPerElement={0} flopwPerRank={1} nsPerElement={2}".format( flopsPerElement, '%.0f' % flopsPerRank, nsPerElement ) 

	a,npRows = math.modf( math.sqrt( float(ranks) ) )
	npRows = int(npRows)
	if a != 0.0:
		sys.exit('ERROR: no integer sqrt ' + ranks );

	print 'FFT3D: ranks={0} x=y=z={1} npRows={2}'.format( ranks, size, npRows )

	motifArgs='nx=' + str(size) + ' ny=' + str(size) + ' nz=' + str(size)
	motifArgs+=' npRow='+str(npRows)
	motifArgs+=' nsPerElement=' + str(nsPerElement)
	motifArgs+=' fwd_fft1=' + str( fudge[0] )
	motifArgs+=' fwd_fft2=' + str( fudge[1] )
	motifArgs+=' fwd_fft3=' + str( fudge[2] )
	motifArgs+=' bwd_fft1=' + str( fudge[3] )
	motifArgs+=' bwd_fft2=' + str( fudge[4] )
	motifArgs+=' bwd_fft3=' + str( fudge[5] )

	return motifArgs
