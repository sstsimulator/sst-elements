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

import sst
from sst.merlin import *

def setRtrParams( networkParams ):

	sst.merlin._params["link_lat"] = networkParams['link_lat']
	sst.merlin._params["link_bw"] = networkParams['link_bw']
	sst.merlin._params["xbar_bw"] = networkParams['link_bw']
	sst.merlin._params["flit_size"] = networkParams['flitSize']
	sst.merlin._params["input_latency"] = networkParams['input_latency']
	sst.merlin._params["output_latency"] = networkParams['output_latency']
	sst.merlin._params["input_buf_size"] = networkParams['buffer_size']
	sst.merlin._params["output_buf_size"] = networkParams['buffer_size']

	if 'xbar_arb' in networkParams.keys():
		sst.merlin._params["xbar_arb"] = networkParams['xbar_arb']

	if "network_inspectors" in networkParams.keys():
		sst.merlin._params["network_inspectors"] = networkParams['network_inspectors']

def setTopoParams( topParams ):
	sst.merlin._params.update( topParams )
