// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "hades.h"

#include <sst/core/debug.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include <stdlib.h>

#include <iostream>
#include <fstream>

#include "functionSM.h"
#include "virtNic.h"

#include "funcSM/api.h"

using namespace SST::Firefly;
using namespace SST;

Hades::Hades( Component* owner, Params& params ) :
    MessageInterface(),
    m_virtNic(NULL),
	m_functionSM( NULL )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc = 
            (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:Hades::@p():@l ", verboseLevel, 0, loc );

    Params nicParams = params.find_prefix_params("nicParams." );

    std::string moduleName = params.find_string("nicModule"); 

    m_virtNic = dynamic_cast<VirtNic*>(owner->loadModuleWithComponent( 
                        moduleName, owner, nicParams ) );
    if ( !m_virtNic ) {
        m_dbg.fatal(CALL_INFO,0," Unable to find nic module'%s'\n",
                                        moduleName.c_str());
    }

    std::string nidListString = params.find_string("nidListString");
    m_dbg.verbose(CALL_INFO,1,0,"nidListString `%s`\n", nidListString.c_str());

	if ( ! nidListString.empty() ) {

		std::istringstream iss(nidListString);

    	m_info.addGroup( Hermes::GroupWorld, initAdjacentMap(iss) );

	}

  	Group* group = m_info.getGroup(Hermes::GroupWorld);

if ( group ) {


    Params tmpParams;
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    int protoNum = 0;
    tmpParams = params.find_prefix_params("ctrlMsg.");
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
    m_dbg.verbose(CALL_INFO,1,0,"numCores %d, coreNum %d\n",
                        m_virtNic->getNumCores(), m_virtNic->getCoreId());

	Group* group = m_info.getGroup(Hermes::GroupWorld);

  	m_dbg.verbose(CALL_INFO,1,0,"numRanks %lu\n", group->size());

	// if there is a group we need to setup the message passing stack
	if ( group ) {

		group->initMyRank();

    	std::map<int,ProtocolAPI*>::iterator iter= m_protocolM.begin();
    	for ( ; iter != m_protocolM.end(); ++iter ) {
        	iter->second->setup();
    	}

    	m_functionSM->setup();
	}

    char buffer[100];
    snprintf(buffer,100,"@t:%#x:%d:Hades::@p():@l ",myNodeId(), myWorldRank());
    m_dbg.setPrefix(buffer);
}

int Hades::myNodeId()
{
    if ( m_virtNic ) {
        return m_virtNic->getVirtNicId();
    } else {
        return -1;
    }
}

Group* Hades::initAdjacentMap( std::istream& nidList )
{
	int nid = 0;
    Group* group = new Group( m_virtNic );

	assert( nidList.peek() != EOF );

	std::string tmp;
	do { 
		char c = nidList.get();
		tmp += c;
	
		if ( c == ',' || nidList.peek() == EOF ) {
			int pos = tmp.find("-");
			int startNid;
			int endNid;
			std::istringstream ( tmp.substr(0, pos ) ) >> startNid;
			if ( EOF != pos ) { 
				std::istringstream ( tmp.substr( pos + 1 ) ) >> endNid; 
			} else {
				endNid = startNid;
			}
			size_t len = (endNid-startNid) + 1;
    		m_dbg.verbose(CALL_INFO,1,0,"nid=%d startNid=%d endNid=%d\n",
								nid,startNid,endNid);
			group->set( nid, startNid, len );
			nid += len;
			tmp.clear();
		}
	} while ( nidList.peek() != EOF );
		
    return group;
}

void Hades::_componentInit(unsigned int phase )
{
    m_virtNic->init( phase );
}

int Hades::myWorldSize()
{
	Group* group = m_info.getGroup(Hermes::GroupWorld);
	if ( group ) { 
    	return group->size();
	} else {
		return -1;
	}
}

Hermes::RankID Hades::myWorldRank() 
{
    int rank = m_info.worldRank();
    m_dbg.verbose(CALL_INFO,1,0,"rank=%d\n",rank);
    if ( -1 == rank ) {
        return -1; 
    } else {
        return rank;
    }
}
