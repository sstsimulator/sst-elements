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

#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "ioapi.h"
#include "dataMovement.h"
#include "entry.h"
#include "info.h"
#include "sst/elements/hermes/msgapi.h"

using namespace SST::Firefly;
using namespace SST;

DataMovement::DataMovement(int verboseLevel, Output::output_location_t loc,
        Info* info ) :
    ProtocolAPI(verboseLevel,loc),
    m_info(info)
{
    m_dbg.setPrefix("@t:DataMovement::@p():@l ");
}

DataMovement::RecvReq* DataMovement::getRecvReq( IO::NodeId src )
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

DataMovement::SendReq* DataMovement::getSendReq(  )
{
    if ( ! m_sendQ.empty() ) {
        SendReq* req = new SendReq;
        req->entry = m_sendQ.front(); 
        m_sendQ.pop_front();

        Hdr& hdr = req->hdr;
        
        hdr.tag     = req->entry->tag;
        hdr.count   = req->entry->count;
        hdr.dtype   = req->entry->dtype;
        hdr.srcRank = m_info->getGroup(req->entry->group)->getMyRank();
        hdr.group   = req->entry->group;

        req->ioVec.resize(2);
        req->ioVec[0].ptr = &hdr;
        req->ioVec[0].len = sizeof(hdr);
        req->ioVec[1].ptr = req->entry->buf;
        req->ioVec[1].len = req->entry->count * 
                        m_info->sizeofDataType(req->entry->dtype);

        req->nodeId = m_info->rankToNodeId(req->entry->group, req->entry->dest);
 
        return req;  
    }

    m_dbg.verbose(CALL_INFO,1,0,"\n");
    return NULL;
}

DataMovement::SendReq* DataMovement::sendIODone( Request *req )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    delete req;
    return NULL;
}

DataMovement::RecvReq* DataMovement::recvIODone( Request *r )
{
    RecvReq* req = static_cast<RecvReq*>(r);
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    if ( RecvReq::RecvHdr == req->state ) { 
        m_dbg.verbose(CALL_INFO,1,0,"srcNodeId=%d count=%d dtype=%d tag=%#x\n",
            req->nodeId,
            req->hdr.count,
            req->hdr.dtype,
            req->hdr.tag );

        req->recvEntry = findMatch( req->hdr );
    
        if ( req->recvEntry ) {
            m_dbg.verbose(CALL_INFO,1,0,"found a posted receive\n");
        }
        req->delay = 10;
        req->state = RecvReq::RecvBody;
    } else {
        finishRecv(r);
        req = NULL;
    }

    return req;
}

DataMovement::Request* DataMovement::delayDone( Request *r )
{
    RecvReq* req = static_cast<RecvReq*>(r);
    if ( 0 == req->hdr.count ) {
        m_dbg.verbose(CALL_INFO,1,0,"no body\n");
        req->msgEntry = new MsgEntry;
        req->msgEntry->hdr       = req->hdr;
        req->msgEntry->srcNodeId = req->nodeId;
        finishRecv( r );
        req = NULL;
    } else {

        req->ioVec.resize(1);
        if ( req->recvEntry ) {
            req->ioVec[0].len = req->recvEntry->count *     
                        m_info->sizeofDataType(req->recvEntry->dtype);
            req->ioVec[0].ptr = req->recvEntry->buf;
        } else {
            req->ioVec[0].len = req->hdr.count * 
                        m_info->sizeofDataType(req->hdr.dtype);
    
            req->msgEntry = new MsgEntry;
            req->msgEntry->buffer.resize( req->ioVec[0].len );
            req->ioVec[0].ptr = &req->msgEntry->buffer[0];
            req->msgEntry->hdr = req->hdr;
            req->msgEntry->srcNodeId = req->nodeId;
        }
    }
    return req;
}

void  DataMovement::finishRecv( Request* r )
{
    RecvReq* req = static_cast<RecvReq*>(r);

    if (  req->recvEntry )  {

        if ( req->recvEntry->req ) {
            m_dbg.verbose(CALL_INFO,1,0,"finish up non blocking match\n");
            req->recvEntry->req->tag = req->hdr.tag;  
            req->recvEntry->req->src = req->hdr.srcRank;  
        } else if ( req->recvEntry->resp ) {
            m_dbg.verbose(CALL_INFO,1,0,"finish up blocking match\n");
            req->recvEntry->resp->tag = req->hdr.tag;  
            req->recvEntry->resp->src = req->hdr.srcRank;  
        } 

        delete req; 

    }  else {
        m_dbg.verbose(CALL_INFO,1,0,"added to unexpected Q\n");
        // no match push on the unexpected Q
        // XXXXXXXXXXXXXXXX  where do limit the size of the unexpected Q
        m_unexpectedMsgQ.push_back( req->msgEntry );
    }
}

bool DataMovement::canPostSend()
{
    return ( m_sendQ.size() < MaxNumSends );
}

void DataMovement::postSendEntry( SendEntry entry )
{
    SendEntry* tmp = new SendEntry;
    *tmp = entry; 
    m_sendQ.push_back( tmp );
}

bool DataMovement::canPostRecv()
{
    return ( m_postedQ.size() < MaxNumPostedRecvs );
}

void DataMovement::postRecvEntry( RecvEntry entry )
{
    RecvEntry* tmp = new RecvEntry;
    *tmp = entry;
    m_postedQ.push_back( tmp );
}
MsgEntry* DataMovement::searchUnexpected( RecvEntry* entry )
{
    m_dbg.verbose(CALL_INFO,1,0,"unexpected size %lu\n",
                                            m_unexpectedMsgQ.size() );
    std::deque< MsgEntry* >::iterator iter = m_unexpectedMsgQ.begin();
    for ( ; iter != m_unexpectedMsgQ.end(); ++iter  ) {

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
        m_unexpectedMsgQ.erase( iter );
        return *iter;
    }
    
    return NULL;
}

RecvEntry* DataMovement::findMatch( Hdr& hdr )
{
    m_dbg.verbose(CALL_INFO,1,0,"posted size %lu\n", m_postedQ.size() );
    std::deque< RecvEntry* >::iterator iter = m_postedQ.begin(); 
    for ( ; iter != m_postedQ.end(); ++iter  ) {
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
