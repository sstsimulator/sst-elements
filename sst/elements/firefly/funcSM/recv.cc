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

#include "funcSM/recv.h"
#include "dataMovement.h"

using namespace SST::Firefly;

RecvFuncSM::RecvFuncSM( int verboseLevel, Output::output_location_t loc,
            Info* info, SST::Link*& progressLink,   
            ProtocolAPI* dm, IO::Interface* io, SST::Link* selfLink ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dataReadyFunctor( IO_Functor(this,&RecvFuncSM::dataReady) ),
    m_toProgressLink( progressLink ),
    m_selfLink( selfLink ),
    m_dm( static_cast<DataMovement*>(dm) ),
    m_io( io ),
    m_event( NULL )
{ 
    m_dbg.setPrefix("@t:RecvFuncSM::@p():@l ");
}

void RecvFuncSM::handleEnterEvent( SST::Event *e ) 
{
    assert( NULL == m_event ); 
    m_event = static_cast< RecvEnterEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_event->entry.req ? "Irecv":"Recv");

    if ( m_event->entry.req ) {
        m_event->entry.req->src = Hermes::AnySrc;
    }

    if ( m_event->entry.resp ) {
        m_event->entry.resp->src = Hermes::AnySrc;
    }
    
    m_entry = m_dm->searchUnexpected( &m_event->entry );

    m_state = WaitMatch;
    m_selfLink->send(10,NULL);
}

void RecvFuncSM::handleSelfEvent( SST::Event *e )
{
    if ( m_state == WaitMatch ) {
        if ( m_entry ) {
            memcpy( m_event->entry.buf, &m_entry->buffer[0],
                        m_entry->buffer.size() );
            if ( m_event->entry.req ) {
                m_event->entry.req->src = m_entry->hdr.srcRank;
                m_event->entry.req->tag = m_entry->hdr.tag;
            } else {
                m_event->entry.resp->src = m_entry->hdr.srcRank;
                m_event->entry.resp->tag = m_entry->hdr.tag;
            }

            delete m_entry; 
            m_state = WaitCopy;
            m_selfLink->send(10,NULL);
            return;
        } else if ( m_dm->canPostRecv() ) {
            m_dm->postRecvEntry( m_event->entry );
        } else {
            assert(0);
        }    

    }
    m_toProgressLink->send(0, NULL );
}

void RecvFuncSM::handleProgressEvent( SST::Event *e )
{
    assert( m_event );
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_event->entry.req ? "Irecv":"Recv");
    if ( m_event->entry.resp && m_event->entry.resp->src == Hermes::AnySrc  ) {
        m_io->setDataReadyFunc( &m_dataReadyFunctor );
    } else {
        m_io->setDataReadyFunc( NULL );
        exit( static_cast< SMEnterEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
    }
}

void RecvFuncSM::dataReady( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( m_event );
    m_io->setDataReadyFunc( NULL );
    m_toProgressLink->send(0, NULL );
}
