#!/usr/bin/env python
'''
Date        : 05/17/2016  
Created by  : Fulya Kaplan
Description : Run a batch of sst jobs with NetworkSim
'''

import os, sys

from optparse import OptionParser
import os.path, math
import numpy as np
import copy


# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)

def get_runtime(options):

    fileName = options.runtimeFile
    fo = open(fileName, "r")

    InfoPair = []

    for line in fo:
        if line.startswith('Simulation is complete, simulated time'):
            InfoPair.append(grep_timeInfo(line, 'SimComplete'))
            break
        #elif line.startswith('Job Finished'):
        #    InfoPair.append(grep_timeInfo(line, 'JobFinished'))
    
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

    fileName = options.outFile
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


def set_file_name(options):
    exp_name = "N%s_alpha%s_%s_%s_%s_iter%s" %(options.N, options.alpha, options.application, options.allocator, options.mapper, options.iteration)
    options.outdir = "%s/%s/%s" %(options.main_sim_path, options.exp_folder, exp_name)
    
    options.runtimeFile = "%s/ember.out" %(options.outdir)
    print options.runtimeFile
    options.networkFile = "%s/networkStats.csv" %(options.outdir)    

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("-e",  action='store', dest="exp_folder", help="Main experiment folder that holds all subfolders of the experiment.")
    parser.add_option("-o",  action='store', dest="outFile", help="File that holds the processed information.")

    (options, args) = parser.parse_args()

    options.main_sim_path = "/mnt/nokrb/fkaplan3/SST/git/sst/sst-elements/src/sst/elements/scheduler/simulations"
    
    '''    
    for N in [1, 2, 4, 8]: # Each job uses 1/N of the machine
        for alpha in [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4]: 
            for application in ['alltoall', 'bisection', 'mesh']:
                for allocator in ['simple', 'spread', 'random']:
                    for mapper in ['libtopomap']:
    '''
    fileName = options.outFile

    InfoPair = []

    for N in [1]: # Each job uses 1/N of the machine
        if N == 1:
            #applications = ['alltoall', 'bisection', 'mesh']
            applications = ['mesh']
        else:
            applications = ['alltoall', 'mesh']

        for application in applications:
            if application == 'bisection':
                allocators = ['simple']
                mappers = ['simple']
            else:
                #allocators = ['simple', 'spread', 'random']
                #mappers = ['libtopomap']
                allocators = ['simple']
                mappers = ['libtopomap']

            for allocator in allocators:
                for mapper in mappers:
                    #fo = open(fileName, "a")
                    #fo.writelines("%s, %s, %s\n" %(application, allocator, mapper))
                    #fo.close()
                    if allocator == 'random':
                        num_iters = 100
                    else:
                        num_iters = 1

                    InfoPair = []
                    for alpha in [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4]:
                    #for alpha in [4]:
                        for iteration in range(num_iters):
                            options.N = N
                            options.alpha = alpha
                            options.application = application
                            options.allocator = allocator
                            options.mapper = mapper
                            options.iteration = iteration

                            set_file_name(options)
                            InfoPair.append(get_runtime(options))
                    record_runtime(options, InfoPair)

    '''
        for alpha in [4]: 
            for application in ['alltoall']:
                for allocator in ['simple']:
                    for mapper in ['libtopomap']:
                        if allocator == 'random':
                            num_iters = 10
                        else:
                            num_iters = 1
                        for iteration in range(num_iters):
                            for motif_iteration in range(10,110,10):
                                options.N = N
                                options.alpha = alpha
                                options.application = application
                                options.allocator = allocator
                                options.mapper = mapper
                                options.iteration = iteration
                                options.motif_iteration = motif_iteration
                                set_file_name(options)
                                InfoPair.append(get_runtime(options))

    record_runtime(options, InfoPair)
    '''


if __name__ == '__main__':
    main()
