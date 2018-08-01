import sst
from sst.merlin import *

import copy

def calcNetMapId( nodeId, nidList ):

    if nidList == 'Null':
        return -1

    pos = 0
    a = nidList.split(',')

    for b in a:
        c = b.split('-')
    
        start = int(c[0])
        stop = start

        if 2 == len(c):
            stop = int(c[1])

        if nodeId >= start and nodeId <= stop:
            return pos + (nodeId - start) 

        pos = pos + ((stop - start) + 1)

    return -1

def calcNetMapSize( nidList ):

    if nidList == 'Null':
        return 0 

    pos = 0
    a = nidList.split(',')

    for b in a:
        c = b.split('-')
    
        xx = 1 
        if 2 == len(c):
            xx = int(c[1]) - int(c[0]) + 1

        pos += xx

    return pos

def calcMaxNode( nidList ):

    if nidList == 'Null':
        return 0 
	
    max = 0
    a = nidList.split(',')

    for b in a:
        c = b.split('-')
    
        tmp = int(c[0]) 
        if 2 == len(c):
           tmp  = int(c[1])

        if tmp > max:
            max = tmp

    return max + 1 

class EmberEP( EndPoint ):
    def __init__( self, jobId, driverParams, nicParams, numCores, ranksPerNode, statNodes, nidList, motifLogNodes, detailedModel ): # added motifLogNodes here
        self.driverParams = driverParams
        self.nicParams = nicParams
        self.numCores = numCores
        self.driverParams['jobId'] = jobId
        self.statNodes = statNodes
        self.nidList = nidList
        self.numNids = calcNetMapSize( self.nidList )
        # in order to create motifLog files only for the desired nodes of a job
        self.motifLogNodes = motifLogNodes
        self.detailedModel = detailedModel

    def getName( self ):
        return "EmberEP"

    def prepParams( self ):
        pass

    def build( self, nodeID, extraKeys ):


        nic = sst.Component( "nic" + str(nodeID), "firefly.nic" )
        nic.addParams( self.nicParams )
        nic.addParams( extraKeys)
        nic.addParam( "nid", nodeID )
        retval = (nic, "rtr", sst.merlin._params["link_lat"] )
 
        built = False 
        if self.detailedModel:
            built = self.detailedModel.build( nodeID, self.numCores )

        memory = None
        if built:
            nic.addLink( self.detailedModel.getNicLink( ), "detailed0", "1ps" )
            memory = sst.Component("memory" + str(nodeID), "thornhill.MemoryHeap")
            memory.addParam( "nid", nodeID )
            #memory.addParam( "verboseLevel", 1 )

        loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
        loopBack.addParam( "numCores", self.numCores )


        # Create a motifLog only for one core of the desired node(s)
        logCreatedforFirstCore = False
        # end

        for x in xrange(self.numCores):
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) +
                                            "_EmberEP", "ember.EmberEngine")

            if built:
                links = self.detailedModel.getThreadLinks( x )
                cpuNum = 0
                for link in links: 
                    ep.addLink(link,"detailed"+str(cpuNum),"1ps")
                    cpuNum = cpuNum + 1

            # Create a motif log only for the desired list of nodes (endpoints)
            # Delete the 'motifLog' parameter from the param list of other endpoints
            if 'motifLog' in self.driverParams:
            	if self.driverParams['motifLog'] != '':
            		if (self.motifLogNodes):
            			for id in self.motifLogNodes:
            				if nodeID == int(id) and logCreatedforFirstCore == False:
                				#print str(nodeID) + " " + str(self.driverParams['jobId']) + " " + str(self.motifLogNodes)
                				#print "Create motifLog for node {0}".format(id)
                				logCreatedforFirstCore = True
                				ep.addParams(self.driverParams)
                			else:
                				tempParams = copy.copy(self.driverParams)
                				del tempParams['motifLog']
                				ep.addParams(tempParams)
                	else:
                		tempParams = copy.copy(self.driverParams)
                		del tempParams['motifLog']
                		ep.addParams(tempParams)
                else:
                	ep.addParams(self.driverParams)      				
            else:
            	ep.addParams(self.driverParams)
           	# end          


            # Original version before motifLog
            #ep.addParams(self.driverParams)

            for id in self.statNodes:
                if nodeID == id:
                    print "printStats for node {0}".format(id)
                    ep.addParams( {'motif1.printStats': 1} )

            ep.addParams( {'hermesParams.netId': nodeID } )
            ep.addParams( {'hermesParams.netMapId': calcNetMapId( nodeID, self.nidList ) } ) 
            ep.addParams( {'hermesParams.netMapSize': self.numNids } ) 
            ep.addParams( {'hermesParams.coreId': x } ) 

            nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) +
                                            "_Link"  )
            nicLink.setNoCut()

            loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) +
                                            "_Link"  )
            loopLink.setNoCut() 

            #ep.addLink(nicLink, "nic", self.nicParams["nic2host_lat"] )
            #nic.addLink(nicLink, "core" + str(x), self.nicParams["nic2host_lat"] )
            ep.addLink(nicLink, "nic", "1ns" )
            nic.addLink(nicLink, "core" + str(x), "1ns" )

            ep.addLink(loopLink, "loop", "1ns")
            loopBack.addLink(loopLink, "core" + str(x), "1ns")

            if built:
                memoryLink = sst.Link( "memory" + str(nodeID) + "core" + str(x) + "_Link"  )
                memoryLink.setNoCut()

                ep.addLink(memoryLink, "memoryHeap", "0 ps")
                memory.addLink(memoryLink, "detailed" + str(x), "0 ns")

        return retval


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
