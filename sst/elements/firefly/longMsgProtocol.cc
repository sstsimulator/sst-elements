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
#include <sst/core/link.h>
#include <sst/core/params.h>

#include <sstream>
#include <string.h>
#include "longMsgProtocol.h"
#include "entry.h"
#include "info.h"

using namespace SST::Firefly;
using namespace SST;

class X : public SST::Event {
  public:
    enum Type { Recv, Send, Wait };
    X( RecvEntry* entry ) : 
        Event(), m_entry( entry ), m_type( Recv ) {} 
    X( SendEntry* entry ) : 
        Event(), m_entry( entry ), m_type( Send ) {} 
    X( Hermes::MessageRequestBase* entry ) : 
        Event(), m_entry( entry ), m_type( Wait ) {} 
    
    Type type() { return m_type; }
    Hermes::MessageRequestBase* entry() { return m_entry; } 
    
  private:
    Hermes::MessageRequestBase* m_entry;    
    Type m_type;
};

LongMsgProtocol::LongMsgProtocol( Component* owner, Params& params ) :
    API( owner, params ),
    m_owner( owner ),
    m_longMsgRegion( 2 )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc =
            (Output::output_location_t) params.find_integer("debug", 0);

    m_my_dbg.init("@t:LongMsgProtocol::@p():@l ", verboseLevel, 0, loc );

    std::stringstream ss;
    ss << this;

    m_my_selfLink = m_owner->configureSelfLink( 
                "LongMsgSelfLink." + ss.str(), "1 ps",
      new Event::Handler<LongMsgProtocol>(this,&LongMsgProtocol::selfHandler));
}

void LongMsgProtocol::setup()
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:LongMsgProtocol::@p():@l ",info()->nodeId(),
                                                info()->worldRank());
    m_my_dbg.setPrefix(buffer);

    API::setup();
    m_regionEventQ = createEventQueue();
}

void LongMsgProtocol::setRetLink(SST::Link* link)
{
    m_my_retLink = link;

    std::stringstream ss;
    ss << this;

    link = m_owner->configureSelfLink( "LongMsgOutLink." + ss.str(), "1 ps",
        new Event::Handler<LongMsgProtocol>(this,&LongMsgProtocol::retHandler));

    API::setRetLink(link);
}

void LongMsgProtocol::selfHandler( SST::Event* e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);
    if ( (*event->callback)() ) {
        delete event->callback;
    }
    delete e;
}


void LongMsgProtocol::retHandler( SST::Event* e )
{
    m_my_retLink->send( 0, e );
}

void LongMsgProtocol::returnToFunction()
{
    m_my_retLink->send(0,NULL);
}

void LongMsgProtocol::schedDelay( int delay, 
                            CtrlMsg::FunctorBase_0<bool>* callback )
{
    m_my_selfLink->send( delay, new SelfEvent( callback ) );
}

void LongMsgProtocol::block( Hermes::MessageRequest req[], 
            Hermes::MessageResponse* resp[] )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"req=[0]%p\n", req[0] );

    assert( m_blockedList.empty() );
    m_blockedList.resize(1);
    m_blockedList[0].req   = req[0]; 
    if ( resp ) {
        m_blockedList[0].resp  = resp[0]; 
    } else {
        m_blockedList[0].resp  = NULL;
    }
    m_blockedList[0].index = NULL; 

    postRecvAny( );
}

void LongMsgProtocol::block( int count, Hermes::MessageRequest req[], 
            Hermes::MessageResponse* resp[],  int* index )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"count=%d any=%s\n",count, 
                    index ? "Any" : "All" );

    assert( m_blockedList.empty() );

    m_blockedList.resize( count );

    for ( int i = 0; i < count; i++ ) {

        m_blockedList[i].req = req[i];
        m_blockedList[i].index = index;

        if ( index ) {
            m_blockedList[i].resp = resp[0];
        } else {
            m_blockedList[i].resp = resp[i];
        }
    }

    postRecvAny( );
}

