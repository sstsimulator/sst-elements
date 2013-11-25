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

#include <string.h>

#include <sst/core/link.h>
#include <sst/core/params.h>

#include <sstream>

#include "ctrlMsg.h"
#include "info.h"

using namespace SST::Firefly;
using namespace SST;

CtrlMsg::CtrlMsg( Component* owner, Params& params ) :
    m_out( NULL ),
    m_info( NULL ),
    m_outLink( NULL ),
    m_retLink( NULL )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc =
            (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:CtrlMsg::@p():@l ", verboseLevel, 0, loc );

    m_matchTime = params.find_integer("matchTime",0);
    m_copyTime = params.find_integer("copyTime",0);

    std::stringstream ss;
    ss << this;

    m_delayLink = owner->configureSelfLink( 
        "CtrlMsgSelfLink." + ss.str(), "1 ps",
        new Event::Handler<CtrlMsg>(this,&CtrlMsg::delayHandler));
}

void CtrlMsg::init( OutBase* jjj, Info* info, Link* link)
{
    m_out = jjj;
    m_info = info;
    m_outLink = link;
}

void CtrlMsg::setup() 
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:CtrlMsg::@p():@l ",m_info->nodeId(), 
                                                m_info->worldRank());
    m_dbg.setPrefix(buffer);
}

IO::NodeId CtrlMsg::canSend()
{
    if ( ! m_sendQ.empty() ) {
        CommReq* commReq = m_sendQ.front();

        SendInfo& info = *commReq->info();

        IO::NodeId dest = m_info->rankToNodeId( info.hdr.group, info.hdr.rank );

        if ( m_activeSendM.find( dest ) == m_activeSendM.end() ) {
            m_activeSendM[ dest ] = commReq; 
            m_sendQ.pop_front();
            m_dbg.verbose(CALL_INFO,1,0,"can send to %d\n",dest);
            return dest;
        } 
    }
    m_dbg.verbose(CALL_INFO,1,0,"nothing to send\n");
    return IO::AnyId;
}

void CtrlMsg::startSendStream( IO::NodeId dest )
{
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d\n",dest);

    assert( m_activeSendM.find( dest ) != m_activeSendM.end() );

    IORequest* req = new IORequest;

    req->callback = new IO_Functor( this, &CtrlMsg::sendIODone, req );
    req->commReq = m_activeSendM[dest];
    req->node = dest;

    SendInfo& info = *req->commReq->info();

    req->hdr.tag = info.hdr.tag;
    req->hdr.rank = m_info->getGroup( info.hdr.group )->getMyRank();
    req->hdr.group = info.hdr.group;
    req->hdr.len = info.hdr.len;

    if ( 0 == req->hdr.len  ) {
        req->state = IORequest::Body;
    } 

    req->ioVec.resize(1);
    req->ioVec[0].ptr = &req->hdr;
    req->ioVec[0].len = sizeof(req->hdr);

    m_dbg.verbose(CALL_INFO,1,0,"tag=%#x rank=%d group=%d len=%lu\n",
            info.hdr.tag, info.hdr.rank,info.hdr.group, info.hdr.len );

    m_out->sendv( req->node, req->ioVec, req->callback );
}

IO::Entry* CtrlMsg::sendIODone( IO::Entry* r )
{
    IORequest* req = static_cast<IORequest*>(r);
    m_dbg.verbose(CALL_INFO,1,0,"%s\n", 
                    req->state == IORequest::Hdr ? "Hdr" : "Body" );
    if ( req->state == IORequest::Hdr ) {
        req->state = IORequest::Body;
        m_out->sendv( req->node, req->commReq->info()->ioVec, req->callback );
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"endStream\n");

        m_activeSendM.erase( req->node );

        endSendStream( req );
    }

    return NULL;
}

void CtrlMsg::startRecvStream( IO::NodeId src )
{
    IORequest* req = new IORequest;

    m_dbg.verbose(CALL_INFO,1,0,"src=%d\n",src);

    req->callback = new IO_Functor( this, &CtrlMsg::recvIODone, req );
    req->node = src;

    req->ioVec.resize(1);
    req->ioVec[0].ptr = &req->hdr;
    req->ioVec[0].len = sizeof(req->hdr);

    m_out->recvv( req->node, req->ioVec, req->callback );
}

