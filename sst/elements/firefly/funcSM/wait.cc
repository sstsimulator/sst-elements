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

#include "funcSM/wait.h"
#include "dataMovement.h"

using namespace SST::Firefly;

WaitFuncSM::WaitFuncSM( int verboseLevel, Output::output_location_t loc,
                 Info* info, SST::Link* progressLink, ProtocolAPI* dm ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dm( static_cast<DataMovement*>( dm ) ),
    m_toProgressLink( progressLink )
{
    m_dbg.setPrefix("@t:WaitFuncSM::@p():@l ");
}

void WaitFuncSM::handleEnterEvent( SST::Event *e) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:WaitFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    m_dbg.verbose(CALL_INFO,1,0,"\n");

    m_event = static_cast< WaitEnterEvent* >(e);

    if ( m_event->req->src != Hermes::AnySrc ) {
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
        return;
    }

    m_toProgressLink->send(0, NULL );
}

void WaitFuncSM::handleProgressEvent( SST::Event *e )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    if ( m_event->req->src != Hermes::AnySrc ) {
        m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x\n",
                    m_event->req->src, m_event->req->tag);
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        // remove entry from m_dm 
        delete m_event;
        m_event = NULL;
        return;
    } else {
        m_dm->sleep();
    }
}
