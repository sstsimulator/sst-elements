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

void Nic::SendMachine::initStream( SendEntryBase* entry )
{
    MsgHdr hdr;

#ifdef NIC_SEND_DEBUG
    ++m_msgCount;
#endif
    hdr.op= entry->getOp();
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

    m_nic.schedCallback( std::bind( &Nic::SendMachine::getPayload, this, entry, ev ), m_txDelay );
}

void Nic::SendMachine::getPayload( SendEntryBase* entry, FireflyNetworkEvent* ev ) 
{
    if ( ! m_inQ->isFull() ) {
	    std::vector< MemOp >* vec = new std::vector< MemOp >; 
        entry->copyOut( m_dbg, m_vc, m_packetSizeInBytes, *ev, *vec ); 
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "enque load from host, %lu bytes\n",ev->bufSize());
        if ( entry->isDone() ) {
            m_inQ->enque( vec, ev, entry->dest(), std::bind( &Nic::SendMachine::finiStream, this, entry ) );
        } else {
            m_inQ->enque( vec, ev, entry->dest() );
            m_nic.schedCallback( std::bind( &Nic::SendMachine::getPayload, this, entry, new FireflyNetworkEvent ), 1);
        }

    } else {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "blocked by host\n");
        m_inQ->wakeMeUp( std::bind( &Nic::SendMachine::getPayload, this, entry, ev ) );
    }
}
void Nic::SendMachine::finiStream( SendEntryBase* entry ) 
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_SEND_MACHINE, "stream done\n");
    if ( entry->shouldDelete() ) {
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_SEND_MACHINE, "delete enetry\n");
        delete entry;
    }
    m_sendQ.pop_front();
    if ( m_sendQ.size() ) {
        initStream( m_sendQ.front() );
    }
}

void  Nic::SendMachine::InQ::enque( std::vector< MemOp >* vec, FireflyNetworkEvent* ev, int dest, Callback callback )
{
    ++m_numPending;

    m_dbg.verbosePrefix(prefix(), CALL_INFO,2,NIC_DBG_SEND_MACHINE, "get timing for packet %" PRIu64 " size=%lu\n",
                 m_pktNum,ev->bufSize());

    m_nic.dmaRead( m_unit, vec,
		std::bind( &Nic::SendMachine::InQ::ready, this,  ev, dest, callback, m_pktNum++ )
    ); 
	// don't put code after this, the callback may be called serially
}

void Nic::SendMachine::InQ::ready( FireflyNetworkEvent* ev, int dest, Callback callback, uint64_t pktNum )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "packet %" PRIu64 " is ready, expected=%" PRIu64 "\n",pktNum,m_expectedPkt);
    assert(pktNum == m_expectedPkt++);

    if ( m_pendingQ.empty() && ! m_outQ->isFull() ) {
        ready2( ev, dest, callback );
    } else  {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "blocked by OutQ\n");
        m_pendingQ.push_back( Entry( ev, dest,callback,pktNum ) );
        if ( 1 == m_pendingQ.size() )  {
            m_outQ->wakeMeUp( std::bind( &Nic::SendMachine::InQ::processPending, this ) );
        }
    }
}

void Nic::SendMachine::InQ::ready2( FireflyNetworkEvent* ev, int dest, Callback callback )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "pass packet to OutQ numBytes=%lu\n", ev->bufSize() );
    --m_numPending;
    m_outQ->enque( ev, dest );
    if ( callback ) {
        callback();
    }
    if ( m_callback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "wakeup send machine\n");
        m_callback( );
        m_callback = NULL;
    }
}

void Nic::SendMachine::InQ::processPending( )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "size=%lu\n", m_pendingQ.size());
    Entry& entry = m_pendingQ.front();
    ready2( entry.ev, entry.dest, entry.callback );

    m_pendingQ.pop_front();
    if ( ! m_pendingQ.empty() ) {
        if ( ! m_outQ->isFull() ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "schedule next\n");
            m_nic.schedCallback( std::bind( &Nic::SendMachine::InQ::processPending, this ) );
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "schedule wakeup\n");
            m_outQ->wakeMeUp( std::bind( &Nic::SendMachine::InQ::processPending, this ) );
        }
    }
}

void Nic::SendMachine::OutQ::enque( FireflyNetworkEvent* ev, int dest )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "size=%lu\n", m_queue.size());
    m_queue.push_back( std::make_pair(ev,dest) );
    if ( ! m_notifyCallback && ! m_scheduled && canSend( ev->bufSize() ) )  {
        runSendQ();
    }
}

void Nic::SendMachine::OutQ::runSendQ(  ) 
{
    m_scheduled = false;
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "queue size=%lu\n",m_queue.size());
    assert( ! m_queue.empty() );

    std::pair< FireflyNetworkEvent*, int>& entry = m_queue.front();

    if ( canSend( entry.first->bufSize() ) ) {
        send( entry );
        m_queue.pop_front();
    } else {
        //assert(0);
        return;
    }

    if ( ! m_queue.empty() ) {
        std::pair< FireflyNetworkEvent*, int>& entry = m_queue.front();
        if ( canSend( entry.first->bufSize() ) ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "schedule runSendQ()\n");
            m_scheduled = true;
            m_nic.schedCallback( std::bind( &Nic::SendMachine::OutQ::runSendQ, this ),1);
        } 
    }

    if ( m_wakeUpCallback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "call wakeup callback\n");
        m_nic.schedCallback( m_wakeUpCallback, 1);
        m_wakeUpCallback = NULL;
    }
}

void Nic::SendMachine::OutQ::send( std::pair< FireflyNetworkEvent*, int>& entry ) 
{
    FireflyNetworkEvent* ev = entry.first;
    assert( ev->bufSize() );

    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    req->dest = m_nic.IdToNet( entry.second );
    req->src = m_nic.IdToNet( m_nic.m_myNodeId );
    req->size_in_bits = ev->bufSize() * 8;
    req->vn = 0;
    req->givePayload( ev );

    if ( (m_nic.m_tracedPkt == m_packetId || m_nic.m_tracedPkt == -2) 
                   && m_nic.m_tracedNode ==  m_nic.getNodeId() ) 
    {
        req->setTraceType( SimpleNetwork::Request::FULL );
        req->setTraceID( m_packetId );
        ++m_packetId;
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE,
					"dst=%" PRIu64 " sending event with %zu bytes packetId=%" PRIu64 "\n",req->dest,
                                                        ev->bufSize(), (uint64_t)m_packetId);
    bool sent = m_nic.m_linkControl->send( req, m_vc );
    assert( sent );
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
