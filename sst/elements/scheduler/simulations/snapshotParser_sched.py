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


class Job:
    def __init__(self):
        self.jobNum     = -1
        self.nodes      = []
        self.numNodes   = -1
        
#The string that holds the script to run ember
emberScript = ''

#

# Open XML document using minidom parser
DOMTree = xml.dom.minidom.parse("%s", fileName)
snapshot = DOMTree.documentElement

# Get all times and jobs in the snapshot
times = snapshot.getElementsByTagName("time")
jobs  = snapshot.getElementsByTagName("job")

for job in jobs:


if __name__ == '__main__':
    if outFile == "" or outFile == "default":
    	print "Error: There is no default value for outFile"
    	sys.exit()
    f = open(outFile,'w')
  
    f.write('# scheduler simulation input file\n')
    f.write('import sst\n')
    f.write('\n')
    f.write('# Define SST core options\n')
    f.write('sst.setProgramOption("run-mode", "both")\n')
    f.write('\n')
    f.write('# Define the simulation components\n')
    f.write('scheduler = sst.Component("myScheduler", \
            "scheduler.schedComponent")\n')
    f.write('scheduler.addParams({\n')
  
    if traceName == "" or traceName == "default":
    	print "Error: There is no default value for traceName"
    	os.remove(outFile)
    	sys.exit()
    f.write('      "traceName" : "' + traceName + '",\n')
    if machine != "" and machine != "default":
    	f.write('      "machine" : "' + machine + '",\n')
    if coresPerNode != "":
    	f.write('      "coresPerNode" : "' + coresPerNode + '",\n')
    if scheduler != "" and scheduler != "default":
    	f.write('      "scheduler" : "' + scheduler + '",\n')
    if FST != "" and FST != "default":
    	f.write('      "FST" : "' + FST + '",\n')
    if allocator != "" and allocator != "default":
    	f.write('      "allocator" : "' + allocator + '",\n')
    if taskMapper != "" and taskMapper != "default":
    	f.write('      "taskMapper" : "' + taskMapper + '",\n')
    if timeperdistance != "" and timeperdistance != "default":
    	f.write('      "timeperdistance" : "' + timeperdistance + '",\n')
    if dMatrixFile != "" and dMatrixFile != "default":
    	f.write('      "dMatrixFile" : "' + dMatrixFile + '",\n')
    if randomSeed != "" and randomSeed != "default":
    	f.write('      "runningTimeSeed" : "' + randomSeed + '",\n')
    f.seek(-2, os.SEEK_END)
    f.truncate()
    f.write('\n})\n')
    f.write('\n')
  
    f.write('# nodes\n')
    if machine.split('[')[0] == 'mesh' or machine.split('[')[0] == 'torus':
    	nums = machine.split('[')[1]
    	nums = nums.split(']')[0]
    	nums = nums.split(',')
    	numberNodes = int(nums[0])*int(nums[1])*int(nums[2])
    for i in range(0, numberNodes):
    	f.write('n' + str(i) + ' = sst.Component("n' + str(i) + \
            '", "scheduler.nodeComponent")\n')
    	f.write('n' + str(i) + '.addParams({\n')
    	f.write('      "nodeNum" : "' + str(i) + '",\n')
    	f.write('})\n')
    f.write('\n')
    
    f.write('# define links\n')
    for i in range(0, numberNodes):
    	f.write('l' + str(i) + ' = sst.Link("l' + str(i) + '")\n')
    	f.write('l' + str(i) + '.connect( (scheduler, "nodeLink' + str(i) + \
            '", "0 ns"), (n' + str(i) + ', "Scheduler", "0 ns") )\n')
    f.write('\n')
    
    f.close()

