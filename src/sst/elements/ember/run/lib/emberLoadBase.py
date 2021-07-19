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

import sys,pprint

pp = pprint.PrettyPrinter(indent=4)

import topoConfig as TopoConfig
import platConfig as Platform
import merlin as Merlin
import loadUtils as LoadUtils
import loadInfo as LoadInfo
import nicConfig as NicConfig
import rtrConfig as RtrConfig
import emberConfig as EmberConfig
import nullEmber as NullEmber

def getOptions():
	return NicConfig.getOptions() + RtrConfig.getOptions() + EmberConfig.getOptions()

def run( opts, platParamsName, topo, shape, jobs, perNicParams = None ):

	topoInfo = TopoConfig.getTopoInfo( topo, shape )
	topoObj = TopoConfig.getTopoObj( topo )

	print 'Platform: configuration "{0}"'.format( platParamsName )
	print 'Network: topo={0} shape={1} numNodes={2}'.format( topo, shape, topoInfo.getNumNodes() )

	platParams = Platform.getParams( platParamsName )

	nicConfig = NicConfig.NicConfig( platParams.nicParams, opts, perNicParams )
	rtrConfig = RtrConfig.RtrConfig( platParams.networkParams, opts )

	hermesParams = platParams.hermesParams
	emberParams = platParams.emberParams

	Merlin.setTopoParams( topoInfo.getParams() )
	Merlin.setRtrParams( rtrConfig.getParams() )

	nullEmber = NullEmber.create( emberParams, hermesParams)

	loadInfo = LoadInfo.LoadInfo( nicConfig, topoInfo.getNumNodes(), nullEmber )

	topoObj.setEndPointFunc( loadInfo.setNode )

	for job in jobs:

		if None == job.getNidlist():
			nidList = LoadUtils.genNidList( topoInfo.getNumNodes(), \
					job.getNumNodes(), job.getRandom() )
			job.setNidList( nidList )
		job.printInfo()

		loadInfo.addEmberConfig( EmberConfig.EmberConfig( emberParams, hermesParams, job, opts ) )

	topoObj.prepParams()
	topoObj.build()
