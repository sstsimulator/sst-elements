#! /usr/bin/env python

import sys,getopt,os

from subprocess import call

path = os.getcwd()

shape = "2"
numCores = 1
debug = 0
sstRanksPerNode=1
sstNodes=1
motif="Barrier"
motifArgs=""

try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["shape=","debug=","numCores=",
                    "sstRanksPerNode=","sstNodes=","motif=","motifArgs="])

except getopt.GetopError as err:
    print str(err)
    sys.exit(2)

for o, a in opts:
    if o in ("--numCores"):
        numCores = a
    elif o in ("--shape"):
        shape = a
    elif o in ("--debug"):
        debug = a
    elif o in ("--sstRanksPerNode"):
        sstRanksPerNode = a
    elif o in ("--sstNodes"):
        sstNodes = a
    elif o in ("--motif"):
        motif = a
    elif o in ("--motifArgs"):
        motifArgs = a
    else:
        assert False, "unhandle option"

def calcNumNodes( shape ):
    tmp = shape.split( 'x' )
    num = 1
    for d in tmp:
        num = num * int(d)
    return num

numNodes = calcNumNodes( shape )

sstRanks = int(int(sstNodes) * int(sstRanksPerNode)) 

if 0 == sstRanks:
	sys.exit("how many sst mpi ranks?")

print "MPI: numNodes={0} ranksPerNode={1} totalRanks={2}".format(
                                sstNodes,sstRanksPerNode,sstRanks) 

print "Sim: numNodes={0} numCores={1}".format(numNodes,numCores)

network = "--topo=torus --shape=" + shape  

cmdLine="--cmdLine=\\\"" + motif + " " + motifArgs + "\\\"\""

print "cmd={0}".format(cmdLine)

modelOptions="--model-options=\"--printStats=0 --debug=0 " + network + " --numCores=" + str(numCores) + " " + cmdLine

jobname = motif + "-" + str(numNodes) + "-" + str(numCores) + ".%j.out"

args = [ "-N " + str(sstNodes),"--exclusive","--ntasks-per-node="+str(sstRanksPerNode),"--output=" + jobname, "./batch.py",  str(sstRanks), modelOptions ]

call( ["echo"] + args )
call( ["sbatch"] + args )
