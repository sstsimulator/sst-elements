// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

       class Ctx {
            std::string m_prefix;
            const char* prefix() { return m_prefix.c_str(); }
          public:
            Ctx( Output& output, RecvMachine& rm, int pid, int qsize ) :
                    m_dbg(output), m_rm(rm), m_pid(pid), m_maxQsize(qsize)
            {
                m_prefix = "@t:"+ std::to_string(rm.nic().getNodeId()) +":Nic::RecvMachine::Ctx" + std::to_string(pid) + "::@p():@l ";
            }

            int getHostReadDelay() { return m_rm.m_hostReadDelay; }
            StreamBase* newStream( FireflyNetworkEvent* );
            DmaRecvEntry* findPut( int src, MsgHdr& hdr, RdmaMsgHdr& rdmahdr );
            EntryBase* findRecv( int srcNode, int srcPid, MsgHdr& hdr, MatchMsgHdr& matchHdr  );
            SendEntryBase* findGet( int srcNode, int srcPid, RdmaMsgHdr& rdmaHdr );

            void regMemRgn( int rgnNum, MemRgnEntry* entry ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX, "rgnNum=%d\n",rgnNum);
                m_rm.m_nic.m_recvCtxData[m_pid].m_memRgnM[rgnNum] = entry;
            }
            void regGetOrigin( int key, DmaRecvEntry* entry ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX, "key=%d\n",key);
                m_rm.m_nic.m_recvCtxData[m_pid].m_getOrgnM[key] = entry;
            }
            void postRecv( DmaRecvEntry* entry ) {
                // check to see if there are active streams for this pid
                // if there are they may be blocked waiting for the host to post a recv

                if (  ! m_blockedStreamQ.empty() ) {
                    if ( m_blockedStreamQ.front()->postedRecv( entry ) ) {
                        m_blockedStreamQ.pop();
                        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX,"m_blockedStreamQ.size()=%zu\n",m_blockedStreamQ.size());
                        return;
                    }
                }
                m_rm.m_nic.m_recvCtxData[m_pid].m_postedRecvs.push_back( entry );
            }

            Nic::Shmem* getShmem() {
                return  m_rm.m_nic.m_shmem;
            }

            std::queue<StreamBase*> m_blockedStreamQ;
            void needRecv( StreamBase* stream ) {

                m_blockedStreamQ.push( stream );
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,NIC_DBG_RECV_CTX, "pid %d blocked, srcNode=%d srcPid=%d m_blockedStreamQ.size()=%zu\n",
                    stream->getSrcPid(), stream->getSrcNode(), stream->getSrcPid(), m_blockedStreamQ.size() );

                m_rm.m_nic.notifyNeedRecv( stream->getMyPid(), stream->getSrcNode(), stream->getSrcPid(), stream->length() );
            }


            Hermes::MemAddr findShmem(  int core, Hermes::Vaddr addr, size_t length ) {
                return m_rm.nic().findShmem( core, addr, length );
            }

            void checkWaitOps( int core, Hermes::Vaddr addr, size_t length ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,NIC_DBG_RECV_CTX,"\n");
                m_rm.m_nic.m_shmem->checkWaitOps( core, addr, length );
            }


            void calcNicMemDelay( int unit, std::vector< MemOp>* ops, std::function<void()> callback ) {
                m_rm.nic().calcNicMemDelay( unit, m_pid, ops, callback );
            }

            void schedCallback( Callback callback, uint64_t delay = 0 ) {
                m_rm.nic().schedCallback( callback, delay );
            }

            void runSend( int num, SendEntryBase* entry ) {
                m_rm.nic().qSendEntry( entry );
            }

            // this function is called by a Stream object, we don't want to delete ourself,
            // break the cycle by scheduling a callback
            void deleteStream( StreamBase* stream ) {
                m_rm.nic().schedCallback( [=]()
                    {

                        m_dbg.verbosePrefix(prefix(),CALL_INFO_LAMBDA,"deleteStream",1,NIC_DBG_RECV_CTX,"%p\n",stream);
                        m_rm.decActiveStream();
                        delete stream;
                    }
                );
            }

            SimTime_t getRxMatchDelay() {
                return m_rm.m_rxMatchDelay;
            }

            int allocAckUnit() {
                return m_rm.m_nic.allocNicAckUnit( );
            }
            int allocRecvUnit() {
                return m_rm.m_nic.allocNicRecvUnit( m_pid );
            }

            Nic& nic() { return m_rm.nic(); }
            int getMaxQsize() { return m_maxQsize; }

          private:
            Output&         m_dbg;
            RecvMachine&    m_rm;
            int                              m_pid;
            int             m_maxQsize;
        };
