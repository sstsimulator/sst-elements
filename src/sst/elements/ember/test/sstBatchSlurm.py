#! /usr/bin/env python

#This script is launched by sbatch, originating from run.py

import sys
import os
from subprocess import call
import argparse

# Wall clock time (HH:MM:SS) - once the job exceeds this time, the job will be terminated (default is 5 minutes)
#SBATCH --time=48:00:00        

slurm_nnodes = os.getenv('SLURM_NNODES')

parser = argparse.ArgumentParser()
parser.add_argument('--sst-script',default='/home/tgroves/sst-elements/src/sst/elements/ember/test/emberLoad.py') 
parser.add_argument('--model-options') 
parser.add_argument('--verbose', action='store_true' )
parser.add_argument('--mpiRanksPerNode', default=16 )
parser.add_argument('--mpiRanks', default=0 )

cmdline = parser.parse_args()

if slurm_nnodes and cmdline.verbose:
    print 'SLURM_NNODES', slurm_nnodes 
    print 'SLURM_NODELIST', os.getenv('SLURM_NODELIST')
    print 'SLURM_TASKS_PER_NODE', os.getenv('SLURM_TASKS_PER_NODE')
    print 'SLURM_NTASKS_PER_CORE', os.getenv('SLURM_NTASKS_PER_CORE')

args =  [ 'time' ]
args +=  [ 'mpiexec' ]

# Nodes	
if cmdline.mpiRanks:
	args += ['-n']
	args += [ str( cmdline.mpiRanks ) ]

# tasks
#if cmdline.mpiRanksPerNode:
#	args += ['--ntasks-per-node=' +cmdline.mpiRanksPerNode ]
	
args += ['sst']
#args += ['--verbose']
#args += ['--output-config']
args += ['--partitioner']
args += ['roundrobin']

if cmdline.model_options:
    args += ['--model-options=' +  cmdline.model_options]

args += [cmdline.sst_script]

if cmdline.verbose:
    print "batch: {0}".format( args )

call( args )
