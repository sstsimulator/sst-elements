// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;
using namespace SST::Interfaces;

Nic::SendMachine::~SendMachine() 
{
    while ( ! m_sendQ.empty() ) {
        delete m_sendQ.front();
        m_sendQ.pop_front();
    }
}

void Nic::SendMachine::state_0( SendEntry* entry )
{
    MsgHdr hdr;

#ifdef NIC_SEND_DEBUG
    ++m_msgCount;
#endif
    hdr.op= entry->getOp();
    hdr.tag = entry->tag();
    hdr.len = entry->totalBytes();
    hdr.dst_vNicId = entry->dst_vNic();
    hdr.src_vNicId = entry->local_vNic(); 

    m_dbg.verbose(CALL_INFO,1,16,"0: setup hdr, src_vNic=%d, send dstNid=%d "
                    "dst_vNic=%d tag=%#x bytes=%lu\n", m_vc,
                    hdr.src_vNicId,entry->node(), 
                    hdr.dst_vNicId, hdr.tag, entry->totalBytes()) ;

    FireflyNetworkEvent* ev = new FireflyNetworkEvent;
    ev->bufAppend( &hdr, sizeof(hdr) );

    m_nic.schedCallback( 
        std::bind( &Nic::SendMachine::state_1, this, entry, ev ), 
        m_txDelay 
    );
}

void Nic::SendMachine::state_1( SendEntry* entry, FireflyNetworkEvent* ev ) 
{
    if ( ! canSend( m_packetSizeInBytes ) ) {
        m_dbg.verbose(CALL_INFO,2,16,"%d: send busy\n",m_vc);
        setCanSendCallback( 
            std::bind( &Nic::SendMachine::state_1, this, entry, ev ) 
        );
        return;
    } 

    m_dbg.verbose(CALL_INFO,2,16,"%d: copyOut\n",m_vc);
    copyOut( m_dbg, *ev, *entry ); 
    m_nic.dmaRead( 
        std::bind( &Nic::SendMachine::state_2, this, entry, ev ),  
        0,0
    ); 
}    

void Nic::SendMachine::state_2( SendEntry* entry, FireflyNetworkEvent *ev ) 
{
    m_dbg.verbose(CALL_INFO,2,16,"%d: send network packet\n",m_vc);
    assert( ev->bufSize() );

    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    req->dest = m_nic.IdToNet( entry->node() );
    req->src = m_nic.IdToNet( m_nic.m_myNodeId );
    req->size_in_bits = ev->bufSize() * 8;
    req->vn = 0;
    req->givePayload( ev );

    if ( (m_nic.m_tracedPkt == m_packetId || m_nic.m_tracedPkt == -2) 
                    && m_nic.m_tracedNode ==  m_nic.getNodeId() ) 
    {
        req->setTraceType( SimpleNetwork::Request::FULL );
        req->setTraceID( m_packetId );
    }
    ++m_packetId;
    m_dbg.verbose(CALL_INFO,2,16,"%d: sending event with %lu bytes\n",m_vc,
                                                        ev->bufSize());
    bool sent = m_nic.m_linkControl->send( req, m_vc );
    assert( sent );

    if ( entry->isDone() ) {
        m_dbg.verbose(CALL_INFO,1,16,"%d: send entry done\n",m_vc);
        entry->notify();
        delete entry;

        if ( ! canSend( m_packetSizeInBytes ) ) {
            m_dbg.verbose(CALL_INFO,2,16,"%d: send busy\n",m_vc);
            setCanSendCallback( 
                std::bind( &Nic::SendMachine::state_3, this ) 
            );
        } else {
            state_3();
        }

    } else {
        if ( ! canSend( m_packetSizeInBytes ) ) {
            m_dbg.verbose(CALL_INFO,2,16,"%d: send busy\n",m_vc);
            setCanSendCallback( 
                std::bind( &Nic::SendMachine::state_1, this, 
                                        entry, new FireflyNetworkEvent ) 
            );
        } else {
            state_1( entry, new FireflyNetworkEvent );
        }
    }
}    

void Nic::SendMachine::copyOut( Output& dbg,
                    FireflyNetworkEvent& event, Nic::Entry& entry )
{
    dbg.verbose(CALL_INFO,3,16,"%d: ioVec.size()=%lu\n", 
                                    m_vc, entry.ioVec().size() );

    for ( ; entry.currentVec < entry.ioVec().size() &&
                event.bufSize() <  m_packetSizeInBytes;
                entry.currentVec++, entry.currentPos = 0 ) {

        dbg.verbose(CALL_INFO,3,1,"vec[%lu].len %lu\n",entry.currentVec,
                    entry.ioVec()[entry.currentVec].len );

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = m_packetSizeInBytes - event.bufSize();
            size_t fromLen = entry.ioVec()[entry.currentVec].len -
                                                        entry.currentPos;

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,3,1,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            const char* from = 
                    (const char*) entry.ioVec()[entry.currentVec].ptr + 
                                                        entry.currentPos;

            if ( entry.ioVec()[entry.currentVec].ptr ) {
                event.bufAppend( from, len );
            } else {
                event.bufAppend( NULL, len );
            }

            entry.currentPos += len;
            if ( event.bufSize() == m_packetSizeInBytes &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len ) {
                break;
            }
        }
    }
    dbg.verbose(CALL_INFO,3,1,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
}

void Nic::SendMachine::printStatus( Output& out ) {
#ifdef NIC_SEND_DEBUG
    if ( ! m_nic.m_linkControl->spaceToSend( m_vc, packetSizeInBits() ) ) {
        out.output("%lu: %d: SendMachine `%s` msgCount=%d runCount=%d "
                    "can send %d\n",
                Simulation::getSimulation()->getCurrentSimCycle(),
                m_nic.m_myNodeId,state(m_state),m_msgCount,m_runCount,
                m_nic.m_linkControl->spaceToSend( m_vc, packetSizeInBits() ));
    }
#endif
}

#if 0
static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,1,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,1,"%#03x\n",(unsigned char)buf[i]);
    }
}
#endif
