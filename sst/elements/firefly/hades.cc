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
    m_virtNic(NULL),
	m_functionSM( NULL ),
    m_gt( Info::Dense )
{
    m_dbg.init("@t:Hades::@p():@l ", 
        params.find_integer("verboseLevel",0),
        //0,
        0,
        Output::STDOUT );

    Params nicParams = params.find_prefix_params("nicParams." );

    std::string moduleName = params.find_string("nicModule"); 

    m_virtNic = dynamic_cast<VirtNic*>(owner->loadModuleWithComponent( 
                        moduleName, owner, nicParams ) );
    if ( !m_virtNic ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nic module'%s'\n",
                                        moduleName.c_str());
    }

    m_nidListString = params.find_string("nidListString");
    m_dbg.verbose(CALL_INFO,1,0,"nidListString `%s`\n", 
                                            m_nidListString.c_str());

    std::string gt = params.find_string("mapType");

    if ( 0 == gt.compare( "dense" )  ) {
        m_gt = Info::Dense;
    } else if ( 0 == gt.compare( "identity" ) ) {
        m_gt = Info::Identity;
    } else if ( 0 == gt.compare( "random" ) ) {
        m_gt = Info::Random;
    } else if ( ! gt.empty() ) {
        assert(0);
    }
#if 0
	if ( ! nidListString.empty() ) {

		std::istringstream iss(nidListString);

    	m_info.addGroup( MP::GroupWorld, initAdjacentMap(iss) );

	}

  	Group* group = m_info.getGroup(MP::GroupWorld);

if ( group ) {

    Params tmpParams;
    m_dbg.verbose(CALL_INFO,1,0,"\n");
#endif
    int protoNum = 0;
    Params tmpParams = params.find_prefix_params("ctrlMsg.");
    m_protocolM[ protoNum ] = 
        dynamic_cast<ProtocolAPI*>(owner->loadModuleWithComponent(
                            "firefly.CtrlMsgProto", owner, tmpParams ) );

    assert( m_protocolM[ protoNum ]);
    m_protocolM[ protoNum ]->init( &m_info, m_virtNic );

    m_protocolMapByName[ m_protocolM[ protoNum ]->name() ] =
                                                m_protocolM[ protoNum ];
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_protocolM[ protoNum ]->name().c_str());

    Params funcParams = params.find_prefix_params("functionSM.");

    m_functionSM = new FunctionSM( funcParams, owner, m_info, m_enterLink,
                                    m_protocolMapByName );
}

Hades::~Hades()
{
    while ( ! m_protocolM.empty() ) {
        delete m_protocolM.begin()->second;
        m_protocolM.erase( m_protocolM.begin() );
    }
    
    if ( m_functionSM ) delete m_functionSM;
    if ( m_virtNic ) delete m_virtNic;
}

void Hades::printStatus( Output& out )
{
    std::map<int,ProtocolAPI*>::iterator iter= m_protocolM.begin();
    for ( ; iter != m_protocolM.end(); ++iter ) {
        iter->second->printStatus(out);
    }
    m_functionSM->printStatus( out );
}

void Hades::_componentSetup()
{
    m_dbg.verbose(CALL_INFO,1,0,"nodeId %d numCores %d, coreNum %d\n",
      m_virtNic->getNodeId(), m_virtNic->getNumCores(), m_virtNic->getCoreId());

	if ( ! m_nidListString.empty() ) {
        Info::GroupType gt;
		
  	    Group* group = m_info.getGroup( m_info.newGroup( MP::GroupWorld, m_gt ) );

		std::istringstream iss( m_nidListString );

        initAdjacentMap(iss, group, m_virtNic->getNumCores() ); 

  	    m_dbg.verbose(CALL_INFO,1,0,"numRanks %u\n", group->getSize());
        int nid = m_virtNic->getNodeId();

  	    m_dbg.verbose(CALL_INFO,1,0,"nid %u\n", nid);

        for ( int i =0; i < group->getSize(); i++ ) {
            if ( nid == group->getMapping( i ) ) {
  	            m_dbg.verbose(CALL_INFO,1,0,"rank %d -> nid %d\n", i, nid );
                group->setMyRank( i );
                break;
            } 
        }
	}

    std::map<int,ProtocolAPI*>::iterator iter= m_protocolM.begin();
    for ( ; iter != m_protocolM.end(); ++iter ) {
       	iter->second->setup();
    }

  	m_functionSM->setup();

    char buffer[100];
    snprintf(buffer,100,"@t:%#x:%d:Hades::@p():@l ",
                                    m_virtNic->getNodeId(), getNid());
    m_dbg.setPrefix(buffer);
}

void Hades::initAdjacentMap( std::istream& nidList, Group* group, int numCores )
{
	int nid = 0;

	assert( nidList.peek() != EOF );

	std::string tmp;
	do { 
		char c = nidList.get();
		tmp += c;
	
		if ( c == ',' || nidList.peek() == EOF ) {
			if ( ! tmp.compare("Null") ) {
				continue;
			}
			size_t pos = tmp.find("-");
			int startNid;
			int endNid;
			std::istringstream ( tmp.substr(0, pos ) ) >> startNid;
			if ( std::string::npos != pos ) { 
				std::istringstream ( tmp.substr( pos + 1 ) ) >> endNid; 
				endNid = ((endNid + 1) * numCores ) -1;
			} else {
				endNid = (startNid + numCores) - 1;
			}
			size_t len = ( endNid - startNid ) + 1;
    		m_dbg.verbose(CALL_INFO,1,0,"nid=%d startNid=%d endNid=%d\n",
					nid, startNid, endNid);

            if ( numCores > 1 ) {
    		    m_dbg.verbose(CALL_INFO,1,0,"nid=%d startNid=%d endNid=%lu\n",
					nid, startNid*numCores, endNid*numCores );
            }

			group->initMapping( nid, startNid * numCores, len );
			nid += len;
			tmp.clear();
		}
	} while ( nidList.peek() != EOF );
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
    m_dbg.verbose(CALL_INFO,1,0,"size=%d\n",size);
    return size;
}

int Hades::getNid() 
{
    int rank = m_info.worldRank();
    m_dbg.verbose(CALL_INFO,1,0,"rank=%d\n",rank);
    return rank;
}
