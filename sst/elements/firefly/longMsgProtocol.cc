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
    CtrlMsg( owner, params ),
    m_owner( owner ),
    m_longMsgKey( 10 )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc =
            (Output::output_location_t) params.find_integer("debug", 0);

    m_my_dbg.init("@t:LongMsgProtocol::@p():@l ", verboseLevel, 0, loc );
}

void LongMsgProtocol::setup()
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:LongMsgProtocol::@p():@l ",info()->nodeId(),
                                                info()->worldRank());
    m_my_dbg.setPrefix(buffer);

    CtrlMsg::setup();
}

void LongMsgProtocol::setRetLink(SST::Link* link)
{
    m_my_retLink = link;

    std::stringstream ss;
    ss << this;

    link = m_owner->configureSelfLink( "LongMsgOutLink." + ss.str(), "1 ps",
        new Event::Handler<LongMsgProtocol>(this,&LongMsgProtocol::retHandler));

    CtrlMsg::setRetLink(link);
}

void LongMsgProtocol::retHandler( SST::Event* e )
{
    m_my_retLink->send( 0, e );
}

void LongMsgProtocol::returnToFunction()
{
    m_my_retLink->send(0,NULL);
}

void LongMsgProtocol::block( Hermes::MessageRequest req[], 
            Hermes::MessageResponse* resp[] )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"\n");

    assert( m_blockedList.empty() );
    m_blockedList.resize(1);
    m_blockedList[0].req   = req[0]; 
    m_blockedList[0].resp  = resp[0]; 
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

            m_my_dbg.verbose(CALL_INFO,1,0,"unblock %d\n",i);

            RecvEntry* recvEntry = dynamic_cast<RecvEntry*>( req );
            if ( recvEntry ) {
                *m_blockedList[i].resp = *recvEntry->resp;
            } else {
                SendEntry* sendEntry = dynamic_cast<SendEntry*>( req );
                *m_blockedList[i].resp = *sendEntry->resp;
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

    if ( sendEntry.resp ) {
        sendEntry.resp->status = true;
    }
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

            if ( resp ) {
                finishRequest( req[i], resp );
            }

            returnToFunction();
        
            //RETURN
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
printf("recv %p\n",recvEntry->resp);
            delete recvEntry->resp;
            delete recvEntry;
        }
    } else {
        SendEntry* sendEntry = dynamic_cast<SendEntry*>(req);
        *resp = *sendEntry->resp;
        if ( sendEntry->req ) {
printf("delete send %p\n",sendEntry->resp);
            delete sendEntry->resp;
            delete sendEntry;
        }
    }
}

//
// postSendEntry
//
void LongMsgProtocol::postSendEntry( SendEntry* entry )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"%s\n", entry->req ? "Isend":"Send");

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

    tag_t   tag;
    if ( len <= ShortMsgLength ) {
        cbe->vec.resize(2);
        cbe->vec[1].len = len; 
        cbe->vec[1].ptr = entry->buf;
        tag = ShortMsgTag;
    } else {
        cbe->vec.resize(1);
        cbe->key = genLongMsgKey();
        tag = LongMsgTag | cbe->key;
    }

    cbe->vec[0].ptr = &cbe->hdr;
    cbe->vec[0].len = sizeof(cbe->hdr);

    IO::NodeId destNode =
            info()->rankToNodeId( cbe->sendEntry->group, cbe->sendEntry->dest );

    sendv( cbe->vec, destNode, tag, callback );
}

//
// postSendEntry_CB
//

