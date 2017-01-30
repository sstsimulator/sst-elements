#!/usr/bin/env python
'''
Date        : 07/17/2015  
Created by  : Fulya Kaplan
Description : This sub-python script parses the snapshot xml file that is dumped by the scheduler. It then creates a runscript for ember simulation and runs the script. Created specifically for detailed simulation of network congestion together with scheduling & application task mapping algorithms.
'''

import os, sys
from xml.dom.minidom import parse
import xml.dom.minidom

from optparse import OptionParser
import math
import numpy as np
import random


class Time:
    def __init__(self):
        self.snapshotTime     = -1
        self.nextArrivalTime  = -1

    def set(self, snapshotTime, nextArrivalTime):
        self.snapshotTime     = snapshotTime
        self.nextArrivalTime  = nextArrivalTime

class Job:
    def __init__(self):
        self.jobNum        = -1
        self.nodeList      = []
        self.numNodes      = -1
        self.numCores      = -1
        self.motifFile     = ''
        self.startingMotif = -1

    def set(self, jobNum, nodeList, motifFile, startingMotif):
        self.jobNum        = jobNum
        self.nodeList      = nodeList
        self.numNodes      = len(nodeList)
        self.numCores      = len(nodeList[0][1])
        self.motifFile     = motifFile
        self.startingMotif = startingMotif


# Function to run linux commands
def run(cmd):
    #print(cmd)
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
    print
    print "snapshotTime: %d" % snapshotTime
    nextArrivalTime_ = time[0].getElementsByTagName('nextArrivalTime')[0]
    nextArrivalTime  = int(nextArrivalTime_.childNodes[0].data)
    print "nextArrivalTime: %d" % nextArrivalTime

    TimeObject.set(snapshotTime, nextArrivalTime)
    # end

    # Create a list of Job objects and populate
    JobObjects = []

    print "***Currently Running Jobs***"
    for job in jobs:
        if job.hasAttribute("number"):
            jobNum = int(job.getAttribute("number"))
            print "Job Number: %d" % jobNum

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
        if options.shuffle == True:
            random.shuffle(NodeList)

        # Find number of cores per node
        numCores = len(NodeList[0][1])

        # Sort NodeList to match the desired mapping in ember
        #Fulya Jan 2016: Commented out sorting, fix later
        #sortedNodeList = sort_NID(NodeList, numCores)
        sortedNodeList = NodeList

        tempJobObject = Job()
        tempJobObject.set(jobNum, sortedNodeList, motifFile, startingMotif)
        JobObjects.append(tempJobObject)
    #end
    
    return (TimeObject, JobObjects)

def sort_NID (nodeList, numCores):

    sorted_nodeList = [[] for i in range(len(nodeList))]

    for node in nodeList:
        index = node[1][0] / numCores
        sorted_nodeList[index] = node
        #print "First Task Num: %s Index: %s" % (node[1][0], index)
        #print sorted_nodeList[index]

    return (sorted_nodeList)

# Generates necessary files for ember to run
def generate_ember_files (TimeObject, JobObjects, options):

    output_folder = os.getenv('SIMOUTPUT')
    if output_folder == None:
        options.output_folder = "./"
    else:
        options.output_folder = output_folder

    loadfile    = generate_loadfile (TimeObject, JobObjects, options)
    mapfile     = generate_mapfile (JobObjects, options)
    execcommand = generate_ember_script (TimeObject, JobObjects, loadfile, mapfile, options)

    return (execcommand)

# Generates a loadfile for ember
def generate_loadfile (TimeObject, JobObjects, options):

    loadfile  = options.output_folder + "loadfile"
    # Open loadfile to write 
    ldfile = open(loadfile, "w")

    # Keywords used in the loadfile format
    J_KEY = "[JOB_ID] "
    N_KEY = "[NID_LIST] "
    M_KEY = "[MOTIF] "

    # Populate loadfile with the job & nodeID & motif info 
    for Job in JobObjects:
        ldfile_str = ""

        # Insert Job Number
        ldfile_str += J_KEY + str(Job.jobNum) + "\n\n"

        #DELETE LATER
        #Job = get_best_cut_nodeList(Job, options)
        #print Job.nodeList

        # Insert Sorted NodeID List
        ldfile_str += N_KEY
        for nodelist in Job.nodeList:
            if(nodelist == Job.nodeList[-1]):
                ldfile_str += str(nodelist[0]) + "\n"
            else:
                ldfile_str += str(nodelist[0]) + ","

        # Insert Motifs from the motifFile
        mFile = open(Job.motifFile, "r")
        mFile_str = mFile.readlines()

        if (Job.startingMotif != 0):
            ldfile_str += M_KEY + "Init\n"

        for motifNum in range(Job.startingMotif, len(mFile_str)):
            if (mFile_str[motifNum] != "\n"):
                ldfile_str += M_KEY + mFile_str[motifNum]
        mFile.close()

        ldfile_str += "\n"

        # Write this job's info to the loadfile
        ldfile.writelines(ldfile_str)
    ldfile.close()

    return (loadfile)

