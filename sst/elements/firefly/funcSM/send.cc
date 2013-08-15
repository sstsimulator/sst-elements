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
            Info* info, SST::Link*& progressLink,
            ProtocolAPI* dm, IO::Interface* io ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dm( static_cast<DataMovement*>(dm) ),
    m_toProgressLink( progressLink ),
    m_event( NULL ),
    m_io( io )
{ 
    m_dbg.setPrefix("@t:SendFuncSM::@p():@l ");
}

void SendFuncSM::handleEnterEvent( SST::Event *e) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:SendFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    assert( NULL == m_event );
    m_event = static_cast< SendEnterEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"\n");
    // isend not supported
    assert ( m_event->entry.req == NULL ); 

    if ( ! m_dm->canPostSend() ) {
        // make progress
         assert(0);
    }

    if ( m_event->entry.req ) {
        m_event->entry.req->src = Hermes::AnySrc;
    }
    m_dm->postSendEntry( m_event->entry );
    m_toProgressLink->send( 0, NULL );
}

void SendFuncSM::handleProgressEvent( SST::Event *e )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( m_event );
    exit( static_cast<SMEnterEvent*>(m_event), 0 );
    delete m_event;
    m_event = NULL;
}
