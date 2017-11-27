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

static void print( Output& dbg, char* buf, int len );

Nic::RecvMachine::~RecvMachine()
{
// for some reason the causes a fault when 
// testSuite_scheduler_DetailedNetwork.sh
// terminates a simulation
#if 0
    while ( ! m_streamMap.empty() ) {
        delete m_streamMap.begin()->second;
        m_streamMap.erase( m_streamMap.begin() );
    }
#endif
}

void Nic::RecvMachine::state_0( FireflyNetworkEvent* ev )
{
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"got a network pkt from %d\n",ev->src);
    assert( ev );

    // is there an active stream for this src node?
    if ( m_streamMap.find( ev->src ) == m_streamMap.end() ) {

#ifdef NIC_RECV_DEBUG
        ++m_msgCount;
#endif
        MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();
        switch ( hdr.op ) {
          case MsgHdr::Msg:
            m_streamMap[ev->src] = new MsgStream( m_dbg, ev, *this );
            break;
          case MsgHdr::Rdma:
            m_streamMap[ev->src] = new RdmaStream( m_dbg, ev, *this );
            break;
          case MsgHdr::Shmem:
            m_streamMap[ev->src] = new ShmemStream( m_dbg, ev, *this );
            break;
        }
    } else {
        m_streamMap[ev->src]->processPkt( ev );
    } 
}

// Need Recv
void Nic::RecvMachine::state_2( FireflyNetworkEvent* ev )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");
    processNeedRecv( 
        ev,
        std::bind( &Nic::RecvMachine::state_0, this, ev )
    );
}

void Nic::RecvMachine::checkNetwork( )
{
    FireflyNetworkEvent* ev = getNetworkEvent(m_vc);
    if ( ev ) {
        state_0( ev );
    } else {
        setNotify();
    }
}

void Nic::RecvMachine::state_move_0( FireflyNetworkEvent* event, StreamBase* stream  )
{
    int src = event->src;
    EntryBase* entry = stream->getRecvEntry();

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
				"src=%d event has %lu bytes\n", src,event->bufSize() );

    std::vector< MemOp >* vec = new std::vector< MemOp >;

    long tmp = event->bufSize();
    bool ret = stream->getRecvEntry()->copyIn( m_dbg, *event, *vec );

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
				"copyIn %lu bytes %s\n", tmp - event->bufSize(), ret ? "stream is done":"");

    m_nic.dmaWrite( vec,
            std::bind( &Nic::RecvMachine::state_move_1, this, event, ret, stream ) ); 

	// don't put code after this, the callback may be called serially
}

void Nic::RecvMachine::state_move_1( FireflyNetworkEvent* event, bool done, StreamBase* stream )
{
    int src = event->src;
    EntryBase* entry = stream->getRecvEntry();

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
		"src=%d currentLen %lu bytes\n", src, entry->currentLen());

    if ( done || stream->length() == entry->currentLen() ) {

        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
				"recv entry done, srcNic=%d\n", src );

        delete m_streamMap[src];
        m_streamMap.erase(src);

        if ( ! event->bufEmpty() ) {
            m_dbg.fatal(CALL_INFO,-1,
                "src=%d buffer not empty, %lu bytes remain\n",
										src,event->bufSize());
        }
    }

    if ( 0 == event->bufSize() ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,
				"network event is done\n");
        delete event;
    }

    checkNetwork();
}

void Nic::RecvMachine::state_move_2( FireflyNetworkEvent* event )
{
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
    delete m_streamMap[event->src];
    m_streamMap.erase(event->src);
    delete event;
    checkNetwork();
}


void Nic::RecvMachine::printStatus( Output& out ) {
#ifdef NIC_RECV_DEBUG
    if ( m_nic.m_linkControl->requestToReceive( 0 ) ) {
        out.output( "%lu: %d: RecvMachine `%s` msgCount=%d runCount=%d,"
            " net event avail %d\n",
            Simulation::getSimulation()->getCurrentSimCycle(),
            m_nic. m_myNodeId, state( m_state), m_msgCount, m_runCount,
            m_nic.m_linkControl->requestToReceive( 0 ) );
    }
#endif
}
