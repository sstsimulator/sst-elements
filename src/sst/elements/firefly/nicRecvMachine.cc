// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

void Nic::RecvMachine::processPkt( FireflyNetworkEvent* ev ) {

	m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE," got a network pkt from node=%d pid=%d for pid=%d size=%zu\n",
                        ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev->bufSize() );

    if ( ev->isCtrl() ) {
		m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"got a control message\n");
		m_ctxMap[ ev->getDestPid() ]->newStream( ev );
    } else {
        processStdPkt( ev );
    }
}

void Nic::RecvMachine::processStdPkt( FireflyNetworkEvent* ev ) {
    bool blocked = false;
    ProcessPairId id = getPPI( ev );

    StreamBase* stream;
    int pid = ev->getDestPid();

    if ( ev->isHdr() ) {

        if ( m_streamMap.find( id ) != m_streamMap.end() ) {
            m_dbg.fatal(CALL_INFO,-1,"no stream for cnode=%d pid=%d for pid=%d\n",ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid());
        }
        stream = m_ctxMap[pid]->newStream( ev );
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"new stream %p %s node=%d pid=%d for pid=%d\n",stream, ev->isTail()? "single packet stream":"",
               ev->getSrcNode(),ev->getSrcPid(), pid );

        if ( ! ev->isTail() ) {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"multi packet stream, set streamMap PPI=0x%" PRIx64 "\n",id );
            m_streamMap[id] = stream;
        }

    } else {
        assert ( m_streamMap.find( id ) != m_streamMap.end() );

        stream = m_streamMap[id];

        if ( ev->isTail() ) {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"tail pkt, clear streamMap PPI=0x%" PRIx64 "\n",id );
            m_streamMap.erase(id);
        } else {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"body packet stream=%p\n",stream );
        }

        if ( stream->isBlocked() ) {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"stream is blocked stream=%p\n",stream );
            stream->setWakeup(
                [=]() {
                    m_dbg.debug(CALL_INFO_LAMBDA,"processStdPkt",1,NIC_DBG_RECV_MACHINE,"stream is unblocked stream=%p\n",stream );
                    stream->processPktBody( ev );
                }
            );
        } else {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n" );
            stream->processPktBody(ev);
        }
    }
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
