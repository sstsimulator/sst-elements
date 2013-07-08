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
#include "functionCtx.h"
#include "entry.h"
#include "nodeInfo.h"

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%d:%d:%s::%s():%d: "fmt, myNodeId(), _myWorldRank(),realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

using namespace SST::Firefly;
using namespace SST;

Hades::Hades( Params& params ) :
    MessageInterface(),
    m_io( NULL )
{
    m_funcLat.resize(FunctionCtx::NumFunctions);
    m_funcLat[FunctionCtx::Init] = 10;
    m_funcLat[FunctionCtx::Fini] = 10;
    m_funcLat[FunctionCtx::Rank] = 1;
    m_funcLat[FunctionCtx::Size] = 1;
    m_funcLat[FunctionCtx::Barrier] = 100;
    m_funcLat[FunctionCtx::Irecv] = 1;
    m_funcLat[FunctionCtx::Isend] = 1;
    m_funcLat[FunctionCtx::Send] = 1;
    m_funcLat[FunctionCtx::Recv] = 1;
    m_funcLat[FunctionCtx::Wait] = 1;

    SST::Component*     owner;

    owner = (SST::Component*) params.find_integer( "owner" );

    m_selfLink = owner->configureSelfLink("Hades", "1 ps",
        new Event::Handler<Hades>(this,&Hades::handleSelfLink));

    Params ioParams = params.find_prefix_params("ioParams." );

    std::ostringstream ownerName;
    ownerName << owner;
    ioParams.insert(
        std::pair<std::string,std::string>("owner", ownerName.str()));

    std::string moduleName = params.find_string("ioModule"); 
    m_io = dynamic_cast<IO::Interface*>(owner->loadModule( moduleName,
                        ioParams));
    if ( !m_io ) {
        _abort(Hades, "ERROR:  Unable to find Hermes '%s'\n",
                                        moduleName.c_str());
    }

    Params nodeParams = params.find_prefix_params("nodeParams.");
    
    m_nodeInfo = new NodeInfo( nodeParams );
    assert( m_nodeInfo );

    DBGX("numCores %d, coreNum %d\n", m_nodeInfo->numCores(),
                                            m_nodeInfo->coreNum());

    int numRanks = params.find_integer("numRanks");
    if ( numRanks <= 0 ) {
        _abort(Hades, "ERROR:  How many global ranks?\n");
    }
    DBGX("numRanks %d\n", numRanks);

    std::string nidListFileName = params.find_string("nidListFile");

    DBGX("nidListFile `%s`\n",nidListFileName.c_str());

    std::ifstream nidListFile( nidListFileName.c_str());
    if ( ! nidListFile.is_open() ) {
        _abort(Hades, "ERROR:  Unable to open nid list '%s'\n",
                                        nidListFileName.c_str() );
    }

    Group* group;

    std::string policy = params.find_string("policy");
    DBGX("load policy `%s`\n",policy.c_str());

    if ( 0 == policy.compare("adjacent") ) {
        group = initAdjacentMap(numRanks, m_nodeInfo->numCores(), nidListFile);
#if 0
    } else if ( 0 == policy.compare("roundRobin") ) {
        group = initRoundRobinMap(numRanks, m_nodeInfo->numCores(), nidListFile); 
#endif
    } else {
        _abort(Hades, "ERROR: unknown load policy `j%s` ",
                                            policy.c_str() );
    }
    m_groupMap[Hermes::GroupWorld] = group;

    nidListFile.close();
}

Hermes::RankID Hades::myWorldRank() 
{
    int rank = m_groupMap[Hermes::GroupWorld]->getMyRank();
    if ( -1 == rank ) {
        _abort(Hades,"%s() rank not set yet\n",__func__);
    } else {
        return rank;
    }
}


void Hades::_componentSetup()
{
    Group* group = m_groupMap[Hermes::GroupWorld];
    for ( unsigned int i = 0; i < group->size(); i++ ) {
        if ( group->getNodeId( i ) == myNodeId() && 
                group->getCoreId( i ) == m_nodeInfo->coreNum() ) 
        {
            group->setMyRank( i );
            break;
        }
    } 

    DBGX( "myRank %d\n", m_groupMap[Hermes::GroupWorld]->getMyRank() );
}

Group* Hades::initAdjacentMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );
    assert( group );

    DBGX("numRanks=%d numCores=%d\n", numRanks, numCores);
    for ( int node = 0; node < numRanks/numCores; node++ ) {
        int nid;
        std::string line;
        getline( nidFile, line );
        sscanf( line.c_str(), "%d", &nid ); 

        for ( int core = 0; core < numCores; core++ ) {
            group->set( node * numCores + core, nid, core );
            DBGX("rank %d is on nid %d\n", node * numCores + core , nid);
        }
    }
    return group;
}

Group* Hades::initRoundRobinMap( int numRanks, 
            int numCores, std::ifstream& nidFile )
{
    Group* group = new Group( numRanks );
    assert( group );
    DBGX("numRanks=%d numCores=%d\n", numRanks, numCores);

    return group;
}

