// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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

bool Nic::RecvMachine::Ctx::processPkt( FireflyNetworkEvent* ev ) {

    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"got a network pkt from node=%d pid=%d stream=%d for pid=%d size=%zu\n",
                        ev->getSrcNode(),ev->getSrcPid(), ev->getSrcStream(), m_pid, ev->bufSize() );

    if ( ev->isCtrl() ) {
        return processCtrlPkt( ev );
    } else {
        return processStdPkt( ev );
    }
}

bool Nic::RecvMachine::Ctx::processCtrlPkt( FireflyNetworkEvent* ev ) {

    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"got a control message\n");
    StreamBase* stream = newStream( ev );

    stream->processPkt( ev );
    return false;
}

bool Nic::RecvMachine::Ctx::processStdPkt( FireflyNetworkEvent* ev ) {
    bool blocked = false;
    SrcKey srcKey = getSrcKey( ev->getSrcNode(), ev->getSrcPid(), ev->getSrcStream() );

    StreamBase* stream;
    if ( ev->isHdr() ) {

        stream = newStream( ev );
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX,"new stream %p %s node=%d pid=%d stream=%d for pid=%d\n",stream, ev->isTail()? "single packet stream":"",
               ev->getSrcNode(),ev->getSrcPid(), ev->getSrcStream(), m_pid );
        assert ( m_streamMap.find(srcKey) == m_streamMap.end() ); 

        if ( ! ev->isTail() ) { 
            m_streamMap[srcKey] = stream;
        }

    } else { 
        assert ( m_streamMap.find(srcKey) != m_streamMap.end() ); 

        stream = m_streamMap[srcKey]; 

        if ( ev->isTail() ) { 
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"tail packet stream=%p\n",stream );
            m_streamMap.erase(srcKey); 
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"body packet stream=%p\n",stream );
        }
    }

    if ( stream->isBlocked() ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"stream is blocked stream=%p\n",stream );
        stream->setWakeup( 
            [=]() {
                m_dbg.verbosePrefix(prefix(),CALL_INFO_LAMBDA,"processStdPkt",2,NIC_DBG_RECV_CTX,"stream is unblocked stream=%p\n",stream );
                stream->processPkt( ev );
                m_rm.checkNetworkForData(); 
            }
        );
        return true;
    } else {
        stream->processPkt(ev);
        return false;
    } 
}

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

    std::deque<DmaRecvEntry*>::iterator iter = m_postedRecvs.begin();

    for ( ; iter != m_postedRecvs.end(); ++iter ) {
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

        m_postedRecvs.erase(iter);
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
    if ( m_memRgnM.find( rdmaHdr.rgnNum ) == m_memRgnM.end() ) {
        assert(0);
    }

    MemRgnEntry* entry = m_memRgnM[rdmaHdr.rgnNum];
    assert( entry );

    m_memRgnM.erase(rdmaHdr.rgnNum);

    return new PutOrgnEntry( m_pid, nic().getSendStreamNum(m_pid), srcNode, srcPid, rdmaHdr.respKey, entry );
}

Nic::DmaRecvEntry* Nic::RecvMachine::Ctx::findPut( int srcNode, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX, "srcNode=%d rgnNum=%d offset=%d respKey=%d\n",
            srcNode, rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey);

    DmaRecvEntry* entry = NULL;
    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_CTX,"GetResp\n");
        entry = m_getOrgnM[ rdmahdr.respKey ];

        m_getOrgnM.erase(rdmahdr.respKey);

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_CTX,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return entry;
}
