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

void Nic::RecvMachine::processPkt( FireflyNetworkEvent* ev )
{
    int destPid = ev->getDestPid();
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"got a network pkt from node=%d pid=%d for pid=%d\n",
                        ev->getSrcNode(),ev->getSrcPid(), destPid );

    SrcKey srcKey = getSrcKey( ev->getSrcNode(), ev->getSrcPid() );

    std::map< SrcKey, StreamBase* >& tmp = m_streamMap[ destPid ];

    // this packet is part of an active stream to a pid
    if ( tmp.find(srcKey) != tmp.end() )  { 

        if ( ev->getIsHdr() ) { 
            assert( ! m_blockedNetworkEvents[ destPid ] ); 
            m_blockedNetworkEvents[ destPid ] = ev;
            checkNetworkForData();
        } else {

            StreamBase* stream =  tmp[srcKey];

            if ( stream->isBlocked() ) {
                stream->setWakeup( std::bind(&Nic::RecvMachine::wakeup, this, stream, ev ));
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"blocked by stream\n");
                // this recv machine is blocked 
            } else {
                stream->processPkt(ev);
            }
        }
    } else {
        int unit = m_nic.allocNicUnit( );
        if ( -1 == unit ) {
            m_blockedNetworkEvent = ev;
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"blocked by unit\n");
            // this recv machine is blocked 
            
        // packet is for a destPid/srcNode/srcPid that is not active 
        } else { 
            //========================================================
            // FIX UNIT ALLOCATION, need one Unit per process
            //========================================================
            m_streamMap[destPid][srcKey] = newStream( unit, ev );
            checkNetworkForData();
        } 
    }
}

Nic::RecvMachine::StreamBase* Nic::RecvMachine::newStream( int unit, FireflyNetworkEvent* ev )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"new stream\n");

#ifdef NIC_RECV_DEBUG
        ++m_msgCount;
#endif
    MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();
    switch ( hdr.op ) {
      case MsgHdr::Msg:
        return new MsgStream( m_dbg, ev, *this, unit, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid() );
        break;
      case MsgHdr::Rdma:
        return new RdmaStream( m_dbg, ev, *this, unit, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid() );
        break;
      case MsgHdr::Shmem:
        return new ShmemStream( m_dbg, ev, *this, unit, ev->getSrcNode(),ev->getSrcPid(), ev->getDestPid() );
        break;
    }
    assert(0);
}

void Nic::RecvMachine::StreamBase::processPkt( FireflyNetworkEvent* ev  ) {

    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"get timing for packet %" PRIu64 " size=%lu\n",
                                                m_pktNum,ev->bufSize());
    assert(m_numPending < m_rm.getMaxQsize() ); 
    ++m_numPending;

    std::vector< MemOp >* vec = new std::vector< MemOp >;
    bool ret = getRecvEntry()->copyIn( m_dbg, *ev, *vec );

    if ( 0 == ev->bufSize() ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "network event is done\n");
        delete ev;
    }
   
    Callback callback = NULL;

    if ( ret || length() == getRecvEntry()->currentLen() ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "this stream is done\n");
        callback = std::bind( &Nic::RecvMachine::clearMapAndDeleteStream, &m_rm, getMyPid(), this );
    }
    m_rm.nic().dmaWrite( m_unit, vec, std::bind( &Nic::RecvMachine::StreamBase::ready, this, callback, m_pktNum++ ) ); 

    m_rm.checkNetworkForData();
}

void Nic::RecvMachine::StreamBase::ready( Callback callback, uint64_t pktNum ) {
    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "packet %" PRIu64 " is ready\n",pktNum);
    assert(pktNum== m_expectedPkt++);
    --m_numPending;
    if ( callback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "we have a callback\n");
        callback();
    }
    if ( m_wakeupCallback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE, "wakeup recv machine\n");
        m_wakeupCallback( );
        m_wakeupCallback = NULL;
    }
}


Nic::EntryBase* Nic::RecvMachine::findRecv( int myPid, int srcNode, int srcPid, MsgHdr& hdr, MatchMsgHdr& matchHdr  )
{
   m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"need a recv entry, srcNic=%d srcPid=%d "
                "myPid=%d tag=%#x len=%lu\n", srcNode, srcPid, myPid, matchHdr.tag, matchHdr.len);

    DmaRecvEntry* entry = NULL;

    std::deque<DmaRecvEntry*>::iterator iter = m_postedRecvs[myPid].begin();

    for ( ; iter != m_postedRecvs[myPid].end(); ++iter ) {
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

        m_postedRecvs[myPid].erase(iter);
        return entry;
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"no match\n");

    return NULL;
}

Nic::SendEntryBase* Nic::RecvMachine::findGet( int pid, int srcNode, int srcPid, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"pid=%d srcNode=%d srcPid=%d rgnNum=%d offset=%d respKey=%d\n",
                        pid, srcNode, srcPid, rdmaHdr.rgnNum, rdmaHdr.offset, rdmaHdr.respKey );
    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches.
    if ( m_memRgnM[ pid ].find( rdmaHdr.rgnNum ) == m_memRgnM[ pid ].end() ) {
        assert(0);
    }

    MemRgnEntry* entry = m_memRgnM[ pid ][rdmaHdr.rgnNum];
    assert( entry );

    m_memRgnM[ pid ].erase(rdmaHdr.rgnNum);

    return new PutOrgnEntry( pid, srcNode, srcPid, rdmaHdr.respKey, entry );
}

Nic::DmaRecvEntry* Nic::RecvMachine::findPut( int pid, int srcNode, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "srcNode=%d rgnNum=%d offset=%d respKey=%d\n",
            srcNode, rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey);

    DmaRecvEntry* entry = NULL;
    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"GetResp\n");
        entry = m_getOrgnM[pid][ rdmahdr.respKey ];

        m_getOrgnM[pid].erase(rdmahdr.respKey);

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return entry;
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