bool LongMsgProtocol::unblock( Hermes::MessageRequest req )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"req=%p numBlocked=%lu\n",req,
                                m_blockedList.size());
    for ( unsigned int i = 0; i < m_blockedList.size(); i++ ) {

        if ( m_blockedList[i].req == req ) {

            RecvEntry* recvEntry = dynamic_cast<RecvEntry*>( req );
            if ( recvEntry ) {
                m_my_dbg.verbose(CALL_INFO,1,0,"unblock RecvEntry\n");
                *m_blockedList[i].resp = *recvEntry->resp;
            }

            // waitany uses index 
            if ( m_blockedList[i].index ) {
                *m_blockedList[i].index = i;
                m_blockedList.clear( );
            } else {
                m_blockedList.erase( m_blockedList.begin() + i );
            }
            break;
        }
    }

    return m_blockedList.empty();
}

void LongMsgProtocol::finishRecvCBE( RecvEntry& recvEntry, MsgHdr& hdr )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"finalize recvEntry data\n");
    recvEntry.setDone();
    recvEntry.resp->status = true;
    recvEntry.resp->src    = hdr.srcRank; 
    recvEntry.resp->tag    = hdr.tag; 
    recvEntry.resp->count  = hdr.count; 
    recvEntry.resp->dtype  = hdr.dtype;; 
}

void LongMsgProtocol::finishSendCBE( SendEntry& sendEntry )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"finalize sendEntry data\n");
    sendEntry.setDone();
}

void LongMsgProtocol::processBlocked( Hermes::MessageRequest req )
{
    if ( unblock( req ) ) {
        m_my_dbg.verbose(CALL_INFO,1,0,"unblocking\n");
        returnToFunction();
    } else {
        m_my_dbg.verbose(CALL_INFO,1,0,"nothing unblocked\n");
        postRecvAny();
    }
}

void LongMsgProtocol::waitAny( int count, Hermes::MessageRequest req[],
                        int *index, Hermes::MessageResponse* resp  )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"count=%d\n",count);
    for ( int i = 0; i < count; i++ ) {
        if ( req[i]->isDone() ) {
            m_my_dbg.verbose(CALL_INFO,1,0,"wait() done\n");
            *index = i;

            if ( resp ) {
                finishRequest( req[i], resp );
            }
            returnToFunction();
            return;
        }   
    }

    block( count, req, &resp, index );
}

void LongMsgProtocol::waitAll( int count, Hermes::MessageRequest _req[],
                                    Hermes::MessageResponse* _resp[]  )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"count=%d\n",count);

    std::vector<Hermes::MessageRequest> req;
    std::vector<Hermes::MessageResponse*> resp;

    for ( int i = 0; i < count; i++ ) {
        if ( ! _req[i]->isDone() ) {
            m_my_dbg.verbose(CALL_INFO,1,0,"%d not done\n",i);
            req.push_back( _req[i] ); 
            resp.push_back( _resp[i] ); 
        } else {

            m_my_dbg.verbose(CALL_INFO,1,0,"%d done\n",i);

            if ( _resp[i] ) {
                finishRequest( _req[i], _resp[i] );
            }
        }
    }

    if ( req.empty() ) {
        returnToFunction();
    } else {
        block( req.size(), &req[0], &resp[0] );
    }
}

void LongMsgProtocol::wait( Hermes::MessageRequest req,
                            Hermes::MessageResponse* resp ) 
{
    m_my_dbg.verbose(CALL_INFO,1,0,"\n");

    if ( req->isDone() ) {
        m_my_dbg.verbose(CALL_INFO,1,0,"wait() done\n");

        if ( resp ) {
            finishRequest( req, resp );
        }

        returnToFunction();

    } else {

        block( &req, &resp );
    }
}

void LongMsgProtocol::finishRequest( Hermes::MessageRequest req,
                                Hermes::MessageResponse* resp)
{
    RecvEntry* recvEntry = dynamic_cast<RecvEntry*>(req);
    if ( recvEntry ) {
        *resp = *recvEntry->resp;
        if ( recvEntry->req ) {
            delete recvEntry->resp;
            delete recvEntry;
        }
    } else {
        SendEntry* sendEntry = dynamic_cast<SendEntry*>(req);
        if ( sendEntry->req ) {
            delete sendEntry;
        }
    }
}

