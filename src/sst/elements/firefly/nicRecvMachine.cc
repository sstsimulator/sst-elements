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
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""got a network pkt from %d\n",ev->src);
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
        m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Recv: ""Msg Operation\n");

        Callback callback;
        if ( findRecv( ev->src, hdr ) ) {
            ev->bufPop( sizeof(MsgHdr) );

            callback = std::bind( &Nic::RecvMachine::state_move_0, this, ev );
        } else {
            callback = std::bind( &Nic::RecvMachine::state_2, this, ev );
        }
        m_nic.schedCallback( callback, m_rxMatchDelay);
    } else if ( MsgHdr::Rdma == hdr.op ) {

        m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Recv: ""RDMA Operation\n");

        RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

        uint64_t delay = 0;
        Callback callback;
        switch ( rdmaHdr.op  ) {

          case RdmaMsgHdr::Put:
          case RdmaMsgHdr::GetResp:
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""Put Op\n");

            assert( findPut( ev->src, hdr, rdmaHdr ) );
            callback = std::bind( &Nic::RecvMachine::state_move_0, this, ev );
            break;

          case RdmaMsgHdr::Get:
            {
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""Get Op\n");
                SendEntry* entry = findGet( ev->src, hdr, rdmaHdr );
                delay = m_hostReadDelay; // host read  delay
                callback = std::bind( &Nic::RecvMachine::state_3, this, ev, entry );
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
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Recv: ""\n");
    processNeedRecv( 
        ev,
        std::bind( &Nic::RecvMachine::state_0, this, ev )
    );
}

// Need Get 
void Nic::RecvMachine::state_3( FireflyNetworkEvent* ev, SendEntry* entry )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Recv: ""\n");
    delete ev;
    m_nic.m_sendMachine[0].run( entry );

    checkNetwork();
}

void Nic::RecvMachine::checkNetwork( )
{
    for ( int i = 0; i < 2; i++ ) {
        FireflyNetworkEvent* ev = getNetworkEvent(i);
        if ( ev ) {
            m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
					"pulled a packet from the network"
                    " vc=%d\n", i);
            if ( i == 0 ) {
                state_0( ev );
            } else {
                state_1( ev );
            }
            return;
        }
    }
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
					"nothing on the network, set notifier\n");
    setNotify();
}

bool Nic::RecvMachine::findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
					"src=%d len=%lu\n",src,hdr.len); 
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
					"rgnNum=%d offset=%d respKey=%d\n",
            rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey); 

    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""GetResp\n");
        m_activeRecvM[src] = m_nic.m_getOrgnM[ rdmahdr.respKey ];


        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
					"%p %lu\n", m_activeRecvM[src],
                    m_activeRecvM[src]->ioVec().size());
        m_nic.m_getOrgnM.erase(rdmahdr.respKey);
        return true;

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return false;
}

Nic::SendEntry* Nic::RecvMachine::findGet( int src, 
                        MsgHdr& hdr, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Recv: ""\n");

    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches. 
    if ( m_nic.m_memRgnM[ hdr.dst_vNicId ].find( hdr.tag ) ==
                                m_nic.m_memRgnM[ hdr.dst_vNicId ].end() ) {
        assert(0);
    }

    assert( hdr.tag == rdmaHdr.rgnNum );
    
    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Recv: "
						"rgnNum %d offset=%d respKey=%d\n",
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
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"need a recv entry, srcNic=%d src_vNic=%d "
                "dst_vNic=%d tag=%#x len=%lu\n", src, hdr.src_vNicId,
						hdr.dst_vNicId, hdr.tag, hdr.len);

    if ( m_recvM[hdr.dst_vNicId].find( hdr.tag ) == m_recvM[hdr.dst_vNicId].end() ) {
		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""did't match tag\n");
        return false;
    }

    RecvEntry* entry = m_recvM[ hdr.dst_vNicId][ hdr.tag ].front();
    if ( entry->node() != -1 && entry->node() != src ) {
		m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"didn't match node  want=%#x src=%#x\n",
											entry->node(), src );
        return false;
    } 
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: ""found a receive entry\n");

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
    int src = event->src;
    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"src=%d event has %lu bytes\n", src,event->bufSize() );

    if ( 0 == m_activeRecvM[ src ]->currentVec && 
             0 == m_activeRecvM[ src ]->currentPos  ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"local_vNic %d, recv srcNic=%d "
                    "src_vNic=%d tag=%x bytes=%lu\n", 
                        m_activeRecvM[ src ]->local_vNic(),
                        src,
                        m_activeRecvM[ src ]->src_vNic(),
                        m_activeRecvM[ src ]->match_tag(),
                        m_activeRecvM[ src ]->match_len() );
    }
    std::vector< DmaVec > vec;

    long tmp = event->bufSize();
    bool ret = copyIn( m_dbg, *m_activeRecvM[ src ], *event, vec );

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"copyIn %lu bytes\n", tmp - event->bufSize());

    m_nic.dmaWrite( vec,
            std::bind( &Nic::RecvMachine::state_move_1, this, event, ret ) ); 

	// don't put code after this, the callback may be called serially
}

void Nic::RecvMachine::state_move_1( FireflyNetworkEvent* event, bool done )
{
    int src = event->src;

    m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
		"src=%d currentLen %lu bytes\n", src, m_activeRecvM[src]->currentLen);

    if ( done || m_activeRecvM[src]->match_len() == 
                        m_activeRecvM[src]->currentLen ) {

        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"recv entry done, srcNic=%d src_vNic=%d\n",
                            src, m_activeRecvM[ src ]->src_vNic() );

        m_activeRecvM[src]->notify();

        delete m_activeRecvM[src];
        m_activeRecvM.erase(src);

        if ( ! event->bufEmpty() ) {
            m_dbg.fatal(CALL_INFO,-1,
                "src=%d buffer not empty, %lu bytes remain\n",
										src,event->bufSize());
        }
    }

    if ( 0 == event->bufSize() ) {
        m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"Recv: "
				"network event is done\n");
        delete event;
    }

    checkNetwork();
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,NIC_DBG_RECV_MACHINE,"Recv: ""addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,"Recv: "
				"%#03x\n",(unsigned char)buf[i]);
    }
}

size_t Nic::RecvMachine::copyIn( Output& dbg, Nic::Entry& entry,
                        FireflyNetworkEvent& event, std::vector<DmaVec>& vec )
{
    dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,"Recv: "
				"ioVec.size()=%lu\n", entry.ioVec().size() );


    for ( ; entry.currentVec < entry.ioVec().size();
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = entry.ioVec()[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.bufSize();
            size_t len = toLen < fromLen ? toLen : fromLen;

			DmaVec tmp;
			tmp.length = len;
			tmp.addr = entry.ioVec()[entry.currentVec].addr.simVAddr +
                                                    entry.currentPos;
            vec.push_back( tmp );

            char* toPtr = (char*) entry.ioVec()[entry.currentVec].addr.backing +
                                                        entry.currentPos;
            dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,"Recv: "
							"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            entry.currentLen += len;
            if ( entry.ioVec()[entry.currentVec].addr.backing ) {
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
                dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,"Recv: "
							"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,3,NIC_DBG_RECV_MACHINE,"Recv: "
				"currentVec=%lu, currentPos=%lu\n",
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
