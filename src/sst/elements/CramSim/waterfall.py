#!/usr/bin/python

import sys

numBanks = 32

cmdDict = {"ACT":"A","READ":"R","WRITE":"W","PRE":"P","REF":"F"}

bankStates = []
for ii in range(numBanks):
    bankStates.append(".")

inFileName = sys.argv[1]

inFile = open(inFileName)

sys.stdout.write('%10s' % " ")
for ii in range(numBanks):
    sys.stdout.write('%3d' % ii)
print

for line in inFile:
    if line[0] != '@': # very simple format checking
        continue
    line = line.rstrip()
    grep = line.split()
    cycle = grep[0][1:]
    cmd = grep[1]

    #reset active banks to | and precharge banks to .
    for ii in range(numBanks):
        if bankStates[ii] == "A" or bankStates[ii] == "W" or bankStates[ii] == "R":
            bankStates[ii] = "|"
        if bankStates[ii] == "P" or bankStates[ii] == "F":
            bankStates[ii] = "."

    doPrint = True
    ref = False
    if cmd == 'REF' and len(grep)>3:
        addr = ""
        ref = True
        bankList = grep[3:]

        for ii in bankList:
            if int(ii) >= numBanks:
                print "Increase numBanks!",ii,"detected, max is",numBanks-1
            bankStates[int(ii)] = cmdDict[cmd]
    else:
        if cmd != 'REF':
            addr = grep[2]
            bankId = int(grep[11])
            if bankId >= numBanks:
                print "Increase numBqanks!",bankId,"detected, max is",numBanks-1
            bankStates[bankId] = cmdDict[cmd]
        else:
            doPrint = False

    if doPrint:
        sys.stdout.write('%9s' % cycle)
        sys.stdout.write(" ")
        for ii in range(numBanks):
            sys.stdout.write('%3s' % bankStates[ii])
        print " ",addr
        

    