//
// postSendEntry
//
// checked
void LongMsgProtocol::postSendEntry( SendEntry* entry )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"%s src=%d tag=%#x nbytes=%d\n", 
            entry->req ? "Isend":"Send", entry->dest, entry->tag,
            entry->count * info()->sizeofDataType( entry->dtype )
         );

    SendCallbackEntry* cbe = new SendCallbackEntry;

    cbe->sendEntry = entry;

    SCBE_Functor* callback =
        new SCBE_Functor( this, &LongMsgProtocol::postSendEntry_CB, cbe ); 

    cbe->hdr.count = entry->count;
    cbe->hdr.dtype = entry->dtype;
    cbe->hdr.tag = entry->tag; 
    cbe->hdr.group = entry->group;
    cbe->hdr.srcRank = info()->getGroup(entry->group)->getMyRank();

    size_t len = entry->count * info()->sizeofDataType( entry->dtype );

    Nic::NodeId destNode =
            info()->rankToNodeId( cbe->sendEntry->group, cbe->sendEntry->dest );
    int   tag;
    if ( len <= ShortMsgLength ) {
        cbe->vec.resize(2);
        cbe->vec[1].len = len; 
        cbe->vec[1].ptr = entry->buf;
        tag = ShortMsgTag;
    } else {
        cbe->vec.resize(1);
        cbe->region = genLongRegion();
        tag = LongMsgTag | cbe->region;
        registerRegion( cbe->region, destNode, entry->buf, len, 
                                                        m_regionEventQ );
        m_regionM[cbe->region] = cbe;
    }

    m_my_dbg.verbose(CALL_INFO,1,0,"region=%#x\n",cbe->region);

    cbe->vec[0].ptr = &cbe->hdr;
    cbe->vec[0].len = sizeof(cbe->hdr);

    sendv( cbe->vec, destNode, tag, callback );
}

//
// postSendEntry_CB
//

// checked
bool LongMsgProtocol::postSendEntry_CB( SendCallbackEntry* scbe )
{
    // long message
    if ( scbe->hdr.count && scbe->vec.size() == 1 ) {

        m_my_dbg.verbose(CALL_INFO,1,0,"long\n");

        if ( NULL == scbe->sendEntry->req ) {
            Hermes::MessageRequest req = scbe->sendEntry;
            block( &req, NULL ); 
        } else  {
            returnToFunction();
        }

    } else {
        m_my_dbg.verbose(CALL_INFO,1,0,"short Msg end\n");

        finishSendCBE( *scbe->sendEntry );

        m_my_dbg.verbose(CALL_INFO,1,0,"pass control to function\n");
        returnToFunction();

        delete scbe;
    }

    // delete callback functor
    return true; 
}

// checked
bool LongMsgProtocol::processRegionEvents()
{
    if( ! m_regionEventQ->empty() ) {
        CtrlMsg::region_t region = m_regionEventQ->front();
        m_regionEventQ->pop_front();

        m_my_dbg.verbose(CALL_INFO,1,0,"region %#x \n", region );
        SendCallbackEntry* scbe = m_regionM[region];
        finishSendCBE( *scbe->sendEntry );

        unregisterRegion( region );
        m_regionM.erase( region );

        schedDelay( 0, new SCBE_Functor( 
              this, &LongMsgProtocol::processSendEntryFini_CB, scbe ) ); 

        return true;
    }
    return false;
}

//checked
bool LongMsgProtocol::processSendEntryFini_CB( SendCallbackEntry* cbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"\n");
    // state has changed, something may have become unblocked
    processBlocked( cbe->sendEntry ) ;
    delete cbe;
    return true;
}

// checked
void LongMsgProtocol::postRecvAny(  )
{
    RecvCallbackEntry* rcbe = new RecvCallbackEntry;

    m_my_dbg.verbose(CALL_INFO,1,0,"\n");

    if ( processRegionEvents() ) {
        return;
    }

    rcbe->recvEntry = NULL;

    RCBE_Functor* callback =
        new RCBE_Functor( this, &LongMsgProtocol::postRecvAny_irecv_CB, rcbe );

    rcbe->vec.resize(2);

    rcbe->buf.resize( ShortMsgLength );

    rcbe->vec[0].ptr = &rcbe->hdr;
    rcbe->vec[0].len = sizeof(rcbe->hdr);
    rcbe->vec[1].len = rcbe->buf.size();
    rcbe->vec[1].ptr = &rcbe->buf[0];

    irecvv( rcbe->vec, CtrlMsg::AnyNid, CtrlMsg::AnyTag, &rcbe->commReq,
                            callback );

    setUsrPtr( &rcbe->commReq, rcbe );
}

