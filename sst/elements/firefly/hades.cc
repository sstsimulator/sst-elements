// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"
#include "hades.h"

#include "sst/core/debug.h"
#include "sst/core/component.h"
#include "sst/core/params.h"

#include <stdlib.h>

#include <iostream>
#include <fstream>

#include "ioapi.h"
#include "functionSM.h"
#include "entry.h"
#include "nodeInfo.h"

#include "funcSM/api.h"

#include "dataMovement.h"
#include "ctrlMsg.h"

using namespace SST::Firefly;
using namespace SST;

Hades::Hades( Params& params ) :
    MessageInterface(),
    m_pendingSends( 0 ),
    m_io( NULL ),
    m_completedIO( NULL ),
    m_delay( NULL )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc = 
            (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:Hades::@p():@l ", verboseLevel, 0, loc );

    m_owner = (SST::Component*) params.find_integer( "owner" );

    m_enterLink = m_owner->configureSelfLink("HadesEnterLink", "1 ps",
        new Event::Handler<Hades>(this,&Hades::enterEventHandler));

    Params ioParams = params.find_prefix_params("ioParams." );

    std::ostringstream m_ownerName;
    m_ownerName << m_owner;
    ioParams.insert(
        std::pair<std::string,std::string>("owner", m_ownerName.str()));

    std::string moduleName = params.find_string("ioModule"); 
    m_io = dynamic_cast<IO::Interface*>(m_owner->loadModule( moduleName,
                        ioParams));
    if ( !m_io ) {
        m_dbg.fatal(CALL_INFO,0,1,0," Unable to find Hermes '%s'\n",
                                        moduleName.c_str());
    }

    Params nodeParams = params.find_prefix_params("nodeParams.");
    
    m_nodeInfo = new NodeInfo( nodeParams );

    m_dbg.verbose(CALL_INFO,1,0,"numCores %d, coreNum %d\n",
                        m_nodeInfo->numCores(), m_nodeInfo->coreNum());

    int numRanks = params.find_integer("numRanks");
    if ( numRanks <= 0 ) {
        m_dbg.fatal(CALL_INFO,0,1,0,"How many global ranks?\n");
    }
    m_dbg.verbose(CALL_INFO,1,0,"numRanks %d\n", numRanks);

    std::string nidListFileName = params.find_string("nidListFile");

    m_dbg.verbose(CALL_INFO,1,0,"nidListFile `%s`\n",nidListFileName.c_str());

    std::ifstream nidListFile( nidListFileName.c_str());
    if ( ! nidListFile.is_open() ) {
        m_dbg.verbose(CALL_INFO,0,1,"Unable to open nid list '%s'\n",
                                        nidListFileName.c_str() );
    }

    Group* group;

    std::string policy = params.find_string("policy");
    m_dbg.verbose(CALL_INFO,1,0,"load policy `%s`\n",policy.c_str());

    if ( 0 == policy.compare("adjacent") ) {
        group = initAdjacentMap(numRanks, m_nodeInfo->numCores(), nidListFile);
#if 0
    } else if ( 0 == policy.compare("roundRobin") ) {
        group = initRoundRobinMap(numRanks, m_nodeInfo->numCores(), nidListFile); 
#endif
    } else {
        group = NULL; // get rid of compiler warning
        m_dbg.fatal(CALL_INFO,0,1,0,"unknown load policy `j%s` ",
                                            policy.c_str() );
    }
    m_info.addGroup( Hermes::GroupWorld, group);

    nidListFile.close();

    m_protocolM[0] = 
        new DataMovement( params.find_prefix_params("dataMovement."), &m_info );

    m_protocolM[1] = 
        new CtrlMsg( params.find_prefix_params("ctrlMsg."), &m_info);

    m_sendIter = m_protocolM.begin();

    Params funcParams = params.find_prefix_params("functionSM.");

    m_functionSM = new FunctionSM( funcParams, m_owner, m_info, m_enterLink,
                                    m_protocolM[0], m_protocolM[1] );
}