bool LongMsgProtocol::postSendEntry_CB( SendCallbackEntry* scbe )
{
    // long message
    if ( scbe->hdr.count && scbe->vec.size() == 1 ) {

        m_my_dbg.verbose(CALL_INFO,1,0,"long\n");

        assert( m_longSendM.find( scbe->key ) == m_longSendM.end() );
        // save the Send info for LongMsgRdy msg
        m_longSendM[ scbe->key ] = scbe;

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

void LongMsgProtocol::postRecvAny(  )
{
    RecvCallbackEntry* rcbe = new RecvCallbackEntry;

    m_my_dbg.verbose(CALL_INFO,1,0,"\n");

    rcbe->recvEntry = NULL;

    RCBE_Functor* callback =
        new RCBE_Functor( this, &LongMsgProtocol::postRecvAny_irecv_CB, rcbe );

    rcbe->vec.resize(2);

    rcbe->buf.resize( ShortMsgLength );

    rcbe->vec[0].ptr = &rcbe->hdr;
    rcbe->vec[0].len = sizeof(rcbe->hdr);
    rcbe->vec[1].len = rcbe->buf.size();
    rcbe->vec[1].ptr = &rcbe->buf[0];
    rcbe->commReq.userInfo = rcbe;

    irecvv( rcbe->vec, AnyRank, AnyTag, &rcbe->commReq,
                            &rcbe->ctrlResponse, callback );
}

bool LongMsgProtocol::postRecvAny_irecv_CB( RecvCallbackEntry* rcbe  )
{
    m_commReqs.insert( &rcbe->commReq );  

    XXX_Functor* callback =
        new XXX_Functor( this, &LongMsgProtocol::waitAny_CB, NULL );

    m_my_dbg.verbose(CALL_INFO,1,0,"m_commReq.size()=%lu\n",m_commReqs.size());
    CtrlMsg::waitAny( m_commReqs, callback );

    // delete callback functor
    return true;
}

bool LongMsgProtocol::waitAny_CB( void* )
{
    RecvCallbackEntry *cbe = NULL;
    std::set<CommReq*>::iterator iter = m_commReqs.begin();
    for ( ; iter != m_commReqs.end(); ++iter ) {
        if ( (*iter)->isDone() ) {
            cbe = static_cast<RecvCallbackEntry*>( (*iter)->userInfo );
            m_commReqs.erase( iter );
            break;
        }
    }
    assert( cbe );
    
    // if there's a RecvEntry, a long message body has been received
    if ( cbe->recvEntry ) {
        finishRecvCBE( *cbe->recvEntry, cbe->hdr );

        // state has changed, something may have become unblocked
        processBlocked( cbe->recvEntry );
    } else {
        processRecvAny( cbe );
    }

    // delete callback functor
    return true;
}

//
// processRecvAny()
//
void LongMsgProtocol::processRecvAny( RecvCallbackEntry* cbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x\n",
                            cbe->ctrlResponse.rank, cbe->ctrlResponse.tag);

    if ( ( cbe->ctrlResponse.tag & TagMask ) == LongMsgRdyTag ) {


        processLongMsgRdyMsg( cbe->ctrlResponse.tag & ~TagMask,
                         cbe->ctrlResponse.rank );
        delete cbe;

    } else {

        std::deque<RecvEntry*>::iterator iter = m_postedQ.begin();

        // check for posted receives
        for ( ; iter != m_postedQ.end(); ++iter ) {
            if ( checkMatch( cbe->hdr, *(*iter) ) ) {
                m_my_dbg.verbose(CALL_INFO,1,0,"found match\n");
                cbe->recvEntry = *iter;
                m_postedQ.erase( iter );
                break;
            } 
        }

        if ( cbe->recvEntry ) {

            if ( ( cbe->ctrlResponse.tag & TagMask ) == ShortMsgTag)  {
                processShortMsg( cbe );
            } else {
                processLongMsg( cbe );
            }
        } else {
            m_my_dbg.verbose(CALL_INFO,1,0,"save unexpected\n");
            m_unexpectedQ.push_back( cbe );
            postRecvAny();
        }
    }
}

void LongMsgProtocol::processShortMsg( RecvCallbackEntry* cbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"received a short message\n");

    size_t len = cbe->recvEntry->count * 
                    info()->sizeofDataType( cbe->recvEntry->dtype );

    memcpy( cbe->recvEntry->buf, &cbe->buf[0], len );

    finishRecvCBE( *cbe->recvEntry, cbe->hdr );

    // got here from blocking postRecvEntry
    if ( m_blockedList.empty() ) {
        returnToFunction();
    } else {
        // state has changed, something may have become unblocked
        processBlocked( cbe->recvEntry );
    }
    delete cbe;
}

void LongMsgProtocol::processLongMsg( RecvCallbackEntry* cbe )
{
    // reuse the RecvcCallbackEntry for receiving the body
    cbe->key       = cbe->ctrlResponse.tag & ~TagMask;
    cbe->srcNode   = cbe->ctrlResponse.rank; 

    m_my_dbg.verbose(CALL_INFO,1,0,"src=%d key=%d\n", cbe->srcNode, cbe->key );

    RCBE_Functor* callback = new RCBE_Functor( 
              this, &LongMsgProtocol::processLongMsg_irecv_CB, cbe ); 

    cbe->vec.resize(1);
    cbe->vec[0].ptr = cbe->recvEntry->buf;
    cbe->vec[0].len = cbe->recvEntry->count * 
                    info()->sizeofDataType( cbe->recvEntry->dtype );

    cbe->commReq.userInfo = cbe;

    irecvv( cbe->vec, cbe->srcNode, cbe->key | LongMsgBdyTag, 
                &cbe->commReq, &cbe->ctrlResponse, callback  );
}

