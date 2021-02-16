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

	#include "muxUnit.h"

	class CacheUnit : public Unit {

		struct Entry {
			enum Op { Load, Store } op;
            Entry( Op op, UnitBase* src, MemReq* req, SimTime_t time, Callback* callback=NULL ) :
                op(op), src(src), req(req), time(time), callback(callback) {}
			UnitBase* src;
			Callback* callback;
			MemReq* req;
			SimTime_t time;
			Hermes::Vaddr addr;
			SimTime_t startTime;
		};
        std::string stats;
      public:
        CacheUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* memory, int cacheSize, int cacheLineSize, int numMSHR, std::string name ) :
            Unit( model, dbg ),  m_memory(memory), m_numPending(0), m_blockedSrc(NULL), m_numMSHR(numMSHR), m_scheduled(false),
			m_cacheLineSize(cacheLineSize), m_qSize(numMSHR), m_numIssuedLoads(0),
            m_cache( cacheSize ), m_blockedOnMemUnit(false)
		{
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name + "CacheUnit::@p():@l ";
            stats = std::to_string(id) + ":SimpleMemoryModel::" + name + "CacheUnit:: ";

       		m_hitCnt = model.registerStatistic<uint64_t>(name + "_cache_hits");
       		m_totalCnt = model.registerStatistic<uint64_t>(name + "_cache_total");
			assert( m_numMSHR <= cacheSize );
        }
		~CacheUnit() {
			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"numPending=%d,  \n",m_numPending);
		}

        bool m_blockedOnMemUnit;;
		int m_numIssuedLoads;
		int m_numMSHR;
		int m_qSize;
		int m_numPending;

		bool m_scheduled;
		UnitBase* m_blockedSrc;
		std::queue<Entry*> m_blockedQ;
		std::queue<Entry*> m_blockedDone;
    	Statistic<uint64_t>* m_hitCnt;
    	Statistic<uint64_t>* m_totalCnt;

        bool store( UnitBase* src, MemReq* req ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
			//assert( (req->addr & (m_cacheLineSize - 1) ) == 0 );
			return addEntry( new Entry( Entry::Store, src, req, m_model.getCurrentSimTimeNano()  ) );
		}

        bool load( UnitBase* src, MemReq* req, Callback* callback ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
            //assert( (req->addr & (m_cacheLineSize - 1) ) == 0 );
			return addEntry( new Entry( Entry::Load, src, req, m_model.getCurrentSimTimeNano(), callback ) );
		}

		void resume( UnitBase* src = NULL ) {
            m_blockedOnMemUnit = false;
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"blocked=%lu bockedDone=%lu numPending=%d\n",
								m_blockedQ.size(), m_blockedDone.size(), m_numPending );

			while ( ! m_blockedOnMemUnit && ! m_blockedDone.empty() ) {
				Entry* entry = m_blockedDone.front();
				m_blockedDone.pop();
				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"process blocked done, addr=%#" PRIx64 "\n",entry->addr);
				loadDone2(entry);
			}

			schedule();
		}

	  private:

		bool addEntry( Entry* entry ) {
			entry->req->addr = alignAddr( entry->req->addr );
       		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"%s addr=%#" PRIx64 " pending=%d\n",
					entry->op == Entry::Load?"Load":"Store",entry->req->addr,m_numPending);

            incNumPending();

            checkHitNew( entry );

			if ( m_numPending == m_qSize ) {
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"pending Q is full\n");
				m_blockedSrc = entry->src;
				return true;
			} else {
				return false;
			}
		}

		void checkHitNew( Entry* entry ) {
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"\n");
            if ( checkHit( entry, false ) ) {
       	        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"queue blocked %s addr=%#" PRIx64 "\n", entry->op == Entry::Load?"Load":"Store",entry->req->addr);
				m_blockedQ.push( entry );
            }
        }
		void checkHitRetry( ) {
            m_scheduled = false;
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"\n");
			while ( ! blocked() && ! m_blockedQ.empty() ) {
				Entry* entry = m_blockedQ.front();
				if (  ! checkHit( entry, true ) )  {
					m_blockedQ.pop();
				}
			}
		}

		bool checkHit( Entry* entry, bool flag ) {
            if ( flag ) {
           	    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"retry\n");
            }
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"%s addr=%#" PRIx64 "\n", entry->op == Entry::Load?"Load":"Store",entry->req->addr);
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
					m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"blocked addr=%#" PRIx64 "\n",entry->req->addr);
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
				delete entry->req;
			}
			delete entry;
		}

		bool blocked() {

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"memUnitBlocked=%d outOfMSHR=%d\n",
					m_blockedOnMemUnit, m_numIssuedLoads == m_numMSHR);
			return m_blockedOnMemUnit || m_numIssuedLoads == m_numMSHR;
		}

		void miss( Entry* entry ) {

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"%p\n",entry);
			Callback* cb = new Callback;
			entry->addr = entry->req->addr;
			entry->startTime = m_model.getCurrentSimTimeNano();
			*cb = std::bind(&CacheUnit::loadDone, this, entry );
		    m_blockedOnMemUnit = m_memory->load( this, entry->req, cb );

            // Note that the load deletes the request, so the req pointer is no longer valid
            entry->req=NULL;
            assert( m_numIssuedLoads < m_numMSHR);
		    ++m_numIssuedLoads;
		}

		void loadDone( Entry* entry ){
			if ( m_blockedOnMemUnit ) {
				m_blockedDone.push( entry );
			} else {
				loadDone2( entry );
				schedule();
			}
		}

		void loadDone2( Entry* entry )
		{
			Hermes::Vaddr addr = entry->addr;
   			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"done, op=%s addr=%#" PRIx64 " latency=%" PRIu64 " numIssued=%d\n",
                            entry->op == Entry::Load ? "load" : "store",
							addr,m_model.getCurrentSimTimeNano()- entry->startTime, m_numIssuedLoads);

			Hermes::Vaddr evictAddr = m_cache.evict();

			MemReq* req = new MemReq( evictAddr, m_cacheLineSize );
			m_blockedOnMemUnit = m_memory->store( this, req );

			if ( entry->callback ) {
                m_model.schedCallback( 0, entry->callback );
			}
			decNumPending();
			if ( m_pendingMap[addr] ) {
				while ( ! m_pendingMap[addr]->empty() ) {
   			    	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"done addr=%#" PRIx64 "\n", addr);
					decNumPending();
					if ( m_pendingMap[addr]->front()->callback ) {
						m_model.schedCallback( 0, m_pendingMap[addr]->front()->callback );
					}
					if ( m_pendingMap[addr]->front()->req ) {
						delete m_pendingMap[addr]->front()->req;
					}
					delete m_pendingMap[addr]->front();
					m_pendingMap[addr]->pop();
				}
				m_qHeap.free( m_pendingMap[addr] );
			}
			m_pendingMap.erase( addr );
			if ( entry->req ) {
				delete entry->req;
			}
			delete entry;

            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"insert addr=%#" PRIx64 "\n",addr);
			m_cache.insert( addr );

			--m_numIssuedLoads;
		}

		void schedule() {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,CACHE_MASK,"scheduled=%d blocked=%d numBlocked=%zu`\n",
                    m_scheduled, blocked(), m_blockedQ.size());
			if ( ! m_scheduled && ! blocked() && ! m_blockedQ.empty() ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"scheduled checkHitRetry addr=%#" PRIx64 "\n",
										m_blockedQ.front()->req->addr);
				Callback* cb = new Callback;
				*cb = std::bind( &CacheUnit::checkHitRetry, this );
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
