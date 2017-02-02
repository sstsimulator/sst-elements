#!/usr/bin/env python
'''
Date        : 08/17/2015  
Created by  : Fulya Kaplan
Description : This script is used to generate a database of ember running times for various applications (CTH, miniamr...) and number of nodes.
'''

import os, sys

from optparse import OptionParser
import math
import numpy as np
import copy

class Application:
    def __init__(self):
        self.numNodes     = -1
        self.numBoundaryIter  = -1
        self.numAllreduceIter = -1
        self.computeTime = -1
        self.nx = -1
        self.ny = -1
        self.nz = -1
        self.numTimeSteps = -1
        self.timeStepMotif = []

    def set(self, numNodes, numBoundaryIter, numAllreduceIter, computeTime, nx, ny, nz, numTimeSteps):
        self.numNodes     = numNodes
        self.numBoundaryIter  = numBoundaryIter
        self.numAllreduceIter = numAllreduceIter
        self.computeTime = computeTime
        self.nx = nx
        self.ny = ny
        self.nz = nz
        self.numTimeSteps = numTimeSteps

        self.timeStepMotif.append("Halo3D iterations=%s computetime=%s doreduce=0 nx=%s ny=%s nz=%s\n" % (self.numBoundaryIter, self.computeTime, self.nx, self.ny, self.nz))
        self.timeStepMotif.append("Allreduce iterations=%s compute=0\n" % (self.numAllreduceIter))

# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)

# Generates necessary files for ember to run
def generate_ember_files (options, Application):

    loadfile    = generate_loadfile (Application)
    execcommand = generate_ember_script (options, loadfile)

    return (execcommand)

# Generates a loadfile for ember
def generate_loadfile (Application):

    loadfile  = "loadfile"
    #loadfile  = "CTH_%s.phase" % (Application.numNodes * 4)
    # Open loadfile to write 
    ldfile = open(loadfile, "w")

    # Keywords used in the loadfile format
    J_KEY = "[JOB_ID] "
    N_KEY = "[NID_LIST] "
    M_KEY = "[MOTIF] "

    '''
    J_KEY = ""
    N_KEY = ""
    M_KEY = ""
    '''

    # Populate loadfile with the job & nodeID & motif info 
    ldfile_str = ""

    # Insert Job Number
    ldfile_str += J_KEY + str(0) + "\n\n"

    # Insert NodeID List
    ldfile_str += N_KEY
    for nodeID in range(Application.numNodes):
        if(nodeID == Application.numNodes - 1):
            ldfile_str += str(nodeID) + "\n"
        else:
            ldfile_str += str(nodeID) + ","

    # Insert motifs based on the Application parameters
    ldfile_str += M_KEY + "Init\n"

    for it in range(Application.numTimeSteps):
        ldfile_str += M_KEY + Application.timeStepMotif[0]
        ldfile_str += M_KEY + Application.timeStepMotif[1]

    ldfile_str += M_KEY + "Stop\n"
    ldfile_str += M_KEY + "Fini\n"

    # Write this job's info to the loadfile
    ldfile.writelines(ldfile_str)
    ldfile.close()

    return (loadfile)

# Generates a script for ember to run
def generate_ember_script (options, loadfile):
    
    emberLoad = "emberLoad.py"
    
    # Generate commandline string to execute
    execcommand = "sst "
    #execcommand += " --model-options=\"--topo=torus --shape=5x4x4 --numCores=4 --netFlitSize=8B --netPktSize=1024B --netBW=4GB/s --emberVerbose=0 --printStats=1"
    execcommand += " --model-options=\"--topo=dragonfly --shape=7:2:2:4 --numCores=4 --netFlitSize=8B --netPktSize=1024B --netBW=4GB/s --emberVerbose=0 --printStats=1"
    execcommand += " --embermotifLog=/home/fkaplan/SST/scratch/src/sst-simulator/sst/elements/scheduler/simulations/motif"
    execcommand += " --rankmapper=ember.CustomMap"
    execcommand += " --loadFile=" + loadfile + "\""
    execcommand += " " + emberLoad + " > " + options.emberOutFile + "\n"
    #execcommand += " " + emberLoad + "\n"

    return (execcommand)

