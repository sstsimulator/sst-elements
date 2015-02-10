
import sst
from sst.merlin import *

class EmberEP( EndPoint ):
    def __init__( self, jobId, driverParams, nicParams, numCores, ranksPerNode ):
        self.driverParams = driverParams
        self.nicParams = nicParams
        self.numCores = numCores
        self.driverParams['jobId'] = jobId

    def getName( self ):
        return "EmberEP"

    def prepParams( self ):
        pass

    def build( self, nodeID, link, extraKeys ):
        nic = sst.Component( "nic" + str(nodeID), "firefly.nic" )
        nic.addParams( self.nicParams )
        nic.addParams( extraKeys)
        nic.addParam( "nid", nodeID )
        nic.addLink( link, "rtr", "10ns" )

        loopBack = sst.Component("loopBack" + str(nodeID), "firefly.loopBack")
        loopBack.addParam( "numCores", self.numCores )

        for x in xrange(self.numCores):
            ep = sst.Component("nic" + str(nodeID) + "core" + str(x) +
                                            "_EmberEP", "ember.EmberEngine")
            ep.addParams(self.driverParams)
            nicLink = sst.Link( "nic" + str(nodeID) + "core" + str(x) +
                                            "_Link"  )
            nicLink.setNoCut()

            loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) +
                                            "_Link"  )
            loopLink.setNoCut() 

            ep.addLink(nicLink, "nic", "150ns")
            nic.addLink(nicLink, "core" + str(x), "150ns")

            ep.addLink(loopLink, "loop", "1ns")
            loopBack.addLink(loopLink, "core" + str(x), "1ns")


class LoadInfo:

	def __init__(self, nicParams, epParams, numNodes, numCores ):
		print "system configuration: numNodes={0} coresPerNode={1}".format( numNodes, numCores )
		self.nicParams = nicParams
		self.epParams = epParams
		self.numNodes = int(numNodes)
		self.numCores = int(numCores)
		self.nicParams["num_vNics"] = numCores
		self.map = []
		nullMotif = [{
			'cmd' : "-nidList=Null Null",
			'printStats' : 0,
			'api': "",
			'spyplotmode': 0
		}]

		self.nullEP, nidlist = self.foo( -1, self.readWorkList( nullMotif ) )
		self.nullEP.prepParams()

	def foo( self, jobId, x ):
		nidList, ranksPerNode, params = x

		params.update( self.epParams )
		params['hermesParams.nidListString'] = nidList 
		ep = EmberEP( jobId, params, self.nicParams, self.numCores, ranksPerNode )

		ep.prepParams()
		return (ep, nidList)
		
	def initFile(self, extra, fileName ):
		fo = open(fileName)
		jobId = 0
		for line in iter(fo.readline,b''):
			if  line[0] != '#' and False == line.isspace():
				self.map.append( self.foo( jobId, self.readWorkList( extra, [line] ) ) )
				jobId += 1
		fo.close()
		self.verifyLoadInfo()

	def initWork(self, workList ):
		for work in workList:
			self.map.append( self.foo( 0, self.readWorkList( work ) ) )
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
				print "Job: -nidList={0} -ranksPerNode={1} {2}".format( nidList, ranksPerNode, cmdList )

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
