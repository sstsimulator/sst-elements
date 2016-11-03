// Copyright 2013-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2016, Sandia Corporation
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

#include "funcSM/waitAny.h"

using namespace SST::Firefly;

WaitAnyFuncSM::WaitAnyFuncSM( SST::Params& params ) :
    FunctionSMInterface( params ),
    m_event( NULL )
{
}

void WaitAnyFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    assert( NULL == m_event );
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    m_event = static_cast< WaitAnyStartEvent* >(e);

    proto()->waitAny( m_event->count, m_event->req,
                                    m_event->index, m_event->resp );
}

void WaitAnyFuncSM::handleEnterEvent( Retval& retval )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    retval.setExit(0);
    delete m_event;
    m_event = NULL;
}