# Gets a commandline string, puts it into a shell file and runs
def run_ember (execcommand):

    run(execcommand)

def get_runtime(options):

    fileName = "%s/%s" %(options.folder, options.emberOutFile)
    fo = open(fileName, "r")

    InfoPair = []

    for line in fo:
        if line.startswith('Simulation is complete, simulated time'):
            InfoPair.append(grep_timeInfo(line, 'SimComplete'))
            break
        elif line.startswith('Job Finished'):
            InfoPair.append(grep_timeInfo(line, 'JobFinished'))
    
    fo.close()

    return (InfoPair)

def reorder_InfoPair(InfoPair):

    ordered_InfoPair = copy.deepcopy(InfoPair)

    for pair in InfoPair:
        index = pair[1] + 1
        ordered_InfoPair[index][0] = pair[0]
        ordered_InfoPair[index][1] = pair[1]

    return (ordered_InfoPair)

def record_runtime(options, InfoPair):

    fileName = options.runtimeOutFile
    fo = open(fileName, "a")

    #InfoPair = reorder_InfoPair(InfoPair)

    line = ""
    for pair in InfoPair:
        line += "%d\t" % (int(pair[0][0]))
        #line += "%d\t" % (int(pair[0]))
    line += "\n"

    fo.writelines(line)
    fo.close()

# Extracts the necessary time, jobNum info from the given string
def grep_timeInfo(string, mode):

    if mode == 'SimComplete':
        string = string.split(' ')
        number = float(string[5])
        unit   = string[6].split('\n')[0]
        jobNum = -1

        time = convertToMicro(number, unit)
        return ([time, jobNum])

    elif mode == 'JobFinished':
        string = string.split(' ')
        jobNum = int(string[2].split(':')[1])
        number = float(string[3].split(':')[1])
        unit   = string[4].split('\n')[0]

        time = convertToMicro(number, unit)
        return ([time, jobNum])

# Converts the units for the time info to microseconds
def convertToMicro(number, unit):

    if (unit == 'Ks'):
        convertedNum = number*1000000000
    elif (unit == 's'):
        convertedNum = number*1000000
    elif (unit == 'ms'):
        convertedNum = number*1000
    elif (unit == 'us'):
        convertedNum = number
    elif (unit == 'ns'):
        convertedNum = float(number/1000)
    else:
        print "ERROR: Valid time units: [Ks | s | ms | us | ns]"
        sys.exit(0)

    return (convertedNum)

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--emberOut",  action='store', dest="emberOutFile", help="Name of the ember output file.")
    parser.add_option("--runtimeOut",  action='store', dest="runtimeOutFile", help="Name of the file to record the running times.")
    (options, args) = parser.parse_args()


    fo = open(options.runtimeOutFile, "w")
    fo.close()
        
    #for application in ['GD99_b', 'Sandi_authors', 'GD98_c', 'grid1_dual', 'grid1', 'sphere3', 'alltoall']:
    #for application in ['grid1', 'sphere3', 'alltoall']:
    for application in ['alltoall']:
    #for application in ['GD99_b']:
        InfoPair = []
        #for alpha in [0.125, 0.25, 0.5, 0.625, 0.75, 0.875, 1]:
        for alpha in [1, 1.5, 2, 2.5, 3, 3.5, 4]:
        #for mapper in ['PaCMap']:
        #for mapper in ['spread', 'simple', 'PaCMap', 'random']:
        #for mapper in ['libtopomap', 'nearestamap', 'PaCMap', 'random']:
            mapper = 'random'
            if mapper == 'random':
                num_iters = 1
            else:
                num_iters = 1 
            for iteration in range(num_iters):
                options.folder = "N1_allexperiments_absolute/N1_alpha%s_alltoall_simple_libtopomap_iter0" %(alpha)
                #execcommand = "./run_DetailedNetworkSim.py --emberOut ember.out --schedPy test_MappingImpact_%s_%s.py " % (mapper, application)
                #run(execcommand)
                InfoPair.append(get_runtime(options))
        record_runtime(options, InfoPair)

            #fileName = options.runtimeOutFile
            #fo = open(fileName, "a")
            #fo.writelines("\n")
    fo.close()

if __name__ == '__main__':
    main()
