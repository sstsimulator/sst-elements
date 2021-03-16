// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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

    m_dbg.debug(CALL_INFO,1,0,"%s buf=%p count=%d type=%d dest=%d tag=%#x\n",
                m_event->req ? "Isend":"Send",
                &m_event->buf,
                m_event->count,
                m_event->dtype,
                m_event->dest,
                m_event->tag );

	if ( NULL == m_event->req ) {
		proto()->send( m_event->buf, m_event->count, m_event->dtype, m_event->dest,
			m_event->tag, m_event->group );
	} else {
		proto()->isend( m_event->buf, m_event->count, m_event->dtype, m_event->dest,
			m_event->tag, m_event->group, m_event->req );
	}
}

void SendFuncSM::handleEnterEvent( Retval& retval )
{
    delete m_event;
    m_event = NULL;
    retval.setExit(0);
}
