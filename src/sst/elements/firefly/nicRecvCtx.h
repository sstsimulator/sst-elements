       class Ctx {
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            Ctx( Output& output, RecvMachine& rm, int pid, int qsize ) : m_dbg(output), m_rm(rm), m_pid(pid), 
                    m_blockedNetworkEvent(NULL), m_maxQsize(qsize) {
                m_prefix = "@t:"+ std::to_string(rm.nic().getNodeId()) +":Nic::RecvMachine::Ctx" + std::to_string(pid) + "::@p():@l ";
            }

            bool processPkt( FireflyNetworkEvent* ev );
            DmaRecvEntry* findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr );
            EntryBase* findRecv( int srcNode, int srcPid, MsgHdr& hdr, MatchMsgHdr& matchHdr  );
            SendEntryBase* findGet( int srcNode, int srcPid, RdmaMsgHdr& rdmaHdr );

            void regMemRgn( int rgnNum, MemRgnEntry* entry ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "rgnNum=%d\n",rgnNum);
                m_memRgnM[ rgnNum ] = entry;
            }
            void regGetOrigin( int key, DmaRecvEntry* entry ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "key=%d\n",key);
                m_getOrgnM[ key ] = entry;
            }
            void postRecv( DmaRecvEntry* entry ) {
                // check to see if there are active streams for this pid
                // if there are they may be blocked waiting for the host to post a recv
                if ( ! m_streamMap.empty() ) {
                    std::map< SrcKey, StreamBase* >& tmp = m_streamMap;
                    std::map< SrcKey, StreamBase* >::iterator iter = tmp.begin();
                    for ( ; iter != tmp.end(); ++iter ) {
                        if( iter->second->postedRecv( entry ) ) {
                            return;
                        }
                    }
                }
                m_postedRecvs.push_back( entry );
            }

            Nic::Shmem* getShmem() {
                return  m_rm.m_nic.m_shmem;
            }

            void needRecv( StreamBase* stream ) {

                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE, "pid%d blocked, srcNode=%d srcPid=%d\n",
                    stream->getSrcPid(), stream->getSrcNode(), stream->getSrcPid() );

                m_rm.m_nic.notifyNeedRecv( stream->getMyPid(), stream->getSrcNode(), stream->getSrcPid(), stream->length() );
            }


            Hermes::MemAddr findShmem(  int core, Hermes::Vaddr addr, size_t length ) {
                return m_rm.nic().findShmem( core, addr, length );
            }

            void checkWaitOps( int core, Hermes::Vaddr addr, size_t length ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");
                m_rm.m_nic.m_shmem->checkWaitOps( core, addr, length );
            }


            void calcNicMemDelay( int unit, std::vector< MemOp>* ops, std::function<void()> callback ) {
                m_rm.nic().calcNicMemDelay( unit, m_pid, ops, callback );
            }

            void schedCallback( Callback callback, uint64_t delay = 0 ) {
                m_rm.nic().schedCallback( callback, delay );
            }

            void runSend( int num, SendEntryBase* entry ) {
                m_rm.nic().m_sendMachine[0]->run( entry );
            } 

            // this function is called by a Stream object, we don't want to delete ourself,
            // break the cycle by scheduling a callback
            void deleteStream( StreamBase* stream ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"%p\n",stream);
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::Ctx::deleteStream2, this, stream ) );
            }

            void clearMapAndDeleteStream( StreamBase* stream ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"%p\n",stream);
                m_rm.nic().schedCallback( std::bind( &Nic::RecvMachine::Ctx::deleteStream2, this, stream ) );
                clearStreamMap( stream->getSrcKey() );
            }

            void deleteStream2( StreamBase* stream ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"%p\n",stream);
                m_rm.nic().freeNicUnit( stream->getUnit() );
                delete stream;
            }
            SimTime_t getRxMatchDelay() {
                return m_rm.m_rxMatchDelay;
            }

            int allocNicUnit() {
                return m_rm.m_nic.allocNicUnit( m_pid );
            }

            void clearStreamMap( SrcKey key ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_MACHINE,"\n");
                m_streamMap.erase(key);
                if ( m_blockedNetworkEvent ) {
                    FireflyNetworkEvent* ev = m_blockedNetworkEvent;
                    if(  getSrcKey( ev->getSrcNode(), ev->getSrcPid() ) == key ) {
                        m_blockedNetworkEvent = NULL;
                        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_MACHINE,"have blocked packet\n");
                        if ( ! processPkt( ev ) ) {
                            m_rm.checkNetworkForData();
                        }
                    }
                } else { 
                }
            }   

            Nic& nic() { return m_rm.nic(); }
            int getMaxQsize() { return m_maxQsize; }

          private:
            StreamBase* newStream( int unit, FireflyNetworkEvent* );
            Output&         m_dbg;
            RecvMachine&    m_rm;
            int                              m_pid;
            std::map< SrcKey, StreamBase*>   m_streamMap;
            FireflyNetworkEvent*             m_blockedNetworkEvent;
            std::map< int, DmaRecvEntry* >   m_getOrgnM;
            std::map< int, MemRgnEntry* >    m_memRgnM;
            std::deque<DmaRecvEntry*>        m_postedRecvs;
            int             m_maxQsize;
        };
