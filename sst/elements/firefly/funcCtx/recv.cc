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
    m_posted( false )
{ 
    if ( m_req ) {
        m_req->src = AnySrc; 
    }
}

void RecvCtx::runPre( ) 
{
    DBGX("\n");

    if ( m_obj->canPostRecv() ) {
        m_obj->postRecvEntry( m_target, m_count, m_dtype, m_source,
                m_tag, m_group, m_resp, m_req, m_retFunc );

        m_posted = true;
    }
}

bool RecvCtx::runPost( )
{
    DBGX("\n");

    if ( m_posted && ( m_type == Irecv || m_req->src != AnySrc ) ) {
        return false;
    } else {
        return true;
    }
 }
