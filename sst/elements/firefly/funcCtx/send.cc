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
#include "funcCtx/send.h"
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

SendCtx::SendCtx( Addr target, uint32_t count, PayloadDataType dtype,
        RankID source, uint32_t tag, Communicator group,
        MessageRequest* req, 
        Functor* retFunc, FunctionType type, Hades* obj ) :
    FunctionCtx( retFunc, type, obj ),
    m_state( RunProgress ),
    m_target( target ),
    m_count( count ),
    m_dtype( dtype ),
    m_source( source ),
    m_tag( tag ),
    m_group( group ),
    m_req( req )
{ 
    DBGX("target=%p count=%d dtype=%d source=%d tag=%d group=%d %p\n",
        m_target, m_count, m_dtype, m_source, m_tag, m_group, m_req );

    m_sendEntry = new SendEntry( m_target, m_count, m_dtype, m_source,
           m_tag, m_group, m_req, m_obj->m_groupMap[m_group]->getMyRank(),
            m_retFunc, obj->sizeofDataType( dtype) );
}

SendCtx::~SendCtx()
{
    delete m_sendEntry;
}

bool SendCtx::run( ) 
{
    bool retval = false;

    switch ( m_state ) {
    case RunProgress:
        DBGX( "RunProgress\n" );
        m_obj->runProgress(this);
        m_state = PrePost;
        break;

    case PrePost:
        DBGX( "PrePost\n" );
        if ( ! m_obj->canPostSend() ) {
            // need to implement blocked on full Q
            _abort( SendCtx, "send Q is full\n" );
        }
        m_obj->postSendEntry( m_sendEntry );
        m_obj->runProgress( this );

        // there is no such thing as an Isend, send ctx must block will send
        // happens
        m_state = Wait;
        break;

    case Wait:
        DBGX( "Wait\n" );
        retval = true;
        break;
    }
    return retval;
}
