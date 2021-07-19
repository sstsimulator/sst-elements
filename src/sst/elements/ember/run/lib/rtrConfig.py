# Copyright 2009-2021 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2021, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import pprint
import myprint

pp = pprint.PrettyPrinter(indent=4)

def getOptions():
	return ["netBW=", "netFlitSize=", "rtrArb=", "netInspect="]

class RtrConfig:
	def __init__( self, params, opts ):
		self._params = params

		for o,a in opts:
			if o in ('--netBW='):
				self._params['link_bw'] = a

			elif o in ('--netFlitSize='):
				self._params['flitSize'] = a

			elif o in ('--rtrArb='):
				self._params['xbar_arb'] = a

			elif o in ('--netInspect='):
				self._params['network_inspectors'] = a

		#myprint.printParams( 'RtrConfig:', self._params );

	def getParams(self ):
		return self._params