IO::Entry* CtrlMsg::recvIODone( IO::Entry* r )
{
    IORequest* req = static_cast<IORequest*>(r);

    m_dbg.verbose(CALL_INFO,1,0,"%s\n", 
                    req->state == IORequest::Hdr ? "Hdr" : "Body" );

    if ( IORequest::Hdr == req->state ) {
    
        int delay;

        m_dbg.verbose(CALL_INFO,1,0,"got data from %d\n", req->node );

        req->commReq = findPostedRecv( req->hdr, delay );    

        if ( req->commReq ) {
            m_dbg.verbose(CALL_INFO,1,0,"found a posted receive\n");
            req->commReq->setResponse( req->hdr.rank, req->hdr.tag, 
                                                    req->hdr.len  );
        }

        m_dbg.verbose(CALL_INFO,1,0,"delay %d\n",delay);
        m_delayLink->send( delay, new DelayEvent( req ) );

    } else {

        if ( ! req->commReq ) {
            m_dbg.verbose(CALL_INFO,1,0,"posted unexpected\n");
            m_unexpectedQ.push_back( req ); 
        }
        endRecvStream( req );
    }

    return NULL;
}

void CtrlMsg::delayHandler( SST::Event* e )
{
    DelayEvent* event = static_cast<DelayEvent*>(e);
    
    if ( event->type == DelayEvent::FromOther ) {
        m_dbg.verbose(CALL_INFO,1,0,"FromOther\n");
        recvBody( event->ioRequest );
    } else if ( event->type == DelayEvent::FromFunc)  {

        m_dbg.verbose(CALL_INFO,1,0,"FromFunc\n");
        delete event->ioRequest;

        event->commReq->setDone();

        if ( event->blocked ) {
            passCtrlToHades(0);
        } else {
            passCtrlToFunction(0);
        }
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"Functor %p\n", event->functor);
        CBF* tmp = (*event->functor)();
        if ( tmp ) {
            delete tmp;
        }
    }
    
    delete event;
}

void CtrlMsg::recvBody( IORequest* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"setup %s body %lu bytes\n",
                req->commReq ? "matched" : "unexpected", req->hdr.len );

    req->state = IORequest::Body;

    if ( req->commReq ) {
        if ( req->hdr.len ) {
            req->ioVec.resize( req->commReq->info()->ioVec.size() );

            size_t left = req->hdr.len;
            for ( unsigned int i = 0; i < req->ioVec.size() && left; i++ ) {
                m_dbg.verbose(CALL_INFO,1,0,"%lu %p\n",
                                    req->commReq->info()->ioVec[i].len, 
                                    req->commReq->info()->ioVec[i].ptr);

                req->ioVec[i].len = left > req->commReq->info()->ioVec[i].len ?
                    req->commReq->info()->ioVec[i].len : left;

                req->ioVec[i].ptr = req->commReq->info()->ioVec[i].ptr;

                m_dbg.verbose(CALL_INFO,1,0,"%lu %p\n",
                                    req->ioVec[i].len, 
                                    req->ioVec[i].ptr);

                left -= req->ioVec[i].len;
            }
        }
        
    } else {
        if ( req->hdr.len ) {
            req->buf.resize(req->hdr.len);
            req->ioVec[0].len = req->buf.size();
            req->ioVec[0].ptr = &req->buf[0];
        }
    }

    if ( req->hdr.len ) {
        m_out->recvv( req->node, req->ioVec, req->callback );
    } else {
        if ( ! req->commReq ) {
            m_dbg.verbose(CALL_INFO,1,0,"posted unexpected\n");
            m_unexpectedQ.push_back( req ); 
        }
        endRecvStream( req );
    }
}

