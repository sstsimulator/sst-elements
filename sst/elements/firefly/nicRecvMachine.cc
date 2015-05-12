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

void Nic::RecvMachine::run( )
{
    m_dbg.verbose(CALL_INFO,1,0,"RecvMachine\n");

#ifdef NIC_RECV_DEBUG
    ++m_runCount;
#endif
    bool blocked = false;
    do { 
        switch ( m_state ) {
          case NeedPkt:
            m_dbg.verbose(CALL_INFO,1,0,"NeedPkt\n");

            m_mEvent = getNetworkEvent(0);
            if ( ! m_mEvent ) {
                m_dbg.verbose(CALL_INFO,1,0,"no packet available\n");
                setNotify();
                blocked = true;
                break;
            }
            m_state = HavePkt;

          case HavePkt:
            m_dbg.verbose(CALL_INFO,1,0,"event has %lu bytes\n",
                                                    m_mEvent->bufSize() );

            // is there an active stream for this src node?
            if ( m_activeRecvM.find( m_mEvent->src ) == m_activeRecvM.end() ) {
    
                SelfEvent* event = new SelfEvent;

                // note that m_state and enry are set by this function
                uint64_t delay = processFirstEvent( 
                            *m_mEvent, m_state, event->entry );

                m_dbg.verbose(CALL_INFO,1,0,"state=%d\n",m_state);

                // if the packet was a "Get" it created a SendEntry,
                // schedule the send and continue receiving packets
                // else 
                // schedule a delay that runs the recvMachine
                if ( event->entry ) {
                    event->type = SelfEvent::RunSendMachine;
                    m_state = NeedPkt;
                    delete m_mEvent;
                } else {
                    event->type = SelfEvent::RunRecvMachine;
                    clearNotify();
                    blocked = true;
                } 
                m_nic.schedEvent( event, delay ); 

                break;
            }
            m_state = Move;

          case Move:
          case Put:
            m_dbg.verbose(CALL_INFO,1,0,"Move\n");
            assert( m_mEvent );
            if ( ! moveEvent( m_mEvent ) ) {
                clearNotify();
                m_state = WaitWrite;
                blocked = true;
                break; 
            } 

            m_state = NeedPkt; 
            break;

          case WaitWrite:
            m_state = Move;
            break;

          case NeedRecv: 
            m_dbg.verbose(CALL_INFO,1,0,"NeedRecv\n"); 
            processNeedRecv( m_mEvent );
            m_state = HavePkt; 
            blocked = true;
            break;
        }
        m_dbg.verbose(CALL_INFO,1,0,"state=%d %s\n",m_state,
                    blocked?"blocked":"not blocked");
    } while ( ! blocked );
}

uint64_t Nic::RecvMachine::processFirstEvent( FireflyNetworkEvent& mEvent,
                        RecvMachine::State& state, Entry*& sEntry  )
{
    int delay = 0;
        
    MsgHdr& hdr = *(MsgHdr*) mEvent.bufPtr();

#ifdef NIC_RECV_DEBUG
    ++m_msgCount;
#endif
    if ( MsgHdr::Msg == hdr.op ) {
        m_dbg.verbose(CALL_INFO,2,0,"Msg Op\n");

        delay = m_rxMatchDelay;

        if ( findRecv( mEvent.src, hdr ) ) {
            mEvent.bufPop( sizeof(MsgHdr) );
            state = Move;
        } else {
            state = NeedRecv;
        }

    } else if ( MsgHdr::Rdma == hdr.op ) {

        RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) mEvent.bufPtr( sizeof(MsgHdr) );

        switch ( rdmaHdr.op  ) {

          case RdmaMsgHdr::Put:
          case RdmaMsgHdr::GetResp:
            m_dbg.verbose(CALL_INFO,2,0,"Put Op\n");
            state = Put;
            assert( findPut( mEvent.src, hdr, rdmaHdr ) );
            break;

          case RdmaMsgHdr::Get:
            m_dbg.verbose(CALL_INFO,2,0,"Get Op\n");
            sEntry = findGet( mEvent.src, hdr, rdmaHdr );
            delay = m_hostReadDelay; // host read  delay
            break;

          default:
            assert(0);
        }

        mEvent.bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );
    }

    return delay;
}

