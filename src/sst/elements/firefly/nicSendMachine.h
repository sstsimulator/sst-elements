// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.   


#include <sst/core/interfaces/simpleNetwork.h>

class SendMachine {

      public:
        SendMachine( Nic& nic, Output& output ) :
            m_nic(nic), m_dbg(output), m_txDelay(50),
			m_packetId( 0 ), m_notifyCallback(NULL)
#ifdef NIC_SEND_DEBUG
            , m_msgCount(0)
#endif
        { }

        ~SendMachine();

        void init( int txDelay, int packetSizeInBytes, int vc ) {
            m_txDelay = txDelay;
            m_packetSizeInBytes = packetSizeInBytes;
			m_vc = vc;
        }

        void run( SendEntry* entry  ) {
            m_sendQ.push_back( entry );
            if ( 1 == m_sendQ.size() ) {
                state_0( m_sendQ.front() );
            } else {
                m_dbg.verbose(CALL_INFO,1,NIC_DBG_SEND_MACHINE,"Send: "
							"%d: Q send entry\n",m_vc);
            }
        }

        void notify() {
            assert( m_notifyCallback );
            m_nic.schedCallback( m_notifyCallback ); 
            m_notifyCallback = NULL;
        }

        void printStatus( Output& out ); 

      private:

        bool canSend( uint64_t numBytes ) {
           return m_nic.m_linkControl->spaceToSend( m_vc, numBytes * 8); 
        }

        void setCanSendCallback( Callback callback ) {
            assert( ! m_notifyCallback );
            m_notifyCallback = callback;
            m_nic.setNotifyOnSend( m_vc );
        }

        void state_0( SendEntry* );
        void state_1( SendEntry*, FireflyNetworkEvent* );
        void state_2( SendEntry*, FireflyNetworkEvent* );
        void state_3( ) {
            if ( ! canSend( m_packetSizeInBytes ) ) {
                m_dbg.verbose(CALL_INFO,2,NIC_DBG_SEND_MACHINE,"Send: "
							"%d: send busy\n",m_vc);
                    setCanSendCallback(
                    std::bind( &Nic::SendMachine::state_3, this )
                );
                return;
            }
            m_sendQ.pop_front( );
            if ( ! m_sendQ.empty() ) {
                state_0( m_sendQ.front() );
            }
        } 

        void copyOut( Output& dbg, FireflyNetworkEvent& event,
                                Nic::Entry& entry, std::vector<DmaVec>& );

        Nic&        m_nic;
        Output&     m_dbg;

        std::deque<SendEntry*>  m_sendQ;
        int                     m_txDelay;
        unsigned int            m_packetSizeInBytes;
        int                     m_packetId;
        Callback                m_notifyCallback;
		int						m_vc;
#ifdef NIC_SEND_DEBUG
        unsigned int            m_msgCount;
#endif
};