void Hades::_componentInit(unsigned int phase )
{
    m_io->_componentInit(phase);
}

void Hades::clearIOCallback()
{
    m_io->setDataReadyFunc( NULL );
}

void Hades::setIOCallback()
{
    DBGX("\n");
    m_io->setDataReadyFunc( new IO_Functor2(this,&Hades::dataReady) );
}

void Hades::runProgress( FunctionCtx * ctx )
{
    if ( ctx ) {
        m_state = RunRecv;
        m_ctx = ctx;
    }

    switch ( m_state ) {

      case RunRecv:
        m_state = RunSend;
        if ( runRecv() ) {
            break;
        }

      case RunSend:
        m_state = RunCtx;
        if ( runSend() ) {
            break;
        }

      case RunCtx:
        DBGX("RunCtx\n");
        sendCtxDelay(0,m_ctx); 
        break;
    } 
}

bool Hades::runSend( )
{
    DBGX("\n");
    if ( ! m_sendQ.empty() ) {
        SendEntry* entry = m_sendQ.front(); 

        m_sendQ.pop_front();

        IOEntry* ioEntry = new IOEntry;
        ioEntry->callback = new IO_Functor( this, &Hades::sendIODone, ioEntry );
             
        m_io->sendv( rankToNodeId( entry->group, entry->rank),
                                    entry->vec, ioEntry->callback ); 

        return true;  
    }
    return false;
}

bool Hades::runRecv( )
{
    DBGX("\n");
    IO::NodeId src = IO::AnyId;
    size_t len = m_io->peek( src ); 

    if ( len == 0 ) {
        return false;
    } 
    DBGX("%lu bytes avail from %d\n",len, src);

    readHdr( src );
    return true;
}

UnexpectedMsgEntry* Hades::searchUnexpected( RecvEntry* entry )
{
    DBGX("unexpected size %lu\n", m_unexpectedMsgQ.size() );
    std::deque< MsgEntry* >::iterator iter = m_unexpectedMsgQ.begin();
    for ( ; iter != m_unexpectedMsgQ.end(); ++iter  ) {

        DBGX("dtype %d %d\n",entry->dtype,(*iter)->hdr.dtype);
        if ( entry->dtype != (*iter)->hdr.dtype ) {
            continue;
        } 

        DBGX("count %d %d\n",entry->count,(*iter)->hdr.count);
        if ( entry->count != (*iter)->hdr.count ) { 
            continue;
        } 

        DBGX("tag want %d, incoming %d\n",entry->tag,(*iter)->hdr.tag);
        if ( entry->tag != Hermes::AnyTag && 
                entry->tag != (*iter)->hdr.tag ) {
            continue;
        } 

        DBGX("group want %d, incoming %d\n", entry->group, (*iter)->hdr.group);
        if ( entry->group != (*iter)->hdr.group ) { 
            continue;
        } 

        DBGX("rank want %d, incoming %d\n", entry->rank, (*iter)->hdr.srcRank);
        if ( entry->rank != Hermes::AnySrc &&
                    entry->rank != (*iter)->hdr.srcRank ) {
            continue;
        } 
        m_unexpectedMsgQ.erase( iter );
        return static_cast<UnexpectedMsgEntry*>(*iter);
    }
    
    return false;
}

void Hades::dataReady( IO::NodeId src )
{
    DBGX("call run()\n");
    runProgress(m_ctx);
}

void Hades::readHdr( IO::NodeId src )
{
    DBGX("src=%d\n", src );

    IncomingMsgEntry* incoming = new IncomingMsgEntry; 
    incoming->srcNodeId = src;
    incoming->callback = new IO_Functor(this, &Hades::ioRecvHdrDone, incoming);

    incoming->vec.resize(1);
    incoming->vec[0].ptr = &incoming->hdr;
    incoming->vec[0].len = sizeof(incoming->hdr);

    m_io->recvv( src, incoming->vec, incoming->callback );
}

IO::Entry* Hades::ioRecvHdrDone( IO::Entry* e )
{
    IncomingMsgEntry* incoming = static_cast<IncomingMsgEntry*>(e);
    DBGX("srcNodeId=%d count=%d dtype=%d tag=%#x\n",
            incoming->srcNodeId,
            incoming->hdr.count,
            incoming->hdr.dtype,
            incoming->hdr.tag );

    MatchEvent* event = static_cast<MatchEvent*>(&m_selfEvent);
    event->type = SelfEvent::Match;
    event->incomingEntry = incoming;

    event->incomingEntry->recvEntry = findMatch( incoming );

    m_selfLink->send( 10, event );
    return NULL;
}


IO::Entry* Hades::sendIODone( IO::Entry* e )
{
    DBGX("call run()\n");
    runProgress(NULL);
    return e;
}


