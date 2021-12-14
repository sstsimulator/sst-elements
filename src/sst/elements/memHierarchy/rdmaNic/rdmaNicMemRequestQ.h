    class MemRequestQ {

        struct SrcChannel {
            SrcChannel( int maxSrcQsize ) : maxSrcQsize(maxSrcQsize), pendingCnts(0) {}
            bool full() {
                return  ! ( queue.size() + waiting.size() + ready.size() < maxSrcQsize); 
            }
            std::queue<void*>       waiting;
            std::set<void*>         ready;
            std::queue<MemRequest*> queue; 
            int maxSrcQsize;
            int pendingCnts;
        };

      public:
        MemRequestQ( RdmaNic* nic, int maxPending, int maxSrcQsize, int numSrcs ) : 
            m_nic(nic), m_maxPending(maxPending), m_curSrc(0), m_reqSrcQs(numSrcs,maxSrcQsize), m_pendingPair(NULL,NULL)
        {}

        virtual ~MemRequestQ() { }
        void print( Cycle_t cycle ) {
            printf("%" PRIu64 " %d:  pendingReq=%zu :",cycle, Nic().m_nicId, m_pendingReq.size() );
            for ( int i = 0; i < m_reqSrcQs.size(); i++) {
                printf("src=%d queue=%zu waiting=%zu ready=%zu, ",i,m_reqSrcQs[i].queue.size(), m_reqSrcQs[i].waiting.size(), m_reqSrcQs[i].ready.size() );
            }
            printf("\n");
        }

        bool full( int srcNum ) { 
            return  m_reqSrcQs[srcNum].full();
        }
        RdmaNic& Nic() { return *m_nic; }

        void reserve( int srcNum, void* key ) {
            return m_reqSrcQs[srcNum].waiting.push(key);
        }

        bool reservationReady( int srcNum, void* key ) {
            if ( m_reqSrcQs[srcNum].ready.find( key ) != m_reqSrcQs[srcNum].ready.end() ) {
                m_reqSrcQs[srcNum].ready.erase(key);
                return true;
            } 
            return false;
        }

        void sendReq( MemRequest* req ) {

			Interfaces::StandardMem::Request* stdMemReq;
            std::vector<uint8_t> payload;
            req->reqTime=Simulation::getSimulation()->getCurrentSimCycle();

            switch ( req->m_op ) {
              case MemRequest::Read:
                Nic().dbg.debug(CALL_INFO,1,DBG_MEMEVENT_FLAG,"read addr=%#" PRIx64 " dataSize=%d\n",req->addr,req->dataSize);
				stdMemReq = new StandardMem::Read(req->addr, req->dataSize, 0, 0, 0, req->id);
                break;

              case MemRequest::Write:

                if ( req->buf.empty() ) {
                    Nic().dbg.debug(CALL_INFO,1,DBG_MEMEVENT_FLAG,"write addr=%#" PRIx64 " data=%llu dataSize=%d\n",
                                    req->addr,req->data,req->dataSize);
                    for ( int i = 0; i < req->dataSize; i++ ) {
                        payload.push_back( (req->data >> i*8) & 0xff );
						printf("%x ", (req->data >> i*8) & 0xff  );
                    }
					printf("\n");
					stdMemReq = new StandardMem::Write(req->addr, req->dataSize, payload);
                } else {
                    Nic().dbg.debug(CALL_INFO,1,DBG_MEMEVENT_FLAG,"write addr=%#" PRIx64 " dataSize=%d\n",req->addr,req->dataSize);
					stdMemReq = new StandardMem::Write(req->addr, req->dataSize, req->buf);
#if 0
                    for ( int i = 0; i < req->dataSize/4; i++ ) {
						for ( int j = 3; j >=0; j-- ) {
							printf("%02x", req->buf[i*4+j]  );
						}
						printf(" ");
                    }
					printf("\n");
#endif
                }
                break;

              case MemRequest::Fence:
                assert(0);
                break;
            }

            if ( stdMemReq ) {
				//stdMemReq->setNoncacheable();
				Nic().m_dmaLink->send( stdMemReq );
                m_pendingMap[ req->addr ].push_back( req );
                m_pendingReq[ stdMemReq->getID() ] = req;
#if 0
                ev->setDst(Nic().m_link->findTargetDestination(req->addr));
                if ( Nic().m_link->spaceToSend( ev ) ) {
                    m_pendingMap[ req->addr ].push_back( req );
                    m_pendingReq[ ev->getID() ] = req;
                    Nic().m_link->send(ev);
                } else {
                    Nic().dbg.debug(CALL_INFO,1,DBG_MEMEVENT_FLAG,"link blocked\n");
                    m_pendingPair = std::pair<MemRequest*,MemEventBase*>(req,ev);
                }
#endif
            }
        }

        void fence( int srcNum ) {
            m_reqSrcQs[srcNum].queue.push( new MemRequest(srcNum) );
        } 

        void write( int srcNum, uint64_t addr, int dataSize, uint8_t* data, MemRequest::Callback* callback = NULL ) {
			assert( ! full(srcNum) );
            Nic().dbg.debug(CALL_INFO,1,DBG_X_FLAG,"srcNum=%d addr=%#" PRIx64 " dataSize=%d\n",srcNum,addr,dataSize);
            m_reqSrcQs[srcNum].queue.push( new MemRequest( srcNum, addr, dataSize, data, callback ) );
        }

        void write( int srcNum, uint64_t addr, int dataSize, uint64_t data, MemRequest::Callback* callback = NULL ) {
			assert( ! full(srcNum) );
            Nic().dbg.debug(CALL_INFO,1,DBG_X_FLAG,"srcNum=%d addr=%#" PRIx64 " data=%llu dataSize=%d\n",srcNum,addr,data,dataSize);
            m_reqSrcQs[srcNum].queue.push( new MemRequest( srcNum, addr, dataSize, data, callback ) );
        }

        void read( int srcNum, uint64_t addr, int dataSize, int readId, MemRequest::Callback* callback = NULL  ) {
			assert( ! full(srcNum) );
            Nic().dbg.debug(CALL_INFO,1,DBG_X_FLAG,"srcNum=%d addr=%#" PRIx64 " dataSize=%d\n",srcNum,addr,dataSize);
            m_reqSrcQs[srcNum].queue.push( new MemRequest( srcNum, addr, dataSize, readId, callback ) );
        }

        std::map< uint64_t, std::list<MemRequest* > > m_pendingMap;

        std::queue< std::pair< StandardMem::Request*, MemRequest*> > m_retryQ;

        void handleResponse( Interfaces::StandardMem::Request* resp ) {
			Nic().dbg.debug(CALL_INFO,1,DBG_X_FLAG," Resp id=%d\n", resp->getID()  );
            try {
                MemRequest* req = m_pendingReq.at( resp->getID() );

                bool drop = false;
                try {
                    auto& q = m_pendingMap.at(req->addr);
                    auto iter = q.begin(); 

                    int pos = 0;
                    for ( ; iter != q.end(); ++iter ) {
                        if ( (*iter) == req ) {
                            q.erase(iter);
                            break;
                        }
                        ++pos;
                    }

                    if ( false ) {
//                    if ( ! resp->getSuccess() ) {
                        if ( req->m_op == MemRequest::Read || q.size() == pos ) {
#if 0
                            printf("retry request for 0x%" PRIx64 "\n", req->addr );
#endif
                            m_retryQ.push( std::make_pair(resp, req) );
                        } else {
                            drop = true;
#if 0
                            printf("drop request for 0x%" PRIx64 " %d %zu\n", req->addr, pos, q.size() );
#endif
                        }
                    } 

                    if ( q.empty() ) {
                        m_pendingMap.erase(req->addr);
                    }

                } catch (const std::out_of_range& oor) {
                    Nic().out.fatal(CALL_INFO,-1,"Can't find request\n");
                }

                if ( true || drop ) {
//                if ( resp->getSuccess() || drop ) {
                    m_pendingReq.erase( resp->getID() );
                    req->handleResponse( resp);
                    --m_reqSrcQs[req->src].pendingCnts;
                    delete req;
                }

            } catch (const std::out_of_range& oor) {
                Nic().out.fatal(CALL_INFO,-1,"Can't find request\n");
            } 
        }

        bool process( int num = 1) {
            bool worked = false;
//            Nic().m_statPendingMemResp->addData( m_pendingReq.size() );
#if 0
            if ( m_pendingPair.first ) {
                if ( Nic().m_link->spaceToSend( m_pendingPair.second ) ) {
                    m_pendingMap[ m_pendingPair.first->addr ].push_back( m_pendingPair.first );
                    m_pendingReq[ m_pendingPair.second->getID() ] = m_pendingPair.first;
                    Nic().m_link->send( m_pendingPair.second );
                    m_pendingPair.first = NULL;
                } else {
                    Nic().dbg.debug(CALL_INFO,1,DBG_MEMEVENT_FLAG,"link blocked\n");
                }
                return true;
            }
#endif
            if ( ! m_retryQ.empty() ) {
				assert(0);
                auto entry = m_retryQ.front(); 

#if 0
                printf("%s() handle retry %" PRIx64 " \n",__func__,entry.second->addr);
#endif

#if 0
               if ( Nic().m_link->spaceToSend( entry.first->getNACKedEvent() ) ) {
                	m_pendingReq[ entry.first->getID() ] = entry.second;
                	m_pendingMap[entry.second->addr].push_back( entry.second );
                    Nic().m_link->send( entry.first->getNACKedEvent() );
                	delete entry.first;
                	m_retryQ.pop();
                } else {
                    Nic().dbg.debug(CALL_INFO,1,DBG_MEMEVENT_FLAG,"link blocked\n");
                }
#endif

                return true;
            }
            if ( m_pendingReq.size() < m_maxPending ) {
                for ( int i = 0; i < m_reqSrcQs.size(); i++ ) {
                    int pos = (i + m_curSrc) % m_reqSrcQs.size();
                     
                    std::queue<MemRequest*>& q = m_reqSrcQs[pos].queue;

                    if ( ! q.empty() ) {
                        worked = true;
                        if ( q.front()->isFence() ) {
                            if ( m_reqSrcQs[pos].pendingCnts ) {
                                continue;
                            } else {
								delete q.front();
                                q.pop();
                            }
                        }

                        ++m_reqSrcQs[pos].pendingCnts;

                        sendReq( q.front());
                        q.pop();
                        auto& w = m_reqSrcQs[pos].waiting;
                        if ( ! w.empty() ) {
                            m_reqSrcQs[pos].ready.insert( w.front() ); 
                            w.pop();
                        }
                        break;
                    }
                }
                m_curSrc = ( m_curSrc + 1 ) % m_reqSrcQs.size();
            }
            return worked;
        }


      protected:
        std::map< StandardMem::Request::id_t, MemRequest* > m_pendingReq;
        std::vector< SrcChannel > m_reqSrcQs;
      private:
        RdmaNic* m_nic;
        int m_curSrc;
        int m_maxPending;
        std::pair<MemRequest*,StandardMem::Request*> m_pendingPair;

    };
