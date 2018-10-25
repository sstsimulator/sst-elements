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


#include <sst/core/interfaces/simpleNetwork.h>

class SendMachine {

        class OutQ {
            typedef std::function<void()> Callback;

            struct Entry  {
                Entry( std::pair< FireflyNetworkEvent*, int> data,  Callback callback = NULL ) : data(data), callback(callback) {}
                std::pair< FireflyNetworkEvent*, int>  data;
                Callback callback;
            };

            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:

            OutQ( Nic& nic, Output& output, int myId, int maxQsize ) : 
                m_nic(nic), m_dbg(output), m_id(myId), m_maxQsize(maxQsize),
                m_wakeUpCallback(NULL), m_lastEnq(0), m_enqCnt(0), m_qCnt(0)
            {
                m_prefix = "@t:"+ std::to_string(nic.getNodeId()) +":Nic::SendMachine" + std::to_string(myId) + "::OutQ::@p():@l ";
            }

            void enque( FireflyNetworkEvent* ev, int dest, Callback );

            bool isFull() { 
                return m_qCnt == m_maxQsize; 
            }

            void wakeMeUp( Callback  callback) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_SEND_MACHINE, "set wakeup\n");
                assert(! m_wakeUpCallback);
                m_wakeUpCallback = callback;
            }

            bool empty() {
                return m_qCnt == 0;
            }

            void pop( Callback callback ) {
                if ( m_wakeUpCallback ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_SEND_MACHINE, "call wakeup callback\n");
                    m_nic.schedCallback( m_wakeUpCallback, 0);
                    m_wakeUpCallback = NULL;
                }
				if ( callback ) {
					callback();
				}
             
				--m_qCnt;
            }

          private:

            Nic&        m_nic;
            Output&     m_dbg;
            int         m_id;
			int 		m_qCnt;
            int         m_maxQsize;
            Callback    m_wakeUpCallback;

			SimTime_t   m_lastEnq;
			int         m_enqCnt;
        };

        class InQ {

            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            typedef std::function<void()> Callback;

            InQ(Nic& nic, Output& output, int myId, OutQ* outQ, int maxQsize ) : 
                m_nic(nic), m_dbg(output), m_outQ(outQ), m_numPending(0),m_maxQsize(maxQsize), m_callback(NULL),
                m_pktNum(0), m_expectedPkt(0)
            {
                m_prefix = "@t:"+ std::to_string(nic.getNodeId()) +":Nic::SendMachine" + std::to_string(myId)  +"::InQ::@p():@l ";
            }

            bool isFull() {
                return m_numPending == m_maxQsize;
            }

            void  enque( int unit, int pid, std::vector< MemOp >* vec, FireflyNetworkEvent* ev, int dest, Callback callback = NULL );
        
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
            uint64_t    m_pktNum;
            uint64_t    m_expectedPkt;
            std::queue<Entry> m_pendingQ;
        };

      public:

        SendMachine( Nic& nic, int nodeId, int verboseLevel, int verboseMask, int myId,
              int packetSizeInBytes, int pktOverhead, int maxQsize, int unit, bool flag = false ) :
            m_nic(nic), m_id(myId), m_packetSizeInBytes( packetSizeInBytes - pktOverhead ), 
            m_unit(unit), m_pktOverhead(pktOverhead), m_activeEntry(NULL), m_I_manage( flag ), m_numSent(0)
        {
            char buffer[100];
            snprintf(buffer,100,"@t:%d:Nic::SendMachine%d::@p():@l ",nodeId,myId);

            m_dbg.init(buffer, verboseLevel, verboseMask, Output::STDOUT);
            m_outQ = new OutQ( nic, m_dbg, myId, maxQsize );
            m_inQ = new InQ( nic, m_dbg, myId, m_outQ, maxQsize );
        }

        ~SendMachine() { }

        int getNumSent() { return m_numSent; }

        bool isBusy() {
            return m_activeEntry;
        }       

        void run( SendEntryBase* entry ) {
            m_dbg.debug(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "new stream\n");
            assert( ! m_I_manage );
            m_activeEntry = entry;
            streamInit( entry );
        }

        void qSendEntry( SendEntryBase* entry ) {
            m_dbg.debug(CALL_INFO,2,NIC_DBG_SEND_MACHINE, "new stream\n");
            assert( m_I_manage );
            m_sendQ.push( entry );
            if ( m_sendQ.size() == 1 ) {
                streamInit( entry );
            }
        }

        int getId() { return m_id; }

      private:

        void streamInit( SendEntryBase* );
        void getPayload( SendEntryBase*, FireflyNetworkEvent* );
        void streamFini( SendEntryBase* );

        int     m_id;
        Nic&    m_nic;
        Output  m_dbg;
        OutQ*   m_outQ;
        InQ*    m_inQ;
        int     m_packetSizeInBytes;
        int     m_unit;
        int     m_pktOverhead;
        bool    m_I_manage;
        SendEntryBase* m_activeEntry;
        std::queue< SendEntryBase* > m_sendQ;

        int m_numSent;
};
