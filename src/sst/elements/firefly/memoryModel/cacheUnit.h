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
    
	#include "muxUnit.h"

	class CacheUnit : public Unit {

		struct Entry {
			enum Op { Load, Store } op;
			void init( Op _op, UnitBase* _src, MemReq* _req, SimTime_t _time, Callback* _callback=NULL ) {
				op = _op; src = _src; req = _req; time = _time; callback = _callback; }
			UnitBase* src;
			Callback* callback;
			MemReq* req;
			SimTime_t time;
		};
        std::string stats;
      public:
        CacheUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* memory, int cacheSize, int cacheLineSize, int numMSHR, std::string name ) :
            Unit( model, dbg ),  m_memory(memory), m_numPending(0), m_blockedSrc(NULL), m_numMSHR(numMSHR), m_scheduled(false),
			m_cacheLineSize(cacheLineSize), m_qSize(numMSHR), m_numIssuedLoads(0),
            m_cache( cacheSize ), m_blockedOnStore(false), m_blockedOnLoad(false)
		{
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name + "CacheUnit::@p():@l ";
            stats = std::to_string(id) + ":SimpleMemoryModel::" + name + "CacheUnit:: ";

       		m_hitCnt = model.registerStatistic<uint64_t>(name + "_cache_hits");
       		m_totalCnt = model.registerStatistic<uint64_t>(name + "_cache_total");
			assert( m_numMSHR <= cacheSize );
        }

        bool m_blockedOnStore;
        bool m_blockedOnLoad;
		int m_numIssuedLoads;
		int m_numMSHR;
		int m_qSize;
		int m_numPending;

		bool m_scheduled;
		UnitBase* m_blockedSrc; 
		std::queue<Entry*> m_blockedQ;
    	Statistic<uint64_t>* m_hitCnt;
    	Statistic<uint64_t>* m_totalCnt;

		ThingHeap<Entry> m_entryHeap;
        bool store( UnitBase* src, MemReq* req ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

            //assert( (req->addr & (m_cacheLineSize - 1) ) == 0 );
            Entry* entry = m_entryHeap.alloc();
			entry->init( Entry::Store, src, req, m_model.getCurrentSimTimeNano()  ) ; 
			return addEntry( entry );
		}

        bool load( UnitBase* src, MemReq* req, Callback* callback ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

            //assert( (req->addr & (m_cacheLineSize - 1) ) == 0 );
            Entry* entry = m_entryHeap.alloc();
			entry->init( Entry::Load, src, req, m_model.getCurrentSimTimeNano(), callback ) ; 
			return addEntry( entry );
		}

		void resume( UnitBase* src = NULL ) {
            const char* ptr = (const char*) src;
            if ( ptr[0] == 'R' ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"Load\n");
                m_blockedOnLoad = false;
            } else if ( ptr[0] == 'W' ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"Store\n");
                m_blockedOnStore = false;
            } else {
                assert(0);
            }
			schedule();
		}

	  private:
		
		bool addEntry( Entry* entry ) {
			entry->req->addr = alignAddr( entry->req->addr );
       		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"%s addr=%#" PRIx64 " pending=%d\n", 
					entry->op == Entry::Load?"Load":"Store",entry->req->addr,m_numPending);

            incNumPending();

            //m_model.schedCallback( 0, std::bind( &CacheUnit::checkHit, this, entry ) );
            checkHit( entry );
				
			//if (  m_blockedQ.size() == m_qSize ) {
			if ( m_numPending == m_qSize ) {
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"pending Q is full\n");
				m_blockedSrc = entry->src;
				return true;
			} else {
				return false;
			}
		}

		void checkHit( Entry* entry ) {
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"\n");
            if ( checkHit2( entry, false ) ) {
       	        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"queue blocked %s addr=%#" PRIx64 "\n", entry->op == Entry::Load?"Load":"Store",entry->req->addr);
				m_blockedQ.push( entry );	
            }
        }

		bool checkHit2( Entry* entry, bool flag ) {
            if ( flag ) {
           	    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"retry\n");
            }
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"%s addr=%#" PRIx64 "\n", entry->op == Entry::Load?"Load":"Store",entry->req->addr);
            m_scheduled = false;
            if ( m_cache.isValid( entry->req->addr ) ) { 
				m_totalCnt->addData(1);
           	    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"done, hit %s addr=%#" PRIx64 "\n",
                                                entry->op == Entry::Load?"Load":"Store",entry->req->addr);
				m_hitCnt->addData(1);
				hit( entry );
			} else if ( isPending(entry->req->addr ) ) {
				m_totalCnt->addData(1);
				m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"pending addr=%#" PRIx64 "\n",entry->req->addr);

           		if ( NULL == m_pendingMap[entry->req->addr] ) {
           			m_pendingMap[entry->req->addr] = m_qHeap.alloc();
				}
           		m_pendingMap[entry->req->addr]->push( entry );
			} else {

				if ( ! blocked() && ( m_blockedQ.empty() || flag ) ) {
					m_totalCnt->addData(1);
       	            m_pendingMap[entry->req->addr] = NULL;
					miss( entry );
				} else {
                    assert( ! flag );
                    return true;
				}
			}
            return false;
		}

		void hit( Entry* entry ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);

			decNumPending();

            m_cache.updateAge( entry->req->addr );
			if ( entry->callback ) {
				m_model.schedCallback( 0, entry->callback );
			}
			if ( entry->req ) {
				m_model.memReqFree( entry->req );
			}
			m_entryHeap.free( entry );
		}

		bool blocked() {
			return m_blockedOnStore || m_blockedOnLoad || m_numIssuedLoads == m_numMSHR; 
		}

		void miss( Entry* entry ) {

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"%p\n",entry);
			Callback* cb = m_model.cbAlloc();
			*cb = std::bind(&CacheUnit::loadDone, this, entry, entry->req->addr, m_model.getCurrentSimTimeNano() ); 
		    m_blockedOnLoad = m_memory->load( this, entry->req, cb );
                            
            // Note that the load deletes the request, so the req pointer is no longer valid
            entry->req=NULL;
            assert( m_numIssuedLoads < m_numMSHR);
		    ++m_numIssuedLoads;
		}

		void loadDone( Entry* entry, Hermes::Vaddr addr, SimTime_t startTime )
		{
   			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"done, op=%s addr=%#" PRIx64 " latency=%" PRIu64 " numIssued=%d\n",
                            entry->op == Entry::Load ? "load" : "store",                            
							addr,m_model.getCurrentSimTimeNano()-startTime, m_numIssuedLoads);

			Hermes::Vaddr evictAddr = m_cache.evict();

			MemReq* req = m_model.memReqAlloc();
			req->init( evictAddr, m_cacheLineSize );
			m_blockedOnStore = m_memory->store( this, req );

			if ( entry->callback ) {
                m_model.schedCallback( 0, entry->callback );
			}
			decNumPending();
			if ( m_pendingMap[addr] ) {
				while ( ! m_pendingMap[addr]->empty() ) {
   			    	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"done\n");
					decNumPending();
					if ( m_pendingMap[addr]->front()->callback ) {
						m_model.schedCallback( 0, m_pendingMap[addr]->front()->callback );
					}
					if ( m_pendingMap[addr]->front()->req ) {
						m_model.memReqFree( m_pendingMap[addr]->front()->req );
					}
					m_entryHeap.free( m_pendingMap[addr]->front() );
					m_pendingMap[addr]->pop();
				}
				m_qHeap.free( m_pendingMap[addr] );
			}
			m_pendingMap.erase( addr );
			if ( entry->req ) {
				m_model.memReqFree( entry->req );
			}
            m_entryHeap.free( entry );

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"insert addr=%#" PRIx64 "\n",addr);
			m_cache.insert( addr );

			--m_numIssuedLoads;

			schedule();
		}

		void schedule() {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"scheduled=%d blocked=%d numBlocked=%zu`\n",
                    m_scheduled, blocked(), m_blockedQ.size());
			if ( ! m_scheduled && ! blocked() && ! m_blockedQ.empty() ) {
                Entry* entry = m_blockedQ.front();
                m_blockedQ.pop();
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"scheduled checkHit\n");
				Callback* cb = m_model.cbAlloc();
				*cb = std::bind( &CacheUnit::checkHit2, this, entry, true );
            	m_model.schedCallback( 0, cb );
				m_scheduled = true;
			}
		}

		void incNumPending() {
			++m_numPending;
			assert( m_numPending <= m_qSize ); 
         	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"numPending=%d\n",m_numPending);
        }
		void decNumPending() {
			assert(m_numPending);
			--m_numPending;
			assert( m_numPending >=0 ); 
         	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"numPending=%d\n",m_numPending);
			if ( m_blockedSrc ) { 
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"unblock src schedResume\n");
               	m_model.schedResume( 0, m_blockedSrc );
               	m_blockedSrc = NULL;
			} 
		}

		Hermes::Vaddr  alignAddr( Hermes::Vaddr addr ) {
			return addr & ~(m_cacheLineSize-1);
		}	

        bool isPending( Hermes::Vaddr addr ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#lx %s numPending=%lu loads\n",
			//		addr, m_pendingMap.find(addr) != m_pendingMap.end() ? "True":"False",m_pendingMap.size());
            return m_pendingMap.find(addr) != m_pendingMap.end();
        }

		int m_cacheLineSize;
        Unit* m_memory;
        Cache m_cache;

        std::map<Hermes::Vaddr, std::queue<Entry*>* > m_pendingMap;
		ThingHeap<std::queue<Entry*>> m_qHeap;
    };