RecvEntry* Hades::findMatch( IncomingMsgEntry* incoming )
{
    DBGX("posted size %lu\n", m_postedQ.size() );
    std::deque< RecvEntry* >::iterator iter = m_postedQ.begin(); 
    for ( ; iter != m_postedQ.end(); ++iter  ) {
        DBGX("dtype %d %d\n",incoming->hdr.dtype,(*iter)->dtype);
        if ( incoming->hdr.dtype != (*iter)->dtype ) {
            continue;
        } 

        DBGX("count %d %d\n",incoming->hdr.count,(*iter)->count);
        if ( (*iter)->count !=  incoming->hdr.count ) {
            continue;
        } 

        DBGX("tag incoming=%d %d\n",incoming->hdr.tag,(*iter)->tag);
        if ( (*iter)->tag != Hermes::AnyTag && 
                (*iter)->tag !=  incoming->hdr.tag ) {
            continue;
        } 

        DBGX("group incoming=%d %d\n", incoming->hdr.group, (*iter)->group);
        if ( (*iter)->group !=  incoming->hdr.group ) {
            continue;
        } 

        DBGX("rank incoming=%d %d\n", incoming->hdr.srcRank, (*iter)->rank);
        if ( (*iter)->rank != Hermes::AnySrc &&
                    incoming->hdr.srcRank != (*iter)->rank ) {
            continue;
        } 

        RecvEntry* entry = *iter;
        m_postedQ.erase(iter);
        return entry;
    }
    
    return NULL;
}

void Hades::handleSelfLink( Event* e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);
    switch( event->type )
    {
    case SelfEvent::Return:
        handleCtxReturn(e);
        break;
    case SelfEvent::Match:
        handleMatchDelay(e);
        break;
    case SelfEvent::CtxDelay:
        handleCtxDelay(e);
        break;
    }
}

void Hades::handleCtxDelay( Event *e )
{
    DBGX("call ctx->run()\n");
    CtxDelayEvent& event = *static_cast<CtxDelayEvent*>(e);
    if ( event.ctx->run() ) {
        delete event.ctx;
    }
}

void Hades::handleCtxReturn( Event *e )
{
    RetFuncEvent* event = static_cast<RetFuncEvent*>(e);
    DBGX("call return function  retval %d\n", event->m_retval);
    (*event->m_retFunc)(event->m_retval);
}


void Hades::handleMatchDelay( Event *e )
{
    MatchEvent* event = static_cast<MatchEvent*>(e);
    IncomingMsgEntry* incoming = event->incomingEntry;

    Hdr& hdr = event->incomingEntry->hdr;
   
    if ( hdr.count ) {
        incoming->vec.resize(1);
        incoming->vec[0].len = hdr.count * sizeofDataType(hdr.dtype); 

        DBGX("incoming body %lu\n",incoming->vec[0].len);
        if ( ! event->incomingEntry->recvEntry ) {
            incoming->buffer.resize( incoming->vec[0].len );
            incoming->vec[0].ptr = &incoming->buffer[0];
        } else {
            incoming->vec[0].ptr = event->incomingEntry->recvEntry->buf;
        }
    
        delete incoming->callback;
        incoming->callback = new IO_Functor( 
                            this, &Hades::ioRecvBodyDone, incoming );
    
        m_io->recvv( incoming->srcNodeId, incoming->vec, incoming->callback );
    } else {
        DBGX("no body\n");
        if ( ioRecvBodyDone( incoming ) ) {
            delete incoming;
        } 
    }
}

IO::Entry* Hades::ioRecvBodyDone( IO::Entry* e )
{
    IncomingMsgEntry* incoming = static_cast<IncomingMsgEntry*>(e);
    RecvEntry* recvEntry = incoming->recvEntry;

    // if the IncomingEntry has a recvEntry it means there was a match
    if (  recvEntry )  {

        if ( recvEntry->req ) {
            DBGX("finish up non blocking match\n")
            recvEntry->req->tag = incoming->hdr.tag;  
            recvEntry->req->src = incoming->hdr.srcRank;  
        } else if ( recvEntry->resp ) {
            DBGX("finish up blocking match\n")
            recvEntry->resp->tag = incoming->hdr.tag;  
            recvEntry->resp->src = incoming->hdr.srcRank;  
        } else {
            assert( recvEntry->retFunc );
        }

        // we are done with the recvEntry
        delete recvEntry; 

        // we are done with the IncomingEntry return it and it will be deleted
    }  else {
        DBGX("added to unexpected Q\n");
        // no match push on the unexpected Q
        // XXXXXXXXXXXXXXXX  where do limit the size of the unexpected Q
        m_unexpectedMsgQ.push_back( incoming );
        e = NULL;
    }
    runProgress(NULL);
    
    return e;
}

bool Hades::canPostSend()
{
    return ( m_sendQ.size() < MaxNumSends );
}

void Hades::postSendEntry( SendEntry* entry )
{
    m_sendQ.push_back( entry );
}

bool Hades::canPostRecv()
{
    return ( m_postedQ.size() < MaxNumPostedRecvs );
}

void Hades::postRecvEntry( RecvEntry* entry )
{
    m_postedQ.push_back( entry );
}
