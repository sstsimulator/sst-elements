
class Ctx;
class StreamBase {
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            StreamBase(Output& output, Ctx* ctx, int unit, int srcNode, int srcPid, int myPid );
            virtual ~StreamBase();

            virtual SimTime_t getRxMatchDelay() { return m_ctx->getRxMatchDelay(); }
            virtual Callback getStartCallback() { return m_startCallback; }
            virtual SimTime_t getStartDelay()   { return m_startDelay; }

            bool postedRecv( DmaRecvEntry* entry );

            virtual void processPkt( FireflyNetworkEvent* ev );
            bool isBlocked();
            void needRecv( FireflyNetworkEvent* ev );

            void setWakeup( Callback callback )    {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"\n");
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

            SimTime_t       m_startDelay;
            Callback        m_startCallback;
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