// checked  
bool LongMsgProtocol::postRecvAny_irecv_CB( RecvCallbackEntry* rcbe  )
{
    XXX_Functor* callback =
        new XXX_Functor( this, &LongMsgProtocol::waitAny_CB );

    m_my_dbg.verbose(CALL_INFO,1,0,"\n");
    API::wait( &rcbe->commReq, callback );

    // delete callback functor
    return true;
}

// checked
bool LongMsgProtocol::waitAny_CB( CtrlMsg::CommReq* req )
{
    int numMatch = 0;

    if ( ! req ) {
        if ( processRegionEvents() ) {
            processRegionEvents();
            return true;
        } else {
            assert(0);
        }
    }

    RecvCallbackEntry *cbe = static_cast<RecvCallbackEntry*>( getUsrPtr(req) );
    assert( cbe );

    m_my_dbg.verbose(CALL_INFO,1,0,"\n");

    // check for posted receives
    std::deque<RecvEntry*>::iterator iter = m_postedQ.begin();
    for ( ; iter != m_postedQ.end(); ++iter ) {
        ++numMatch;
        if ( checkMatch( cbe->hdr, *(*iter) ) ) {
            m_my_dbg.verbose(CALL_INFO,1,0,"found match\n");
            cbe->recvEntry = *iter;
            m_postedQ.erase( iter );
            break;
        } 
    }

    schedDelay( calcMatchDelay( numMatch), new RCBE_Functor( 
              this, &LongMsgProtocol::waitAnyDelay_CB, cbe ) ); 
    return true;
}

// checked
bool LongMsgProtocol::waitAnyDelay_CB( RecvCallbackEntry* cbe )
{
    CtrlMsg::Status status;
    getStatus( &cbe->commReq, &status );

    m_my_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x\n",
                            status.nid, status.tag);

    if ( cbe->recvEntry ) {
        if ( ( status.tag & TagMask ) == ShortMsgTag)  {
            processShortMsg( cbe );
        } else {
            processLongMsg( cbe );
        }
    } else {
        m_my_dbg.verbose(CALL_INFO,1,0,"save unexpected\n");
        m_unexpectedQ.push_back( cbe );
        postRecvAny();
    }
    return true;
}

// checked
void LongMsgProtocol::processShortMsg( RecvCallbackEntry* cbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"received a short message\n");

    size_t len = cbe->recvEntry->count * 
                    info()->sizeofDataType( cbe->recvEntry->dtype );

    memcpy( cbe->recvEntry->buf, &cbe->buf[0], len );

    schedDelay( calcCopyDelay(len), new RCBE_Functor( 
              this, &LongMsgProtocol::processShortMsg_CB, cbe ) ); 
}

// checked
bool LongMsgProtocol::processShortMsg_CB( RecvCallbackEntry* cbe )
{
    finishRecvCBE( *cbe->recvEntry, cbe->hdr );

    // got here from blocking postRecvEntry
    if ( m_blockedList.empty() ) {
        returnToFunction();
    } else {
        // state has changed, something may have become unblocked
        processBlocked( cbe->recvEntry );
    }
    delete cbe;
    return true;
}

// checked
void LongMsgProtocol::processLongMsg( RecvCallbackEntry* cbe )
{
    // reuse the RecvcCallbackEntry for receiving the body
    CtrlMsg::Status status;
    getStatus( &cbe->commReq, &status );

    CtrlMsg::region_t region = status.tag & ~TagMask;
    CtrlMsg::nid_t nid = status.nid; 
    size_t length = cbe->recvEntry->count *
                info()->sizeofDataType( cbe->recvEntry->dtype );

    RCBE_Functor* callback = new RCBE_Functor( 
              this, &LongMsgProtocol::processLongMsg_CB, cbe ); 

    m_my_dbg.verbose(CALL_INFO,1,0,"src=%d region=%#x\n", nid, region );

    read( nid, region, cbe->recvEntry->buf, length, callback );
}

