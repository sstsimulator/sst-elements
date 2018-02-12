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


#include <sst/core/interfaces/simpleNetwork.h>

class SendMachine {

        class OutQ {
            typedef std::function<void()> Callback;
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:

            OutQ( Nic& nic, Output& output, int vc, int maxQsize) : 
                m_nic(nic), m_dbg(output), m_maxQsize(maxQsize),
                m_vc(vc), m_wakeUpCallback(NULL), m_notifyCallback(NULL), m_scheduled(false)
            {
                m_prefix = "@t:"+ std::to_string(nic.getNodeId()) +":Nic::SendMachine::OutQ::@p():@l ";
            }

            void enque( FireflyNetworkEvent* ev, int dest );

            bool isFull() { 
                return m_queue.size() == m_maxQsize; 
            }

            void wakeMeUp( Callback  callback) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "set wakeup\n");
                assert(! m_wakeUpCallback);
                m_wakeUpCallback = callback;
            }

            void notify() {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "network is unblocked\n");
                assert( m_notifyCallback );
                m_scheduled = true;
                m_nic.schedCallback( m_notifyCallback ); 
                m_notifyCallback = NULL;
            }

          private:
            void send( std::pair< FireflyNetworkEvent*, int>& );

            void runSendQ( );

            bool canSend( uint64_t numBytes ) {
                bool ret = m_nic.m_linkControl->spaceToSend( m_vc, numBytes * 8); 
                if ( ! ret ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_SEND_MACHINE, "network is blocked\n");
                    setCanSendCallback( std::bind( &Nic::SendMachine::OutQ::runSendQ, this ) ); 
                }
                return ret;
            }

            void setCanSendCallback( Callback callback ) {
                assert( ! m_notifyCallback );
                m_notifyCallback = callback;
                m_nic.setNotifyOnSend( m_vc );
            }

            Nic&        m_nic;
            Output&     m_dbg;
            int         m_vc;
            int         m_maxQsize;
            bool        m_scheduled;
            static int  m_packetId;
            Callback    m_notifyCallback;
            Callback    m_wakeUpCallback;

            std::deque< std::pair< FireflyNetworkEvent*, int> > m_queue;
        };

        class InQ {


            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            typedef std::function<void()> Callback;

            InQ(Nic& nic, Output& output, OutQ* outQ, int unit, int maxQsize ) : 
                    m_nic(nic), m_dbg(output), m_outQ(outQ), m_numPending(0),m_maxQsize(maxQsize), m_callback(NULL),
                     m_unit( unit ), m_pktNum(0), m_expectedPkt(0)
            {
                m_prefix = "@t:"+ std::to_string(nic.getNodeId()) +":Nic::SendMachine::InQ::@p():@l ";
            }

            bool isFull() {
                return m_numPending == m_maxQsize;
            }

            void  enque( std::vector< MemOp >* vec, FireflyNetworkEvent* ev, int dest, Callback callback = NULL );
        
            void wakeMeUp( Callback  callback) {
                assert(!m_callback);
                m_callback = callback;
            }

          private:
            struct Entry { 
                Entry( FireflyNetworkEvent* ev, int dest, Callback callback, uint64_t pktNum ) : 
                            ev(ev), dest(dest), callback(callback), pktNum(pktNum) {}
                FireflyNetworkEvent* ev;
                int dest;
                Callback callback;
                uint64_t pktNum;
            };

            void ready( FireflyNetworkEvent* ev, int dest, Callback callback, uint64_t pktNum );
            void ready2( FireflyNetworkEvent* ev, int dest, Callback callback );
            void processPending();

            Nic&        m_nic;
            Output&     m_dbg;
            OutQ*       m_outQ;
            Callback    m_callback;
            int         m_numPending;
            int         m_maxQsize;
            int         m_unit;
            uint64_t    m_pktNum;
            uint64_t    m_expectedPkt;
            std::deque<Entry> m_pendingQ;
        };

      public:
        typedef std::function<void()> Callback;
        SendMachine( Nic& nic, int nodeId, int verboseLevel, int verboseMask,
              int txDelay, int packetSizeInBytes, int vc, int unit, int maxQsize  ) :
            m_nic(nic), m_txDelay( txDelay ), m_packetSizeInBytes( packetSizeInBytes), m_vc(vc)
        {
            assert( unit >= 0 );
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::SendMachine::@p():@l vc=%d ",nodeId,vc);

            m_dbg.init(buffer, verboseLevel, verboseMask, Output::STDOUT);
            m_outQ = new OutQ( nic, m_dbg, vc, maxQsize );
            m_inQ = new InQ( nic, m_dbg, m_outQ, unit, maxQsize );
        }

        ~SendMachine() { }

        void run( SendEntryBase* entry  ) {
            m_sendQ.push_back( entry );
            if ( 1 == m_sendQ.size() ) {
                initStream( m_sendQ.front() );
            } else {
                m_dbg.verbose(CALL_INFO,1,NIC_DBG_SEND_MACHINE, "Q send entry\n");
            }
        }

        void notify() {
            m_outQ->notify();
        }

        void printStatus( Output& out ); 

      private:

        void initStream( SendEntryBase* );
        void getPayload( SendEntryBase*, FireflyNetworkEvent* );
        void finiStream( SendEntryBase* );

        Nic&    m_nic;
        Output  m_dbg;
        OutQ*   m_outQ;
        InQ*    m_inQ;
        int     m_txDelay;
        int     m_packetSizeInBytes;
        int     m_vc;

        std::deque<SendEntryBase*>  m_sendQ;
};
