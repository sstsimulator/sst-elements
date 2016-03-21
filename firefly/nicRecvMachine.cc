// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
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
    for ( unsigned i = 0; i < m_recvM.size(); i++ ) {
        std::map< int, std::deque<RecvEntry*> >::iterator iter;

        for ( iter = m_recvM[i].begin(); iter != m_recvM[i].end(); ++iter ) {
            while ( ! (*iter).second.empty() ) {
                delete (*iter).second.front();
                (*iter).second.pop_front(); 
            }
        }
    }

    while ( ! m_activeRecvM.empty() ) {
        delete m_activeRecvM.begin()->second;
        m_activeRecvM.erase( m_activeRecvM.begin() );
    }
}

void Nic::RecvMachine::state_0( FireflyNetworkEvent* ev )
{
    m_dbg.verbose(CALL_INFO,1,1,"got a network pkt\n");
    assert( ev );

    // is there an active stream for this src node?
    if ( m_activeRecvM.find( ev->src ) == m_activeRecvM.end() ) {
        state_1( ev );
    } else {
        state_move_0( ev );
    } 
}

void Nic::RecvMachine::state_1(  FireflyNetworkEvent* ev )
{
#ifdef NIC_RECV_DEBUG
    ++m_msgCount;
#endif
    MsgHdr& hdr = *(MsgHdr*) ev->bufPtr();

    if ( MsgHdr::Msg == hdr.op ) {
        m_dbg.verbose(CALL_INFO,1,1,"Msg Operation\n");

        Callback callback;
        if ( findRecv( ev->src, hdr ) ) {
            ev->bufPop( sizeof(MsgHdr) );

            callback = std::bind( &Nic::RecvMachine::state_move_0, this, ev );
        } else {
            callback = std::bind( &Nic::RecvMachine::state_2, this, ev );
        }
        m_nic.schedCallback( callback, m_rxMatchDelay);
    } else if ( MsgHdr::Rdma == hdr.op ) {

        m_dbg.verbose(CALL_INFO,1,1,"RDMA Operation\n");

        RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

        uint64_t delay = 0;
        Callback callback;
        switch ( rdmaHdr.op  ) {

          case RdmaMsgHdr::Put:
          case RdmaMsgHdr::GetResp:
            m_dbg.verbose(CALL_INFO,2,1,"Put Op\n");

            assert( findPut( ev->src, hdr, rdmaHdr ) );
            callback = std::bind( &Nic::RecvMachine::state_move_0, this, ev );
            break;

          case RdmaMsgHdr::Get:
            {
                m_dbg.verbose(CALL_INFO,2,1,"Get Op\n");
                SendEntry* entry = findGet( ev->src, hdr, rdmaHdr );
                delay = m_hostReadDelay; // host read  delay
                callback = std::bind( &Nic::RecvMachine::state_3, this, entry );
                delete ev;
            }
            break;

          default:
            assert(0);
        }

        ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );

        m_nic.schedCallback( callback, delay );
    }
}

// Need Recv
void Nic::RecvMachine::state_2( FireflyNetworkEvent* ev )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    processNeedRecv( 
        ev,
        std::bind( &Nic::RecvMachine::state_0, this, ev )
    );
}

// Need Get 
void Nic::RecvMachine::state_3( SendEntry* entry )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");
    m_nic.m_sendMachine[0].run( entry );

    checkNetwork();
}

void Nic::RecvMachine::checkNetwork( )
{
    FireflyNetworkEvent* ev = getNetworkEvent(0);
    if ( ev ) {
        m_dbg.verbose(CALL_INFO,1,1,"pulled a packet from the network\n");
        state_0( ev );
    } else {
        m_dbg.verbose(CALL_INFO,1,1,"nothing on the network, set notifier\n");
        setNotify();
    }
}

bool Nic::RecvMachine::findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbose(CALL_INFO,2,1,"src=%d len=%lu\n",src,hdr.len); 
    m_dbg.verbose(CALL_INFO,2,1,"rgnNum=%d offset=%d respKey=%d\n",
            rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey); 

    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,1,"GetResp\n");
        m_activeRecvM[src] = m_nic.m_getOrgnM[ rdmahdr.respKey ];


        m_dbg.verbose(CALL_INFO,2,1,"%p %lu\n", m_activeRecvM[src],
                    m_activeRecvM[src]->ioVec().size());
        m_nic.m_getOrgnM.erase(rdmahdr.respKey);
        return true;

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,1,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return false;
}

Nic::SendEntry* Nic::RecvMachine::findGet( int src, 
                        MsgHdr& hdr, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbose(CALL_INFO,1,1,"\n");

    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches. 
    if ( m_nic.m_memRgnM[ hdr.dst_vNicId ].find( hdr.tag ) ==
                                m_nic.m_memRgnM[ hdr.dst_vNicId ].end() ) {
        assert(0);
    }

    assert( hdr.tag == rdmaHdr.rgnNum );
    
    m_dbg.verbose(CALL_INFO,1,1,"rgnNum %d offset=%d respKey=%d\n",
                        rdmaHdr.rgnNum, rdmaHdr.offset, rdmaHdr.respKey );

    MemRgnEntry* entry = m_nic.m_memRgnM[ hdr.dst_vNicId ][rdmaHdr.rgnNum]; 
    assert( entry );

    m_nic.m_memRgnM[ hdr.dst_vNicId ].erase(rdmaHdr.rgnNum); 

    return new PutOrgnEntry( entry->vNicNum(), 
                                    src, hdr.src_vNicId, 
                                    rdmaHdr.respKey, entry );
}


