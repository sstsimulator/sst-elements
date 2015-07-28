#!/usr/bin/env python
'''
Date        : 07/17/2015  
Created by  : Fulya Kaplan
Description : This script parses the snapshot xml file that is dumped by the scheduler. It then creates a runscript for ember simulation and runs the script. Created specifically for detailed simulation of network congestion together with scheduling & application mapping algorithms.
'''
#This script is not complete yet!

import os, sys
from xml.dom.minidom import parse
import xml.dom.minidom

from optparse import OptionParser
import math
import numpy as np


class Time:
    def __init__(self):
        self.snapshotTime     = -1
        self.nextArrivalTime  = -1

    def set(self, snapshotTime, nextArrivalTime):
        self.snapshotTime     = snapshotTime
        self.nextArrivalTime  = nextArrivalTime

class Job:
    def __init__(self):
        self.jobNum     = -1
        self.nodeList   = []
        self.numNodes   = -1
        self.motifFile  = ''
        self.startingMotif = -1

    def set(self, jobNum, nodeList, motifFile, startingMotif):
        self.jobNum = jobNum
        self.nodeList = nodeList
        self.numNodes = len(nodeList)
        self.motifFile = motifFile
        self.startingMotif = startingMotif

# Function to run linux commands
def run(cmd, options):
    print(cmd)
    if not options.norun:
        os.system(cmd)

# Parser for the xml file that holds the snapshot from the scheduler
def parse_xml (options):

    fileName = options.xmlFile
    # Open XML document using minidom parser
    DOMTree = xml.dom.minidom.parse(fileName)
    snapshot = DOMTree.documentElement

    # Get all times and jobs in the snapshot
    time = snapshot.getElementsByTagName("time")
    jobs  = snapshot.getElementsByTagName("job")

    # Create a Time object and populate
    TimeObject = Time()

    snapshotTime_ = time[0].getElementsByTagName('snapshotTime')[0]
    snapshotTime  = int(snapshotTime_.childNodes[0].data)
    print "snapshotTime: %d" % snapshotTime
    nextArrivalTime_ = time[0].getElementsByTagName('nextArrivalTime')[0]
    nextArrivalTime  = int(nextArrivalTime_.childNodes[0].data)
    print "nextArrivalTime: %d" % nextArrivalTime

    TimeObject.set(snapshotTime, nextArrivalTime)
    # end

    # Create a list of Job objects and populate
    JobObjects = []

    for job in jobs:
        print "***Job***"
        if job.hasAttribute("number"):
            jobNum = int(job.getAttribute("number"))
            print "Number: %d" % jobNum

        motifFile_ = job.getElementsByTagName('motifFile')[0]
        motifFile  = str(motifFile_.childNodes[0].data)
        #print "motifFile: %s" % motifFile

        startingMotif_ = job.getElementsByTagName('startingMotif')[0]
        startingMotif  = int(startingMotif_.childNodes[0].data)
        #print "startingMotif: %d" % startingMotif

        nodes = job.getElementsByTagName('node')
        NodeList = []
        #NodeList = [[0, []] for i in range(nodes.length)]
        #print nodes[0].childNodes[1].childNodes[0].data
        for node in nodes:
            nodeID = int(node.getAttribute("number"))
            #print "Node: %d" % nodeID

            tasks_ = node.getElementsByTagName('tasks')[0]
            tasks  = str(tasks_.childNodes[0].data)
            #print "TaskList string: %s" % tasksStr
            tasks  = tasks.split(',')
            #print tasksStr

            temp = [nodeID, []]
            for i in range(len(tasks) - 1):
                temp[1].append(int(tasks[i]))
            NodeList.append(temp)
        #print NodeList

        tempJobObject = Job()
        tempJobObject.set(jobNum, NodeList, motifFile, startingMotif)
        JobObjects.append(tempJobObject)
    #end
    
    return (TimeObject, JobObjects)

# Gets a commandline string, puts it into a shell file and runs
def run_ember (execcommand = None):

    print(execcommand)
    shfile = "runEmber.sh"
    print(shfile)
    shellfile = open(shfile, "w")
    print(shellfile)
    shellfile.writelines(execcommand)
    shellfile.close()
    
    cmd  = "chmod +x %s" % shfile
    run(cmd, options)
    cmd  = "%s " % shfile
    run(cmd, options)

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--xml",  action='store', dest="xmlFile", help="Name of the xml file that holds the current scheduler snapshot.") 
    (options, args) = parser.parse_args()
    
    TimeObject, JobObjects = parse_xml (options = options)
    #generate_ember_script ()
    #run_ember ()

if __name__ == '__main__':
    main()

'''
#################################################################################################################
def runEXP_NOPCM (sampling_int= None, options = None, indir = None, outdir = None, ptrace = None, config = None, floorplan = None, thickness = None, conductivity = None, melting_pnt = None, q_file = None, model = None):
    print(indir)
    print(outdir)

####Shellfile and Outfile for EXPERIMENT
    shfile = "%s/%s_nopcm_%s_%s_%s.sh" %  (outdir, options.ename, floorplan, thickness, conductivity)
    outfile = "%s/%s_nopcm_%s_%s.out" %  (outdir, floorplan, thickness, conductivity)

####check if the outfiles already exist
    if os.path.exists(outfile):
        if options.force:
            run("rm -f "+outfile, options)
        else:
            print("error: file exists. " + outfile)
            sys.exit(1)

    if outfile in outFiles:
        print("error: two sims going to the same outfile.")
        print(outfile)
        sys.exit(1)
    outFiles.append(outfile)   
#########################################################################

    execcommand = "source /mnt/nokrb/fkaplan3/tools/addsimulator.sh\n"
    execcommand += "date\n"
    
    if options.mode == "steady":
        
        execcommand += "pushd ../HotSpot-5.02_varCp/\n"
        execcommand += "./hotspot -c ./config_files/" + config + ".config"
        execcommand += " -steady_file ./" + outdir + "/BUpcm_" + thickness + "_" + conductivity +"_" + floorplan + "_" +  ptrace + ".steady"
        execcommand += " -PCM_model off -melting_pnt " + melting_pnt
        execcommand += " -melting_model " + model
        execcommand += " -grid_layer_file ./lcf_files/" + floorplan + "_lcf_files/" + floorplan + "_nopcm_" + thickness + "_" + conductivity + ".lcf"
        execcommand += " -smoothing_width none"
        execcommand += " -latent_heat none"
        execcommand += " -sim_time " + options.sim_time
        execcommand += "\npopd\n"
        execcommand += "date\n"
        
'''