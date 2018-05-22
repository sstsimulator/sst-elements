# Copyright 2009-2018 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2018, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys

def getOptions():
	return [ "detailedNameModel=", "detailedModelParams=", "detailedModelNodes=" ]

class DetailedModel:
    def getName(self):
        pass
    def build(self,nodeID,numCores):
        pass 
    def getThreadLink(self,core):
        pass
    def getNicLink(self):
        pass

def getModel( model, params ):

	#print 'getModel() model={0} params={1}'.format(model,params)

	try:
		modelModule = __import__( model, fromlist=[''] )

	except:
		sys.exit('Failed: could not import detailed model `{0}`'.format(model) )

	try:
		modelParams = __import__( params, fromlist=[''] )
	except:
		sys.exit('Failed: could not import detailed model params `{0}`'.format(params) )

	return modelModule.getModel(modelParams.params)
