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

#include "dataMovement.h"
#include "entry.h"
#include "info.h"

using namespace SST::Firefly;
using namespace SST;

DataMovement::DataMovement( SST::Params params, Info* info, SST::Link* link ) :
    m_info(info),
    m_outLink( link ),
    m_retLink( NULL ),
    m_sendReqKey( 0 ),
    m_recvReqKey( 0 ),
    m_blockedReq( NULL )
{
    int verboseLevel = params.find_integer("verboseLevel",0);
    Output::output_location_t loc =
            (Output::output_location_t)params.find_integer("debug", 0);

    m_dbg.init("@t:DataMovement::@p():@l ", verboseLevel, 0, loc );

    m_matchTime = params.find_integer("matchTime",0);
    m_copyTime = params.find_integer("copyTime",0);
}

void DataMovement::setup() 
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:DataMovement::@p():@l ",m_info->nodeId(),
                                                m_info->worldRank());
    m_dbg.setPrefix(buffer);
}

DataMovement::Request* DataMovement::getRecvReq( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    RecvReq* req = new RecvReq;
    Hdr& hdr = req->hdr;
    req->delay = 0;

    req->nodeId = src;

    req->ioVec.resize(1);
    req->ioVec[0].ptr = &hdr;
    req->ioVec[0].len = sizeof(hdr);
    req->state = RecvReq::RecvHdr;
    
    return req;
}

DataMovement::Request* DataMovement::getSendReq(  )
{
    if ( ! m_longMsgReqQ.empty() ) {
        RecvReq* recvReq = m_longMsgReqQ.front();
        m_longMsgReqQ.pop_front();
        SendReq* sendReq = new SendReq;
        m_dbg.verbose(CALL_INFO,1,0,"send LongMsgReq to %d\n",
                                        recvReq->nodeId);
        sendReq->ioVec.resize(1);
        sendReq->ioVec[0].ptr = &sendReq->hdr;
        sendReq->ioVec[0].len = sizeof(sendReq->hdr);
        sendReq->nodeId = recvReq->nodeId;   
        sendReq->entry = NULL;

        sendReq->hdr.msgType = Hdr::LongMsgReq;
        sendReq->hdr.sendKey = recvReq->hdr.sendKey;
        sendReq->hdr.recvKey = saveRecvReq( recvReq );
        return sendReq;
    }

    if ( ! m_longMsgRespQ.empty() ) {
        RecvReq* recvReq = m_longMsgRespQ.front();
        m_longMsgRespQ.pop_front();
        
        SendReq* sendReq = findSendReq( recvReq->hdr.sendKey );
        assert(sendReq);

        m_dbg.verbose(CALL_INFO,1,0,"send LongMsgResp to %d\n",
                                                        recvReq->nodeId);
        sendReq->hdr.msgType = Hdr::LongMsgResp;
        sendReq->hdr.recvKey = recvReq->hdr.recvKey;
        sendReq->hdr.sendKey = -1;
        sendReq->ioVec.resize(2);
        
        sendReq->ioVec[0].ptr = &sendReq->hdr;
        sendReq->ioVec[0].len = sizeof(sendReq->hdr);
        sendReq->ioVec[1].ptr = sendReq->entry->buf; 
        sendReq->ioVec[1].len = sendReq->entry->count * 
                    m_info->sizeofDataType( sendReq->entry->dtype );
        delete recvReq;
        return sendReq;
    }

    if ( ! m_sendQ.empty() ) {
        m_dbg.verbose(CALL_INFO,1,0,"send Match msg\n");
        SendReq* req = new SendReq;
        req->hdr.msgType = Hdr::MatchMsg;
        req->entry = m_sendQ.front(); 
        m_sendQ.pop_front();

        Hdr& hdr = req->hdr;
        
        hdr.mHdr.tag     = req->entry->tag;
        hdr.mHdr.count   = req->entry->count;
        hdr.mHdr.dtype   = req->entry->dtype;
        hdr.mHdr.srcRank = m_info->getGroup(req->entry->group)->getMyRank();
        hdr.mHdr.group   = req->entry->group;

        size_t len = req->entry->count * 
                    m_info->sizeofDataType(req->entry->dtype);

        if ( len <= ShortMsgLength ) {
            req->ioVec.resize(2);
            req->ioVec[1].ptr = req->entry->buf;
            req->ioVec[1].len = req->entry->count * 
                        m_info->sizeofDataType(req->entry->dtype);
        } else {
            req->ioVec.resize(1);
            hdr.sendKey = saveSendReq( req );
            hdr.recvKey = -1;
        }
        req->ioVec[0].ptr = &hdr;
        req->ioVec[0].len = sizeof(hdr);

        req->nodeId = m_info->rankToNodeId(req->entry->group, req->entry->dest);
 
        return req;  
    }

    return NULL;
}

