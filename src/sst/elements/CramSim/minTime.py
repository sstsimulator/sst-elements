#!/usr/bin/python

import sys

numChans = 1
numRanks = 2
numBankGroups = 4
numBanks = 4
numBankIds = 32

lastCmd = dict()
lastCmdLine = dict()
minDict = dict()
minLineDict = dict()

structList = ["SAME_BANK", "SAME_BGROUP", "SAME_RANK", "SAME_CHAN"]
cmdList = ["ACT","READ","WRITE","PRE","REF"]

bankId2Chan = []
bankId2Rank = []
bankId2BankGroup = []
bankId2Bank = []

for struct in structList:
    minDict[struct] = dict()
    minLineDict[struct] = dict()
    for cmd1 in cmdList:
        minDict[struct][cmd1] = dict()
        minLineDict[struct][cmd1] = dict()
        for cmd2 in cmdList:
            minDict[struct][cmd1][cmd2] = -1
            minLineDict[struct][cmd1][cmd2] = ["none", "none"]

for cmd1 in cmdList:
    lastCmd[cmd1] = []
    lastCmdLine[cmd1] = []
    for chan in range(numChans):
        lastCmd[cmd1].append([])
        lastCmdLine[cmd1].append([])
        for rank in range(numRanks):
            lastCmd[cmd1][chan].append([])
            lastCmdLine[cmd1][chan].append([])
            for group in range(numBankGroups):
                lastCmd[cmd1][chan][rank].append([])
                lastCmdLine[cmd1][chan][rank].append([])
                for bank in range(numBanks):
                    lastCmd[cmd1][chan][rank][group].append(-1)
                    lastCmdLine[cmd1][chan][rank][group].append("none")

for ii in range(numBankIds):
    bankId2Chan.append(-1)
    bankId2Rank.append(-1)
    bankId2BankGroup.append(-1)
    bankId2Bank.append(-1)

seenBankIds = dict();
        
inFileName = sys.argv[1]

inFile = open(inFileName)

for line in inFile:
    if line[0] != '@': # very simple format checking
        continue
    line = line.rstrip()
    grep = line.split()
    cycle = int(grep[0][1:])
    cmd = grep[1]

    bankId = int(grep[-1])

    if bankId not in seenBankIds:
        if len(grep) < 5:
            print "grep isn't long enough to get bankId stuff!!\n", bankId, bankId2Chan[bankId], line
            exit(-1)

        seenBankIds[bankId] = 1
        
        chan = int(grep[4])
        pchan = int(grep[5])
        rank = int(grep[6])
        bankGroup = int(grep[7])
        bank = int(grep[8])
        
        bankId2Chan[bankId] = chan
        bankId2Rank[bankId] = rank
        bankId2BankGroup[bankId] = bankGroup
        bankId2Bank[bankId] = bank
    else:
        chan = bankId2Chan[bankId]
        pchan = 0 ## not supported for now
        rank = bankId2Rank[bankId]
        bankGroup = bankId2BankGroup[bankId]
        bank = bankId2Bank[bankId]
            
            
    if bankId >= numBankIds:
        print "Increase numBankIds!",bankId,"detected, max is",numBankIds-1
        exit(-1)

    for calCmd in cmdList:
        for calChan in range(numChans):
            for calRank in range(numRanks):
                for calGroup in range(numBankGroups):
                    for calBank in range(numBanks):
                                
                        if calChan == chan:
                            if calRank == rank:
                                if calGroup == bankGroup:
                                    if calBank == bank:
                                        struct = "SAME_BANK"
                                    else: # bank !=
                                        struct = "SAME_BGROUP"
                                else: # bankGroup !=
                                    struct = "SAME_RANK"
                            else: # rank !=
                                struct = "SAME_CHAN"

                            if lastCmd[calCmd][calChan][calRank][calGroup][calBank] != -1:
                                delta = cycle - lastCmd[calCmd][calChan][calRank][calGroup][calBank]
                                if minDict[struct][calCmd][cmd] != -1:
                                    if delta < minDict[struct][calCmd][cmd]:
                                        minDict[struct][calCmd][cmd] = delta
                                        minLineDict[struct][calCmd][cmd][0] = lastCmdLine[calCmd][calChan][calRank][calGroup][calBank]
                                        minLineDict[struct][calCmd][cmd][1] = line
                                else:
                                    minDict[struct][calCmd][cmd] = delta
                                    minLineDict[struct][calCmd][cmd][0] = lastCmdLine[calCmd][calChan][calRank][calGroup][calBank]
                                    minLineDict[struct][calCmd][cmd][1] = line
                                                        
    lastCmd[cmd][chan][rank][bankGroup][bank] = cycle
    lastCmdLine[cmd][chan][rank][bankGroup][bank] = line


for struct in structList:
    print(struct)
    sys.stdout.write("\t")
    for cmd1 in cmdList:
        sys.stdout.write('%6s' % cmd1)
    sys.stdout.write("\n")

    for cmd1 in cmdList:
        sys.stdout.write(cmd1)
        sys.stdout.write("\t")
        for cmd2 in cmdList:
            sys.stdout.write('%6d' % minDict[struct][cmd1][cmd2])
        sys.stdout.write("\n")
    print

for struct in structList:
    print(struct)

    for cmd1 in cmdList:
        for cmd2 in cmdList:
            print cmd2,"after",cmd1, minDict[struct][cmd1][cmd2]
            print minLineDict[struct][cmd1][cmd2][0]
            print minLineDict[struct][cmd1][cmd2][1]
        print
            
    print
