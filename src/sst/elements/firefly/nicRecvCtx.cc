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

Nic::RecvMachine::StreamBase* Nic::RecvMachine::Ctx::newStream( FireflyNetworkEvent* ev )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX,"\n");

#ifdef NIC_RECV_DEBUG
        ++m_msgCount;
#endif
    MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();
    switch ( hdr.op ) {
      case MsgHdr::Msg:
        return new MsgStream( m_dbg, this, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev );
        break;
      case MsgHdr::Rdma:
        return new RdmaStream( m_dbg, this, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev );
        break;
      case MsgHdr::Shmem:
        return new ShmemStream( m_dbg, this, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev );
        break;
    }
    assert(0);
}

Nic::EntryBase* Nic::RecvMachine::Ctx::findRecv( int srcNode, int srcPid, MsgHdr& hdr, MatchMsgHdr& matchHdr  )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"need a recv entry, srcNic=%d srcPid=%d "
                "tag=%#x len=%lu\n", srcNode, srcPid, matchHdr.tag, matchHdr.len);

    DmaRecvEntry* entry = NULL;

    auto& q = m_rm.m_nic.m_recvCtxData[m_pid].m_postedRecvs;
    auto iter = q.begin();

    for ( ; iter != q.end(); ++iter ) {
        entry = (*iter);
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_CTX,"check Posted Recv with tag=%#x node=%d\n",entry->tag(),entry->node());

        if ( matchHdr.tag != entry->tag() ) {
            continue;
        }
        if ( -1 != entry->node() && srcNode != entry->node() ) {
            continue;
        }
        if ( entry->totalBytes() < matchHdr.len ) {
            assert(0);
        }

        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_CTX,"found recv entry, size %lu\n",entry->totalBytes());

        q.erase(iter);
        return entry;
    }
    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_CTX,"no match\n");

    return NULL;
}

Nic::SendEntryBase* Nic::RecvMachine::Ctx::findGet( int srcNode, int srcPid, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX,"srcNode=%d srcPid=%d rgnNum=%d offset=%d respKey=%d\n",
                        srcNode, srcPid, rdmaHdr.rgnNum, rdmaHdr.offset, rdmaHdr.respKey );
    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches.
    auto& m = m_rm.m_nic.m_recvCtxData[m_pid].m_memRgnM;

    if ( m.find( rdmaHdr.rgnNum ) == m.end() ) {
        assert(0);
    }

    MemRgnEntry* entry = m[rdmaHdr.rgnNum];
    assert( entry );

    m.erase(rdmaHdr.rgnNum);

    int vn = nic().m_getRespSmallVN;
    size_t length = 0;
    for ( int i = 0; i <  entry->iovec().size(); i++ ) {
        length += entry->iovec().at(i).len;
    }
    if ( length >  nic().m_getRespSize ) {
        vn = nic().m_getRespLargeVN;
    }
    return new PutOrgnEntry( m_pid, srcNode, srcPid, rdmaHdr.respKey, entry, vn );
}

Nic::DmaRecvEntry* Nic::RecvMachine::Ctx::findPut( int srcNode, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX, "srcNode=%d rgnNum=%d offset=%d respKey=%d\n",
            srcNode, rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey);

    auto& m = m_rm.m_nic.m_recvCtxData[m_pid].m_getOrgnM;
    DmaRecvEntry* entry = NULL;
    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_CTX,"GetResp\n");
        entry = m[ rdmahdr.respKey ];

        m.erase(rdmahdr.respKey);

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_CTX,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return entry;
}
