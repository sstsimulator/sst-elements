// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "hades.h"

#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include "hadesMisc.h"
#include "sst/elements/thornhill/detailedCompute.h"

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "functionSM.h"
#include "virtNic.h"

#include "funcSM/api.h"

using namespace SST::Firefly;
using namespace SST;

Hades::Hades( ComponentId_t id, Params& params ) :
    OS( id, params ),
    m_virtNic(NULL),
    m_detailedCompute(NULL),
    m_memHeapLink(NULL)
{
    m_dbg.init("@t:Hades::@p():@l ",
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",0),
        Output::STDOUT );

    Params tmpParams = params.get_scoped_params("ctrlMsg");

    m_proto = loadUserSubComponent<CtrlMsg::API>( "proto" );

    Params funcParams = params.get_scoped_params("functionSM");

    m_numNodes = params.find<int>("numNodes",0);

    m_functionSM = loadAnonymousSubComponent<FunctionSM>( "firefly.functionSM","", 0, ComponentInfo::SHARE_NONE, funcParams, m_proto );

    tmpParams = params.get_scoped_params("nicParams" );

    std::string moduleName = params.find<std::string>("nicModule");

    m_virtNic = loadUserSubComponent<VirtNic>("virtNic", ComponentInfo::SHARE_NONE);

    if ( !m_virtNic ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nic SubComponent '%s'\n", moduleName.c_str());
    }

    moduleName = params.find<std::string>("nodePerf", "firefly.SimpleNodePerf");

    tmpParams = params.get_scoped_params("nodePerf" );
    m_nodePerf = dynamic_cast<NodePerf*>(loadModule(
                                        moduleName, tmpParams ) );
    if ( !m_nodePerf ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nodePerf module'%s'\n",
                                        moduleName.c_str());
    }

    Params dtldParams = params.get_scoped_params( "detailedCompute" );
    std::string dtldName =  dtldParams.find<std::string>( "name" );

    if ( ! dtldName.empty() ) {
        m_detailedCompute = loadUserSubComponent<Thornhill::DetailedCompute>("detailedCompute", ComponentInfo::SHARE_NONE);
    }

    std::string memName = params.get_scoped_params( "memoryHeapLink" ).find<std::string>( "name" );

    if ( ! memName.empty() ) {
        m_memHeapLink = loadUserSubComponent<Thornhill::MemoryHeapLink>( "memoryHeap", ComponentInfo::SHARE_NONE );

        if ( m_memHeapLink && ! m_memHeapLink->isConnected() ) {
            m_memHeapLink = NULL;
        }
    }

    m_netMapSize = params.find<int>("netMapSize",-1);
    assert(m_netMapSize > -1 );

	if ( m_netMapSize > 0 ) {

    	int netId = params.find<int>("netId",-1);
    	int netMapId = params.find<int>("netMapId",-1);

    	if ( -1 == netMapId ) {
        	netMapId = netId;
    	}

    	m_dbg.debug(CALL_INFO,1,2,"netId=%d netMapId=%d netMapSize=%d\n",
            netId, netMapId, m_netMapSize );

        m_netMapName = params.find<std::string>( "netMapName" );
        assert( ! m_netMapName.empty() );

        // m_sreg = getGlobalSharedRegion( m_netMapName,
        //             m_netMapSize*sizeof(int), new SharedRegionMerger());

        m_sreg.initialize(m_netMapName, m_netMapSize);
        if ( 0 == params.find<int>("coreId",0) ) {
            m_sreg.write( netMapId, netId );
        }
        m_sreg.publish();
	}
}

Hades::~Hades()
{
    delete m_functionSM;
}
void Hades::finish(  )
{
    m_proto->finish();
}

void Hades::_componentSetup()
{
    m_dbg.debug(CALL_INFO,1,1,"nodeId %d numCores %d, coreNum %d\n",
      m_virtNic->getNodeId(), m_virtNic->getNumCores(), m_virtNic->getCoreId());

	if ( m_netMapSize > 0 ) {

    	Group* group = m_info.getGroup(
        	m_info.newGroup( MP::GroupWorld, Info::NetMap ) );
    	group->initMapping( &(*m_sreg.begin()),
					m_netMapSize, m_virtNic->getNumCores() );

    	int nid = m_virtNic->getNodeId();

    	for ( int i =0; i < group->getSize(); i++ ) {
        	if ( nid == group->getMapping( i ) ) {
           		m_dbg.debug(CALL_INFO,1,2,"rank %d -> nid %d\n", i, nid );
            	group->setMyRank( i );
            	break;
        	}
		}

    	m_dbg.debug(CALL_INFO,1,2,"nid %d, numRanks %u, myRank %u \n",
								nid, group->getSize(),group->getMyRank() );
	}

    char buffer[100];
    snprintf(buffer,100,"@t:%#x:%d:Hades::@p():@l ",
                                    m_virtNic->getNodeId(), getRank());
    m_dbg.setPrefix(buffer);

    m_proto->setVars( getInfo(), getNic(), getMemHeapLink(), m_functionSM->getRetLink() );
    m_functionSM->setup(getInfo() );

    if (  m_detailedCompute ) {
        m_dbg.verbose(CALL_INFO, 1, 0,"detailed compute connected\n");
    }
    if ( m_memHeapLink ) {
        m_dbg.verbose(CALL_INFO, 1, 0,"memHeap connected\n");
    }
}

void Hades::_componentInit(unsigned int phase )
{
    m_virtNic->init( phase );
}

int Hades::getNodeNum()
{
    return m_virtNic->getRealNodeId();
}

int Hades::getRank()
{
    int rank = m_info.worldRank();
    m_dbg.debug(CALL_INFO,1,1,"rank=%d\n",rank);
    return rank;
}
