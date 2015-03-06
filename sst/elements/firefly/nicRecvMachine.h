// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class RecvMachine {

        enum State { NeedPkt, HavePkt, Move, WaitWrite,
                            Put, NeedRecv } m_state;

      public:
        RecvMachine( Nic& nic, Output& output ) : m_state(NeedPkt),
            m_nic(nic), m_dbg(output), m_rxMatchDelay( 100 ) { }
        ~RecvMachine();
        void init( int numVnics, int rxMatchDelay ) {
            m_recvM.resize( numVnics );
            m_rxMatchDelay = rxMatchDelay;
        }
        void run();

        void addDma( int vNic, int tag, RecvEntry* entry) {
            m_recvM[ vNic ][ tag ].push_back( entry );
            if ( HavePkt == m_state ) {
                run();
            }
        }

      private:
        uint64_t processFirstEvent( MerlinFireflyEvent&, State&, Entry* & );
        bool findRecv( int src, MsgHdr& );
        SendEntry* findGet( int src, MsgHdr& hdr, RdmaMsgHdr& rdmaHdr );
        bool findPut(int src, MsgHdr& hdr, RdmaMsgHdr& rdmaHdr );
        bool moveEvent( MerlinFireflyEvent* );
        size_t copyIn( Output& dbg, Nic::Entry& entry,
                    MerlinFireflyEvent& event );

        void processNeedRecv( MerlinFireflyEvent* event ) {
            MsgHdr& hdr = *(MsgHdr*) &event->buf[0];
            m_nic.notifyNeedRecv( hdr.dst_vNicId, hdr.src_vNicId,
                     event->src, hdr.tag, hdr.len);
        }

        MerlinFireflyEvent* getMerlinEvent(int vc ) {
            MerlinFireflyEvent* event =
                static_cast<MerlinFireflyEvent*>(
                                    m_nic.m_linkControl->recv( vc ) );
            if ( event ) {
                event->src = m_nic.NetToId( event->src );
            }
            return event;
        }
        void setNotify() {
            m_nic.m_linkControl->setNotifyOnReceive(
                                    m_nic.m_recvNotifyFunctor );
        }

        void clearNotify() {
            m_nic.m_linkControl->setNotifyOnReceive( NULL );
        }

        Nic&        m_nic;
        Output&     m_dbg;

        MerlinFireflyEvent* m_mEvent;
        int                 m_rxMatchDelay;
        std::map< int, RecvEntry* >     m_activeRecvM;

        std::vector< std::map< int, std::deque<RecvEntry*> > > m_recvM;
};
