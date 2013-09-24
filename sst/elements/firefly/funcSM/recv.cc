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

#include "funcSM/recv.h"
#include "dataMovement.h"

using namespace SST::Firefly;

RecvFuncSM::RecvFuncSM( int verboseLevel, Output::output_location_t loc,
            Info* info, SST::Link* progressLink,   
            ProtocolAPI* dm, SST::Link* selfLink ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_toProgressLink( progressLink ),
    m_selfLink( selfLink ),
    m_dm( static_cast<DataMovement*>(dm) ),
    m_event( NULL )
{ 
    m_dbg.setPrefix("@t:RecvFuncSM::@p():@l ");
}

void RecvFuncSM::handleEnterEvent( SST::Event *e ) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:RecvFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    assert( NULL == m_event ); 
    m_event = static_cast< RecvEnterEvent* >(e);

    m_dbg.verbose(CALL_INFO,1,0,"%s buf=%p count=%d type=%d src=%d tag=%#x \n",
                m_event->entry.req ? "Irecv":"Recv", 
                m_event->entry.buf,
                m_event->entry.count,
                m_event->entry.dtype,
                m_event->entry.src,
                m_event->entry.tag );

    if ( m_event->entry.req ) {
        m_event->entry.req->src = Hermes::AnySrc;
    }

    if ( m_event->entry.resp ) {
        m_event->entry.resp->src = Hermes::AnySrc;
    }
    
    int delay;
    m_entry = m_dm->searchUnexpected( &m_event->entry, delay );

    m_dbg.verbose(CALL_INFO,1,0,"%s, match delay %d\n",
                        m_entry?"found match":"no match", delay);
    m_state = WaitMatch;
    m_selfLink->send(delay,NULL);
}

void RecvFuncSM::finish( RecvEntry* rEntry, MsgEntry* mEntry )
{
    if ( rEntry->req ) {
        rEntry->req->src = mEntry->hdr.srcRank;
        rEntry->req->tag = mEntry->hdr.tag;
    } else {
        rEntry->resp->src = mEntry->hdr.srcRank;
        rEntry->resp->tag = mEntry->hdr.tag;
    }
    delete m_entry;
    m_entry = NULL;
}

void RecvFuncSM::handleSelfEvent( SST::Event *e )
{
    if ( m_state == WaitMatch ) {
        if ( m_entry ) {
            if (!  m_entry->buffer.empty() ) { 
                memcpy( m_event->entry.buf, &m_entry->buffer[0],
                        m_entry->buffer.size() );
                m_state = WaitCopy;
                m_selfLink->send( m_dm->getCopyDelay(m_entry->buffer.size()), NULL );
                return;
            } else if ( 0 == m_entry->hdr.count ) {
                finish( &m_event->entry, m_entry );
            } else {
                m_dm->completeLongMsg( m_entry, &m_event->entry );
            }

        } else if ( m_dm->canPostRecv() ) {
            m_dm->postRecvEntry( m_event->entry );
        } else {
            assert(0);
        }    

    } else if ( m_state == WaitCopy ) {
        finish( &m_event->entry, m_entry );
    } else {
        assert(0);
    }
    m_toProgressLink->send(0, NULL );
}

void RecvFuncSM::handleProgressEvent( SST::Event *e )
{
    assert( m_event );
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_event->entry.req ? "Irecv":"Recv");
    if ( m_event->entry.resp && m_event->entry.resp->src == Hermes::AnySrc  ) {
        m_dm->sleep();
        m_toProgressLink->send(0, NULL );
    } else {
        exit( static_cast< SMEnterEvent*>(m_event), 0 );
        delete m_event;
        m_event = NULL;
    }
}
