#!/usr/bin/env python
'''
Date        : 04/06/2016  
Created by  : Fulya Kaplan
Description : This script is used to generate a communication matrix to be used to test the bisection bandwidth. There are two groups of task. A task from group #1 communicates with each of the tasks from group #2 and not with any tasks within its own group.
'''

import os, sys

from optparse import OptionParser
import math
import numpy as np

# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)


def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("-n",  action='store', type=int, dest="numTasks", help="Number of tasks.")
    parser.add_option("-f",  action='store', dest="mtxfile", help="Name of the matrix file.")
    parser.add_option("-p",  action='store', dest="pattern", help="Communication pattern.")
    (options, args) = parser.parse_args()


    numTasks = options.numTasks
    if (numTasks % 2 != 0):
        sys.exit("Error: numTasks must be an integer multiple of 2")
    else:
        tasksPerGroup = numTasks / 2

    if options.pattern == "bisection":
        #Bisection pattern
        fo = open(options.mtxfile, "w")
        fo.writelines("%%MatrixMarket matrix coordinate pattern symmetric\n")
        fo.writelines("%d\t%d\t%d\n" % (numTasks, numTasks, (tasksPerGroup * tasksPerGroup)))

        for i in range(1, tasksPerGroup+1):
            for j in range(tasksPerGroup+1, numTasks+1):
                fo.writelines("%d\t%d\n" % (i, j))

        fo.close()
      
    
    elif options.pattern == "alltoall":
        #All to all pattern
        fo = open(options.mtxfile, "w")
        fo.writelines("%%MatrixMarket matrix coordinate pattern symmetric\n")
        fo.writelines("%d\t%d\t%d\n" % (numTasks, numTasks, (numTasks * (numTasks-1))))

        for i in range(1, numTasks+1):
            for j in range(1, numTasks+1):
                if i != j: 
                    fo.writelines("%d\t%d\n" % (i, j))

        fo.close()
    
    

if __name__ == '__main__':
    main()
