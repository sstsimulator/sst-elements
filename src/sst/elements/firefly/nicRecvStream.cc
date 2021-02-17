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

Nic::RecvMachine::StreamBase::StreamBase(Output& output, Ctx* ctx, int srcNode, int srcPid, int myPid ) :
    m_dbg(output), m_ctx(ctx), m_unit(-1), m_srcNode(srcNode), m_srcPid(srcPid), m_myPid( myPid ),
    m_recvEntry(NULL),m_sendEntry(NULL), m_numPending(0), m_pktNum(0), m_expectedPkt(0), m_hdrPkt(NULL)
{
    m_prefix = "@t:"+ std::to_string(ctx->nic().getNodeId()) +":Nic::RecvMachine::StreamBase::@p():@l ";
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_STREAM,"this=%p\n",this);
    m_start = m_ctx->nic().getCurrentSimTimeNano();
}

Nic::RecvMachine::StreamBase::~StreamBase() {

    size_t totalBytes = 0;
    if ( m_recvEntry ) {
        m_recvEntry->notify( m_srcPid, m_srcNode, m_matched_tag, m_matched_len );
        totalBytes = m_recvEntry->totalBytes();
        delete m_recvEntry;
    }

    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_STREAM,"this=%p bytes=%zu latency=%" PRIu64 "\n",this,
                                            totalBytes, m_ctx->nic().getCurrentSimTimeNano()-m_start);

    if ( m_sendEntry ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_STREAM,"core=%d targetNode=%d targetCore=%d\n",
                getMyPid(), m_sendEntry->dest(), m_sendEntry->dst_vNic() );
        m_ctx->nic().qSendEntry( m_sendEntry );
    }
}

void Nic::RecvMachine::StreamBase::processPktBody( FireflyNetworkEvent* ev  ) {

    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"get timing for packet %" PRIu64 " size=%lu\n",
                                                m_pktNum,ev->bufSize());
    assert(m_numPending < m_ctx->getMaxQsize() );
    ++m_numPending;

	m_ctx->nic().m_recvStreamPending->addData( m_numPending );

    std::vector< MemOp >* vec = new std::vector< MemOp >;
    bool ret = getRecvEntry()->copyIn( m_dbg, *ev, *vec );

    if ( 0 == ev->bufSize() ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM, "network event is done\n");
        delete ev;
    }

    Callback callback = NULL;
    bool finished = ret || length() == getRecvEntry()->currentLen();

    m_ctx->nic().dmaWrite( m_unit, m_myPid, vec,
            std::bind( &Nic::RecvMachine::StreamBase::ready, this, finished, m_pktNum++ ) );
}

void Nic::RecvMachine::StreamBase::ready( bool finished, uint64_t pktNum ) {
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM, "packet %" PRIu64 " is ready\n",pktNum);
    assert(pktNum== m_expectedPkt++);
    --m_numPending;

    if ( finished ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM, "this stream is done\n");
        m_ctx->deleteStream( this );
    }
    if ( m_wakeupCallback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_STREAM, "wakeup recv machine\n");
        m_ctx->schedCallback( m_wakeupCallback );
        m_wakeupCallback = NULL;
    }
}

bool Nic::RecvMachine::StreamBase::postedRecv( DmaRecvEntry* entry ) {
    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM,"\n");
    if ( ! m_hdrPkt ) return false;

    FireflyNetworkEvent* event = m_hdrPkt;

    MsgHdr& hdr = *(MsgHdr*) event->bufPtr();
    MatchMsgHdr& matchHdr = *(MatchMsgHdr*) ((unsigned char*)event->bufPtr() + sizeof( MsgHdr ));

    int srcNode = event->getSrcNode();

    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM,"event tag %#x, posted recv tag %#x\n",matchHdr.tag, entry->tag());
    if ( entry->tag() != matchHdr.tag ) {
        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM,"did't match tag\n");
        return false;
    }

    if ( entry->node() != -1 && entry->node() != srcNode ) {
        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM, "didn't match node  want=%#x src=%#x\n", entry->node(), srcNode );
        return false;
    }
    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM, "recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < matchHdr.len ) {
        assert(0);
    }
    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM, "matched stream=%p\n",this);

    m_recvEntry = entry;
    event->bufPop( sizeof(MsgHdr) + sizeof(MatchMsgHdr));
    event->clearHdr();

    processPktBody(m_hdrPkt);
    m_hdrPkt = NULL;
    return true;
}
