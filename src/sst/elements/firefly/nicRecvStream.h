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

class Ctx;
class StreamBase {
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            StreamBase(Output& output, Ctx* ctx, int srcNode, int srcPid, int myPid );
            virtual ~StreamBase();

            virtual SimTime_t getRxMatchDelay() { return m_ctx->getRxMatchDelay(); }
            bool postedRecv( DmaRecvEntry* entry );

            virtual void processPkt( FireflyNetworkEvent* ev ) {
                if ( ev->isHdr() ) {
                    processPktHdr(ev);
                } else {
                    processPktBody(ev);
                }
            }
            virtual void processPktHdr( FireflyNetworkEvent* ev ) { assert(0); }
            virtual void processPktBody( FireflyNetworkEvent* ev );

            virtual bool isBlocked();
            void needRecv( FireflyNetworkEvent* ev );
        
            void qSend( SendEntryBase* entry ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"\n");
                m_ctx->runSend( m_unit, entry );
                m_ctx->deleteStream( this );
            }

            void setWakeup( Callback callback )    {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"\n");
                m_wakeupCallback = callback;
            }

            SrcKey getSrcKey()              { return (SrcKey) m_srcNode << 32 | m_srcPid; }
            RecvEntryBase* getRecvEntry()   { return m_recvEntry; }
            virtual size_t length()         { return m_matched_len; }
            int getSrcPid()                 { return m_srcPid; }
            int getSrcNode()                { return m_srcNode; }
            int getMyPid()                  { return m_myPid; }
            int getUnit()                   { return m_unit; }

          protected:

            void ready( bool, uint64_t pktNum );

            FireflyNetworkEvent* m_blockedNeedRecv;
            Callback        m_wakeupCallback;
            Output&         m_dbg;
            Ctx*            m_ctx;
            SendEntryBase*  m_sendEntry;
            RecvEntryBase*  m_recvEntry;
            int             m_srcNode;
            int             m_srcPid;
            int             m_myPid;
            int             m_matched_len;
            int             m_matched_tag;
            int             m_unit;
            SimTime_t       m_start;
            int             m_numPending;
            int             m_maxQsize;
            uint64_t        m_pktNum;
            uint64_t        m_expectedPkt;
        };