DataMovement::Request* DataMovement::sendIODone( Request *r )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SendReq* req = static_cast<SendReq*>(r);

    if ( req->hdr.msgType == Hdr::MatchMsg ) { 
        size_t len = req->entry->count * 
                    m_info->sizeofDataType(req->entry->dtype);

        if ( len <= ShortMsgLength ) {
            req->entry->setDone();
            delete req;
        }
    } else {
        if ( req->entry ) {
            req->entry->setDone();
        }
        delete req;
    }
    return NULL;
}

DataMovement::Request* DataMovement::recvIODone( Request *r )
{
    RecvReq* req = static_cast<RecvReq*>(r);
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    if ( RecvReq::RecvHdr == req->state ) { 

        m_dbg.verbose(CALL_INFO,1,0,"srcNodeId=%d msgType=%d\n",
                req->nodeId, req->hdr.msgType );

        if ( req->hdr.msgType == Hdr::MatchMsg ) {

            m_dbg.verbose(CALL_INFO,1,0,"count=%d dtype=%d tag=%#x\n",
                req->hdr.mHdr.count,
                req->hdr.mHdr.dtype,
                req->hdr.mHdr.tag );

            req->recvEntry = findMatch( req->hdr.mHdr, req->delay );
            req->state = RecvReq::RecvBody;
    
            if ( req->recvEntry ) {
                m_dbg.verbose(CALL_INFO,1,0,"found a posted receive\n");
            }

            if ( 0 == req->delay ) {
                return delayDone( r );
            }
        } else if ( req->hdr.msgType == Hdr::LongMsgReq ) {
            m_dbg.verbose(CALL_INFO,1,0,"LongMsgReq\n");
            m_longMsgRespQ.push_back( req );
            req = NULL;
        } else if ( req->hdr.msgType == Hdr::LongMsgResp ) {
            m_dbg.verbose(CALL_INFO,1,0,"LongMsgResp\n");
            RecvReq* oldReq  = findRecvReq( req->hdr.recvKey );
            assert(oldReq);
            delete req;
            req = oldReq;
            req->delay = 0;
            req->state = RecvReq::RecvBody;
            req->ioVec.resize(1);
            req->ioVec[0].ptr = req->recvEntry->buf;  
            req->ioVec[0].len =req->hdr.mHdr.count * 
                    m_info->sizeofDataType(req->hdr.mHdr.dtype);
        } else {
            assert(0);
        }
    } else {
        if ( req->recvEntry ) {
            finishRecv(r);
        }
        req = NULL;
    }

    return req;
}

