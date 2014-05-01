
import sst
from sst.merlin import *

class EmberEP( EndPoint ):
    def __init__( self, driverParams, nicParams, numCores ):
        self.driverParams = driverParams
        self.nicParams = nicParams
        self.numCores = numCores

    def getName( self ):
        return "EmberEP"

    def prepParams( self ):
        pass

    def foo( self, nodeID ):
        return True

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
		print "numNodes", numNodes
		self.nicParams = nicParams
		self.epParams = epParams
		self.numNodes = numNodes
		self.numCores = numCores
		self.nicParams["num_vNics"] = numCores
		self.map = []
		self.nullEP, nidlist = self.foo( self.readCmdLine("Null nidList=") )
		self.nullEP.prepParams()

	def foo( self, x ):
		nidList, numCores, params = x

		params.update( self.epParams )
		params['hermesParams.nidListString'] = nidList 
		ep = EmberEP( params, self.nicParams, numCores )

		ep.prepParams()
		return (ep, nidList)
		
	def initFile(self, fileName ):
		fo = open(fileName)
		for line in iter(fo.readline,b''):
			if  line[0] != '#':
				self.map.append( self.foo( self.readCmdLine(line ) ) )
		fo.close()
		self.verifyLoadInfo()

	def initCmd(self, cmd ):
		self.map.append( self.foo( self.readCmdLine( cmd ) ) )
		self.verifyLoadInfo()

	def readCmdLine(self, cmdLine ):
		print "cmdLine=", repr(cmdLine)
		cmdList = cmdLine.split()
		nidList = cmdList[1].split("=")[1]
		#numCores = int(cmdList[2].split("=")[1])
		numCores = 1
		cmdList.pop(1);
		return ( nidList, numCores, self.parseCmd("ember.", "Motif", cmdList) ) 

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

	def inRange( self, nid, start, len ):
		if nid >= start:
			if nid < (start + len):
				return True	
		return False

	def setNode(self,nodeId):
		for ep, nidList in self.map:
			tmp = nidList.split(':')
			if self.inRange( nodeId, int(tmp[0]), int(tmp[1]) ):
				return ep 
		return self.nullEP
