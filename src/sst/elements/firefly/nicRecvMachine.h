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


class RecvMachine {

    #include "nicShmemRecvMachine.h"

        
    typedef uint64_t SrcKey;
    typedef std::function<void()> Callback;

    static SrcKey getSrcKey(int srcNode, int srcPid, int srcStream) { 
		union Key {
			uint64_t value;
			struct x {
				uint16_t pid : 12;
				uint16_t stream : STREAM_NUM_SIZE;
				uint32_t nid : 20;
			} x;
		} tmp;

		tmp.x.pid = srcPid;
		tmp.x.nid = srcNode;
		tmp.x.stream = srcStream;
		return tmp.value; 
	}

    #include "nicRecvStream.h"
    #include "nicRecvCtx.h"
    #include "nicMsgStream.h"
    #include "nicRdmaStream.h"
    #include "nicShmemStream.h"

      public:

        RecvMachine( Nic& nic, int vc, int numVnics, 
                int nodeId, int verboseLevel, int verboseMask,
                int rxMatchDelay, int hostReadDelay, int maxQsize, int maxActiveStreams ) :
            m_nic(nic), 
            m_vc(vc), 
            m_rxMatchDelay( rxMatchDelay ),
            m_hostReadDelay( hostReadDelay ),
            m_notifyCallback( false ),
            m_numActiveStreams( 0 ),
            m_maxActiveStreams( maxActiveStreams ),
            m_blockedPkt(NULL),
            m_numMsgRcvd(0),
            m_hostBlockingTime(0)
        { 
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::RecvMachine::@p():@l vc=%d ",nodeId,m_vc);

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
            assert( m_numActiveStreams >= 0 );
             
            if ( m_blockedPkt ) {
      		    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"unblocked\n");
                assert( m_blockedPkt );
                processPkt2( m_blockedPkt );
                m_blockedPkt = NULL;
            }
        }

    protected:
        Nic&        m_nic;
        Output      m_dbg;
        int         m_hostReadDelay;
        std::vector< Ctx* >   m_ctxMap;

        void checkNetworkForData() {
            FireflyNetworkEvent* ev = getNetworkEvent( m_vc );
            if ( ev ) {
                m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"packet available\n");
                m_nic.schedCallback( std::bind( &Nic::RecvMachine::processPkt, this, ev ));
            } else {
                m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"network idle\n");
                setNotify();
            }
        }
        
	private:
        void setNotify( ) {
            assert( ! m_notifyCallback );
            if( ! m_notifyCallback ) {
                m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE, "\n");
                m_nic.m_linkRecvWidget->setNotify(
                                    std::bind(&Nic::RecvMachine::processNetworkData, this), m_vc );
                m_notifyCallback = true;
            }
        }

        void processNetworkData() {
            m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE, "\n");

            // this notifier was called by the LinkControl object, the RecvMachine may 
            // re-install the LinkControl notifier, if it does there would be a cycle
            // this schedCallback breaks the cycle
            m_nic.schedCallback(  [=](){ processPkt( getNetworkEvent( m_vc ) ); } );
            m_notifyCallback = false;
        }


        int m_maxActiveStreams; 
        int m_numActiveStreams;
        FireflyNetworkEvent* m_blockedPkt;
        virtual void processPkt( FireflyNetworkEvent* ev ) {

            if ( ev->isHdr() ) {
                ++m_numActiveStreams;
            } 

            if ( m_numActiveStreams == m_maxActiveStreams + 1) {
      		    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"blocked on available streams\n");
                assert( ! m_blockedPkt );
                m_blockedPkt = ev;
            } else {
                processPkt2( ev );
            } 
        }
                
        virtual void processPkt2( FireflyNetworkEvent* ev ) {
            // if the event was consumed, we can can check for the next
            if ( ! m_ctxMap[ ev->getDestPid() ]->processPkt( ev ) ) {
                checkNetworkForData();
            } else {
      		    m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"blocked by context\n");
				m_hostBlockingTime = Simulation::getSimulation()->getCurrentSimCycle();
            }
        }

        FireflyNetworkEvent* getNetworkEvent(int vc ) {
            SST::Interfaces::SimpleNetwork::Request* req =
                m_nic.m_linkControl->recv(vc);

            if ( m_hostBlockingTime ) {
                uint64_t latency = Simulation::getSimulation()->getCurrentSimCycle() - m_hostBlockingTime;
				if ( latency ) {
                	m_nic.m_hostStall->addData( latency );
				}
                m_hostBlockingTime = 0;
            }

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

        SimTime_t m_hostBlockingTime;		
        int m_numMsgRcvd;
        int m_receivedPkts;
        int         m_vc;
        int         m_rxMatchDelay;
        bool        m_notifyCallback; 

};
