
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
            loopLink = sst.Link( "loop" + str(nodeID) + "core" + str(x) +
                                            "_Link"  )
            ep.addLink(nicLink, "nic", "150ns")
            nic.addLink(nicLink, "core" + str(x), "150ns")

            ep.addLink(loopLink, "loop", "1ns")
            loopBack.addLink(loopLink, "core" + str(x), "1ns")


class LoadInfo:

	def __init__(self, nicParams, epParams, numNodes, numCores ):
		print "numNodes", numNodes, "numCores", numCores
		self.nicParams = nicParams
		self.epParams = epParams
		self.numNodes = int(numNodes)
		self.numCores = int(numCores)
		self.nicParams["num_vNics"] = numCores
		self.map = []
		self.nullEP, nidlist = self.foo( -1, self.readCmdLine("Null nidList=") )
		self.nullEP.prepParams()

	def foo( self, jobId, x ):
		nidList, ranksPerNode, params = x

		params.update( self.epParams )
		params['hermesParams.nidListString'] = nidList 
		ep = EmberEP( jobId, params, self.nicParams, self.numCores, ranksPerNode )

		ep.prepParams()
		return (ep, nidList)
		
	def initFile(self, fileName ):
		fo = open(fileName)
		jobId = 0
		for line in iter(fo.readline,b''):
			if  line[0] != '#':
				self.map.append( self.foo( jobId, self.readCmdLine(line ) ) )
				jobId += 1
		fo.close()
		self.verifyLoadInfo()

	def initCmd(self, cmd ):
		self.map.append( self.foo( 0, self.readCmdLine( cmd ) ) )
		self.verifyLoadInfo()

	def readCmdLine(self, cmdLine ):
		print "cmdLine=", repr(cmdLine)
		cmdList = cmdLine.split()
		motifCmd = [] 
		motifCmd.append( cmdList.pop(0) )

		ranksPerNode = self.numCores 
		nidList = []
		while len(cmdList):
			arg = cmdList.pop(0)

			if "ranksPerNode" == arg.split("=")[0]:
				ranksPerNode = int(arg.split("=")[1])
			elif "nidList" == arg.split("=")[0]:
				nidList = arg.split("=")[1]
			else:
				motifCmd.append( arg )

		if 0 == len(nidList):
			nidList = "0-" + str(self.numNodes-1) 
			
		print "nidList", nidList, "ranksPerNode", ranksPerNode

		if  ranksPerNode > self.numCores:
			sys.exit("Error: " + str(ranksPerNode) + " ranksPerNode is greater than "+
						str(self.numCores) + " coresPerNode")
		return ( nidList, ranksPerNode, self.parseCmd("ember.", "Motif", motifCmd) ) 

	def parseCmd(self, motifPrefix, motifSuffix, cmdList ):
		motif = {}
		motif['motif'] = motifPrefix + cmdList[0] + motifSuffix
		cmdList.pop(0)
		for x in cmdList:
			y = x.split("=")
			motif['motifParams.' + y[0]] = y[1]
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
