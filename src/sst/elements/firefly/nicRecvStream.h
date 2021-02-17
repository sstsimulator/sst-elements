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

class Ctx;
class StreamBase {
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            StreamBase(Output& output, Ctx* ctx, int srcNode, int srcPid, int myPid );
            virtual ~StreamBase();

            virtual SimTime_t getRxMatchDelay() { return m_ctx->getRxMatchDelay(); }
            bool postedRecv( DmaRecvEntry* entry );
            virtual void processPktBody( FireflyNetworkEvent* ev );

            virtual bool isBlocked( ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"%d\n",m_numPending == m_ctx->getMaxQsize());
                return m_numPending == m_ctx->getMaxQsize();
            }

            void needRecv() {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"stream=%p\n",this);
                m_ctx->needRecv( this );
            }

            void qSend( SendEntryBase* entry ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"\n");
                m_ctx->runSend( m_unit, entry );
                m_ctx->deleteStream( this );
            }

            void setWakeup( Callback callback )    {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_STREAM,"\n");
                m_wakeupCallback = callback;
            }

            RecvEntryBase* getRecvEntry()   { return m_recvEntry; }
            virtual size_t length()         { return m_matched_len; }
            int getSrcPid()                 { return m_srcPid; }
            int getSrcNode()                { return m_srcNode; }
            int getMyPid()                  { return m_myPid; }
            int getUnit()                   { return m_unit; }

          protected:

            void ready( bool, uint64_t pktNum );

            FireflyNetworkEvent* m_hdrPkt;
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
