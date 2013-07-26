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

#include "ctrlMsg.h"
#include "info.h"

using namespace SST::Firefly;
using namespace SST;

CtrlMsg::CtrlMsg(int verboseLevel, Output::output_location_t loc, Info* info ) :
    ProtocolAPI(verboseLevel,loc),
    m_info(info)
{
#if 0
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:CtrlMsg::@p():@l ",info->nodeId(), 
                                                info->worldRank());
    m_dbg.setPrefix(buffer);
#endif
    m_dbg.setPrefix("@t:CtrlMsg::@p():@l ");
}

CtrlMsg::SendReq* CtrlMsg::getSendReq( ) 
{
    if ( ! m_sendQ.empty() ) {
        SendReq* req = new SendReq;  
        assert( req );
        m_dbg.verbose(CALL_INFO,1,0,"\n");

        req->entry = m_sendQ.front();
        m_sendQ.pop_front();

        Hdr& hdr = req->hdr;

        hdr.type = 0xf00d;
        hdr.srcRank = m_info->getGroup(req->entry->group)->getMyRank();
        hdr.group   = req->entry->group;
        hdr.len     = req->entry->len;

        req->ioVec.resize(2);
        req->ioVec[0].ptr = &hdr;
        req->ioVec[0].len = sizeof(hdr);
        req->ioVec[1].ptr = req->entry->buf;
        req->ioVec[1].len = req->entry->len;

        req->nodeId = m_info->rankToNodeId(req->entry->group, req->entry->node);
        return req;
    }
    return NULL;
}

CtrlMsg::RecvReq* CtrlMsg::getRecvReq( IO::NodeId src )
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

CtrlMsg::SendReq* CtrlMsg::sendIODone( Request* r   )
{
    SendReq* req = static_cast<SendReq*>(r);
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    req->entry->done = true;
    delete req;
    return  NULL;
}

CtrlMsg::MsgReq* CtrlMsg::findMatch( Hdr& hdr )
{
    m_dbg.verbose(CALL_INFO,1,0,"posted size %lu\n",m_postedQ.size());
    std::deque< MsgReq* >:: iterator iter = m_postedQ.begin();
    for ( ; iter != m_postedQ.end(); ++iter ) {
        MsgReq* req = *iter;
        m_dbg.verbose(CALL_INFO,1,0,"type %d %d\n", hdr.type, (*iter)->type);
        if ( hdr.type != (*iter)->type ) {
            continue;
        }
        m_dbg.verbose(CALL_INFO,1,0,"rank %d %d\n",hdr.srcRank,(*iter)->node);
        if ( hdr.srcRank != (*iter)->node ) {
            continue;
        }
        m_dbg.verbose(CALL_INFO,1,0,"group %d %d\n",hdr.group, (*iter)->group);
        if ( hdr.group != (*iter)->group ) {
            continue;
        }
        m_dbg.verbose(CALL_INFO,1,0,"type %d %d\n",hdr.type, (*iter)->type);
        if ( hdr.type != (*iter)->type ) {
            continue;
        }
        m_dbg.verbose(CALL_INFO,1,0,"len %lu %lu\n",hdr.len, (*iter)->len);
        if ( hdr.len != (*iter)->len ) {
            continue;
        }
        m_postedQ.erase(iter);
        return req;
    }
    return NULL;
}

CtrlMsg::RecvReq* CtrlMsg::searchUnexpected( MsgReq* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"unexpected size %lu\n", m_unexpectedQ.size() );
    std::deque< RecvReq* >::iterator iter = m_unexpectedQ.begin();
    for ( ; iter != m_unexpectedQ.end(); ++iter  ) {

        m_dbg.verbose(CALL_INFO,1,0,"type want %d, incoming %d\n",
                                            req->type, (*iter)->hdr.type );
        if ( req->type != (*iter)->hdr.type ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"rank want %d, incoming %d\n",
                                            req->node, (*iter)->hdr.srcRank);
        if ( req->node != (*iter)->hdr.srcRank ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"group want %d, incoming %d\n",
                                            req->group, (*iter)->hdr.group);
        if ( req->group != (*iter)->hdr.group ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"len %lu %lu\n",
                                            req->len, (*iter)->hdr.len );
        if ( req->len != (*iter)->hdr.len ) {
            continue;
        }

        m_unexpectedQ.erase( iter );
        return *iter;
    }

    return NULL;
}

CtrlMsg::RecvReq* CtrlMsg::recvIODone( Request* r )
{
    RecvReq* req = static_cast<RecvReq*>(r);
    
    if ( RecvReq::RecvHdr == req->state ) {

        m_dbg.verbose(CALL_INFO,1,0,"got message from %d\n",req->nodeId);
        req->entry = findMatch( req->hdr );    
        req->state = RecvReq::RecvBody;

        if ( req->entry ) {
            m_dbg.verbose(CALL_INFO,1,0,"found match\n" );
            if ( req->entry->len ) {
                m_dbg.verbose(CALL_INFO,1,0,"getBody %lu bytes\n",
                                            req->entry->len );
                m_dbg.verbose(CALL_INFO,1,0,"%lu %p\n",
                                            req->entry->len,req->entry->buf);
                req->ioVec[0].len = req->entry->len;
                req->ioVec[0].ptr = req->entry->buf;
            } else {
                req->entry->done = true;
                delete req;
                req = NULL;
            }
        } else {
            m_dbg.verbose(CALL_INFO,1,0, "unexpected\n" );
            if ( req->hdr.len ) {
                req->buf.resize(req->hdr.len);
                req->ioVec[0].len = req->buf.size();
                req->ioVec[0].ptr = &req->buf[0];
            }
            m_unexpectedQ.push_back( req ); 
            req = NULL;
        }

    } else {
        if ( req->entry ) {
            req->entry->done = true;
            delete req;
        }
        req = NULL;
    }
    
    return req;
}

CtrlMsg::Request* CtrlMsg::delayDone( Request* )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    return NULL;
}

void CtrlMsg::recv( void* buf, size_t len, int src, int group, MsgReq* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p len=%lu src=%d\n",buf,len,src);
    
    req->done = false;

    req->buf = buf;
    req->len = len;
    req->node = src;
    req->group = group;
    req->type = 0xf00d;

    m_dbg.verbose(CALL_INFO,1,0,"%lu\n", m_unexpectedQ.size());
    if ( ! m_unexpectedQ.empty() ) {
        assert(0);
        RecvReq *xxx = searchUnexpected( req );
        if ( xxx ) {
            m_dbg.verbose(CALL_INFO,1,0,"found unexpected\n");
            assert(0);
            memcpy( req->buf, &xxx->buf[0], req->len );
            req->done = true;
            delete xxx;
        }
    } 
    m_postedQ.push_back( req );
}

void CtrlMsg::send( void* buf, size_t len, int dest, int group, MsgReq* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p len=%lu dest=%d\n",buf,len,dest);
    req->done = false;

    req->buf = buf;
    req->len = len;
    req->node = dest;
    req->group = group;
    req->type = 0xf00d;

    m_sendQ.push_back( req );
}
bool CtrlMsg::test( MsgReq * req )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    return req->done;
}
