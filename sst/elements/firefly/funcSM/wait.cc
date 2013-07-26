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

#include "funcSM/wait.h"
#include "dataMovement.h"

using namespace SST::Firefly;

WaitFuncSM::WaitFuncSM( int verboseLevel, Output::output_location_t loc,
                 Info* info, SST::Link*& progressLink,
                 ProtocolAPI* dm, IO::Interface* io ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dataReadyFunctor( IO_Functor(this,&WaitFuncSM::dataReady) ),
    m_dm( static_cast<DataMovement*>( dm ) ),
    m_toProgressLink( progressLink ),
    m_io( io )
{
    m_dbg.setPrefix("@t:WaitFuncSM::@p():@l ");
}

void WaitFuncSM::handleEnterEvent( SST::Event *e) 
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    m_event = static_cast< WaitEnterEvent* >(e);

    if ( m_event->req->src != Hermes::AnyTag ) {
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
    if ( m_event->req->src != Hermes::AnyTag ) {
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        // remove entry from m_dm 
        delete m_event;
        m_event = NULL;
        return;
    } else {
        m_io->setDataReadyFunc( &m_dataReadyFunctor );
    }
}

void WaitFuncSM::dataReady( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( m_event );
    m_io->setDataReadyFunc( NULL );
    m_toProgressLink->send(0, NULL );
}