void CtrlMsg::endStream( IORequest* req )
{
    if ( req->commReq ) {

        m_dbg.verbose(CALL_INFO,1,0,"mark CommReq done\n");

        req->commReq->setDone();
    
        if ( m_blockedReqs.find( req->commReq ) != m_blockedReqs.end() ) {
            m_blockedReqs.erase( req->commReq );
            passCtrlToFunction(0);
        } else if ( req->commReq->doneFunc() ) {
            m_delayLink->send( 0, 
                    new DelayEvent( req->commReq->doneFunc() ) );
        } else {
            passCtrlToHades(0);
        }
        delete req;

    } else {
        int        delay = 0;
        IORequest* ioReq;

        m_dbg.verbose(CALL_INFO,1,0,"unexpected msg stream end\n");

        std::set<CommReq*>::iterator iter = m_blockedReqs.begin();
        for ( ; iter != m_blockedReqs.end(); ++iter ) 
        {
            if ( ! (*iter)->isSend() &&
                ( ioReq = processUnexpectedMatch( (*iter), delay ) ) )
            {
                m_dbg.verbose(CALL_INFO,1,0,"found unexpected msg, delay=%d\n",
                                                     delay);
                removePostedRecv( (*iter) ); 
                m_delayLink->send( delay, 
                            new DelayEvent( (*iter), ioReq, false ) );
                m_blockedReqs.erase( iter );

                // RETURN
                return;
            } 
        }
        passCtrlToHades(delay);
    }
}

void CtrlMsg::endSendStream( IORequest* req )
{
    endStream( req );
}

void CtrlMsg::endRecvStream( IORequest* req )
{
    endStream( req );
}

