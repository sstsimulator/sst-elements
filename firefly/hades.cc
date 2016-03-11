// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "hades.h"

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "functionSM.h"
#include "virtNic.h"

#include "funcSM/api.h"

using namespace SST::Firefly;
using namespace SST;

Hades::Hades( Component* owner, Params& params ) :
    OS( owner ),	
    m_virtNic(NULL)
{
    m_dbg.init("@t:Hades::@p():@l ", 
        params.find_integer("verboseLevel",0),
        params.find_integer("verboseMask",0),
        Output::STDOUT );

    Params tmpParams = params.find_prefix_params("nicParams." );

    std::string moduleName = params.find_string("nicModule"); 

    m_virtNic = dynamic_cast<VirtNic*>(owner->loadModuleWithComponent( 
                        moduleName, owner, tmpParams ) );
    if ( !m_virtNic ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nic module'%s'\n",
                                        moduleName.c_str());
    }

    moduleName = params.find_string("nodePerf", "firefly.SimpleNodePerf"); 

    tmpParams = params.find_prefix_params("nodePerf." );
    m_nodePerf = dynamic_cast<NodePerf*>(owner->loadModule( 
                                        moduleName, tmpParams ) );
    if ( !m_nodePerf ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nodePerf module'%s'\n",
                                        moduleName.c_str());
    }

    m_netMapSize = params.find_integer("netMapSize",-1);
    assert(m_netMapSize > -1 );

	if ( m_netMapSize > 0 ) {

    	int netId = params.find_integer("netId",-1);
    	int netMapId = params.find_integer("netMapId",-1);

    	if ( -1 == netMapId ) {
        	netMapId = netId; 
    	}

    	m_dbg.verbose(CALL_INFO,1,2,"netId=%d netMapId=%d netMapSize=%d\n",
            netId, netMapId, m_netMapSize );

        m_netMapName = params.find_string( "netMapName" );
        assert( ! m_netMapName.empty() );

        m_sreg = getGlobalSharedRegion( m_netMapName,
                    m_netMapSize*sizeof(int), new SharedRegionMerger());
        m_sreg->modifyArray( netMapId, netId );
        m_sreg->publish();
	}
}

Hades::~Hades()
{
    if ( m_virtNic ) delete m_virtNic;
}

void Hades::_componentSetup()
{
    m_dbg.verbose(CALL_INFO,1,1,"nodeId %d numCores %d, coreNum %d\n",
      m_virtNic->getNodeId(), m_virtNic->getNumCores(), m_virtNic->getCoreId());

	if ( m_netMapSize > 0 ) {

    	Group* group = m_info.getGroup( 
        	m_info.newGroup( MP::GroupWorld, Info::NetMap ) );
    	group->initMapping( m_sreg->getPtr<const int*>(),
					m_netMapSize, m_virtNic->getNumCores() );

    	int nid = m_virtNic->getNodeId();

    	for ( int i =0; i < group->getSize(); i++ ) {
        	if ( nid == group->getMapping( i ) ) {
           		m_dbg.verbose(CALL_INFO,1,2,"rank %d -> nid %d\n", i, nid );
            	group->setMyRank( i );
            	break;
        	} 
		}

    	m_dbg.verbose(CALL_INFO,1,2,"nid %d, numRanks %u, myRank %u \n",
								nid, group->getSize(),group->getMyRank() );
	}

    char buffer[100];
    snprintf(buffer,100,"@t:%#x:%d:Hades::@p():@l ",
                                    m_virtNic->getNodeId(), getNid());
    m_dbg.setPrefix(buffer);
}

void Hades::_componentInit(unsigned int phase )
{
    m_virtNic->init( phase );


}

int Hades::getNumNids()
{
    int size = -1;
	Group* group = m_info.getGroup(MP::GroupWorld);
	if ( group ) { 
    	size = group->getSize();
	}
    m_dbg.verbose(CALL_INFO,1,1,"size=%d\n",size);
    return size;
}

int Hades::getNid() 
{
    int rank = m_info.worldRank();
    m_dbg.verbose(CALL_INFO,1,1,"rank=%d\n",rank);
    return rank;
}
