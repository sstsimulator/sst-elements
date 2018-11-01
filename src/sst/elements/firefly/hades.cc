// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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

Hades::Hades( Component* owner, Params& params ) :
    OS( owner ),	
    m_virtNic(NULL),
    m_detailedCompute(NULL),
    m_memHeapLink(NULL)
{
    m_dbg.init("@t:Hades::@p():@l ", 
        params.find<uint32_t>("verboseLevel",0),
        params.find<uint32_t>("verboseMask",0),
        Output::STDOUT );

    Params tmpParams = params.find_prefix_params("ctrlMsg.");
    m_proto = dynamic_cast<ProtocolAPI*>(owner->loadSubComponent(
                                    "firefly.CtrlMsgProto", owner, tmpParams ) );

    Params funcParams = params.find_prefix_params("functionSM.");

    m_numNodes = params.find<int>("numNodes",0); 
    m_functionSM = new FunctionSM( funcParams, owner, m_proto );

    tmpParams = params.find_prefix_params("nicParams." );

    std::string moduleName = params.find<std::string>("nicModule"); 

    m_virtNic = dynamic_cast<VirtNic*>(owner->loadModuleWithComponent( 
                        moduleName, owner, tmpParams ) );
    if ( !m_virtNic ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nic module'%s'\n",
                                        moduleName.c_str());
    }

    moduleName = params.find<std::string>("nodePerf", "firefly.SimpleNodePerf");

    tmpParams = params.find_prefix_params("nodePerf." );
    m_nodePerf = dynamic_cast<NodePerf*>(owner->loadModule( 
                                        moduleName, tmpParams ) );
    if ( !m_nodePerf ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nodePerf module'%s'\n",
                                        moduleName.c_str());
    }

    Params dtldParams = params.find_prefix_params( "detailedCompute." );
    std::string dtldName =  dtldParams.find<std::string>( "name" );

    if ( ! dtldName.empty() ) {

        m_detailedCompute = dynamic_cast<Thornhill::DetailedCompute*>( loadSubComponent(
                            dtldName, dtldParams ) );

        assert( m_detailedCompute );
        if ( ! m_detailedCompute->isConnected() ) {
            delete m_detailedCompute;
            m_detailedCompute = NULL;
        }
    }

    Params memParams = params.find_prefix_params( "memoryHeapLink." );
    std::string memName =  memParams.find<std::string>( "name" );

    if ( ! memName.empty() ) {

        m_memHeapLink = dynamic_cast<Thornhill::MemoryHeapLink*>( loadSubComponent(
                            memName, memParams ) );

        if ( ! m_memHeapLink->isConnected() ) {
            delete m_memHeapLink;
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

        m_sreg = getGlobalSharedRegion( m_netMapName,
                    m_netMapSize*sizeof(int), new SharedRegionMerger());

        if ( 0 == params.find<int>("coreId",0) ) {
            m_sreg->modifyArray( netMapId, netId );
        }

        m_sreg->publish();
	}
	Group* group = m_info.getGroup( m_info.newGroup( MP::GroupMachine, Info::Identity ) );
	group->initMapping(  m_numNodes );
}

Hades::~Hades()
{
    if ( m_virtNic ) delete m_virtNic;
    delete m_proto;
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
    	group->initMapping( m_sreg->getPtr<const int*>(),
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
