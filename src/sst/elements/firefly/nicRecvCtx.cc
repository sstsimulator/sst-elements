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

bool Nic::RecvMachine::Ctx::processPkt( FireflyNetworkEvent* ev ) {

    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"got a network pkt from node=%d pid=%d for pid=%d\n",
                        ev->getSrcNode(),ev->getSrcPid(), m_pid );
    SrcKey srcKey = getSrcKey( ev->getSrcNode(), ev->getSrcPid() );

    bool blocked = false;
    // this packet is part of an active stream to a pid
    if ( m_streamMap.find(srcKey) != m_streamMap.end() )  {

        if ( ev->getIsHdr() ) {
            assert( ! m_blockedNetworkEvent );
            m_blockedNetworkEvent = ev;
            blocked = true; 
        } else {

            StreamBase* stream =  m_streamMap[srcKey];

            if ( stream->isBlocked() ) {
                stream->setWakeup( 
                        [=]() {
                            processPkt( ev );
                            m_rm.checkNetworkForData(); 
                        }
                    );
                        
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"blocked by stream\n");
                // this recv machine is blocked
                blocked = true; 
                
            } else {
                stream->processPkt(ev);
            }
        }
    } else { // packet is for a destPid/srcNode/srcPid that is not active
        
        StreamBase* stream = m_streamMap[srcKey] = newStream( allocNicUnit(), ev );

        Callback callback = stream->getStartCallback(); 
        SimTime_t delay = stream->getStartDelay();
        if ( delay ) {
            assert( callback );
            schedCallback( 
                [=](){ 
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");
                    callback(); 
                    m_rm.checkNetworkForData(); 
                }, 
                delay );
            blocked = true;
        } else {
            if ( callback ) {
                callback();
            }
        }
    }
    return blocked;
}

Nic::RecvMachine::StreamBase* Nic::RecvMachine::Ctx::newStream( int unit, FireflyNetworkEvent* ev )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"new stream\n");

#ifdef NIC_RECV_DEBUG
        ++m_msgCount;
#endif
    MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();
    switch ( hdr.op ) {
      case MsgHdr::Msg:
        return new MsgStream( m_dbg, this, unit, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev );
        break;
      case MsgHdr::Rdma:
        return new RdmaStream( m_dbg, this, unit, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev );
        break;
      case MsgHdr::Shmem:
        return new ShmemStream( m_dbg, this, unit, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid(), ev );
        break;
    }
    assert(0);
}

Nic::EntryBase* Nic::RecvMachine::Ctx::findRecv( int srcNode, int srcPid, MsgHdr& hdr, MatchMsgHdr& matchHdr  )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"need a recv entry, srcNic=%d srcPid=%d "
                "tag=%#x len=%lu\n", srcNode, srcPid, matchHdr.tag, matchHdr.len);

    DmaRecvEntry* entry = NULL;

    std::deque<DmaRecvEntry*>::iterator iter = m_postedRecvs.begin();

    for ( ; iter != m_postedRecvs.end(); ++iter ) {
        entry = (*iter);
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"check Posted Recv with tag=%#x node=%d\n",entry->tag(),entry->node());

        if ( matchHdr.tag != entry->tag() ) {
            continue;
        }
        if ( -1 != entry->node() && srcNode != entry->node() ) {
            continue;
        }
        if ( entry->totalBytes() < matchHdr.len ) {
            assert(0);
        }

        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"found recv entry, size %lu\n",entry->totalBytes());

        m_postedRecvs.erase(iter);
        return entry;
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"no match\n");

    return NULL;
}

Nic::SendEntryBase* Nic::RecvMachine::Ctx::findGet( int srcNode, int srcPid, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"srcNode=%d srcPid=%d rgnNum=%d offset=%d respKey=%d\n",
                        srcNode, srcPid, rdmaHdr.rgnNum, rdmaHdr.offset, rdmaHdr.respKey );
    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches.
    if ( m_memRgnM.find( rdmaHdr.rgnNum ) == m_memRgnM.end() ) {
        assert(0);
    }

    MemRgnEntry* entry = m_memRgnM[rdmaHdr.rgnNum];
    assert( entry );

    m_memRgnM.erase(rdmaHdr.rgnNum);

    return new PutOrgnEntry( m_pid, srcNode, srcPid, rdmaHdr.respKey, entry );
}

Nic::DmaRecvEntry* Nic::RecvMachine::Ctx::findPut( int srcNode, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE, "srcNode=%d rgnNum=%d offset=%d respKey=%d\n",
            srcNode, rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey);

    DmaRecvEntry* entry = NULL;
    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"GetResp\n");
        entry = m_getOrgnM[ rdmahdr.respKey ];

        m_getOrgnM.erase(rdmahdr.respKey);

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return entry;
}
