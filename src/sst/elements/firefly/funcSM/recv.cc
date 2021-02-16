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

    m_dbg.debug(CALL_INFO,1,0,"%s buf=%p count=%d type=%d src=%d tag=%#x \n",
                m_event->req ? "Irecv":"Recv",
                &m_event->buf,
                m_event->count,
                m_event->dtype,
                m_event->src,
                m_event->tag );

    if ( m_event->req == NULL ) {
		proto()->recv( m_event->buf, m_event->count, m_event->dtype, m_event->src,
			m_event->tag, m_event->group, m_event->resp );
	} else {
		proto()->irecv( m_event->buf, m_event->count, m_event->dtype, m_event->src,
			m_event->tag, m_event->group, m_event->req );
	}
}

void RecvFuncSM::handleEnterEvent( Retval& retval )
{
	retval.setExit(0);
    delete m_event;
    m_event = NULL;
	retval.setExit(0);
}
