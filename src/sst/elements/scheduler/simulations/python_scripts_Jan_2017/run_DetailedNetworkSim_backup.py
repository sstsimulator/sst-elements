#!/usr/bin/env python
'''
Date        : 08/04/2015  
Created by  : Fulya Kaplan
Description : This main python script runs scheduler and ember simulations successively until completion.
'''

import os, sys
from xml.dom.minidom import parse
import xml.dom.minidom

from optparse import OptionParser
import math
import numpy as np

# Function to run linux commands
def run(cmd):
    #print(cmd)
    rtn = os.system(cmd)
    return rtn

def clear_files(options):

    erFile = open(options.emberRunningFile, 'w')
    ecFile = open(options.emberCompletedFile, 'w')

    erFile.close()
    ecFile.close()

def delete_logs(options):

    cmd = "rm %s/motif*.log" %(options.output_folder)
    run(cmd)

def run_sim (options):

    # Run scheduler for the first time and create the first snapshot
    init_cmd  = "sst %s" %(options.schedPythonFile)
    run(init_cmd)

    # Do the following in a loop until the simulation is completed
    # Parse scheduler snapshot->run ember->Parse ember output->run scheduler->...
    if options.shuffle == True:
        ember_cmd = "./%s --xml %s --alpha %s --link_arrangement %s --routing %s --rankmapper %s --shuffle > %s" %(options.sched_parser, options.xmlFile, options.alpha, options.link_arrangement, options.routing, options.rankmapper, options.emberOutFile)
    else:
        ember_cmd = "./%s --xml %s --alpha %s --link_arrangement %s --routing %s --rankmapper %s > %s" %(options.sched_parser, options.xmlFile, options.alpha, options.link_arrangement, options.routing, options.rankmapper, options.emberOutFile)

    #ember_cmd = "./%s --xml %s > %s" %(options.sched_parser, options.xmlFile, options.emberOutFile)
    #ember_cmd = "./%s --xml %s --alpha %s" %(options.sched_parser, options.xmlFile, options.alpha)
    run(ember_cmd)

    '''
    while( is_not_empty(options.xmlFile) ):
        ember_cmd = "./%s --xml %s > %s" %(options.sched_parser, options.xmlFile, options.emberOutFile)
        run(ember_cmd)

        sched_cmd = "./%s --xml %s --emberOut %s --schedPy %s --ember_completed %s --ember_running %s " %(options.ember_parser, options.xmlFile, options.emberOutFile, options.schedPythonFile, options.emberCompletedFile, options.emberRunningFile)
        Rtnvalue = run(sched_cmd)
        if Rtnvalue != 0 :
             sys.stderr.write(" run(sched_cmd)  returned NON zero \n")
             sys.exit(1)
    '''

    delete_logs(options)

def is_not_empty(fileName):

    try:
        if os.stat(fileName).st_size > 0:
            return True
        else:
            return False 
    except OSError:
        print "No file"

'''
def set_path_emberLoad(options):

    emberpath="/home/fkaplan/SST/scratch/src/sst-simulator/sst/elements/ember/test"
    cmd = "sed -i \"s|PATH|%s|g\" emberLoad.py" %(emberpath)
    print cmd
    run(cmd)
'''
def grep_set_fileNames(options):

    # Parser script names are defined by default
    options.sched_parser = "snapshotParser_sched.py"
    options.ember_parser = "snapshotParser_ember.py"

    output_folder = os.getenv('SIMOUTPUT')
    if output_folder == None:
        options.output_folder = "./"
    else:
        options.output_folder = output_folder

    options.emberOutFile = options.output_folder + options.emberOutFile

    # Grep other file names from the scheduler input configuration file
    inputPythonFile = open(options.schedPythonFile, 'r')

    for line in inputPythonFile:
        if "traceName" in line:
            temp = line.split(':')[1]
            temp = temp.split('\"')[1]
            temp = temp.split('/')[-1]
            options.xmlFile = options.output_folder + temp + ".snapshot.xml"
            print options.xmlFile
        elif "completedJobsTrace" in line:
            temp = line.split(':')[1]
            temp = temp.split('\"')[1]
            options.emberCompletedFile = options.output_folder + temp
            #print options.emberCompletedFile
        elif "runningJobsTrace" in line:
            temp = line.split(':')[1]
            temp = temp.split('\"')[1]
            options.emberRunningFile = options.output_folder + temp
            #print options.emberRunningFile

    inputPythonFile.close()

    return (options)


def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--emberOut",  action='store', dest="emberOutFile", help="Name of the ember output file.")
    parser.add_option("--schedPy",  action='store', dest="schedPythonFile", help="Name of the python file that holds the scheduler parameters.")
    parser.add_option("--alpha",  action='store', dest="alpha", help="Alpha = Global_link_BW / Local_link_BW.") 
    parser.add_option("--link_arrangement",  action='store', dest="link_arrangement", help="Global link arrangement for dragonfly.") 
    parser.add_option("--routing",  action='store', dest="routing", help="Routing algorithm.") 
    parser.add_option("--rankmapper",  action='store', dest="rankmapper", help="Custom or linear mapping.") 
    parser.add_option("--shuffle",  action='store_true', dest="shuffle", help="Random shuffling of the node list order.") 

    (options, args) = parser.parse_args()

    options = grep_set_fileNames(options)
    clear_files(options)
    #set_path_emberLoad(options)
    run_sim(options)
    

if __name__ == '__main__':
    main()
