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

#include "entry.h"
#include "funcCtx/recv.h"
#include "hades.h"

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

using namespace SST::Firefly;
using namespace Hermes;

RecvCtx::RecvCtx( Addr target, uint32_t count, PayloadDataType dtype,
                RankID source, uint32_t tag, Communicator group,
                MessageResponse* resp, MessageRequest* req,
                Functor* retFunc, FunctionType type, Hades* obj ) :
    FunctionCtx( retFunc, type, obj ),
    m_target( target ),
    m_count( count ),
    m_dtype( dtype ),
    m_source( source ),
    m_tag( tag ),
    m_group( group ),
    m_resp( resp ),
    m_req( req ),
    m_state( RunProgress )
{ 
    DBGX("target=%p count=%d dtype=%d source=%d tag=%d group=%d %p %p\n",
        m_target, m_count, m_dtype, m_source, m_tag, m_group, m_req, m_resp );

    if ( m_req ) {
        m_req->src = AnySrc; 
    }

    m_recvEntry = new RecvEntry(m_target, m_count, m_dtype, m_source,
                m_tag, m_group, m_resp, m_req, m_retFunc);
}

RecvCtx::~RecvCtx()
{
    delete m_recvEntry;
}

bool RecvCtx::run( ) 
{
    bool retval = false;
    switch( m_state ) {
    case RunProgress:
        DBGX( "RunProgress %p\n", this );
        m_obj->runProgress(this);
        m_state = PrePost;
        break;

    case PrePost:
        DBGX( "PrePost\n" );
        m_recvEntry->msgEntry = m_obj->searchUnexpected( m_recvEntry );
        if ( m_recvEntry->msgEntry ) {
            DBGX("found match in unexpected list\n");
            m_state = WaitMatch;
            m_obj->sendCtxDelay( 10, this ); 
        } else if ( m_obj->canPostRecv() ) {
            m_obj->postRecvEntry(m_recvEntry);
            retval = true;
        } else {
            _abort(RecvCtc,"receive Q is full\n");
        }
        break; 
    case WaitMatch:
        DBGX("WaitMatch\n");

        memcpy( m_recvEntry->buf, &m_recvEntry->msgEntry->buffer[0],
                        m_recvEntry->msgEntry->buffer.size() );
        if ( m_type == Irecv ) {
            m_recvEntry->req->src = m_recvEntry->msgEntry->hdr.srcRank;
            m_recvEntry->req->tag = m_recvEntry->msgEntry->hdr.tag;
        } else {
            m_recvEntry->resp->src = m_recvEntry->msgEntry->hdr.srcRank;
            m_recvEntry->resp->tag = m_recvEntry->msgEntry->hdr.tag;
        }
        delete m_recvEntry->msgEntry;
        m_obj->sendCtxDelay( 10, this ); 
        m_state = WaitCopy;
        break;
    case WaitCopy:
        DBGX("WaitCopy\n");
        retval = true;
        break;
    }
    return retval;
}