DataMovement::Request* DataMovement::delayDone( Request *r )
{
    RecvReq* req = static_cast<RecvReq*>(r);

    m_dbg.verbose(CALL_INFO,1,0,"\n");

    size_t len = req->hdr.mHdr.count * 
                    m_info->sizeofDataType(req->hdr.mHdr.dtype);

    if ( ! req->recvEntry ) {
        m_dbg.verbose(CALL_INFO,1,0,"no match save unexpected MsgEntry\n");
        req->msgEntry = new MsgEntry;
        req->msgEntry->hdr       = req->hdr.mHdr;
        req->msgEntry->srcNodeId = req->nodeId;
        req->msgEntry->sendKey = req->hdr.sendKey;

        m_unexpectedMsgQ.push_back( req->msgEntry );

        if ( len && len <= ShortMsgLength ) {
            m_dbg.verbose(CALL_INFO,1,0,"unexpected body %lu bytes\n",len); 
            req->ioVec.resize(1);
            req->ioVec[0].len = len;
            req->msgEntry->buffer.resize( req->ioVec[0].len );
            req->ioVec[0].ptr = &req->msgEntry->buffer[0];
        } else {
            m_dbg.verbose(CALL_INFO,1,0,"zero length or long unexpected\n"); 
            req = NULL;
        }
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"setup match\n");

        if ( len > ShortMsgLength ) {
            m_dbg.verbose(CALL_INFO,1,0,"queue LongMsgReq\n");
            m_longMsgReqQ.push_back( req );
            req = NULL;
        } else if ( len ) {
            m_dbg.verbose(CALL_INFO,1,0,"match body %lu bytes\n", len);
            req->ioVec.resize(1);
            req->ioVec[0].len = len;
            req->ioVec[0].ptr = req->recvEntry->buf;
        } else {
            m_dbg.verbose(CALL_INFO,1,0,"zero length match\n");
            finishRecv( r );
            req = NULL;
        }
    }

    return req;
}

void DataMovement::completeLongMsg( MsgEntry* msgEntry, RecvEntry* recvEntry )
{
    m_dbg.verbose(CALL_INFO,1,0,"queue LongMsgReq\n");
    
    RecvReq* req = new RecvReq;
    req->recvEntry = recvEntry; 
    req->nodeId = static_cast<MsgEntry*>(msgEntry)->srcNodeId;   
    req->hdr.mHdr = static_cast<MsgEntry*>(msgEntry)->hdr;
    req->hdr.sendKey = static_cast<MsgEntry*>(msgEntry)->sendKey;
    req->hdr.recvKey = saveRecvReq( req );
    m_longMsgReqQ.push_back( req );
}

void  DataMovement::finishRecv( Request* r )
{
    RecvReq* req = static_cast<RecvReq*>(r);

    req->recvEntry->setDone();  
    if ( req->recvEntry->resp ) {
        m_dbg.verbose(CALL_INFO,1,0,"finish up blocking match %p\n",
                                            req->recvEntry);
        req->recvEntry->resp->tag = req->hdr.mHdr.tag;  
        req->recvEntry->resp->src = req->hdr.mHdr.srcRank;  
    }
    delete req; 
}

bool DataMovement::unblocked()
{
    if ( m_blockedReq && m_blockedReq->isDone()) {
        delete m_blockedReq;
        m_blockedReq = NULL;
        m_dbg.verbose(CALL_INFO,1,0,"pass control to Function\n");
        m_retLink->send(0,NULL);
        return true;
    } 
    return false; 
}

void DataMovement::wait( Hermes::MessageRequestBase* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"block %p\n", req);
    assert( !m_blockedReq );
    m_blockedReq = req;
    m_outLink->send(0,NULL);
}

void DataMovement::postSendEntry( SendEntry* entry )
{
    // We should block until there is room but for now punt  
    assert( m_sendQ.size() < MaxNumSends );

    m_dbg.verbose(CALL_INFO,1,0,"\n");

    m_sendQ.push_back( entry );

    m_retLink->send(0,NULL);
}

