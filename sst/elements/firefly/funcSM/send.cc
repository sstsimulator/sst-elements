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

#include "funcSM/send.h"
#include "dataMovement.h"

using namespace SST::Firefly;

SendFuncSM::SendFuncSM( int verboseLevel, Output::output_location_t loc,
            Info* info, ProtocolAPI* dm ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dm( static_cast<DataMovement*>(dm) ),
    m_event( NULL )
{ 
    m_dbg.setPrefix("@t:SendFuncSM::@p():@l ");
}

void SendFuncSM::handleStartEvent( SST::Event *e) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:SendFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    assert( NULL == m_event );
    m_event = static_cast< SendStartEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s buf=%p count=%d type=%d dest=%d tag=%#x\n",
                m_event->entry.req ? "Isend":"Send",
                m_event->entry.buf,
                m_event->entry.count,
                m_event->entry.dtype,
                m_event->entry.dest,
                m_event->entry.tag );

    // isend not supported
    assert ( m_event->entry.req == NULL ); 

    if ( m_event->entry.req == NULL ) {
        m_event->entry.req = &m_req;
        m_req.src = Hermes::AnySrc;
    }

    if ( ! m_dm->canPostSend() ) {
        // make progress
         assert(0);
    }

    if ( m_event->entry.req ) {
        m_event->entry.req->src = Hermes::AnySrc;
    }
    m_dm->postSendEntry( m_event->entry );
    m_dm->enter();
}

void SendFuncSM::handleEnterEvent( SST::Event *e )
{
    assert( m_event );
    if (  m_event->entry.req == NULL && m_req.src == Hermes::AnySrc ) {
        m_dbg.verbose(CALL_INFO,1,0,"not ready\n");
        m_dm->sleep();
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"done\n");
        exit( static_cast<SMStartEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
    }
}
