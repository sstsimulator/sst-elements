#! /usr/bin/env python

import sys,getopt,os

from subprocess import call

motif="Halo3D"
path = os.getcwd()

iterations = 1
msgSize = 0
shape = "2"
numCores = 1
debug = 0
sstRanks=0
pex = 1 
pey = 1 
pez = 1 
peflops=0
copyTime=0

merlinBW = "500Mhz"
bytesPerFlit = 32 

def main():
    global iterations
    global msgSize
    global motif 
    global shape
    global numCores
    global debug
    global pex
    global pey
    global pez
    global sstRanks
    global peflops 
    global copyTime

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["msgSize=","iter=","motif=","shape=","debug=",
						"numCores=","pex=","pez=","pey=","sstRanks=","peflops=","copyTime="])
    except getopt.GetopError as err:
        print str(err)
        sys.exit(2)
    for o, a in opts:
        if o in ("--iter"):
            iterations = a
        elif o in ("--msgSize"):
            msgSize = a
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
        elif o in ("--sstRanks"):
            sstRanks = a
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

if 0 == sstRanks:
	sys.exit("how many sst mpi ranks?")

print "numNodes", numNodes, ", numCores", numCores, ", msgSize", msgSize, ", iterations", iterations

network = "--topo=torus --shape=" + shape  
network += " --merlinBW=" + merlinBW
network += " --bytesPerFlit=" + str(bytesPerFlit)

cmdExtra=""

if motif in ("Halo3D"):
	cmdExtra=" pex=" + str(pex)+ " pey=" + str(pey)+  " pez=" + str(pez)
	cmdExtra+=" nx=80 ny=80 nz=192"
	cmdExtra+=" peflops=" + str(peflops)
	cmdExtra+=" copytime=" + str(copyTime)
	cmdExtra+=" itemspercell=" + str(16)

cmdLine="--cmdLine=\\\"" + motif + " iterations=" + str(iterations) + cmdExtra + "\\\"\""

modelOptions="--model-options=\"--printStats=1 --debug=0 " + network + " --numCores=" + str(numCores) + " " + cmdLine

jobname = motif + "-" + str(numNodes) + "-" + str(numCores) + "-" + str(iterations) + "-" + str(peflops) + ".%j.out"

args = [ "-N 2","--exclusive","--ntasks-per-node=8","--output=" + jobname, "./batch.py",  str(sstRanks), modelOptions ]

call( ["echo"] + args )
call( ["sbatch"] + args )
