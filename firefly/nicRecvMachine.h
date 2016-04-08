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


class RecvMachine {

      public:
        RecvMachine( Nic& nic, Output& output ) :
            m_nic(nic), m_dbg(output), m_rxMatchDelay( 100 ),
            m_hostReadDelay( 200 ), m_blockedCallback( NULL ),
            m_notifyCallback( false )
#ifdef NIC_RECV_DEBUG
            , m_msgCount(0) 
#endif
        { 
        }

        ~RecvMachine();

        void init( int numVnics, int rxMatchDelay, int hostReadDelay ) {
            m_recvM.resize( numVnics );
            m_rxMatchDelay = rxMatchDelay;
            m_hostReadDelay = hostReadDelay;
        }

        void notify( int vc ) {
            m_nic.schedCallback( std::bind( &Nic::RecvMachine::state_n0, this, vc ) );
            m_notifyCallback = false;
        }

        void addDma( int vNic, int tag, RecvEntry* entry) {
            m_recvM[ vNic ][ tag ].push_back( entry );
            if ( m_blockedCallback ) {
                
                m_nic.schedCallback( m_blockedCallback );
                m_blockedCallback = NULL; 
            }
        }
        void printStatus( Output& out );

        void setNotify( ) {
            assert( ! m_notifyCallback );
            m_nic.m_linkControl->setNotifyOnReceive(
                                    m_nic.m_recvNotifyFunctor );
            m_notifyCallback = true;
        }

      private:
        void state_0( FireflyNetworkEvent* );
        void state_1( FireflyNetworkEvent* );
        void state_2( FireflyNetworkEvent* );
        void state_3( SendEntry* );
        void state_move_0( FireflyNetworkEvent* );
        void state_move_1( FireflyNetworkEvent* );
        void checkNetwork();

        bool findRecv( int src, MsgHdr& );
        SendEntry* findGet( int src, MsgHdr& hdr, RdmaMsgHdr& rdmaHdr );
        bool findPut(int src, MsgHdr& hdr, RdmaMsgHdr& rdmaHdr );
        size_t copyIn( Output& dbg, Nic::Entry& entry,
                    FireflyNetworkEvent& event );

        void state_n0( int vc ) {
            if ( vc == 0 ) {
                state_0( getNetworkEvent( vc ) );
            } else {
                state_1( getNetworkEvent( vc ) );
            }
        }

        void processNeedRecv( FireflyNetworkEvent* event, Callback callback ) {
            m_blockedCallback = callback;
            MsgHdr& hdr = *(MsgHdr*) event->bufPtr();
            m_nic.notifyNeedRecv( hdr.dst_vNicId, hdr.src_vNicId,
                     event->src, hdr.tag, hdr.len);
        }

        FireflyNetworkEvent* getNetworkEvent(int vc ) {
            SST::Interfaces::SimpleNetwork::Request* req =
                m_nic.m_linkControl->recv(vc);
            if ( req ) {
                Event* payload = req->takePayload();
                if ( NULL == payload ) return NULL;
                FireflyNetworkEvent* event =
                    static_cast<FireflyNetworkEvent*>(payload);
                event->src = m_nic.NetToId( req->src );
                delete req;
                return event;
            } else {
                return NULL;
            }
        }


        Nic&        m_nic;
        Output&     m_dbg;

        int                 m_rxMatchDelay;
        int                 m_hostReadDelay;
        Callback            m_blockedCallback;
        bool            m_notifyCallback; 
        std::map< int, RecvEntry* >     m_activeRecvM;

        std::vector< std::map< int, std::deque<RecvEntry*> > > m_recvM;
        
#ifdef NIC_RECV_DEBUG 
        unsigned int        m_msgCount;
#endif
};