bool LongMsgProtocol::processLongMsg_irecv_CB( RecvCallbackEntry* rcbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"recv for Long Msg body posted\n");

    m_commReqs.insert( &rcbe->commReq );

    SendCallbackEntry* scbe = new SendCallbackEntry;
     
    SCBE_Functor* callback = new SCBE_Functor( 
                    this, &LongMsgProtocol::longMsgSendRdy_CB, scbe ); 

    scbe->vec.resize(0);

    sendv( scbe->vec, rcbe->srcNode, rcbe->key | LongMsgRdyTag, callback );
    
    // delete callback functor
    return true;
}

bool LongMsgProtocol::longMsgSendRdy_CB( SendCallbackEntry* cbe )
{
    m_my_dbg.verbose(CALL_INFO,1,0,"Rdy Msg sent\n");

    delete cbe; 

    // got here from blocking postRecvEntry
    if ( m_blockedList.empty() ) {
        returnToFunction();
    } else {
        // state has changed, something may have become unblocked
        processBlocked( cbe->sendEntry );  
    }

    // delete callback functor
    return true;
}

void LongMsgProtocol::processLongMsgRdyMsg( int key, int dest ) 
{
    m_my_dbg.verbose(CALL_INFO,1,0,"got LongMsgRdy message key=%#x\n", key);

    assert( m_longSendM.find( key ) != m_longSendM.end() );

    SendCallbackEntry* scbe = m_longSendM[ key ];

    m_longSendM.erase( key );

    SCBE_Functor* callback = new SCBE_Functor(
              this, &LongMsgProtocol::processLongMsgRdyMsg_CB, scbe );

    scbe->vec.resize(1);
    scbe->vec[0].len = scbe->sendEntry->count *
                    info()->sizeofDataType( scbe->sendEntry->dtype );

    scbe->vec[0].ptr = scbe->sendEntry->buf;

    // Send the body of a Long Message
    sendv( scbe->vec, dest , scbe->key | LongMsgBdyTag, callback );
}

bool LongMsgProtocol::processLongMsgRdyMsg_CB( SendCallbackEntry* cbe ) 
{
    m_my_dbg.verbose(CALL_INFO,1,0,"Long Msg body sent\n");

    // the user send is now complete
    finishSendCBE( *cbe->sendEntry );

    delete cbe;

    // state has changed, something may have become unblocked
    processBlocked( cbe->sendEntry ) ;

    return true;
}

//
// postRecvEntry()
//

void LongMsgProtocol::postRecvEntry( RecvEntry* entry )
{
    RecvCallbackEntry* cbe = NULL;
    m_my_dbg.verbose(CALL_INFO,1,0,"%s\n", entry->req ? "Irecv":"Recv");

    // We should block until there is room but for now punt 
    assert( m_postedQ.size() < MaxNumPostedRecvs );

    std::deque<RecvCallbackEntry*>::iterator iter = m_unexpectedQ.begin();

    m_my_dbg.verbose(CALL_INFO,1,0,"%lu\n",m_unexpectedQ.size());
    // check for posted receives
    for ( ; iter != m_unexpectedQ.end(); ++iter ) {
        cbe = *iter;
        if ( checkMatch( cbe->hdr, *entry) ) {
            m_my_dbg.verbose(CALL_INFO,1,0,"found unexpected match\n");
            m_unexpectedQ.erase( iter );
            break;
        } 
        cbe = NULL; 
    }

    if ( cbe ) { 
        cbe->recvEntry = entry;

        if ( ( cbe->ctrlResponse.tag & TagMask ) == ShortMsgTag)  {
            processShortMsg( cbe );
        } else {
            processLongMsg( cbe );
        }

    } else {

        m_postedQ.push_back( entry );
        
        // Non-blocking
        if ( entry->req ) {
            m_my_dbg.verbose(CALL_INFO,1,0,"pass control to function\n");
            returnToFunction();
        } else {
            Hermes::MessageRequest req = entry;
            block( &req, &entry->resp );
        }
    }
}

//
// checkMatch()
//

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

    m_my_dbg.verbose(CALL_INFO,1,0,"tag want %d, incoming %d\n",
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
