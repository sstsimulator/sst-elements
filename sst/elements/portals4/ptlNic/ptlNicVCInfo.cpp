// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "ptlNic.h"
#include "./debug.h"

using namespace SST::Portals4;

PtlNic::VCInfo::VCInfo( PtlNic& nic ) :
    m_nic( nic ),
    m_rtrEvent( NULL )
{
    TRACE_ADD(VCinfo);
}

void PtlNic::VCInfo::setVC( int vc )
{
    m_vc = vc;
}

void PtlNic::VCInfo::addMsg( SendEntry* entry )
{
    m_sendQ.push_back( entry );
}

void PtlNic::VCInfo::process()
{
    processRtrQ();
    processSendQ();
}

void PtlNic::VCInfo::processSendQ()
{
    if ( m_sendQ.empty() ) return;
    
    SendEntry* entry = m_sendQ.front();

    if ( ! entry->hdrDone() ) {
        XXX* xxx = new XXX;
        xxx->offset = 0;
        xxx->head = true;
        xxx->tail = entry->dmaDone();
        xxx->destNid = entry->destNid();
        xxx->buf.resize( sizeof( *entry->ptlHdr() ) );
        xxx->callback = NULL;
        memcpy( &xxx->buf.at(0), entry->ptlHdr(), xxx->buf.size() );
        m_rtrQ.push_back( xxx );
        entry->setHdrDone();
    }

    while ( entry->bytesLeft() ) {
        XXX* xxx = new XXX;
        size_t size = m_nic.dmaEngine().maxXfer();

        size = entry->bytesLeft() < size ? entry->bytesLeft() : size;

        xxx->offset = 0;
        xxx->head = false;
        xxx->tail = false; 
        xxx->destNid = entry->destNid();
        xxx->buf.resize(size);
        xxx->callback = 
            new XXXCallback( this, &PtlNic::VCInfo::dmaCallback, xxx );

        m_nic.dmaEngine().read( entry->vaddr(), &xxx->buf.at(0), 
                                    xxx->buf.size(), xxx->callback ); 
        entry->incOffset( size );
    }    

    if ( entry->dmaDone() ) {
        if ( entry->callback() && (*entry->callback())() )  {
            PRINT_AT(VCInfo,"delete callback\n");
            delete entry->callback();
        }
        PRINT_AT(VCInfo,"delete SendEntry\n");
        delete entry;
        m_sendQ.pop_front();
    }
}

bool PtlNic::VCInfo::dmaCallback( XXX* xxx )
{
    PRINT_AT(VCInfo,"dma completed, move to rtrQ\n");
    m_sendQ.front()->incDmaed( xxx->buf.size() );
    if ( m_sendQ.front()->dmaDone() ) {
        xxx->tail = true;
    }
    
    m_rtrQ.push_back( xxx );
    // returning true deletes this callback
    return true;
}

void PtlNic::VCInfo::processRtrQ()
{
    while ( 1 ) {
        if ( m_rtrEvent ) {
            if ( m_nic.send2Rtr( m_rtrEvent ) ) {
                m_rtrEvent = NULL;
            } else {
                // if the rtr is busy
                return;
            } 
        }
        
        if ( m_rtrQ.empty() ) return;

        XXX* xxx = m_rtrQ.front();

        m_rtrEvent = new RtrEvent(); 
        m_rtrEvent->type = RtrEvent::Packet;
        m_rtrEvent->packet.vc = m_vc;
        m_rtrEvent->packet.srcNum = m_nic.nid();
        m_rtrEvent->packet.destNum = xxx->destNid;

        int nbytes = xxx->buf.size() - xxx->offset;
        if ( nbytes >  PKT_SIZE * sizeof(int) ) {
            nbytes = PKT_SIZE * sizeof(int);
        }

        m_rtrEvent->packet.sizeInFlits = 1 + (nbytes / 8);
        m_rtrEvent->packet.sizeInFlits += nbytes % 8 ? 1 : 0;

        PRINT_AT(VCInfo,"srcNid=%d destNid=%d\n", m_rtrEvent->packet.srcNum,
                                    m_rtrEvent->packet.destNum );
        PRINT_AT(VCInfo,"nbytes=%d flits=%d\n",
                                nbytes,m_rtrEvent->packet.sizeInFlits);

        CtrlFlit* cFlit = (CtrlFlit*) m_rtrEvent->packet.payload; 
        cFlit->s.nid = m_nic.nid();
        cFlit->s.head = xxx->head;

        if ( xxx->tail && xxx->offset + nbytes == xxx->buf.size() ) {
            cFlit->s.tail = true; 
        } else {
            cFlit->s.tail = false; 
        }

        memcpy( &m_rtrEvent->packet.payload[2], 
                            &xxx->buf.at(xxx->offset), nbytes );
        xxx->offset += nbytes;
        if ( xxx->offset == xxx->buf.size() ) {
            PRINT_AT(VCInfo,"delete XXX\n");
            m_rtrQ.pop_front();
            delete xxx;
        }
    }
}
