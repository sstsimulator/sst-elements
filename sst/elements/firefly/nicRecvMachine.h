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

#undef FOREACH_ENUM

#define FOREACH_ENUM(NAME) \
    NAME(NeedPkt) \
    NAME(HavePkt) \
    NAME(Move) \
    NAME(WaitWrite) \
    NAME(Put) \
    NAME(NeedRecv) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

class RecvMachine {

        enum State {
            FOREACH_ENUM(GENERATE_ENUM)
        } m_state;

      public:
        RecvMachine( Nic& nic, Output& output ) : m_state(NeedPkt),
            m_nic(nic), m_dbg(output), m_rxMatchDelay( 100 ),
            m_hostReadDelay( 200 )
#ifdef NIC_RECV_DEBUG
            , m_msgCount(0), m_runCount(0) 
#endif
        { }
        ~RecvMachine();
        void init( int numVnics, int rxMatchDelay, int hostReadDelay ) {
            m_recvM.resize( numVnics );
            m_rxMatchDelay = rxMatchDelay;
            m_hostReadDelay = hostReadDelay;
        }
        void run();

        void addDma( int vNic, int tag, RecvEntry* entry) {
            m_recvM[ vNic ][ tag ].push_back( entry );
            if ( HavePkt == m_state ) {
                run();
            }
        }
        void printStatus( Output& out ) {
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

        const char* state( State state ) {
            switch( state ) {
                case NeedPkt:
                    return "NeedPkt";
                case HavePkt:
                    return "HavePkt";
                case Move:
                    return "HavePkt";
                case WaitWrite:
                    return "HavePkt";
                case Put:
                    return "HavePkt";
                case NeedRecv:
                    return "HavePkt";
            }
            assert(0);
        }

      private:
        uint64_t processFirstEvent( FireflyNetworkEvent&, State&, Entry* & );
        bool findRecv( int src, MsgHdr& );
        SendEntry* findGet( int src, MsgHdr& hdr, RdmaMsgHdr& rdmaHdr );
        bool findPut(int src, MsgHdr& hdr, RdmaMsgHdr& rdmaHdr );
        bool moveEvent( FireflyNetworkEvent* );
        size_t copyIn( Output& dbg, Nic::Entry& entry,
                    FireflyNetworkEvent& event );

        void processNeedRecv( FireflyNetworkEvent* event ) {
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
            }
            else {
                return NULL;
            }
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

        FireflyNetworkEvent* m_mEvent;
        int                 m_rxMatchDelay;
        int                 m_hostReadDelay;
        std::map< int, RecvEntry* >     m_activeRecvM;

        std::vector< std::map< int, std::deque<RecvEntry*> > > m_recvM;
        static const char*  m_enumName[];
#ifdef NIC_RECV_DEBUG 
        unsigned int    m_msgCount;
        unsigned int    m_runCount;
#endif
};