void Hades::enterEventHandler(SST::Event* )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    if ( m_delay ) {
        completedDelay();
    }

    if ( m_completedIO ) {
        completedIO();
    }

    if ( ! pendingSend() ) {
        runSend();
    }
    
    if ( ! m_delay ) {
        runRecv();
    }

    if ( m_io->pending() || functionIsBlocked()  ) {
        m_dbg.verbose(CALL_INFO,1,0,"pass control to I/O SM\n");
        m_io->enter( m_enterLink );
    } else if ( ! m_delay )  {
        // return to function SM
        m_dbg.verbose(CALL_INFO,1,0,"pass control to function SM\n");
        m_functionSM->sendProgressEvent( NULL );
    }
}

void Hades::completedDelay()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    IO::Entry* tmp = NULL;
    if ( m_delay ) {
        tmp = delayDone( m_delay );
    }
    if ( tmp ) {
        delete tmp;
    }
    m_delay = NULL;
}

void Hades::completedIO()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    IO::Entry* tmp = NULL;
    switch (  m_completedIO->ioType ) {
    case AAA::SendWireHdrDone:
        tmp = sendWireHdrDone( m_completedIO );
        break;
    case AAA::SendIODone:
        tmp = sendIODone( m_completedIO );
        break;
    case AAA::RecvWireHdrDone:
        tmp = recvWireHdrDone( m_completedIO );
        break;
    case AAA::RecvIODone:
        tmp = recvIODone( m_completedIO );
        break;
    } 
    
    if ( tmp ) {
        delete tmp;
    }
    m_completedIO = NULL;
}

Hermes::RankID Hades::myWorldRank() 
{
    int rank = _myWorldRank();
    m_dbg.verbose(CALL_INFO,1,0,"rank=%d\n",rank);
    if ( -1 == rank ) {
        m_dbg.fatal(CALL_INFO,0,1,0,"%s() rank not set yet\n",__func__);
        return -1; 
    } else {
        return rank;
    }
}

void Hades::_componentSetup()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    Group* group = m_info.getGroup(Hermes::GroupWorld);
    for ( unsigned int i = 0; i < group->size(); i++ ) {
        m_dbg.verbose(CALL_INFO,2,0,"group rank %d, %d %d\n",
                                    i,group->getNodeId( i ),myNodeId() );
        if ( group->getNodeId( i ) == myNodeId() && 
                group->getCoreId( i ) == m_nodeInfo->coreNum() ) 
        {
            m_dbg.verbose(CALL_INFO,1,0,"set rank to %d\n",i);
            group->setMyRank( i );
            break;
        }
    } 

    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:Hades::@p():@l ",myNodeId(), myWorldRank());
    m_dbg.setPrefix(buffer);

    m_dbg.verbose(CALL_INFO,1,0, "myRank %d\n",
                m_info.getGroup(Hermes::GroupWorld)->getMyRank() );

    m_protocolM[0]->setup();
    m_protocolM[1]->setup();

    m_functionSM->setup();
}

Group* Hades::initAdjacentMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );

    m_dbg.verbose(CALL_INFO,2,0,"numRanks=%d numCores=%d\n",numRanks,numCores);

    int nid = -1;
    for ( int node = 0; node < numRanks/numCores; node++ ) {

        if ( ! nidFile.is_open()  ) { 
            ++nid;
        } else {
            std::string line;
            getline( nidFile, line );
            int ret = sscanf( line.c_str(), "%d", &nid ); 
            if( ret != 1 ) {
                _abort(Hades, "ERROR: nidList is not long enough, want %d %d\n", numRanks, ret);
            }
        }

        for ( int core = 0; core < numCores; core++ ) {
            group->set( node * numCores + core, nid, core );
            m_dbg.verbose(CALL_INFO,2,0,"rank %d is on nid %d\n",
                                            node * numCores + core , nid);
        }
    }
    return group;
}

Group* Hades::initRoundRobinMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );
    m_dbg.verbose(CALL_INFO,1,0,"numRanks=%d numCores=%d\n",
                            numRanks, numCores);

    return group;
}

void Hades::_componentInit(unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_io->_componentInit(phase);
}

bool Hades::functionIsBlocked() {
    std::map<int,ProtocolAPI*>::iterator iter ;
    for ( iter = m_protocolM.begin(); iter != m_protocolM.end(); ++iter ) {
        if ( iter->second->blocked() ) {
            m_dbg.verbose(CALL_INFO,1,0,"blocked\n"); 
            return true;
        } 
    }
    return false;
}