void DataMovement::postRecvEntry( RecvEntry* recvEntry )
{
    // We should block until there is room but for now punt  
    assert( m_postedQ.size() < MaxNumPostedRecvs );

    m_dbg.verbose(CALL_INFO,1,0,"\n");

    int delay = 0;
    MsgEntry* msgEntry = searchUnexpected( recvEntry, delay );

    if ( msgEntry ) {
        m_dbg.verbose(CALL_INFO,1,0,"found unexpected message\n");

        if ( recvEntry->resp ) {
            recvEntry->resp->src = msgEntry->hdr.srcRank;
            recvEntry->resp->tag = msgEntry->hdr.tag;
        }

        size_t len = recvEntry->count *
                        m_info->sizeofDataType( recvEntry->dtype );
        if ( len <= ShortMsgLength ) {
            m_dbg.verbose(CALL_INFO,1,0,"complete short msg\n");
            memcpy( recvEntry->buf, &msgEntry->buffer[0],
                                    msgEntry->buffer.size());
            delay += getCopyDelay( msgEntry->buffer.size() );

            recvEntry->setDone();
        } else {
            completeLongMsg( msgEntry, recvEntry );
        }
    } else {
        m_postedQ.push_back( recvEntry );
    }
    
    m_dbg.verbose(CALL_INFO,1,0,"delay %d\n",delay);
    m_retLink->send(delay,NULL);
}

MsgEntry* DataMovement::searchUnexpected( RecvEntry* e, int& delay )
{
    delay = 0;
    RecvEntry* entry = static_cast<RecvEntry*>(e);
    m_dbg.verbose(CALL_INFO,1,0,"unexpected size %lu\n",
                                            m_unexpectedMsgQ.size() );
    std::deque< MsgEntry* >::iterator iter = m_unexpectedMsgQ.begin();
    for ( ; iter != m_unexpectedMsgQ.end(); ++iter  ) {

        delay += m_matchTime;

        m_dbg.verbose(CALL_INFO,1,0,"dtype %d %d\n",
                                    entry->dtype,(*iter)->hdr.dtype);
        if ( entry->dtype != (*iter)->hdr.dtype ) {
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"count %d %d\n",
                                    entry->count,(*iter)->hdr.count);
        if ( entry->count != (*iter)->hdr.count ) { 
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"tag want %d, incoming %d\n",
                                    entry->tag,(*iter)->hdr.tag);
        if ( entry->tag != Hermes::AnyTag && 
                entry->tag != (*iter)->hdr.tag ) {
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"group want %d, incoming %d\n",
                                     entry->group, (*iter)->hdr.group);
        if ( entry->group != (*iter)->hdr.group ) { 
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"rank want %d, incoming %d\n",
                                    entry->src, (*iter)->hdr.srcRank);
        if ( entry->src != Hermes::AnySrc &&
                    entry->src != (*iter)->hdr.srcRank ) {
            continue;
        } 
        MsgEntry* tmp = *iter;
        m_unexpectedMsgQ.erase( iter );
        return tmp;
    }
    
    return NULL;
}

RecvEntry* DataMovement::findMatch( MatchHdr& hdr, int& delay )
{
    delay = 0;
    m_dbg.verbose(CALL_INFO,1,0,"posted size %lu\n", m_postedQ.size() );
    std::deque< RecvEntry* >::iterator iter = m_postedQ.begin(); 
    for ( ; iter != m_postedQ.end(); ++iter  ) {

        delay += m_matchTime;

        m_dbg.verbose(CALL_INFO,1,0,"dtype %d %d\n",
                                                hdr.dtype, (*iter)->dtype );
        if ( hdr.dtype != (*iter)->dtype ) {
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"count %d %d\n",hdr.count,(*iter)->count);
        if ( (*iter)->count !=  hdr.count ) {
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"tag incoming=%d %d\n",
                                             hdr.tag,(*iter)->tag);
        if ( (*iter)->tag != Hermes::AnyTag && 
                (*iter)->tag !=  hdr.tag ) {
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"group incoming=%d %d\n",
                                            hdr.group, (*iter)->group);
        if ( (*iter)->group !=  hdr.group ) {
            continue;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"rank incoming=%d %d\n",
                                            hdr.srcRank, (*iter)->src);
        if ( (*iter)->src != Hermes::AnySrc &&
                    hdr.srcRank != (*iter)->src ) {
            continue;
        } 

        RecvEntry* entry = *iter;
        m_postedQ.erase(iter);
        return entry;
    }
    
    return NULL;
}