CtrlMsg::CommReq* CtrlMsg::findPostedRecv( MsgHdr& hdr, int& delay )
{
    delay = 0;
    m_dbg.verbose(CALL_INFO,1,0,"posted size %lu\n",m_postedQ.size());

    std::deque< CommReq* >:: iterator iter = m_postedQ.begin();
    for ( ; iter != m_postedQ.end(); ++iter ) {

        RecvInfo& info = *static_cast<RecvInfo*>( (*iter)->info() );

        m_dbg.verbose(CALL_INFO,1,0,"tag: list=%#x msg=%#x\n", 
                                                    info.hdr.tag, hdr.tag );
        delay += m_matchTime;
        if ( ( AnyTag != info.hdr.tag ) && ( hdr.tag != info.hdr.tag ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"rank: list=%d msg=%d\n",
                                                    info.hdr.rank, hdr.rank );
        if ( ( AnyRank != info.hdr.rank ) && ( hdr.rank != info.hdr.rank ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"length: list=%lu msg=%lu\n",
                                                    info.hdr.len, hdr.len );
#if 0
        if ( hdr.len != info.hdr.len ) {
#endif
        if ( hdr.len > info.hdr.len ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"group: list=%d msg=%d\n",
                                                    info.hdr.group, hdr.group );
        if ( hdr.group != info.hdr.group ) {
            continue;
        }

        CommReq* req = *iter;
        m_postedQ.erase(iter);
        return req;
    }
    return NULL;
}

CtrlMsg::IORequest* CtrlMsg::findUnexpectedRecv( RecvInfo& info, int& delay )
{
    delay = 0;
    m_dbg.verbose(CALL_INFO,1,0,"unexpected size %lu\n", m_unexpectedQ.size() );

    std::deque< IORequest* >::iterator iter = m_unexpectedQ.begin();
    for ( ; iter != m_unexpectedQ.end(); ++iter  ) {

        delay += m_matchTime;

        MsgHdr& tmpHdr = (*iter)->hdr; 

        m_dbg.verbose(CALL_INFO,1,0,"tag %#x %#x\n", tmpHdr.tag, info.hdr.tag );
        if ( ( AnyTag != info.hdr.tag ) && (  tmpHdr.tag != info.hdr.tag ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"node %d %d\n", tmpHdr.rank, info.hdr.rank );
        if ( ( AnyRank != info.hdr.rank ) && ( tmpHdr.rank != info.hdr.rank ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"len %lu %lu\n", tmpHdr.len, info.hdr.len );
#if 0
        if ( ( (size_t) -1 != info.hdr.len ) && ( tmpHdr.len != info.hdr.len )){
            continue;
        }
#endif

        m_dbg.verbose(CALL_INFO,1,0,"group %d %d\n", 
                                tmpHdr.group, info.hdr.group );
        if ( tmpHdr.group != info.hdr.group ) {
            continue;
        }

        IORequest* req = *iter;
        m_dbg.verbose(CALL_INFO,1,0,"matched unexpected, delay=%d\n",delay);
        m_unexpectedQ.erase( iter );
        return req;
    }

    return NULL;
}

void CtrlMsg::send( void* buf, size_t len, int dest, int tag, int group,
                                        CBF::Functor* functor ) 
{
    CommReq* req = &m_xxxReq;

    _send( true, buf, len, dest, tag, group, req, functor );
}

void CtrlMsg::sendv(std::vector<IoVec>& ioVec, int dest, int tag, int group,
                                        CBF::Functor* functor )
{
    CommReq* req = &m_xxxReq;

    _sendv( true, ioVec, dest, tag, group, req, functor );
}

void CtrlMsg::sendv(std::vector<IoVec>& ioVec, int dest, int tag,
                                        CBF::Functor* functor )
{
    CommReq* req = &m_xxxReq;

    _sendv( true, ioVec, dest, tag, Hermes::GroupWorld, req, functor );
}

void CtrlMsg::isendv(std::vector<IoVec>& ioVec, int dest, int tag,
                            CommReq* req, CBF::Functor* functor )
{
    _sendv( false, ioVec, dest, tag, Hermes::GroupWorld, req, functor );
}

void CtrlMsg::recv( void* buf, size_t len, int src, int tag, int group,
                                            CBF::Functor* functor )
{
    CommReq* req = &m_xxxReq;
    _recv( true, buf, len, src, tag, group, req, functor );
}

void CtrlMsg::recvv( std::vector<IoVec>& ioVec, int src, int tag, 
                                Response* response, CBF::Functor* functor )
{
    CommReq* req = &m_xxxReq;
    _recvv( true, ioVec, src, tag, Hermes::GroupWorld, req,
                                                NULL, response, functor );
}


void CtrlMsg::irecv( void* buf, size_t len, int src, int tag, int group, 
                                    CommReq* req, CBF::Functor* functor )
{
    _recv( false, buf, len, src, tag, group, req, functor );
}

void CtrlMsg::irecvv(std::vector<IoVec>& ioVec, int src, int tag, int group,
                            CommReq* req, CBF::Functor* functor )
{
    _recvv( false, ioVec, src, tag, group, req, NULL,  NULL, functor );
}

void CtrlMsg::irecvv(std::vector<IoVec>& ioVec, int src, int tag,
                    CommReq* req, CBF::Functor* doneFunc, 
                        Response* resp, CBF::Functor* retFunc )
{
    _recvv( false, ioVec, src, tag, Hermes::GroupWorld, req, doneFunc,
                    resp, retFunc );
}

void CtrlMsg::_send( bool blocked, void* buf, size_t len, int dest, int tag,
                                int group, CommReq* req, CBF::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p len=%lu dest=%d tag=%#x group=%d\n",
                                        buf,len,dest,tag,group);

    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    _sendv( blocked, ioVec, dest, tag, group, req, functor );
}

void CtrlMsg::_recv( bool blocked, void* buf, size_t len, int src, int tag,
                                int group, CommReq* req, CBF::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p len=%lu src=%d\n",buf,len,src);
    
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    _recvv( blocked, ioVec, src, tag, group, req, NULL, NULL, functor );
}

void CtrlMsg::_sendv( bool blocked, std::vector<IoVec>& ioVec, int dest,
                    int tag, int group, CommReq* req, CBF::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d tag=%#x group=%d\n", dest,tag,group);

    m_returnFunctor = functor;

    req->initSend( blocked, ioVec, dest, tag, group  );
    req->setDoneFunc( NULL );

    m_sendQ.push_back( req );

    if ( blocked ) {
        m_blockedReqs.insert( req );
        passCtrlToHades(0);
    } else {
        passCtrlToFunction(0);
    }
}

void CtrlMsg::passCtrlToFunction( int delay )
{
    m_dbg.verbose(CALL_INFO,1,0,"back to Function delay=%d\n", delay);

    if ( m_returnFunctor ) {
        m_delayLink->send( delay, new DelayEvent( m_returnFunctor ) );
    } else {
        m_retLink->send( delay, NULL );
    }
}
void CtrlMsg::passCtrlToHades( int delay )
{
    m_dbg.verbose(CALL_INFO,1,0,"handoff to Hades delay=%d\n", delay );
    m_outLink->send( delay, NULL );
}

void CtrlMsg::_recvv( bool blocked, std::vector<IoVec>& ioVec, int src,
    int tag, int group, CommReq* req, CBF::Functor* doneFunc, 
                    Response* resp, CBF::Functor* retFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x group=%d\n", src,tag,group);

    m_returnFunctor = retFunc;

    req->initRecv( blocked, ioVec, src, tag, group, resp );
    req->setDoneFunc( doneFunc );

    int delay;
    IORequest* ioReq = processUnexpectedMatch( req, delay );

    if ( ioReq ) { 
        m_dbg.verbose(CALL_INFO,1,0,"found unexpected msg, delay %d\n", delay);
        m_delayLink->send( delay, new DelayEvent( req, ioReq, false) );
    } else {

        m_dbg.verbose(CALL_INFO,1,0,"postedQ.push_back()\n");
        m_postedQ.push_back( req );

        if ( blocked ) {
            m_blockedReqs.insert( req );
            passCtrlToHades( delay );
        } else {
            passCtrlToFunction( delay );
        }
    }
}

void CtrlMsg::wait( CommReq* req, CBF::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    std::set<CommReq*> reqs;
    reqs.insert(req);
    waitAny( reqs, functor );
}

void CtrlMsg::waitAny( std::set<CommReq*>& reqs, CBF::Functor* functor )
{
    int delay = 0;

    m_returnFunctor = functor;

    std::set<CommReq*>::iterator iter = reqs.begin();
    for ( ; iter != reqs.end(); ++iter )  {

        if ( (*iter)->isDone() ) {
            passCtrlToFunction(delay);

            // RETURN
            return;
        } else  { 
            IORequest* ioReq;
            if ( ! (*iter)->isSend() &&
                    ( ioReq = processUnexpectedMatch( (*iter), delay ) ) )
            {
                m_dbg.verbose(CALL_INFO,1,0,"found unexpected msg, delay=%d\n",
                                                        delay);
                removePostedRecv( (*iter) );
                m_delayLink->send(delay, new DelayEvent( (*iter), ioReq, false));

                // RETURN;
                return;
            }
        }
    }

    assert( m_blockedReqs.empty() );
    m_blockedReqs = reqs;
    passCtrlToHades( delay );
}


void CtrlMsg::removePostedRecv( CommReq* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"%p\n",req);
    std::deque< CommReq* >:: iterator iter = m_postedQ.begin();
    for ( ; iter != m_postedQ.end(); ++iter ) {

        if ( req == *iter ) {
            m_dbg.verbose(CALL_INFO,1,0,"found %p\n",req);
            m_postedQ.erase(iter);
            break;
        }
    }
}

CtrlMsg::IORequest* CtrlMsg::processUnexpectedMatch( CommReq* req, int& delay )
{
    IORequest* xxx;
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    delay = 0;

    assert( ! req->isDone() );

    if ( ( xxx = findUnexpectedRecv( 
                            *static_cast<RecvInfo*>( req->info() ), delay ) ) )
    {
        m_dbg.verbose(CALL_INFO,1,0,"found match, delay=%d\n",delay);
            
        req->setResponse( xxx->hdr.rank, xxx->hdr.tag, xxx->hdr.len );

        assert( req->info() );
        m_dbg.verbose(CALL_INFO,1,0,"unexpected, %lu bytes\n", xxx->buf.size());

        size_t offset = 0;

        for ( unsigned int i=0; i < req->info()->ioVec.size() && 
                        offset < xxx->buf.size(); i++ ) {

            m_dbg.verbose(CALL_INFO,1,0,"%lu\n",req->info()->ioVec[i].len);

            size_t len = req->info()->ioVec[i].len < xxx->buf.size() - offset ?
                          req->info()->ioVec[i].len : xxx->buf.size() - offset;

            memcpy( req->info()->ioVec[i].ptr, &xxx->buf[offset], len );

            offset += len;

            delay += getCopyDelay( len );
        }
    }

    return xxx;
}
