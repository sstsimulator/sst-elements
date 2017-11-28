// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#if 0
    while ( ! m_sendQ.empty() ) {
        delete m_sendQ.front();
        m_sendQ.pop_front();
    }
#endif
}

void Nic::SendMachine::state_0( SendEntryBase* entry )
{
    MsgHdr hdr;

#ifdef NIC_SEND_DEBUG
    ++m_msgCount;
#endif
    hdr.op= entry->getOp();
    hdr.len = entry->totalBytes();
    hdr.dst_vNicId = entry->dst_vNic();
    hdr.src_vNicId = entry->local_vNic(); 

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_SEND_MACHINE,
					"setup hdr, src_vNic=%d, send dstNid=%d "
                    "dst_vNic=%d bytes=%lu\n",
                    hdr.src_vNicId, entry->dest(), 
                    hdr.dst_vNicId, entry->totalBytes() ) ;

    FireflyNetworkEvent* ev = new FireflyNetworkEvent;

    ev->bufAppend( &hdr, sizeof(hdr) );
    ev->bufAppend( entry->hdr(), entry->hdrSize() );

    m_nic.schedCallback( 
        std::bind( &Nic::SendMachine::state_1, this, entry, ev ), 
        m_txDelay 
    );
}

void Nic::SendMachine::state_1( SendEntryBase* entry, FireflyNetworkEvent* ev ) 
{
    if ( ! canSend( m_packetSizeInBytes ) ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "send busy\n");
        setCanSendCallback( 
            std::bind( &Nic::SendMachine::state_1, this, entry, ev ) 
        );
        return;
    } 

	std::vector< MemOp >* vec = new std::vector< MemOp >; 

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "copyOut\n");

    entry->copyOut( m_dbg, m_vc, m_packetSizeInBytes, *ev, *vec ); 

    m_nic.dmaRead( vec,
		std::bind( &Nic::SendMachine::state_2, this, entry, ev )
    ); 
	// don't put code after this, the callback may be called serially
}    

void Nic::SendMachine::state_2( SendEntryBase* entry, FireflyNetworkEvent *ev ) 
{
    assert( ev->bufSize() );

    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    req->dest = m_nic.IdToNet( entry->dest() );
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
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE,
					"dst=%" PRIu64 " sending event with %zu bytes packetId=%lu\n",req->dest,
                                                        ev->bufSize(), (uint64_t)m_packetId);
    ++m_packetId;
    bool sent = m_nic.m_linkControl->send( req, m_vc );
    assert( sent );

    if ( entry->isDone() ) {
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_SEND_MACHINE, "send entry done\n");
        if ( entry->shouldDelete() ) {
            delete entry;
        } 

        if ( ! canSend( m_packetSizeInBytes ) ) {
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "send busy\n");
            setCanSendCallback( 
                std::bind( &Nic::SendMachine::state_3, this ) 
            );
        } else {
            state_3();
        }

    } else {
        if ( ! canSend( m_packetSizeInBytes ) ) {
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "send busy\n");
            setCanSendCallback( 
                std::bind( &Nic::SendMachine::state_1, this, 
                                        entry, new FireflyNetworkEvent ) 
            );
        } else {
            state_1( entry, new FireflyNetworkEvent );
        }
    }
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
