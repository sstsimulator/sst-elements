#!/usr/bin/env python
'''
Date        : 07/31/2015  
Created by  : Fulya Kaplan
Description : This sub-python script parses the ember output file. It then creates input files & run script for scheduler simulation and runs it. Created specifically for detailed simulation of network congestion together with scheduling & application task mapping algorithms.
'''

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
        self.soFarRunningTime = -1

    def set(self, jobNum, nodeList, motifFile, startingMotif, soFarRunningTime):
        self.jobNum = jobNum
        self.nodeList = nodeList
        self.numNodes = len(nodeList)
        self.motifFile = motifFile
        self.startingMotif = startingMotif
        self.soFarRunningTime = soFarRunningTime

# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)

# Parser for the ember output file and the motifLogs
def parse_emberOut (options):

    fileName = options.emberOutFile
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

# Extracts the necessary time, jobNum info from the given string
def grep_timeInfo(string, mode):

    if mode == 'SimComplete':
        string = string.split(' ')
        number = float(string[5])
        unit   = string[6].split('\n')[0]
        jobNum = -1

        time = convertToMicro(number, unit)
        return (time, jobNum)

    elif mode == 'JobFinished':
        string = string.split(' ')
        jobNum = int(string[2].split(':')[1])
        number = float(string[3].split(':')[1])
        unit   = string[4].split('\n')[0]

        time = convertToMicro(number, unit)
        return (time, jobNum)

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

        soFarRunningTime_ = job.getElementsByTagName('soFarRunningTime')[0]
        soFarRunningTime  = int(soFarRunningTime_.childNodes[0].data)
        #print "startingMotif: %d" % startingMotif

        tempJobObject = Job()
        tempJobObject.set(jobNum, [], motifFile, startingMotif, soFarRunningTime)
        JobObjects.append(tempJobObject)
    #end
    
    return (TimeObject, JobObjects)

# Generates/updates ember related input files for the next scheduler run
def generate_scheduler_inputs (InfoPair, TimeObject, JobObjects, options):

    emberCompletedFile = options.emberCompletedFile
    emberRunningFile   = options.emberRunningFile
    motifLogprefix     = "motif-"

    ecFile = open(emberCompletedFile, 'a')
    erFile = open(emberRunningFile, 'w')

    # If there are any jobs that are finished, append the emberCompletedFile
    completedJobsList = []
    for emberTime, jobNum in InfoPair:
        if jobNum != -1:
            for Job in JobObjects:
                if Job.jobNum == jobNum:
                    completedJobsList.append(jobNum)
                    line = "%d\t%d\n" % (jobNum, (Job.soFarRunningTime + emberTime))
                    ecFile.writelines(line)
                    break

    # Update the emberRunningFile with current time info
    # Current time = Snapshot time + ember simulation time
    line = "time\t%d\n" % (TimeObject.snapshotTime + emberTime)
    erFile.writelines(line)

    # For jobs that are still running on ember, update the emberRunningFile
    runningJobsList = []
    for Job in JobObjects:
        if Job.jobNum not in completedJobsList:
            runningJobsList.append(Job.jobNum)

            # Read motifLogs to get the current motif number from ember
            motifLog = motifLogprefix + str(Job.jobNum) + ".log"
            #print motifLog
            mFile    = open(motifLog, 'r')
            emberMotifNum = -1

            for l in mFile:
                emberMotifNum += 1

            # Check if there are no lines in the motifLog
            if (emberMotifNum == -1):
                sys.stderr.write("ERROR: Job " + str(Job.jobNum) + " No lines in the motifLog file\n")

                sys.exit(1)
            # There is only one line and it was init (i.e., no new motif has been executed)
            elif (emberMotifNum == 0):
                pass
            # There are more than one lines, subtract the Init motif from the total number motifs
            else:
                emberMotifNum -= 1


            #print emberMotifNum
            mFile.close()

            # Write the updated time and motif counts
            line = "job\t%d\t%d\t%d\n" % (Job.jobNum, (Job.soFarRunningTime + emberTime), (Job.startingMotif + emberMotifNum) )
            erFile.writelines(line)

    ecFile.close()
    erFile.close()

# Runs scheduler simulation with the given python script
def run_scheduler (options):

    execcommand = "sst %s" %(options.schedPythonFile)
    run(execcommand)

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--xml",  action='store', dest="xmlFile", help="Name of the xml file that holds the current scheduler snapshot.") 
    parser.add_option("--emberOut",  action='store', dest="emberOutFile", help="Name of the ember output file.")
    parser.add_option("--schedPy",  action='store', dest="schedPythonFile", help="Name of the python file that holds the scheduler parameters.")
    parser.add_option("--ember_completed",  action='store', dest="emberCompletedFile", help="Name of the file that lists the jobs that have been completed in ember.")
    parser.add_option("--ember_running",  action='store', dest="emberRunningFile", help="Name of the file that lists the jobs that are currently running in ember.") 
    (options, args) = parser.parse_args()

    InfoPair = parse_emberOut(options)
    TimeObject, JobObjects = parse_xml (options)
    generate_scheduler_inputs (InfoPair, TimeObject, JobObjects, options)
    run_scheduler (options)

if __name__ == '__main__':
    main()
