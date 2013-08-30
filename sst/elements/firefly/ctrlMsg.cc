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

CtrlMsg::CtrlMsg(int verboseLevel, Output::output_location_t loc, 
                SST::Params& params, Info* info ) :
    ProtocolAPI(verboseLevel,loc),
    m_info(info),
    m_sleep(false)
{
    m_matchTime = params.find_integer("matchTime",0);
    m_copyTime = params.find_integer("copyTime",0);
}

void CtrlMsg::setup() 
{
    char buffer[100];
    snprintf(buffer,100,"@t:%d:%d:CtrlMsg::@p():@l ",m_info->nodeId(), 
                                                m_info->worldRank());
    m_dbg.setPrefix(buffer);
}

CtrlMsg::Request* CtrlMsg::getSendReq( ) 
{
    if ( ! m_sendQ.empty() ) {
        SendReq* req = new SendReq;  
        assert( req );
        m_dbg.verbose(CALL_INFO,1,0,"\n");

        req->commReq = m_sendQ.front();
        SendInfo& info = *static_cast<SendInfo*>(req->commReq->info);
        m_sendQ.pop_front();

        Hdr& hdr = req->hdr;

        hdr.tag = info.tag;
        hdr.src = m_info->getGroup( info.group )->getMyRank();
        hdr.group = info.group;

        req->ioVec.resize(1 + info.ioVec.size());
        req->ioVec[0].ptr = &hdr;
        req->ioVec[0].len = sizeof(hdr);

        hdr.len = 0;
        for ( unsigned int i = 0; i < info.ioVec.size(); i++ ) {
            req->ioVec[i + 1].ptr = info.ioVec[i].ptr;
            req->ioVec[i + 1].len = info.ioVec[i].len;
            hdr.len += info.ioVec[i].len;
        }

        m_dbg.verbose(CALL_INFO,1,0,"\n");
        req->nodeId = m_info->rankToNodeId( info.group, info.dest );
        return req;
    }
    return NULL;
}

CtrlMsg::Request* CtrlMsg::getRecvReq( IO::NodeId src )
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
 
    m_sleep = false;
    return req;
}

CtrlMsg::Request* CtrlMsg::sendIODone( Request* r )
{
    SendReq* req = static_cast<SendReq*>(r);
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    req->commReq->done = true;
    delete req;
    return  NULL;
}

