import pprint
import sys 
from loadUtils import *
from EmberEP import *

class PartInfo:
	def __init__(self, nicParams, epParams, numNodes, numCores, detailedModel = None ):
		self.nicParams = nicParams
		self.epParams = epParams
		self.numNodes = int(numNodes)
		self.numCores = int(numCores)
		self.detailedModel = detailedModel
		self.endpoint = None 
		self.nicParams["num_vNics"] = numCores

        def setEndpoint( self, endpoint ):
                self.endpoint = endpoint
        
        
class LoadInfo:

        def __init__(self,numNics, baseNicParams, defaultEmberParams):
		self.numNics = int(numNics)
		nullMotif = {
                        'motif0.api': '',
			'motif0.name' : 'ember.NullMotif',
			'motif0.printStats' : 0,
			'motif0.spyplotmode': 0
                        }

                self.parts = {} 
		self.nullEP = EmberEP( -1 , defaultEmberParams, baseNicParams, nullMotif, 1,1,[],'Null',[],None)
		self.nullEP.prepParams()

	def addPart(self, nodeList, nicParams, epParams, numCores, detailedModel = None ):
                self.parts[nodeList] = PartInfo( nicParams, epParams, calcNetMapSize(nodeList), numCores, detailedModel );

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

		numNodes = calcMaxNode( nidList ) 
		if numNodes > self.numNics:
			sys.exit('Error: Requested max nodes ' + str(numNodes) +\
				 ' is greater than available nodes ' + str(self.numNics) ) 

		ep = EmberEP( jobId, epParams, nicParams, motifs, numCores, ranksPerNode, statNodes, nidList, motifLogNodes, detailedModel ) # added motifLogNodes here

		ep.prepParams()
		return ep

	def initWork(self, nidList, workList, statNodes ):
		for jobid, work in workList:
			self.parts[nidList].setEndpoint( self.createEP( jobid, nidList, self.parts[nidList].numCores, self.readWorkList( jobid, nidList, work ), statNodes, self.parts[nidList].detailedModel ) )

	def readWorkList(self, jobid, nidList, workList ):
		tmp = {}
		tmp['motif_count'] = len(workList) 
		for i, work in enumerate( workList ) :
			cmdList = work['cmd'].split()

			tmpList = nidList[:10]
			if len(nidList) > 10:
				tmpList = tmpList + '...'
			print "EMBER: Job={} nidList=\'{}\' Motif=\'{}\'".format( jobid, tmpList, ' '.join(cmdList) )
			del work['cmd']

			motif = self.parseCmd( "ember.", "Motif", cmdList, i )

			for x in work.items():
				motif[ 'motif' + str(i) + '.' + x[0] ] = x[1] 

			tmp.update( motif )

		return tmp

	def parseCmd(self, motifPrefix, motifSuffix, cmdList, cmdNum ):
		motif = {} 

		tmp = cmdList[0].split('.')
		if  len(tmp) == 2:
			motifPrefix = tmp[0] + '.'
			cmdList[0] = tmp[1]

		tmp = 'motif' + str(cmdNum) + '.name'
		motif[ tmp ] = motifPrefix + cmdList[0] + motifSuffix
		cmdList.pop(0)
		for x in cmdList:
			y = x.split("=")
			tmp	= 'motif' + str(cmdNum) + '.arg.' + y[0]
			motif[ tmp ] = y[1]

		return motif

	def inRange( self, nid, start, end ):
		if nid >= start:
			if nid <= end:
				return True	
		return False

	def setNode(self,nodeId):
		for nidList, part in self.parts.items():
                        ep = part.endpoint
			x = nidList.split(',')
			for y in x:	
				tmp = y.split('-')

				if 1 == len(tmp):
					if nodeId == int( tmp[0] ):
						return ep 
				else:
					if self.inRange( nodeId, int(tmp[0]), int(tmp[1]) ):
						return ep 
		return self.nullEP
