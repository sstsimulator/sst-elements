#! /usr/bin/env python

from subprocess import call
import sys

numNodes=4
mpiRanksPerNode=24
mpiRanks=mpiRanksPerNode*numNodes

#arbList = ['rr','age','lru','rand']
arbList = ['rand']

linkList = [ 125, 50, 12.5 ]
#linkList = [ 125 ]

#loadOrderList = ['True','false']
#loadOrderList = ['True']
loadOrderList = ['false']

thin = {
	'Name': 'thin',
	'NumNodes': 131072,
	'Links': linkList,
	'Arbitration': arbList,
}

medium = {
	'Name': 'medium',
	'NumNodes': 65536,
	'Links': linkList,
	'Arbitration': arbList,
}

fat = {
	'Name': 'fat',
	'NumNodes': 65536,
	'Links': linkList,
	'Arbitration': arbList,
}

platformList = [ thin, medium, fat ]
#platformList = [ thin ]

sweep3d = { 
	'Name': 'Sweep3D',
	#'Ranks' : [1024,2048,4096,8192,16384,32768,65536,131072]
	'Ranks' : [16384,32768,65536,131072]
}

halo3d = { 
	'Name': 'Halo3D26',
	'Ranks' : [1024,2048,4096,8192,16384,32768,65536,131072]
}

amr = { 
	'Name': '3DAMR',
	'Ranks' : [512,1024,2048,4096,8192,16384,32768,65536]
}

fft3d = { 
	'Name': 'FFT3D',
	'Ranks' : [ 1024,4096,16384,65536,131044 ] 
}


#motifList = [ sweep3d, halo3d, fft3d ]
motifList = [ amr ]
#motifList = [ halo3d ]

for loadOrder in loadOrderList:
	for motif in motifList:
		for ranks in motif['Ranks']:
			for platform in platformList:
				if ranks > platform['NumNodes']:
					continue			

				for link in platform['Links']:
					for arb in platform['Arbitration']:
						callArgs = ['./run.py']
						#callArgs += ['--topo=' + 'torus' ]
						callArgs += ['--motif=' + motif['Name'] ]
						callArgs += ['--motifRanks=' + str(ranks) ]
						callArgs += ['--nodeType=' + platform['Name'] ]
						callArgs += ['--linkType='+ str(link) ]
						callArgs += ['--rtrArb=' + arb ]
						callArgs += ['--rand=' + loadOrder ]
					#callArgs += ['--voltaNodes=' + str(numNodes) ]
						callArgs += ['--mpiRanks=' + str(mpiRanks) ]
						callArgs += ['--mpiRanksPerNode=' + str(mpiRanksPerNode) ]
						g = ''
						for  x in callArgs:				
							g = g + ' ' + x
						print g 
						#call( callArgs ) 
