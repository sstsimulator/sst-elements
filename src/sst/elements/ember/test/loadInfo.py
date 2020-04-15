import pprint
import sys
from loadUtils import *
from EmberEP import *
from paramUtils import *


class PartInfo:
    def __init__(self, nicParams, epParams, numNodes, nicsPerNode, numCores, detailedModel = None ):
        self.nicParams = nicParams
        self.epParams = epParams
        self.numNodes = int(numNodes)
        self.numCores = int(numCores)
        self.detailedModel = detailedModel
        self.nicParams["num_vNics"] = self.numCores//int(nicsPerNode)
        self.nicParams["numCores"] = numCores

class LoadInfo:

    def __init__(self,numNics, nicsPerNode, baseNicParams, defaultEmberParams):
        self.numNics = int(numNics)
        self.nicsPerNode = int(nicsPerNode)
        nullMotif = { 'motif0.name' : 'ember.NullMotif', 'motif0.printStats' : 0, 'motif0.spyplotmode': 0 }

        self.parts = {}
        ep = EmberEP( -1 , defaultEmberParams, baseNicParams, nullMotif, nicsPerNode, 1,1,[],'Null',1,[],None)
        ep.prepParams()
        self.endPointMap = [ ep for i in range(self.numNics)]
        self.globalToLocalNidMap = [ -1 for i in range(self.numNics) ]

    def addPart(self, nodeList, nicParams, epParams, numCores, detailedModel = None ):
        self.parts[nodeList] = PartInfo( nicParams, epParams, calcNetMapSize(nodeList), self.nicsPerNode, numCores, detailedModel );

    def createEP( self, jobId, nidList, ranksPerNode, motifs, statNodes, detailedModel = None ):

        epParams = self.parts[nidList].epParams
        nicParams = self.parts[nidList].nicParams
        numCores = self.parts[nidList].numCores
        # In order to pass the motifLog parameter to only desired nodes of a job
        # Here we choose the first node in the nidList
        motifLogNodes = []
        if (nidList != 'Null' and 'motifLog' in epParams):
            tempnidList = nidList
            if '-' in tempnidList:
                tempnidList = tempnidList.split('-')
            else:
                tempnidList = tempnidList.split(',')
            motifLogNodes.append(tempnidList[0])
        # end

        maxNode = calcMaxNode( nidList )

        if maxNode > self.numNics:
            sys.exit('Error: Requested max nodes ' + str(numNodes) +\
                 ' is greater than available nodes ' + str(self.numNics) )
        numNodes = calcNetMapSize( nidList )

        ep = EmberEP( jobId, epParams, nicParams, motifs, self.nicsPerNode, numCores, ranksPerNode, statNodes, self.globalToLocalNidMap, numNodes, motifLogNodes, detailedModel ) # added motifLogNodes here

        ep.prepParams()
        return ep

    def initWork(self, nidList, workList, statNodes ):
        if len(workList) > 1:
            sys.exit('ERROR: LoadInfo.initWork() invalid argument {0}'.format(workList) )

        jobid = workList[0][0]
        work = workList[0][1]

        ep = self.createEP( jobid, nidList, self.parts[nidList].numCores, self.readWorkList( jobid, nidList, work ), statNodes, self.parts[nidList].detailedModel )
        self.setEndpoint( nidList, ep )

    def setEndpoint( self, nidList, ep ):
        nidList = nidList.split(',')
        pos = 0
        for x in nidList:
            y = x.split('-')

            start = int(y[0])
            if len(y) == 1:
                self.endPointMap[start] = ep
                self.globalToLocalNidMap[start] = pos
                pos += 1
            else:
                end = int(y[1])
                for i in range( start, end + 1 ):
                    self.endPointMap[i] = ep
                    self.globalToLocalNidMap[i] = pos
                    pos += 1

    def readWorkList(self, jobid, nidList, workList ):
        tmp = {}
        tmp['motif_count'] = len(workList)
        print ("EMBER: Job={0}, nidList=\'{1}\'".format( jobid, truncate(nidList) ))
        for i, work in enumerate( workList ) :
            cmdList = work['cmd'].split()

            print ("EMBER: Motif=\'{0}\'".format( ' '.join(cmdList) ))
            del work['cmd']

            motif = self.parseCmd( "ember.", "Motif", cmdList, i )

            for x in list(work.items()):
                motif[ 'motif' + str(i) + '.' + x[0] ] = x[1]

            tmp.update( motif )

        return tmp

    def parseCmd(self, motifPrefix, motifSuffix, cmdList, cmdNum ):
        motif = {}

        key = 'motif' + str(cmdNum) + '.name'
        # Check to see if this is for a non-ember endpoint
        if cmdList[0].find("<") == 0:
            motif[key] = cmdList[0]
            params = {}
            cmdList.pop(0)
            for x in cmdList:
                y = x.split("=",1)
                params[y[0]] = y[1]

            motif["params"] = params
            return motif

        #For ember enpoints
        tmp = cmdList[0].split('.')
        if  len(tmp) >= 2:
            motifPrefix = tmp[0] + '.'
            cmdList[0] = tmp[1]

        motif[ key ] = motifPrefix + cmdList[0] + motifSuffix
        cmdList.pop(0)
        for x in cmdList:
            y = x.split("=")
            tmp    = 'motif' + str(cmdNum) + '.arg.' + y[0]
            motif[ tmp ] = y[1]

        return motif

    def inRange( self, nid, start, end ):
        if nid >= start:
            if nid <= end:
                return True
        return False

    def setNode(self,nodeId):
        if self.endPointMap[nodeId] == None:
            sys.exit('ERROR: endpoint not set for node {0} '.format(nodeId));
        return self.endPointMap[nodeId]