def get_best_cut_nodeList(Job, options):

    if options.link_arrangement == 'absolute':
        if options.alpha == '0.5' or options.alpha == '1':
            for i in range(72):
                Job.nodeList[i][0] = i     
        else:
            i = 0
            for j in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 24, 25, 26, 27, 32, 33, 34, 35, 40, 41, 48, 49, 56, 57, 64, 65, 20, 21, 22, 23, 28, 29, 30, 31, 36, 37, 38, 39, 42, 43, 44, 45, 46, 47, 50, 51, 52, 53, 54, 55, 58, 59, 60, 61, 62, 63, 66, 67, 68, 69, 70, 71]:
                Job.nodeList[i][0] = j
                i += 1

    elif options.link_arrangement == 'relative':
        if options.alpha == '0.5' or options.alpha == '1':
            i = 0
            for j in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 36, 37, 38, 39, 32, 33, 34, 35, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71]:
                Job.nodeList[i][0] = j
                i += 1
        else:
            i = 0
            for j in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23, 28, 29, 30, 31, 36, 37, 50, 51, 58, 59, 64, 65, 66, 67, 18, 19, 24, 25, 26, 27, 32, 33, 34, 35, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 52, 53, 54, 55, 56, 57, 60, 61, 62, 63, 68, 69, 70, 71]:
                Job.nodeList[i][0] = j
                i += 1

    elif options.link_arrangement == 'circulant':
        if options.alpha == '0.5' or options.alpha == '1' or options.alpha == '1.5':
            i = 0
            for j in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 58, 59, 60, 61, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71]:
                Job.nodeList[i][0] = j
                i += 1
        elif options.alpha == '2' or options.alpha == '2.5':
            i = 0
            for j in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, 22, 23, 24, 25, 26, 27, 28, 29, 32, 33, 34, 35, 36, 37, 50, 51, 54, 55, 60, 61, 62, 63, 66, 67, 12, 13, 20, 21, 30, 31, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 52, 53, 56, 57, 58, 59, 64, 65, 68, 69, 70, 71]:
                Job.nodeList[i][0] = j
                i += 1
        else:
            i = 0
            for j in [0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27, 32, 33, 34, 35, 40, 41, 42, 43, 48, 49, 50, 51, 56, 57, 58, 59, 64, 65, 66, 67, 4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31, 36, 37, 38, 39, 44, 45, 46, 47, 52, 53, 54, 55, 60, 61, 62, 63, 68, 69, 70, 71]:
                Job.nodeList[i][0] = j
                i += 1
    else:
        print "Error: Invalid link arrangement!"
        exit(1)

    if options.shuffle:
        first_half = []
        second_half = []
        for i in range(72):
            if i <= 35:
                first_half.append(Job.nodeList[i][0])
            else:
                second_half.append(Job.nodeList[i][0])

        random.shuffle(first_half)
        random.shuffle(second_half)

        for i in range(36):
            Job.nodeList[i][0] = first_half[i]
        for i in range(36,72):
            Job.nodeList[i][0] = second_half[i-36]

    return (Job)


