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

#include "funcSM/send.h"

using namespace SST::Firefly;

SendFuncSM::SendFuncSM( SST::Params& params ) :
    FunctionSMInterface(params),
    m_event( NULL )
{ 
}

void SendFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    assert( NULL == m_event );
    m_event = static_cast< SendStartEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s buf=%p count=%d type=%d dest=%d tag=%#x\n",
                m_event->entry->req ? "Isend":"Send",
                m_event->entry->buf,
                m_event->entry->count,
                m_event->entry->dtype,
                m_event->entry->dest,
                m_event->entry->tag );

    SendEntry* sendEntry;

    if ( m_event->entry->req == NULL ) {
        m_state = Wait;
        sendEntry = m_event->entry;
    } else {
        m_state = Exit;
        sendEntry = new SendEntry;
        *sendEntry = *m_event->entry;

        *sendEntry->req = sendEntry;
    }

    proto()->postSendEntry( sendEntry );
}

void SendFuncSM::handleEnterEvent( Retval& retval )
{
    switch( m_state ) {
      case Wait:
        m_dbg.verbose(CALL_INFO,1,0,"waiting\n");
        proto()->wait( m_event->entry );
        m_state = Exit;
        return;

      case Exit:
        m_dbg.verbose(CALL_INFO,1,0,"done\n");
        retval.setExit(0);
        delete m_event;
        m_event = NULL;
    }
}