bool Hades::runSend( )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

here:
    ProtocolAPI::Request* req = m_sendIter->second->getSendReq();

    if ( req ) {
        // have something to send, configure and send the protocol type
        AAA* aaa = new AAA;

        aaa->protoType    = m_sendIter->first;
        aaa->request = req;
        aaa->ioType = AAA::SendWireHdrDone;

        std::vector<IO::IoVec> ioVec; 
        ioVec.resize(1);
        ioVec[0].ptr = &aaa->protoType;
        ioVec[0].len = sizeof(aaa->protoType); 

        aaa->callback = new IO_Functor( this, &Hades::ioDone, aaa );

        m_io->sendv( req->nodeId, ioVec, aaa->callback );
        ++m_pendingSends;
        return true;
    } else {
        ++m_sendIter;
        if ( m_sendIter != m_protocolM.end() ) {
            goto here;
        } else {
            m_sendIter = m_protocolM.begin();
        }  
    } 

    return false;
}

IO::Entry* Hades::ioDone( IO::Entry* e )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( ! m_completedIO );
    m_completedIO = static_cast<AAA*>(e);
    return NULL;
}

IO::Entry* Hades::sendWireHdrDone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);

    delete aaa->callback;
    aaa->callback = new IO_Functor( this, &Hades::ioDone, aaa );
    aaa->ioType = AAA::SendIODone;
    m_io->sendv( aaa->request->nodeId, aaa->request->ioVec, aaa->callback );
    
    return NULL;
}

IO::Entry* Hades::sendIODone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_protocolM[ aaa->protoType ]->sendIODone( aaa->request ); 
    --m_pendingSends;
    return e;
}

void Hades::runRecv()
{
    IO::NodeId src = m_io->peek( ); 
    if ( src == IO::AnyId ) {
        return;
    }

    m_dbg.verbose(CALL_INFO,1,0,"data coming from src %d\n", src );
    
    AAA* aaa = new AAA; 
    aaa->srcNodeId = src;
    aaa->protoType = -1;
    aaa->callback  = new IO_Functor(this, &Hades::ioDone, aaa);
    aaa->ioType = AAA::RecvWireHdrDone;

    std::vector<IO::IoVec> vec; 

    vec.resize(1);
    vec[0].ptr = &aaa->protoType;
    vec[0].len = sizeof(aaa->protoType);

    m_io->recvv( aaa->srcNodeId, vec, aaa->callback );
}

IO::Entry* Hades::recvWireHdrDone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);

    m_dbg.verbose(CALL_INFO,1,0,"type=%d\n", aaa->protoType );

    aaa->ioType = AAA::RecvIODone;
    aaa->request = m_protocolM[aaa->protoType]->getRecvReq( aaa->srcNodeId );
    assert( aaa->request );
    
    m_io->recvv( aaa->srcNodeId, aaa->request->ioVec, aaa->callback );
   
    return NULL;
}

IO::Entry* Hades::recvIODone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);

    m_dbg.verbose(CALL_INFO,1,0,"type=%d\n", aaa->protoType );

    aaa->request = m_protocolM[ aaa->protoType ]->recvIODone( aaa->request );

    if ( aaa->request ) {
        if ( aaa->request->delay ) {
            m_delay = aaa;
            m_enterLink->send(aaa->request->delay, NULL );
            m_dbg.verbose(CALL_INFO,1,0,"schedule delay %d\n",
                                aaa->request->delay);
        } else {
            m_dbg.verbose(CALL_INFO,1,0,"post recv\n");
            m_io->recvv( aaa->srcNodeId, aaa->request->ioVec, aaa->callback );
        }
        e = NULL;
    
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"receive stream finished\n");
    }
    return e;
}

IO::Entry* Hades::delayDone(AAA* aaa)
{
    aaa->request = m_protocolM[ aaa->protoType ]->delayDone(aaa->request);
    if ( aaa->request ) {
        m_io->recvv( aaa->srcNodeId, aaa->request->ioVec, aaa->callback );
        return NULL;
    } else {
        return aaa;
    }
}
