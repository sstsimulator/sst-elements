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
	return ['platParams='] 

def parseOptions(opts):
    name = 'defaultParams'
    for o,a in opts:
        if o in ('--platParams'):
            name = a
    return name

def getParams( name ):

	platConfig = None
	try:
		platConfig = __import__( name, fromlist=[''] )
	except:
		sys.exit('FATAL: could not import `{0}`'.format(name) )

	return platConfig
