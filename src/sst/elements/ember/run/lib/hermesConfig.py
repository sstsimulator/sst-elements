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

def getOptions():
	return ['hermesVerbose=']

def parseOptions(opts):
	level = 0
	if opts:
		for o,a in opts:
			if o in ('--hermesVerbose'):
				level = int(a)
	
	return level

class HermesConfig:
	def __init__(self, params, opts ):
		self.params = params

	def getParams(self):
		return self.params
