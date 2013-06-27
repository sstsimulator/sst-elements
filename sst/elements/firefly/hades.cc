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

#include "hades.h"
#include "ioapi.h"
#include "functionCtx.h"
#include "entry.h"

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%d:%s::%s():%d: "fmt, myNodeId(), realname ? realname : "?????????", \
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

    m_funcReturnDelayLink = owner->configureSelfLink("Hades.X", "1 ns",
        new Event::Handler<Hades>(this,&Hades::handle_event));

    m_matchDelayLink = owner->configureSelfLink("Hades.Y", "1 ns",
        new Event::Handler<Hades>(this,&Hades::handleMatchDelay));

    Params ioParams = params.find_prefix_params("ioParams." );

    std::ostringstream ownerName;
    ownerName << owner;
    ioParams.insert(
        std::pair<std::string,std::string>("owner", ownerName.str()));

    std::string moduleName = params.find_string("ioModule"); 
    m_io = dynamic_cast<IO::Interface*>(owner->loadModule( moduleName,
                        ioParams));
    if ( !m_io ) {
        _abort(TestDriver, "ERROR:  Unable to find Hermes '%s'\n",
                                        moduleName.c_str());
    }

    if ( 0 ) {
        setIOCallback();
    }

    int size = 2;
    Group group( size, myNodeId() );
    group.mapRank( 0, 0 );
    group.mapRank( 1, 1 );
    m_groupMap[0] = group; 
}

void Hades::_componentInit(unsigned int phase )
{
    m_io->_componentInit(phase);
}

void Hades::setIOCallback()
{
    DBGX("\n");
    m_io->setDataReadyFunc ( new IO_Functor2(this,&Hades::dataReady) );
}

void Hades::run()
{
    DBGX("\n");
    switch ( m_state ) {
      case RunFunctionPre:
        m_ctx->runPre();
        m_state = RunRecv;

      case RunRecv:
        if ( runRecv() ) {
            break;
        }
        m_state = RunSend;

      case RunSend:
        if ( runSend() ) {
            break;
        }
        m_state = RunFunctionPost;

      case RunFunctionPost:
        if ( m_ctx->runPost() ) { 
            m_state = RunRecv; 
            setIOCallback();
        } else {
            delete m_ctx;
        }
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
             
        m_io->sendv( rankToNodeId( entry->group, entry->rank), entry->vec, ioEntry->callback ); 

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
    DBGX("%d bytes avail from %d\n",len, src);

    readHdr( src );
    return true;
}


#if 0
bool Hades::runUnexpectedv( )
{
    IncomingEntry* incoming = searchUnexpected( entry );
    if ( ! incoming ) {
        m_postedQ.push_back( entry );
    } else {
    }
    return false;
}
Hades::IncomingEntry* Hades::searchUnexpected( RecvEntry* entry )
{
    return NULL;
}
#endif

void Hades::dataReady( IO::NodeId src )
{
    run();
}

void Hades::readHdr( IO::NodeId src )
{
    DBGX("src=%d\n", src );

    IncomingEntry* incoming = new IncomingEntry; 
    incoming->src = src;
    incoming->callback = new IO_Functor( this, &Hades::ioRecvHdrDone, incoming );

    incoming->vec.resize(1);
    incoming->vec[0].ptr = &incoming->hdr;
    incoming->vec[0].len = sizeof(incoming->hdr);

    m_io->recvv( src, incoming->vec, incoming->callback );
}

IO::Entry* Hades::ioRecvHdrDone( IO::Entry* e )
{
    IncomingEntry* incoming = static_cast<IncomingEntry*>(e);
    DBGX("src=%d count=%d dtype=%d tag=%#x\n",
            incoming->src,
            incoming->hdr.count,
            incoming->hdr.dtype,
            incoming->hdr.tag );

    MatchEvent* event = new MatchEvent; 
    event->incomingEntry = incoming;

    event->incomingEntry->recvEntry = findMatch( incoming );

    m_matchDelayLink->send( 10, event );
    return NULL;
}


IO::Entry* Hades::sendIODone( IO::Entry* e )
{
    DBGX("\n");
    //IOEntry* entry = static_cast<IOEntry*>( e );
    run();
    return e;
}


RecvEntry* Hades::findMatch( IncomingEntry* incoming )
{
    DBGX("unexpected size %d\n", m_postedQ.size() );
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

        DBGX("tag %d %d\n",incoming->hdr.tag,(*iter)->tag);
        if ( (*iter)->tag != Hermes::AnyTag && 
                (*iter)->tag !=  incoming->hdr.tag ) {
            continue;
        } 

        DBGX("from %d want %d\n", incoming->src, (*iter)->rank);
        if ( (*iter)->rank != Hermes::AnySrc &&
                    incoming->src != (*iter)->rank ) {
            continue;
        } 

        RecvEntry* entry = *iter;
        m_postedQ.erase(iter);
        return entry;
    }
    
    return NULL;
}

