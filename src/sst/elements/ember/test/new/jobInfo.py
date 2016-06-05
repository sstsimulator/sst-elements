
motifDefaults = {
    'cmd' : "",
    'api': "HadesMP",
    'spyplotmode': 0
}

className = 'JobInfo'

class JobInfoBase:
	def __init__(self): 
		self._jobId = None 
		self._numNodes = None 
		self._nidlist = None
		self._ranksPerNode = None
		self._motifDefaults = None 

	def genWorkFlow( self, nodeNum ):
		pass

	def nidlist(self):
		print "nitlist"
		return self._nidlist

	def setNidList(self,nidlist):
		print "setnidlist",nidlist
		#sys.exit()
		self._nidlist = nidlist

	def ranksPerNode(self):
		return self._ranksPerNode

	def getNumNodes(self):
		return self._numNodes

	def jobId(self):
		return self._jobId

	def printMe(self):
		print "jobId={0} numNodes={1} numRanksPerNode={2}".\
			format( self._jobId, self._numNodes, self._ranksPerNode )		


class JobInfoX(JobInfoBase):
	def __init__(self, jobId, defaults = motifDefaults ): 
		self._jobId = jobId
		self._motifDefaults = defaults 
		self._ranksPerNode = 1
		self._motifs = [] 
	
	def addMotif(self, cmd ):
		print "cmd",cmd
		motif = dict.copy( self._motifDefaults )
 		motif['cmd'] = cmd
		self._motifs += [motif]

	def setNumRanksPerNode( self, num ):
		self._ranksPerNode = num

	def setNumNodes( self, num ):
		self._numNodes = num

	def genWorkFlow( self, nodeNum ):
		return self._motifs

	
class JobInfo(JobInfoBase):

	def __init__(self, jobId, numNodes, ranksPerNode, workflow, defaults = motifDefaults ): 
		print className + '()'
		print 'jobId', jobId
		print 'ranksPerNode', ranksPerNode
		self._jobId = jobId
		self._numNodes = numNodes
		self._ranksPerNode = ranksPerNode
		self._genWorkFlow = workflow
		self._motifDefaults = defaults

	def genWorkFlow( self, nodeNum ):
		print className + "::genWorkFlow()"
		return self._genWorkFlow( self._motifDefaults, nodeNum )
