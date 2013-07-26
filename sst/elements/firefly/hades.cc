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
#include "sst/core/serialization/element.h"

#include "sst/core/component.h"

#include <stdlib.h>

#include <iostream>
#include <fstream>

#include "hades.h"
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
    m_progState( RunRecv ),
    m_io( NULL )
{
    m_loc = (Output::output_location_t)params.find_integer("debug", 0);
    m_verboseLevel = params.find_integer("verboseLevel",0);
    m_dbg.init("@t:Hades::@p():@l ", m_verboseLevel, 0, m_loc );

    m_owner = (SST::Component*) params.find_integer( "owner" );

    m_selfLink = m_owner->configureSelfLink("Hades", "1 ps",
        new Event::Handler<Hades>(this,&Hades::handleSelfLink));

    m_toProgressLink = m_owner->configureSelfLink("ToProgress", "1 ps",
        new Event::Handler<Hades>(this,&Hades::handleProgress));

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
    assert( m_nodeInfo );

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
        m_dbg.fatal(CALL_INFO,0,1,0,"Unable to open nid list '%s'\n",
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

    m_protocolM[0] = new DataMovement( m_verboseLevel, m_loc, &m_info);
    m_protocolM[1] = new CtrlMsg( m_verboseLevel, m_loc, &m_info);

    m_sendIter = m_protocolM.begin();

    m_functionSM = new FunctionSM( m_verboseLevel, m_loc, m_owner, m_info,
                    m_protocolM[0], m_io, m_protocolM[1] );
    m_functionSM->setProgressLink( m_toProgressLink );
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
        m_dbg.verbose(CALL_INFO,1,0,"group rank %d, %d %d\n",
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
}

Group* Hades::initAdjacentMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );
    assert( group );

    m_dbg.verbose(CALL_INFO,1,0,"numRanks=%d numCores=%d\n",
                                                numRanks, numCores);
    for ( int node = 0; node < numRanks/numCores; node++ ) {
        int nid;
        std::string line;
        getline( nidFile, line );
        sscanf( line.c_str(), "%d", &nid ); 

        for ( int core = 0; core < numCores; core++ ) {
            group->set( node * numCores + core, nid, core );
            m_dbg.verbose(CALL_INFO,1,0,"rank %d is on nid %d\n",
                                            node * numCores + core , nid);
        }
    }
    return group;
}

Group* Hades::initRoundRobinMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );
    assert( group );
    m_dbg.verbose(CALL_INFO,1,0,"numRanks=%d numCores=%d\n",
                            numRanks, numCores);

    return group;
}

void Hades::_componentInit(unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_io->_componentInit(phase);
}

void Hades::clearIOCallback()
{
    m_io->setDataReadyFunc( NULL );
}

void Hades::setIOCallback()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_io->setDataReadyFunc( new IO_Functor2(this,&Hades::dataReady) );
}

void Hades::handleProgress( Event* e )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_progState = RunRecv;
    handleSelfLink( NULL ); 
}

bool Hades::runSend( )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

here:
    ProtocolAPI::Request* req = m_sendIter->second->getSendReq();

    if ( req ) {
        // have something to send, configure and send the protocol type
        AAA* aaa = new AAA;

        aaa->type    = m_sendIter->first;
        aaa->request = req;

        std::vector<IO::IoVec> ioVec; 
        ioVec.resize(1);
        ioVec[0].ptr = &aaa->type;
        ioVec[0].len = sizeof(aaa->type); 

        aaa->callback = new IO_Functor( this, &Hades::sendWireHdrDone, aaa );

        m_io->sendv( req->nodeId, ioVec, aaa->callback );
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

IO::Entry* Hades::sendWireHdrDone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);
    m_dbg.verbose(CALL_INFO,1,0,"return\n");

    delete aaa->callback;
    aaa->callback = new IO_Functor( this, &Hades::sendIODone, aaa );
    m_io->sendv( aaa->request->nodeId, aaa->request->ioVec, aaa->callback );
    
    return NULL;
}

IO::Entry* Hades::sendIODone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);
    m_dbg.verbose(CALL_INFO,1,0,"return\n");

    m_protocolM[ aaa->type ]->sendIODone( aaa->request ); 

    m_selfLink->send(0, NULL );

    return e;
}
     
bool Hades::runRecv( )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    IO::NodeId src = IO::AnyId;
    size_t len = m_io->peek( src ); 

    if ( src == IO::AnyId  ) {
        return false;
    } 
    m_dbg.verbose(CALL_INFO,1,0,"%lu bytes avail from %d\n",len, src);

    readHdr( src );
    return true;
}

void Hades::dataReady( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"call run()\n");
    handleSelfLink( NULL );
}

void Hades::readHdr( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d\n", src );
    
    AAA* aaa = new AAA; 
    aaa->srcNodeId = src;
    aaa->type      = -1;
    aaa->callback  = new IO_Functor(this, &Hades::recvWireHdrDone, aaa);

    std::vector<IO::IoVec> vec; 

    vec.resize(1);
    vec[0].ptr = &aaa->type;
    vec[0].len = sizeof(aaa->type);

    m_io->recvv( aaa->srcNodeId, vec, aaa->callback );
}

IO::Entry* Hades::recvWireHdrDone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);

    m_dbg.verbose(CALL_INFO,1,0,"type=%d\n", aaa->type );

    delete aaa->callback;
    aaa->callback = new IO_Functor(this, &Hades::recvIODone, aaa);
    aaa->request = m_protocolM[aaa->type]->getRecvReq( aaa->srcNodeId );
    assert( aaa->request );
    
    m_io->recvv( aaa->srcNodeId, aaa->request->ioVec, aaa->callback );
   
    return NULL;
}

IO::Entry* Hades::recvIODone( IO::Entry* e )
{
    AAA* aaa = static_cast<AAA*>(e);

    m_dbg.verbose(CALL_INFO,1,0,"type=%d\n", aaa->type );

    aaa->request = m_protocolM[ aaa->type ]->recvIODone( aaa->request );

    if ( aaa->request ) {
        if ( aaa->request->delay ) {
            SelfEvent* ee = new SelfEvent;
            ee->aaa = aaa;
            m_selfLink->send(aaa->request->delay, ee );
        } else {
            m_io->recvv( aaa->srcNodeId, aaa->request->ioVec, aaa->callback );
        }
        e = NULL;
    
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"receive stream finished\n");
        m_selfLink->send(0, NULL );
    }
    m_dbg.verbose(CALL_INFO,1,0,"return\n");
    return e;
}
void Hades::delayDone(AAA* aaa)
{
    aaa->request = m_protocolM[ aaa->type ]->delayDone(aaa->request);
    if ( aaa->request ) {
        m_io->recvv( aaa->srcNodeId, aaa->request->ioVec, aaa->callback );
    } else {
        delete aaa;
        m_selfLink->send(0, NULL );
    }
}

void Hades::handleSelfLink( Event* e )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = static_cast<SelfEvent*>(e);
    if ( event ) {
        delayDone( event->aaa );
        delete event;
        return;
    }

    switch ( m_progState ) {
      case RunRecv:
        if ( runRecv() ) {
            break;
        }
        m_progState = RunSend;
      case RunSend:
        if ( runSend() ) {
            break;
        }
        m_progState = Return;
      case Return: 
        m_dbg.verbose(CALL_INFO,1,0,"\n");
        m_functionSM->sendProgressEvent( NULL );
    }
}
