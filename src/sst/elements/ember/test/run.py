#! /usr/bin/env python

#This script runs through collecting different options to set the network topology, flops, motif's etc
#Then these parameters are all put into 'sbatch' which calls 'sstBatchSlurm.py', passing the args

from subprocess import call
from datetime import datetime
import os
import sys
import link
import node 
import math
import allpingpong
import alltoall
import alltoallv
import allreduce
import amr 
import bcast
import fft3d
import halo3d26
import halo3d
import naslu
import randommotif
import reduce
import sweep3d

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--nodeType', default='test')
parser.add_argument('--linkType', default='12.5')
parser.add_argument('--rtrArb')
parser.add_argument('--motifRanks', default=64)
parser.add_argument('--motif', default='')
parser.add_argument('--platform', default='default')
parser.add_argument('--dfAlg')
parser.add_argument('--rand', default='False')
parser.add_argument('--interactive', default=False)
parser.add_argument('--mpiRanksPerNode', default=1 )
parser.add_argument('--mpiRanks', default=1 )
parser.add_argument('--topo', default='dragonfly2' )
parser.add_argument('--chamaNodes', default=0 )
parser.add_argument('--time', default='04:00:00' )
parser.add_argument('--account', default='FY140001' )

cmdline = parser.parse_args()
#print cmdline

if cmdline.chamaNodes > 0:
	cmdline.mpiRanksPerNode = 16 
	cmdline.mpiRanks = int(cmdline.chamaNodes) * int(cmdline.mpiRanksPerNode)

print "chamaNodes={0} mpiRanksPerNode={1} mpiRanks={2}".format( cmdline.chamaNodes, cmdline.mpiRanksPerNode, cmdline.mpiRanks)

linkBW, flitSize = link.getLinkParams( cmdline.linkType )
numCores, flopsCore = node.getParams( cmdline.nodeType )

if cmdline.motifRanks == 0:
	sys.exit("ERROR motifRanks")

#print cmdline.motif[0:5] 

extra=''
if cmdline.motif == 'AllPingPong':
	outdir='allpingpong'
	motifArgs = allpingpong.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Alltoall':
	outdir='alltoall'
	motifArgs = alltoall.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Alltoallv':
	outdir='alltoallv'
	motifArgs = alltoallv.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Allreduce':
	outdir='allreduce'
	motifArgs = allreduce.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif[0:5] == '3DAMR':
	outdir='amr'
	cmdline.motif, extra = cmdline.motif.split('.')
	motifArgs = amr.getParams( int(cmdline.motifRanks), float(flopsCore), extra )
elif cmdline.motif == 'Bcast':
	outdir='bcast'
	motifArgs = bcast.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'FFT3D':
	outdir='fft3d'
	motifArgs = fft3d.getParams( cmdline.motifRanks, float(flopsCore) )
elif cmdline.motif == 'Halo3D':
	outdir='halo3d'
	motifArgs = halo3d.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Halo3D26':
	outdir='halo3d26'
	motifArgs = halo3d26.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'NASLU':
	outdir='naslu'
	motifArgs = naslu.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Random':
	outdir='random'
	motifArgs = randommotif.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Reduce':
	outdir='reduce'
	motifArgs = reduce.getParams( int(cmdline.motifRanks), float(flopsCore) )
elif cmdline.motif == 'Sweep3D':
	outdir='sweep3d'
	motifArgs = sweep3d.getParams( int(cmdline.motifRanks), float(flopsCore) )
else:
	sys.exit("ERROR: can't find motif ")


numNodes = int(cmdline.motifRanks)/numCores
	
MaxRanks = cmdline.motifRanks

batch='/home/tgroves/sst-elements/src/sst/elements/ember/test/sstBatchSlurm.py'

motifs = ' --cmdLine=Init'
tmp = cmdline.motif + ' ' + motifArgs 
print  tmp
motifs += ' --cmdLine=\"' + tmp +  '\"'  
motifs += ' --cmdLine=Fini'

while cmdline.motifRanks <= MaxRanks:
	#print "run: ranks={0}".format(cmdline.motifRanks)

	options = '--topo=' + cmdline.topo  
	options += ' --numCores=' + str(numCores)
	options += ' --numNodes=' + str(numNodes)
	options += ' --platform=' + cmdline.platform
	if cmdline.dfAlg:
		options += ' --dfAlg=' + cmdline.dfAlg
	if cmdline.rtrArb:
		options += ' --rtrArb=xbar_arb_' + cmdline.rtrArb 
	if linkBW:
		options += ' --netBW=' + linkBW
		options += ' --netFlitSize=' + flitSize 
	options += ' --randomPlacement=' + cmdline.rand 
	options += motifs

	cmd = ['--verbose']
	cmd += ['--mpiRanks=' + str(cmdline.mpiRanks) ]
	cmd += ['--mpiRanksPerNode=' + str(cmdline.mpiRanksPerNode)]
	cmd += ['--model-options=' + options ]

	if cmdline.interactive:
		print "run: {0}".format( cmd )
		call( [batch] + cmd )
	else:
		callArgs = ['sbatch'] 
		numNodes = int(cmdline.mpiRanks)/int(cmdline.mpiRanksPerNode)	
		callArgs += ['-N ' + str(numNodes) ] 
		callArgs += ['--time=' + str(cmdline.time) ]
		callArgs += ['--account=' + str(cmdline.account) ]
		outdir += '-' + cmdline.topo
		outdir += '-' + cmdline.platform
		if cmdline.dfAlg:
			outdir += '-' + cmdline.dfAlg
		if cmdline.rand == "True":
			outdir += '-rp' 
		if linkBW:
			outdir += '-' + str(linkBW).strip('/s')
		dt = datetime.now().strftime("%y-%m-%d-%H-%M-%S")
		dir = '/home/tgroves/results/exanetworks/' + outdir + '/' + dt + '/'
		if not os.path.exists(dir):
		    os.makedirs(dir)
		# This is to ensure the statistics are placed in the correct directory
		# Requires that PYTHONPATH be set to point to all the imported code
		os.chdir(dir)
		outputFile = '--output=' + dir
		if extra:
			outputFile += extra + '-'
		outputFile += str(cmdline.motifRanks) + '-'
		outputFile += cmdline.linkType + '-' 
		if cmdline.rtrArb:
			outputFile += cmdline.rtrArb + '-'
		if cmdline.rand == "True":
			outputFile += 'rp' 
		else:
			outputFile += 'lp' 
		outputFile += '.out'
		callArgs += [ outputFile]
		callArgs += [batch] + cmd 
		print "run: {0}".format( callArgs )
		call( callArgs )

	cmdline.motifRanks *= 2
