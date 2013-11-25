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

#include "funcSM/recv.h"

using namespace SST::Firefly;

RecvFuncSM::RecvFuncSM( SST::Params& params ) :
    FunctionSMInterface(params),
    m_event( NULL )
{ 
}

void RecvFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    assert( NULL == m_event ); 
    m_event = static_cast< RecvStartEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s buf=%p count=%d type=%d src=%d tag=%#x \n",
                m_event->entry->req ? "Irecv":"Recv", 
                m_event->entry->buf,
                m_event->entry->count,
                m_event->entry->dtype,
                m_event->entry->src,
                m_event->entry->tag );

    RecvEntry* recvEntry;

    // if blocking recv 
    if ( m_event->entry->req == NULL ) {

        m_state = Wait;
        recvEntry = m_event->entry;

    } else {

        m_state = Exit;

        recvEntry = new RecvEntry;
        *recvEntry = *m_event->entry; 

        recvEntry->resp = new Hermes::MessageResponse;

        *recvEntry->req = recvEntry;
    }

    proto()->postRecvEntry( recvEntry );
}

void RecvFuncSM::handleEnterEvent( Retval& retval )
{
    switch( m_state ) {
      case Wait:
        m_dbg.verbose(CALL_INFO,1,0,"waiting\n");
        proto()->wait( m_event->entry, m_event->entry->resp );
        m_state = Exit;
        return;

      case Exit:
        m_dbg.verbose(CALL_INFO,1,0,"done\n");
        retval.setExit(0);
        delete m_event;
        m_event = NULL;
    }
}
