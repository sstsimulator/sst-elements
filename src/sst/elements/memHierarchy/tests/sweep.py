#!/usr/bin/python -O

#Caesar De la Paz III
#cdelapa@sandia.gov

import sys
import  difflib
from filecmp import dircmp
import subprocess
import os
import re

#This script is a example of how to run "sweeps" with a simple script
#Modify it according to your needs
#Output goes to folder "./sweepOutput"

if len(sys.argv) < 3:
    print("Please provide two arguments:  1) Path of executable, 2) Path of reference file")
    sys.exit()

test = sys.argv[1]
referenceFile = sys.argv[2]


#Make sure your 'M5' config file has the environment variable M5_EXE as its executable
os.environ['M5_EXE'] = test

#List of all sweeping parameters
protocols = ["MSI", "MESI"]
replacements = ["mru", "lru", "random", "lfu"]
mshr_sizes = ["32", "64"]
l1_sizes = ["8 KB", "16 KB", "32 KB"]
l1_assocs = ["1", "2", "4" , "8"]
l2_sizes = ["8 KB", "16 KB", "32 KB", "64 KB" ]
l2_assocs = ["4", "8", "16"]


#protocols = ["MSI"]
#replacements = ["mru"]
#mshr_sizes = ["32"]
#l1_sizes = ["8 KB"]
#l1_assocs = ["1"]
#l2_sizes = ["8 KB"]
#l2_assocs = ["4"]

failures = 0
suffix = 0
goldPath = referenceFile
goldFd = open(goldPath, "r")
goldReferenceLines = goldFd.readlines()

for protocol in protocols:
    for repl in replacements:
        for mshr_size in mshr_sizes:
            for l1_size in l1_sizes:
                for l1_assoc in l1_assocs:
                    for l2_assoc in l2_assocs:
                        for l2_size in l2_sizes:
                            os.environ["PROTOCOL"] = protocol
                            os.environ["REPL"] = repl
                            os.environ["MSHR_SIZE"] = mshr_size
                            os.environ["L1_ASSOC"] = l1_assoc
                            os.environ["L1_SIZE"] = l1_size
                            os.environ["L2_ASSOC"] = l2_assoc
                            os.environ["L2_SIZE"] = l2_size


                            outFile = "out" + str(suffix)
                            os.mkdir("./sweepOutput/")
                            outPath = "./sweepOutput/" + outFile
                            suffix += 1
                            
                            print "TEST #" + str(suffix) + ", Fails so far = " + str(failures)

                            command = ["sst sweep.xml"]
                            print("Output File: " + outFile + " Protocol: "+ protocol+ ", Repl: " + repl + ", MSHR Size: "+mshr_size+ ", "),
                            print("L1 Assoc: "+ l1_assoc+ ", L1 Size: "+ l1_size+ ", L2 Assoc: "+ l2_assoc+ ", L2 Size: "+ l2_size)

                            outFd = open(outPath, "w")
                            returnType = subprocess.call(command, stdout=outFd, shell=True)
                            outFd.close()

                            outFd = open(outPath, "r")
                            result = difflib.ndiff(outFd.readlines(), goldReferenceLines)
                            diffResult =  [x for x in result if x.startswith('- ') or x.startswith('+ ')]
                            outFd.close()
                            outFd = open(outPath, "a")

                            outFd.write("\n\n") 
                            outFd.write("Protocol: " + protocol + "\n") 
                            outFd.write("Replacement: " + repl + "\n") 
                            outFd.write("MSHR Size: " + mshr_size + "\n") 
                            outFd.write("L1 Assoc: " + l1_assoc + "\n") 
                            outFd.write("L1 Size: " + l1_size + "\n") 
                            outFd.write("L2 Assoc: " + l2_assoc + "\n") 
                            outFd.write("L2 Size: " + l2_size + "\n") 
                            outFd.write("DIFF: \n") 

                            if len(diffResult) > 5:
                                failures += 1
                                for x in diffResult:
                                    outFd.write(x)
                                outFd.write("FAIL. Diff did not match\n") 

                            outFd.close()
        
if failures > 0:
    print "There were %s failures"%failures
else:
    print "Test was successful"