bool Nic::RecvMachine::findRecv( int src, MsgHdr& hdr )
{
    m_dbg.verbose(CALL_INFO,2,1,"need a recv entry, srcNic=%d src_vNic=%d "
                "dst_vNic=%d tag=%#x len=%lu\n", src, hdr.src_vNicId,
						hdr.dst_vNicId, hdr.tag, hdr.len);

    if ( m_recvM[hdr.dst_vNicId].find( hdr.tag ) == m_recvM[hdr.dst_vNicId].end() ) {
		m_dbg.verbose(CALL_INFO,2,1,"did't match tag\n");
        return false;
    }

    RecvEntry* entry = m_recvM[ hdr.dst_vNicId][ hdr.tag ].front();
    if ( entry->node() != -1 && entry->node() != src ) {
		m_dbg.verbose(CALL_INFO,2,1,"didn't match node  want=%#x src=%#x\n",
											entry->node(), src );
        return false;
    } 
    m_dbg.verbose(CALL_INFO,2,1,"recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,2,1,"found a receive entry\n");

    m_activeRecvM[src] = m_recvM[ hdr.dst_vNicId ][ hdr.tag ].front();
    m_recvM[ hdr.dst_vNicId ][ hdr.tag ].pop_front();

    if ( m_recvM[ hdr.dst_vNicId ][ hdr.tag ].empty() ) {
        m_recvM[ hdr.dst_vNicId ].erase( hdr.tag );
    }
    m_activeRecvM[src]->setMatchInfo( src, hdr );
    m_activeRecvM[src]->setNotifier( 
        new NotifyFunctor_6<Nic,int,int,int,int,size_t,void*>
                (&m_nic,&Nic::notifyRecvDmaDone,
                    m_activeRecvM[src]->local_vNic(),
                    m_activeRecvM[src]->src_vNic(),
                    m_activeRecvM[src]->match_src(),
                    m_activeRecvM[src]->match_tag(),
                    m_activeRecvM[src]->match_len(),
                    m_activeRecvM[src]->key() )
    );

    return true;
}

void Nic::RecvMachine::state_move_0( FireflyNetworkEvent* event )
{
    m_dbg.verbose(CALL_INFO,1,1,"event has %lu bytes\n", event->bufSize() );

    m_nic.dmaWrite(
            std::bind( &Nic::RecvMachine::state_move_1, this, event ), 
            0, event->bufSize() );
}
        
void Nic::RecvMachine::state_move_1( FireflyNetworkEvent* event )
{
    int src = event->src;
    if ( 0 == m_activeRecvM[ src ]->currentVec && 
             0 == m_activeRecvM[ src ]->currentPos  ) {
        m_dbg.verbose(CALL_INFO,1,1,"local_vNic %d, recv srcNic=%d "
                    "src_vNic=%d tag=%x bytes=%lu\n", 
                        m_activeRecvM[ src ]->local_vNic(),
                        src,
                        m_activeRecvM[ src ]->src_vNic(),
                        m_activeRecvM[ src ]->match_tag(),
                        m_activeRecvM[ src ]->match_len() );
    }
    long tmp = event->bufSize();
    if ( copyIn( m_dbg, *m_activeRecvM[ src ], *event ) || 
        m_activeRecvM[src]->match_len() == 
                        m_activeRecvM[src]->currentLen ) {
        m_dbg.verbose(CALL_INFO,1,1,"recv entry done, srcNic=%d src_vNic=%d\n",
                            src, m_activeRecvM[ src ]->src_vNic() );

        m_dbg.verbose(CALL_INFO,2,1,"copyIn %lu bytes\n",
                                            tmp - event->bufSize());

        m_activeRecvM[src]->notify();

        delete m_activeRecvM[src];
        m_activeRecvM.erase(src);

        if ( ! event->bufEmpty() ) {
            m_dbg.fatal(CALL_INFO,-1,
                "buffer not empty, %lu bytes remain\n",event->bufSize());
        }
    }

    m_dbg.verbose(CALL_INFO,2,1,"copyIn %lu bytes\n", tmp - event->bufSize());

    if ( 0 == event->bufSize() ) {
        m_dbg.verbose(CALL_INFO,1,1,"network event is done\n");
        delete event;
    }

    checkNetwork();
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,1,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,1,"%#03x\n",(unsigned char)buf[i]);
    }
}

size_t Nic::RecvMachine::copyIn( Output& dbg, Nic::Entry& entry,
                                        FireflyNetworkEvent& event )
{
    dbg.verbose(CALL_INFO,3,1,"ioVec.size()=%lu\n", entry.ioVec().size() );


    for ( ; entry.currentVec < entry.ioVec().size();
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = entry.ioVec()[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.bufSize();
            size_t len = toLen < fromLen ? toLen : fromLen;

            char* toPtr = (char*) entry.ioVec()[entry.currentVec].ptr +
                                                        entry.currentPos;
            dbg.verbose(CALL_INFO,3,1,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            entry.currentLen += len;
            if ( entry.ioVec()[entry.currentVec].ptr ) {
                void *buf = event.bufPtr();
                if ( buf ) {
                    memcpy( toPtr, buf, len );
                }
                print( dbg, toPtr, len );
            }

            event.bufPop(len);

            entry.currentPos += len;
            if ( event.bufEmpty() &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len )
            {
                dbg.verbose(CALL_INFO,3,1,"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,3,1,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
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