void Hades::handleMatchDelay( Event *e )
{
    DBGX("\n");
    MatchEvent* event = static_cast<MatchEvent*>(e);
    IncomingEntry* incoming = event->incomingEntry;

    Hdr& hdr = event->incomingEntry->hdr;
   
    incoming->vec.resize(1);
    incoming->vec[0].len = hdr.count * sizeofDataType(hdr.dtype); 

    DBGX("%d \n",incoming->vec[0].len);
    if ( ! event->incomingEntry->recvEntry ) {
        incoming->buffer.resize( incoming->vec[0].len );
        incoming->vec[0].ptr = &incoming->buffer;
    } else {
        incoming->vec[0].ptr = event->incomingEntry->recvEntry->buf;
    }
    
    delete incoming->callback;
    incoming->callback = new IO_Functor( 
                            this, &Hades::ioRecvBodyDone, incoming );
    
    m_io->recvv( incoming->src, incoming->vec, incoming->callback );
}

IO::Entry* Hades::ioRecvBodyDone( IO::Entry* e )
{
    DBGX("\n");
    IncomingEntry* incoming = static_cast<IncomingEntry*>(e);
    RecvEntry* recvEntry = incoming->recvEntry;

    // if the IncomingEntry has a recvEntry it means there was a match
    if (  recvEntry )  {
        // if the RecvEntry does not have a req it means this was a blocking
        // recv and calling the retFunc will let the recv return 
        if ( ! recvEntry->req ) {
            DBGX("free blocking recv\n");
            assert( recvEntry->retFunc );
            //sendReturn( Size, recvEntry->retFunc );
        } else {
            DBGX("update request for nonblockig recv\n");
            // update the request info for functions like wait()
            recvEntry->req->tag = incoming->hdr.tag;  
            recvEntry->req->src = incoming->src;  
        }

        // we are done with the recvEntry
        // XXXXXXXXXXXXXXXXXXX make sure it's not in the recv Q
        delete recvEntry; 

        // we are done with the IncomingEntry return it and it will be deleted
    }  else {
        DBGX("added to unexpected Q\n");
        // no match push on the unexpected Q
        // XXXXXXXXXXXXXXXX  where do limit the size of the unexpected Q
        m_unexpectedQ.push_back( incoming );
        e = NULL;
    }
    run();
    
    return e;
}

void Hades::handle_event( Event *e )
{
    RetFuncEvent* event = static_cast<RetFuncEvent*>(e);
    DBGX("call return function  retval %d\n", event->m_retval);
    (*event->m_retFunc)(event->m_retval);
}

void Hades::postSendEntry( Hermes::Addr buf, uint32_t count,
    Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
    Hermes::Communicator group, Hermes::MessageRequest* req,
    Hermes::Functor* retFunc )
{
    SendEntry* entry = new SendEntry;

    DBGX("buf=%p count=%d dtype=%d dest=%d tag=%#x group=%d\n",
                        buf,count,dtype,dest,tag,group);

    entry->buf      = buf; 
    entry->count    = count;
    entry->dtype    = dtype;
    entry->group    = group;
    entry->rank     = dest;
    entry->tag      = tag;
    entry->req      = req;
    entry->retFunc  = retFunc;

    entry->hdr.tag = tag;
    entry->hdr.count = count;
    entry->hdr.dtype = dtype;

    entry->vec.resize(2);
    entry->vec[0].ptr = &entry->hdr;
    entry->vec[0].len = sizeof(entry->hdr);
    entry->vec[1].ptr = buf;
    entry->vec[1].len = count * sizeofDataType( dtype );
    m_sendQ.push_back( entry );
}

bool Hades::canPostSend()
{
    return ( m_sendQ.size() < MaxNumSends );
}
bool Hades::canPostRecv()
{
    return ( m_postedQ.size() < MaxNumPostedRecvs );
}

void Hades::postRecvEntry(Hermes::Addr buf, uint32_t count,
    Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
    Hermes::Communicator group, Hermes::MessageResponse* resp,
    Hermes::MessageRequest* req, Hermes::Functor* retFunc )
{
    RecvEntry* entry = new RecvEntry;
    DBGX("buf=%p count=%d dtype=%d dest=%d tag=%#x group=%d\n",
                    buf,count,dtype,source,tag,group);

    entry->buf      = buf; 
    entry->count    = count;
    entry->dtype    = dtype;
    entry->group    = group;
    entry->rank     = source;
    entry->tag      = tag;
    entry->req      = req;
    entry->resp     = resp;
    entry->retFunc  = retFunc;
    m_postedQ.push_back( entry );
}
    

