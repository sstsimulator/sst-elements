// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "funcSM/commSplit.h"
#include "info.h"

using namespace SST::Firefly;

CommSplitFuncSM::CommSplitFuncSM( SST::Params& params ) :
    FunctionSMInterface( params ),
    m_event( NULL )
{
}

void CommSplitFuncSM::handleStartEvent( SST::Event *e, Retval& retval ) 
{
    assert( NULL == m_event );

    m_event = static_cast< CommSplitStartEvent* >(e);

    assert(0);

#if 0
    Hermes::Communicator newGroup = m_info->addGroup(); 
    m_dbg.verbose(CALL_INFO,1,0,"newGroup=%d\n",newGroup);
    *m_event->newComm = newGroup; 

    retval.setExit(0);
#endif
}

void CommSplitFuncSM::handleEnterEvent( Retval& retval )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    retval.setExit(0);
    delete m_event;
    m_event = NULL;
}
