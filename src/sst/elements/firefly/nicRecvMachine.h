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


class RecvMachine {

    #include "nicShmemRecvMachine.h"

    typedef std::function<void()> Callback;

	typedef uint64_t ProcessPairId;
	static ProcessPairId getPPI(FireflyNetworkEvent* ev) {
		ProcessPairId value;
		value = (uint64_t) ev->getSrcNode() << 32;
		value |= ev->getSrcPid() << 16;
		value |= ev->getDestPid();
		return value;
	}

    #include "nicRecvStream.h"
    #include "nicRecvCtx.h"
    #include "nicMsgStream.h"
    #include "nicRdmaStream.h"
    #include "nicShmemStream.h"

      public:


        RecvMachine( Nic& nic, int vn, int numVnics,
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay, int maxQsize, int maxActiveStreams, int maxPendingPkts ) :
            m_nic(nic),
            m_vn(vn),
            m_rxMatchDelay( rxMatchDelay ),
            m_hostReadDelay( hostReadDelay ),
            m_numActiveStreams( 0 ),
            m_maxActiveStreams( maxActiveStreams ),
            m_numMsgRcvd(0),
            m_clockLat(1),
            m_clocking(false),
            m_numPendingPkts(0),
            m_maxPendingPkts(maxPendingPkts)
        {
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::RecvMachine::@p():@l vn=%d ",nodeId,m_vn);

            m_dbg.init(buffer, verboseLevel, verboseMask, Output::STDOUT);
            setNotify();
            for ( unsigned i=0; i < numVnics; i++) {
                m_ctxMap.push_back( new Ctx( m_dbg, *this, i, maxQsize ) );
            }
        }

        int getNumReceived() { return m_numMsgRcvd; }
        virtual ~RecvMachine(){ }

        void regMemRgn( int pid, int rgnNum, MemRgnEntry* entry ) {
            m_ctxMap[pid]->regMemRgn( rgnNum, entry );
        }

        void regGetOrigin( int pid, int key, DmaRecvEntry* entry ) {
            m_ctxMap[pid]->regGetOrigin( key, entry );
        }
        void postRecv( int pid, DmaRecvEntry* entry ) {
            m_ctxMap[pid]->postRecv( entry );
        }
        Nic& nic() { return m_nic; }
        void printStatus( Output& out );

        void decActiveStream() {
            --m_numActiveStreams;
            m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"numActiveStreams=%d\n",m_numActiveStreams);
            assert( m_numActiveStreams >= 0 );
        }

    protected:
        Nic&        m_nic;
        Output      m_dbg;
        int         m_hostReadDelay;
        std::vector< Ctx* >   m_ctxMap;

	private:
        void processPkt( FireflyNetworkEvent* ev );
        void processStdPkt( FireflyNetworkEvent* ev );

        void setNotify( ) {
            m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");
            m_nic.m_linkRecvWidget->setNotify( std::bind(&Nic::RecvMachine::networkHasPkt, this), m_vn );
        }

        void networkHasPkt() {
            // when this function is call, we will nod longer be notified
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "\n");
            if ( ! m_clocking ) {
                nic().schedCallback( std::bind(  &Nic::RecvMachine::clockHandler, this ), 0 );
                m_clocking = true;
            }
        }
        void clockHandler() {
            m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");

            if ( m_numPendingPkts < m_maxPendingPkts ) {

                FireflyNetworkEvent* ev = getNetworkEvent( m_vn );
                if ( ev ) {
                    ++m_numPendingPkts;
                    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "got packet numPendingPkts=%d\n", m_numPendingPkts );
                    m_pktBuf[ getPPI(ev) ].push( ev );
                }
            } else {
                m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "reached max buffered packets, numPendingPkts=%d\n", m_numPendingPkts );
			}

			auto iter = m_pktBuf.begin();
			while ( iter != m_pktBuf.end() ) {
				FireflyNetworkEvent* ev = iter->second.front();

				m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "packet from node=%d pid=%d for pid=%d %s %s PPI=0x%" PRIx64 "\n",
						ev->getSrcNode(),ev->getSrcPid(),ev->getDestPid(),ev->isHdr() ? "hdr":"",ev->isTail() ? "tail":"",getPPI(ev));

				if ( ev->isCtrl() ) {
					++m_numActiveStreams;
					m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "ctrl packet numActiveStreams=%d m_numPendingPkts=%d\n",m_numActiveStreams,m_numPendingPkts-1);
					processPkt( ev );
					--m_numPendingPkts;
					iter->second.pop();
				} else if ( m_streamMap.find(iter->first) == m_streamMap.end() ) {
					if ( m_numActiveStreams < m_maxActiveStreams ) {
						++m_numActiveStreams;
						m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "new stream numActiveStreams=%d m_numPendingPkts=%d\n",m_numActiveStreams,m_numPendingPkts-1);
						processPkt( ev );
						--m_numPendingPkts;
						iter->second.pop();
					} else {
						m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "can't start new stream numActiveStreams=%d\n",m_numActiveStreams);
					}
				} else {
					if ( ! m_streamMap[iter->first]->isBlocked( ) ) {
						processPkt( ev );
						--m_numPendingPkts;
						m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "stream consumed packet, m_numPendingPkts=%d\n",m_numPendingPkts);
						iter->second.pop();
					} else {
						m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "stream blocked\n");
					}
				}
				if ( iter->second.empty() ) {
					m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "queue is empty clear pktBuf slot\n");
					iter = m_pktBuf.erase( iter );
				} else {
					++iter;
				}
			}

			if ( m_pktBuf.empty() && ! m_nic.m_linkControl->requestToReceive( m_vn )) {
				m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "pktBuf is empty\n");
                setNotify();
                m_clocking = false;
			} else {
				m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "schedule clockHandler m_numPendingPkts=%d m_numActiveStreams=%d\n",m_numPendingPkts,m_numActiveStreams);
                nic().schedCallback( std::bind(  &Nic::RecvMachine::clockHandler, this ), m_clockLat );
			}
        }


        FireflyNetworkEvent* getNetworkEvent(int vn ) {
            SST::Interfaces::SimpleNetwork::Request* req = m_nic.m_linkControl->recv(vn);

            if ( req ) {

				m_nic.m_rcvdPkts->addData(1);


                Event* payload = req->takePayload();
                if ( NULL == payload ) return NULL;

                m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"got packet\n");


                FireflyNetworkEvent* event =
                    static_cast<FireflyNetworkEvent*>(payload);
                event->setSrcNode( m_nic.NetToId( req->src ) );
				m_nic.m_rcvdByteCount->addData( event->payloadSize() );
                delete req;
                if ( ! event->isCtrl() && event->isHdr() ) {
                    ++m_numMsgRcvd;
                }
                return event;
            } else {
                return NULL;
            }
        }

        int m_numPendingPkts;
        int m_maxPendingPkts;
        int m_maxActiveStreams;
        int m_numActiveStreams;
        int m_numMsgRcvd;
        int m_receivedPkts;
        int m_vn;
        int m_rxMatchDelay;

        SimTime_t   m_clockLat;
        bool        m_clocking;

        std::unordered_map<ProcessPairId, std::queue<FireflyNetworkEvent*> > m_pktBuf;
        std::unordered_map<ProcessPairId, StreamBase* >  m_streamMap;
};
