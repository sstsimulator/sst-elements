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

Nic::RecvMachine::StreamBase::StreamBase(Output& output, Ctx* ctx, int unit, int srcNode, int srcPid, int myPid ) :
    m_dbg(output), m_ctx(ctx), m_unit(unit), m_srcNode(srcNode), m_srcPid(srcPid), m_myPid( myPid ),
    m_recvEntry(NULL),m_sendEntry(NULL), m_numPending(0), m_pktNum(0), m_expectedPkt(0), m_blockedNeedRecv(NULL),
    m_startDelay(0), m_startCallback(NULL)
{
    m_prefix = "@t:"+ std::to_string(ctx->nic().getNodeId()) +":Nic::RecvMachine::StreamBase::@p():@l ";
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"this=%p\n",this);
    m_start = m_ctx->nic().getCurrentSimTimeNano();
    assert( unit >= 0 );
}

Nic::RecvMachine::StreamBase::~StreamBase() {
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"this=%p latency=%" PRIu64 "\n",this,
                                            m_ctx->nic().getCurrentSimTimeNano()-m_start);
    if ( m_recvEntry ) {
        m_recvEntry->notify( m_srcPid, m_srcNode, m_matched_tag, m_matched_len );
        delete m_recvEntry;
    }
    if ( m_sendEntry ) {
        m_ctx->nic().m_sendMachine[0]->run( m_sendEntry );
    }
}

bool Nic::RecvMachine::StreamBase::isBlocked()    {
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"%d\n",m_numPending == m_ctx->getMaxQsize());
    return m_numPending == m_ctx->getMaxQsize();
}

void Nic::RecvMachine::StreamBase::needRecv( FireflyNetworkEvent* ev ) {
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
    m_blockedNeedRecv = ev;
    m_ctx->needRecv( this );
}

void Nic::RecvMachine::StreamBase::processPkt( FireflyNetworkEvent* ev  ) {

    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"get timing for packet %" PRIu64 " size=%lu\n",
                                                m_pktNum,ev->bufSize());
    assert(m_numPending < m_ctx->getMaxQsize() );
    ++m_numPending;

    std::vector< MemOp >* vec = new std::vector< MemOp >;
    bool ret = getRecvEntry()->copyIn( m_dbg, *ev, *vec );

    if ( 0 == ev->bufSize() ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "network event is done\n");
        delete ev;
    }

    Callback callback = NULL;
    bool finished = ret || length() == getRecvEntry()->currentLen();

    m_ctx->nic().dmaWrite( m_unit, m_myPid, vec, 
            std::bind( &Nic::RecvMachine::StreamBase::ready, this, finished, m_pktNum++ ) );
}

void Nic::RecvMachine::StreamBase::ready( bool finished, uint64_t pktNum ) {
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "packet %" PRIu64 " is ready\n",pktNum);
    assert(pktNum== m_expectedPkt++);
    --m_numPending;

    if ( finished ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "this stream is done\n");
        m_ctx->clearMapAndDeleteStream( this );
    }

    if ( m_wakeupCallback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE, "wakeup recv machine\n");
        m_wakeupCallback( );
        m_wakeupCallback = NULL;
    }
}

bool Nic::RecvMachine::StreamBase::postedRecv( DmaRecvEntry* entry ) {
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
    if ( ! m_blockedNeedRecv ) return false;

    FireflyNetworkEvent* event = m_blockedNeedRecv;

    MsgHdr& hdr = *(MsgHdr*) event->bufPtr();
    MatchMsgHdr& matchHdr = *(MatchMsgHdr*) ((unsigned char*)event->bufPtr() + sizeof( MsgHdr ));

    int srcNode = event->getSrcNode();

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"event tag %#x, posted recv tag %#x\n",matchHdr.tag, entry->tag());
    if ( entry->tag() != matchHdr.tag ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"did't match tag\n");
        return false;
    }

    if ( entry->node() != -1 && entry->node() != srcNode ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "didn't match node  want=%#x src=%#x\n", entry->node(), srcNode );
        return false;
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < matchHdr.len ) {
        assert(0);
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "matched\n");

    m_recvEntry = entry;
    event->bufPop( sizeof(MsgHdr) + sizeof(MatchMsgHdr));

    processPkt( m_blockedNeedRecv );
    m_blockedNeedRecv = NULL;
    return true;
}
