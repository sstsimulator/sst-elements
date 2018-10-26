
from loadUtils import *
from EmberEP import *

class LoadInfo:

	def __init__(self, nicParams, epParams, numNodes, numCores, numNics, detailedModel = None ):
		self.nicParams = nicParams
		self.epParams = epParams
		self.numNodes = int(numNodes)
		self.numCores = int(numCores)
		self.numNics = int(numNics)
		self.detailedModel = detailedModel
		self.nicParams["num_vNics"] = numCores
		self.map = []
		nullMotif = [{
			'cmd' : "-nidList=Null Null",
			'printStats' : 0,
			'api': "",
			'spyplotmode': 0
		}]

		self.nullEP, nidlist = self.foo( -1, self.readWorkList( nullMotif ), [] )
		self.nullEP.prepParams()

	def foo( self, jobId, x, statNodes, detailedModel = None ):
		nidList, ranksPerNode, params = x
		
		# In order to pass the motifLog parameter to only desired nodes of a job
		# Here we choose the first node in the nidList
		motifLogNodes = []
		if (nidList != 'Null' and 'motifLog' in self.epParams):
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

		params.update( self.epParams )
		ep = EmberEP( jobId, params, self.nicParams, self.numCores, ranksPerNode, statNodes, nidList, motifLogNodes, detailedModel ) # added motifLogNodes here

		ep.prepParams()
		return (ep, nidList)

	def getWorkListFromFile( self, filename, defaultParams ):
		stage1 = []
		for line in open(filename, 'r'):
			line = line.strip()
			if line:
				if line[:1] == '[':
					stage1.append(line)
				elif line[:1] == '#':
					continue;
				else:
					stage1[-1] += ' ' + line

		tmp = []
		nidlist=''
		api=''
    
		for item in stage1:
			tag,str = item.split(' ', 1)
				
			if tag == '[JOB_ID]':	
				api = '' 
				tmp.append([])
				tmp[-1].append( str )
			elif tag == '[API]':	
				api = str
			elif tag == '[NID_LIST]':	
				nidlist = str
				tmp[-1].append( [] )  
			elif tag == '[MOTIF]':	
				tmp[-1][-1].append( dict.copy(defaultParams) )  
				tmp[-1][-1][-1]['cmd'] = '-nidList=' + nidlist + ' ' + str 
				if api :
				    tmp[-1][-1][-1]['api'] = api
		return tmp 
		
	def initFile(self, defaultParams, fileName, statNodeList ):
		work = self.getWorkListFromFile( fileName, defaultParams  )
		for item in work:
			jobid, motifs = item
			self.map.append( self.foo( jobid, self.readWorkList( motifs ), statNodeList, self.detailedModel ) )

		self.verifyLoadInfo()

	def initWork(self, workList, statNodes ):
		for jobid, work in workList:
			self.map.append( self.foo( jobid, self.readWorkList( work ), statNodes, self.detailedModel ) )
		self.verifyLoadInfo()

	def readWorkList(self, workList ):
		tmp = {}
		tmp['motif_count'] = len(workList) 
		for i, work in enumerate( workList ) :
			cmdList = work['cmd'].split()
			del work['cmd']

			ranksPerNode = self.numCores 
			nidList = []
			while len(cmdList):
				if "-" != cmdList[0][0]:
					break

				o, a = cmdList.pop(0).split("=")

				if "-ranksPerNode" == o:
					ranksPerNode = int(a)
				elif "-nidList" == o:
					nidList = a
				else:
					sys.exit("bad argument")	

			if 0 == len(nidList):
				nidList = "0-" + str(self.numNodes-1) 
			
			if "Null" != cmdList[0]:
				print "EMBER: Job: -nidList={0} -ranksPerNode={1} {2}".format( nidList, ranksPerNode, cmdList )

			if  ranksPerNode > self.numCores:
				sys.exit("Error: " + str(ranksPerNode) + " ranksPerNode is greater than "+
						str(self.numCores) + " coresPerNode")

			motif = self.parseCmd( "ember.", "Motif", cmdList, i )

			for x in work.items():
				motif[ 'motif' + str(i) + '.' + x[0] ] = x[1] 

			tmp.update( motif )

		return ( nidList, ranksPerNode, tmp )

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

	def verifyLoadInfo(self):
		#print "verifyLoadInfo", "numNodes", self.numNodes, "numCores", self.numCores
		#for ep,nidList in self.map:
			#print nidList
		return True

	def inRange( self, nid, start, end ):
		if nid >= start:
			if nid <= end:
				return True	
		return False

	def setNode(self,nodeId):
		for ep, nidList in self.map:
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