bool Nic::RecvMachine::findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr )
{
    m_dbg.verbose(CALL_INFO,2,0,"src=%d len=%lu\n",src,hdr.len); 
    m_dbg.verbose(CALL_INFO,2,0,"rgnNum=%d offset=%d respKey=%d\n",
            rdmahdr.rgnNum, rdmahdr.offset, rdmahdr.respKey); 

    if ( RdmaMsgHdr::GetResp == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,0,"GetResp\n");
        m_activeRecvM[src] = m_nic.m_getOrgnM[ rdmahdr.respKey ];


        m_dbg.verbose(CALL_INFO,2,0,"%p %lu\n", m_activeRecvM[src],
                    m_activeRecvM[src]->ioVec().size());
        m_nic.m_getOrgnM.erase(rdmahdr.respKey);
        return true;

    } else if ( RdmaMsgHdr::Put == rdmahdr.op ) {
        m_dbg.verbose(CALL_INFO,2,0,"Put\n");
        assert(0);
    } else {
        assert(0);
    }

    return false;
}

Nic::SendEntry* Nic::RecvMachine::findGet( int src, 
                        MsgHdr& hdr, RdmaMsgHdr& rdmaHdr  )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    // Note that we are not doing a strong check here. We are only checking that
    // the tag matches. 
    if ( m_nic.m_memRgnM[ hdr.dst_vNicId ].find( hdr.tag ) ==
                                m_nic.m_memRgnM[ hdr.dst_vNicId ].end() ) {
        assert(0);
    }

    assert( hdr.tag == rdmaHdr.rgnNum );
    
    m_dbg.verbose(CALL_INFO,1,0,"rgnNum %d offset=%d respKey=%d\n",
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
    m_dbg.verbose(CALL_INFO,2,0,"need a recv entry, srcNic=%d src_vNic=%d "
                "dst_vNic=%d tag=%#x len=%lu\n", src, hdr.src_vNicId,
						hdr.dst_vNicId, hdr.tag, hdr.len);

    if ( m_recvM[hdr.dst_vNicId].find( hdr.tag ) == m_recvM[hdr.dst_vNicId].end() ) {
		m_dbg.verbose(CALL_INFO,2,0,"did't match tag\n");
        return false;
    }

    RecvEntry* entry = m_recvM[ hdr.dst_vNicId][ hdr.tag ].front();
    if ( entry->node() != -1 && entry->node() != src ) {
		m_dbg.verbose(CALL_INFO,2,0,"didn't match node  want=%#x src=%#x\n",
											entry->node(), src );
        return false;
    } 
    m_dbg.verbose(CALL_INFO,2,0,"recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,2,0,"found a receive entry\n");

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

bool Nic::RecvMachine::moveEvent( FireflyNetworkEvent* event )
{
    int src = event->src;
    m_dbg.verbose(CALL_INFO,1,0,"event has %lu bytes\n", event->bufSize() );
    if ( ! m_nic.m_arbitrateDMA->canIWrite( event->bufSize() ) ) {
        return false;
    }

    if ( 0 == m_activeRecvM[ src ]->currentVec && 
             0 == m_activeRecvM[ src ]->currentPos  ) {
        m_dbg.verbose(CALL_INFO,1,0,"local_vNic %d, recv srcNic=%d "
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
        m_dbg.verbose(CALL_INFO,1,0,"recv entry done, srcNic=%d src_vNic=%d\n",
                            src, m_activeRecvM[ src ]->src_vNic() );

        m_dbg.verbose(CALL_INFO,2,0,"copyIn %lu bytes\n",
                                            tmp - event->bufSize());

        m_activeRecvM[src]->notify();

        delete m_activeRecvM[src];
        m_activeRecvM.erase(src);

        if ( ! event->bufEmpty() ) {
            m_dbg.fatal(CALL_INFO,-1,
                "buffer not empty, %lu bytes remain\n",event->bufSize());
        }
    }

    m_dbg.verbose(CALL_INFO,2,0,"copyIn %lu bytes\n", tmp - event->bufSize());

    if ( 0 == event->bufSize() ) {
        m_dbg.verbose(CALL_INFO,1,0,"network event is done\n");
        delete event;
    }
    return true;
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,0,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

size_t Nic::RecvMachine::copyIn( Output& dbg, Nic::Entry& entry,
                                        FireflyNetworkEvent& event )
{
    dbg.verbose(CALL_INFO,3,0,"ioVec.size()=%lu\n", entry.ioVec().size() );


    for ( ; entry.currentVec < entry.ioVec().size();
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = entry.ioVec()[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.bufSize();
            size_t len = toLen < fromLen ? toLen : fromLen;

            char* toPtr = (char*) entry.ioVec()[entry.currentVec].ptr +
                                                        entry.currentPos;
            dbg.verbose(CALL_INFO,3,0,"toBufSpace=%lu fromAvail=%lu, "
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
                dbg.verbose(CALL_INFO,3,0,"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,3,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
}