CtrlMsg::CommReq* CtrlMsg::findMatch( Hdr& hdr, int& delay )
{
    delay = 0;
    m_dbg.verbose(CALL_INFO,1,0,"posted size %lu\n",m_postedQ.size());

    std::deque< CommReq* >:: iterator iter = m_postedQ.begin();
    for ( ; iter != m_postedQ.end(); ++iter ) {

        RecvInfo& info = *static_cast<RecvInfo*>( (*iter)->info );

        //m_dbg.verbose(CALL_INFO,1,0,"tag %d %d\n", info.tag, hdr.tag );
        m_dbg.verbose(CALL_INFO,1,0,"tag: list %#x, hdr %#x\n", 
                                                    info.tag, hdr.tag );
        delay += m_matchTime;
        if ( ( -1 != info.tag ) && ( hdr.tag != info.tag ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"rank %d %d\n", info.src, hdr.src );
        if ( ( -1 != info.src ) && ( hdr.src != info.src ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"len %lu %lu\n", info.len, hdr.len );
        if ( hdr.len != info.len ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"group %d %d\n", info.group, hdr.group );
        if ( hdr.group != info.group ) {
            continue;
        }

        CommReq* req = *iter;
        m_postedQ.erase(iter);
        return req;
    }
    return NULL;
}

CtrlMsg::RecvReq* CtrlMsg::searchUnexpected( RecvInfo& info, int& delay )
{
    delay = 0;
    m_dbg.verbose(CALL_INFO,1,0,"unexpected size %lu\n", m_unexpectedQ.size() );

    std::deque< RecvReq* >::iterator iter = m_unexpectedQ.begin();
    for ( ; iter != m_unexpectedQ.end(); ++iter  ) {

        delay += m_matchTime;

        Hdr& tmpHdr = (*iter)->hdr; 

        m_dbg.verbose(CALL_INFO,1,0,"tag %#x %#x\n", tmpHdr.tag, info.tag );
        if ( ( -1 != info.tag ) && (  tmpHdr.tag != info.tag ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"node %d %d\n", tmpHdr.src, info.src );
        if ( ( -1 != info.src ) && ( tmpHdr.src != info.src ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"len %lu %lu\n", tmpHdr.len, info.len );
        if ( ( (size_t) -1 != info.len ) && ( tmpHdr.len != info.len ) ) {
            continue;
        }

        m_dbg.verbose(CALL_INFO,1,0,"group %d %d\n", tmpHdr.group, info.group );
        if ( tmpHdr.group != info.group ) {
            continue;
        }

        RecvReq* req = *iter;
        m_dbg.verbose(CALL_INFO,1,0,"matched unexpected, delay=%d\n",delay);
        m_unexpectedQ.erase( iter );
        return req;
    }

    return NULL;
}

CtrlMsg::Request* CtrlMsg::recvIODone( Request* r )
{
    RecvReq* req = static_cast<RecvReq*>(r);
    
    if ( RecvReq::RecvHdr == req->state ) {

        m_dbg.verbose(CALL_INFO,1,0,"got message from %d\n",req->nodeId);
        req->commReq = findMatch( req->hdr, req->delay );    
        req->state = RecvReq::RecvBody;

        if ( req->commReq ) {
            m_dbg.verbose(CALL_INFO,1,0,"found a posted receive\n");
        }
        if ( 0 == req->delay ) {
            return delayDone( r );
        }

    } else {
        if ( req->commReq ) {
            req->commReq->done = true;
            delete req;
        }
        req = NULL;
    }
    
    return req;
}

CtrlMsg::Request* CtrlMsg::delayDone( Request* r )
{
    RecvReq* req = static_cast<RecvReq*>(r);
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    if ( req->commReq ) {
        m_dbg.verbose(CALL_INFO,1,0,"found match\n" );
        if ( req->commReq->info->len ) {
            m_dbg.verbose(CALL_INFO,1,0,"getBody %lu bytes\n",
                                        req->commReq->info->len );
            req->ioVec.resize( req->commReq->info->ioVec.size() );
            for ( unsigned int i = 0; i < req->ioVec.size(); i++ ) {
                m_dbg.verbose(CALL_INFO,1,0,"%lu %p\n",
                                    req->commReq->info->ioVec[i].len, 
                                    req->commReq->info->ioVec[i].ptr);
                req->ioVec[i].len = req->commReq->info->ioVec[i].len;
                req->ioVec[i].ptr = req->commReq->info->ioVec[i].ptr;
            }
        } else {
            req->commReq->done = true;
            delete req;
            req = NULL;
        }
    } else {
        m_dbg.verbose(CALL_INFO,1,0, "unexpected %lu bytes\n",req->hdr.len);
        m_unexpectedQ.push_back( req ); 
        if ( req->hdr.len ) {
            req->buf.resize(req->hdr.len);
            req->ioVec[0].len = req->buf.size();
            req->ioVec[0].ptr = &req->buf[0];
        } else {
            req = NULL;
        } 
    }
    return req;
}

void CtrlMsg::recv( void* buf, size_t len, int src, int tag, int group, 
                                    CommReq* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p len=%lu src=%d\n",buf,len,src);
    
    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    recvv( ioVec, src, tag, group, req );
}

void CtrlMsg::send( void* buf, size_t len, int dest, int tag, int group, 
                                    CommReq* req )
{
    m_dbg.verbose(CALL_INFO,1,0,"buf=%p len=%lu dest=%d tag=%#x group=%d\n",
                                        buf,len,dest,tag,group);

    std::vector<IoVec> ioVec(1);
    ioVec[0].ptr = buf;
    ioVec[0].len = len;

    sendv( ioVec, dest, tag, group, req );
}

void CtrlMsg::recvv(std::vector<IoVec>& ioVec, int src, int tag,
                                                        int group, CommReq* req)
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x group=%d\n", src,tag,group);

    req->info = new RecvInfo( ioVec, src, tag, group );
    req->done = false;

    m_postedQ.push_back( req );
}

void CtrlMsg::sendv(std::vector<IoVec>& ioVec, int dest, int tag, 
                                                        int group, CommReq* req)
{
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d tag=%#x group=%d\n", dest,tag,group);
    req->info = new SendInfo( ioVec, dest, tag, group );
    req->done = false;

    m_sendQ.push_back( req );
}



void CtrlMsg::sleep()
{
    m_dbg.verbose(CALL_INFO,1,0,"sleep\n");
    m_sleep = true;
}

bool CtrlMsg::blocked()
{
    return m_sleep;
}

bool CtrlMsg::test( CommReq * req, int& delay )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( req->info );

    delay = 0;

    RecvReq* xxx;
    if (( xxx = searchUnexpected( *static_cast<RecvInfo*>(req->info), delay ) ))
    {
        m_dbg.verbose(CALL_INFO,1,0,"found match, delay=%d\n",delay);
        size_t offset = 0;
            
        for ( unsigned int i=0; i < req->info->ioVec.size(); i++ ) {
            memcpy( req->info->ioVec[i].ptr, &xxx->buf[offset], 
                    req->info->ioVec[i].len );
            offset += req->info->ioVec[i].len;

            delay += getCopyDelay( req->info->ioVec[i].len );
        }
        req->done = true;
        
        std::deque<CommReq*>::iterator iter;
        for ( iter = m_postedQ.begin(); iter != m_postedQ.end(); ++iter ) {
            if ( *iter == req ) {
                m_postedQ.erase( iter );
                break; 
            }
        }
        delete xxx;
    }

    return req->done;
}
