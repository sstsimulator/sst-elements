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

import jobInfo as JobInfo
import emberConfig as EmberConfig

def _genWorkFlow( defaults, nodeNum = None ):

    workFlow = []
    motif = dict.copy( defaults )
    motif['cmd'] = "Null"
    workFlow.append( motif )

    return workFlow

def create( emberParams, hermesParams ):

	jobInfo = JobInfo.JobInfo( -1, -1, 1, _genWorkFlow )
	jobInfo.setNidList( 'Null' )

	return EmberConfig.EmberConfig( emberParams, hermesParams, jobInfo )