def generate_mapfile (JobObjects, options):

    mapfile = options.output_folder + "mapFile.txt"
    # Open mapfile to write 
    mpfile = open(mapfile, "w")

    # Populate mapfile with the job & nodeID & motif info 
    for Job in JobObjects:
        mpfile_str = ""

        # Insert Job Start Identifier
        mpfile_str += "[JOB " + str(Job.jobNum) + " START]\n"

        # Insert Task List for Each Node
        for nodelist in Job.nodeList:
            tasklist = nodelist[1]

            for task in tasklist:
                if(task == tasklist[-1]):
                    mpfile_str += str(task) + "\n"
                else:
                    mpfile_str += str(task) + " "

        # Insert Job End Identifier
        mpfile_str += "[JOB " + str(Job.jobNum) + " END]\n"

        # Write this job's task map info to the mapfile
        mpfile.writelines(mpfile_str)
    mpfile.close()

    return (mapfile)

# Generates a script for ember to run
def generate_ember_script (TimeObject, JobObjects, loadfile, mapfile, options):
    
    emberLoad = "emberLoad.py"
    #currDir   = os.getcwd()

    if options.alpha == None:
        global_bw = "1"
        netBW = "1"
    else:
        global_bw = options.alpha
        if float(options.alpha) < 1:
            netBW = "1"
        else:
            netBW = options.alpha
    print netBW

    # If nextArrivalTime is zero, it means there are no other jobs left to arrive in the future. Do not use stop-at option.
    if TimeObject.nextArrivalTime == 0:
        execcommand = "sst "
    # Ember simulation will start from time zero, so feed it the relative time
    else:
        StopAtTime_ = TimeObject.nextArrivalTime - TimeObject.snapshotTime
        StopAtTime  = str(StopAtTime_) + "us"
        execcommand = "sst --stop-at " + StopAtTime
    # Generate commandline string to execute
    #execcommand += " --model-options=\"--topo=torus --shape=2x2x2 --numCores=1 --netFlitSize=8B --netPktSize=1024B --emberVerbose=0 --debug=0"
    #execcommand += " --model-options=\"--topo=dragonfly2 --shape=2:4:1:9 --routingAlg=%s --link_arrangement=%s --numCores=2 --netFlitSize=8B --netPktSize=1024B --emberVerbose=0 --debug=0" %(options.routing, options.link_arrangement)
    #execcommand += " --model-options=\"--topo=dragonfly2 --shape=2:5:1:11 --routingAlg=%s --link_arrangement=%s --numCores=2 --netFlitSize=8B --netPktSize=1024B --emberVerbose=0 --debug=0" %(options.routing, options.link_arrangement)
    execcommand += " --model-options=\"--topo=dragonfly2 --shape=2:8:1:17 --routingAlg=%s --link_arrangement=%s --numCores=2 --netFlitSize=8B --netPktSize=1024B --emberVerbose=0 --debug=0" %(options.routing, options.link_arrangement)
    execcommand += " --host_bw=1GB/s --group_bw=1GB/s --global_bw=%sGB/s --netBW=%sGB/s" %(global_bw, netBW)
    execcommand += " --embermotifLog=" + options.output_folder + "motif"
    if options.rankmapper == "custom":
        execcommand += " --rankmapper=ember.CustomMap"
    else:
        execcommand += " --rankmapper=ember.LinearMap"        
    execcommand += " --mapFile=" + mapfile
    execcommand += " --networkStatOut=" + options.output_folder + "networkStats.csv"
    execcommand += " --loadFile=" + loadfile + "\""
    execcommand += " " + emberLoad + "\n"

    print execcommand

    return (execcommand)

# Gets a commandline string, puts it into a shell file and runs
def run_ember (execcommand):

    run(execcommand)

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--xml",  action='store', dest="xmlFile", help="Name of the xml file that holds the current scheduler snapshot.") 
    parser.add_option("--alpha",  action='store', dest="alpha", help="Alpha = Global_link_BW / Local_link_BW.") 
    parser.add_option("--link_arrangement",  action='store', dest="link_arrangement", help="Global link arrangement for dragonfly.") 
    parser.add_option("--routing",  action='store', dest="routing", help="Routing algorithm.") 
    parser.add_option("--rankmapper",  action='store', dest="rankmapper", help="Custom or linear mapping.") 
    parser.add_option("--shuffle",  action='store_true', dest="shuffle", help="Random shuffling of the node list order.") 

    (options, args) = parser.parse_args()
    
    TimeObject, JobObjects = parse_xml (options = options)
    execcommand = generate_ember_files (TimeObject, JobObjects, options)
    run_ember (execcommand)

if __name__ == '__main__':
    main()
