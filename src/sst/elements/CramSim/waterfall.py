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

bankList = list()
first = True

for line in inFile:
    if line[0] != '@': # very simple format checking
        continue
    line = line.rstrip()
    grep = line.split()
    cycle = grep[0][1:]
    cmd = grep[1]

    if first:
        first = False
        lastCycle = cycle

    if cycle != lastCycle:
        sys.stdout.write('%9s' % lastCycle)
        sys.stdout.write(" ")
        for ii in range(numBanks):
            sys.stdout.write('%3s' % bankStates[ii])
        print " ",addr

        #reset active banks to | and precharge banks to .
        if len(bankList) > 1:
            for curBankId in bankList:
                if bankStates[curBankId] == "A" or bankStates[curBankId] == "W" or bankStates[curBankId] == "R":
                    bankStates[curBankId] = "|"
                if bankStates[curBankId] == "P" or bankStates[curBankId] == "F":
                    bankStates[curBankId] = "."
        else:
            if bankStates[bankId] == "A" or bankStates[bankId] == "W" or bankStates[bankId] == "R":
                bankStates[bankId] = "|"
            if bankStates[bankId] == "P" or bankStates[bankId] == "F":
                bankStates[bankId] = "."
            
        bankList = list() ## clear the bank list
        
    addr = grep[3]
    bankId = int(grep[-1])
    if bankId >= numBanks:
        print "Increase numBanks!",bankId,"detected, max is",numBanks-1
        exit(-1)
    bankStates[bankId] = cmdDict[cmd]
    bankList.append(bankId)
        
    lastCycle = cycle


    