// checked
bool LongMsgProtocol::processLongMsg_CB( RecvCallbackEntry* cbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"\n");
    finishRecvCBE( *cbe->recvEntry, cbe->hdr );

    // state has changed, something may have become unblocked
    processBlocked( cbe->recvEntry );
    m_my_dbg.verbose(CALL_INFO,1,0,"\n");
    return true;
}

//
// postRecvEntry()
//

// checked
void LongMsgProtocol::postRecvEntry( RecvEntry* entry )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"%s src=%d tag=%#x nbytes=%d\n", 
            entry->req ? "Irecv":"Recv", entry->src, entry->tag,
            entry->count * info()->sizeofDataType( entry->dtype )
         );

    int numMatch = 0;
    RecvCallbackEntry* cbe = NULL;
    // We should block until there is room but for now punt 
    assert( m_postedQ.size() < MaxNumPostedRecvs );

    std::deque<RecvCallbackEntry*>::iterator iter = m_unexpectedQ.begin();

    m_my_dbg.verbose(CALL_INFO,1,0,"unexpected.size() %lu\n",
                                            m_unexpectedQ.size());
    // check for posted receives
    
    for ( ; iter != m_unexpectedQ.end(); ++iter ) {
        cbe = *iter;
        ++numMatch;
        if ( checkMatch( cbe->hdr, *entry) ) {
            cbe->recvEntry = entry;
            m_my_dbg.verbose(CALL_INFO,1,0,"found unexpected match\n");
            m_unexpectedQ.erase( iter );
            break;
        } 
        cbe = NULL; 
    }

    if ( cbe ) {
        schedDelay( calcMatchDelay( numMatch), new RCBE_Functor( 
              this, &LongMsgProtocol::postRecvEntry_CB, cbe ) ); 
    } else {
        schedDelay( calcMatchDelay( numMatch), new RE_Functor( 
              this, &LongMsgProtocol::postRecvEntry_CB, entry ) ); 
    }
}

// checked
bool LongMsgProtocol::postRecvEntry_CB( RecvCallbackEntry* cbe )
{
    assert(0);
    CtrlMsg::Status status;
    getStatus( &cbe->commReq, &status );
    if ( ( status.tag & TagMask ) == ShortMsgTag)  {
        processShortMsg( cbe );
    } else {
        processLongMsg( cbe );
    }
    return true;
}

//checked
bool LongMsgProtocol::postRecvEntry_CB( RecvEntry* entry )
{
    m_postedQ.push_back( entry );
        
    // Non-blocking
    if ( entry->req ) {
        m_my_dbg.verbose(CALL_INFO,1,0,"pass control to function\n");
        returnToFunction();
    } else {
        Hermes::MessageRequest req = entry;
        block( &req, &entry->resp );
    }
    return true;
}

//
// checkMatch()
//

// checked
bool LongMsgProtocol::checkMatch( MsgHdr& hdr, RecvEntry& entry )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"dtype %d %d\n", entry.dtype, hdr.dtype);
    if ( entry.dtype != hdr.dtype ) {
        return false;
    }

    m_my_dbg.verbose(CALL_INFO,1,0,"count %d %d\n", entry.count, hdr.count);
    if ( entry.count != hdr.count ) {
        return false;
    }

    m_my_dbg.verbose(CALL_INFO,1,0,"tag want %#x, incoming %#x\n",
                                    entry.tag, hdr.tag);
    if ( entry.tag != Hermes::AnyTag &&
                entry.tag != hdr.tag ) {
        return false;
    }

    m_my_dbg.verbose(CALL_INFO,1,0,"group want %d, incoming %d\n",
                                     entry.group, hdr.group);
    if ( entry.group != hdr.group ) {
        return false;
    }

    m_my_dbg.verbose(CALL_INFO,1,0,"rank want %d, incoming %d\n",
                                    entry.src, hdr.srcRank);
    if ( entry.src != Hermes::AnySrc &&
                    entry.src != hdr.srcRank ) {
        return false;
    }

    return true;
}
