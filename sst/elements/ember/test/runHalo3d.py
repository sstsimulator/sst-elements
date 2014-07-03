#! /usr/bin/env python

import sys,getopt,os

from subprocess import call

motif="Halo3D"
path = os.getcwd()

iterations = 1
shape = "2"
numCores = 1
debug = 0
sstRanksPerNode=1
sstNodes=1
pex = 1 
pey = 1 
pez = 1 
peflops=0
copyTime=0

def main():
    global iterations
    global motif 
    global shape
    global numCores
    global debug
    global pex
    global pey
    global pez
    global sstRanksPerNode
    global sstNodes
    global peflops 
    global copyTime

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["iter=","motif=",
                "shape=","debug=","numCores=","pex=","pez=","pey=",
                "sstRanksPerNode=","sstNodes=","peflops=","copyTime="])

    except getopt.GetopError as err:
        print str(err)
        sys.exit(2)
    for o, a in opts:
        if o in ("--iter"):
            iterations = a
        elif o in ("--motif"):
            motif = a
        elif o in ("--numCores"):
            numCores = a
        elif o in ("--shape"):
            shape = a
        elif o in ("--debug"):
            debug = a
        elif o in ("--pex"):
            pex = a
        elif o in ("--pey"):
            pey = a
        elif o in ("--pez"):
            pez = a
        elif o in ("--sstRanksPerNode"):
            sstRanksPerNode = a
        elif o in ("--sstNodes"):
            sstNodes = a
        elif o in ("--peflops"):
            peflops = a
        elif o in ("--copyTime"):
            copyTime = a
        else:
            assert False, "unhandle option"

    
main()    

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

print "numNodes", numNodes, ", numCores", numCores, ", iterations", iterations

network = "--topo=torus --shape=" + shape  

cmdExtra=""

if motif in ("Halo3D"):
	cmdExtra=" pex=" + str(pex)+ " pey=" + str(pey)+  " pez=" + str(pez)
	cmdExtra+=" nx=80 ny=80 nz=192"
	cmdExtra+=" peflops=" + str(peflops)
	cmdExtra+=" copytime=" + str(copyTime)
	cmdExtra+=" itemspercell=" + str(16)

cmdLine="--cmdLine=\\\"" + motif + " iterations=" + str(iterations) + cmdExtra + "\\\"\""

modelOptions="--model-options=\"--printStats=0 --debug=0 " + network + " --numCores=" + str(numCores) + " " + cmdLine

jobname = motif + "-" + str(numNodes) + "-" + str(numCores) + "-" + str(iterations) + "-" + str(peflops) + ".%j.out"

args = [ "-N " + str(sstNodes),"--exclusive","--ntasks-per-node="+str(sstRanksPerNode),"--output=" + jobname, "./batch.py",  str(sstRanks), modelOptions ]

call( ["echo"] + args )
call( ["sbatch"] + args )
