    
	#include "muxUnit.h"

	class CacheUnit : public Unit {

		struct Entry {
			enum Op { Load, Store } op;
			Entry( Op op, UnitBase* src, MemReq* req, SimTime_t time, Callback callback=NULL ) : 
				op(op), src(src), req(req), time(time), callback(callback) {}
			~Entry() { delete req; }
			UnitBase* src;
			Callback callback;
			MemReq* req;
			SimTime_t time;
		};
    #include "cacheList.h"
      public:
        CacheUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* memory, int cacheSize, int cacheLineSize, int numMSHR, std::string name ) :
            Unit( model, dbg ), m_cacheSize(cacheSize), m_memory(memory), m_numPending(0), m_blockedSrc(NULL), m_numMSHR(numMSHR), m_scheduled(false),
			m_cacheLineSize(cacheLineSize), m_qSize(m_numMSHR), m_numIssuedLoads(0), m_state( Idle ), m_missEntry(NULL), m_mshrEntry(NULL)
		{
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name + "CacheUnit::@p():@l ";

			assert( m_numMSHR <= m_cacheSize );

			for ( unsigned i = 0; i < m_cacheSize; i++ ) {
				insert( (i + 1 ) * m_cacheLineSize );
			}
        }

		enum State { Idle, BlockedStore, BlockedLoad, BlockedMSHR } m_state;
		int m_numIssuedLoads;
		int m_numMSHR;
		int m_qSize;
		int m_numPending;

		bool m_scheduled;
		UnitBase* m_blockedSrc; 
		Entry* m_missEntry;
		Entry* m_mshrEntry;
		std::deque<Entry*> m_blockedQ;

        bool store( UnitBase* src, MemReq* req ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

			return addEntry( new Entry( Entry::Store, src, req, m_model.getCurrentSimTimeNano()  ) ); 
		}

        bool load( UnitBase* src, MemReq* req, Callback callback ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

			return addEntry( new Entry( Entry::Load, src, req, m_model.getCurrentSimTimeNano(), callback ) ); 
		}

		void resume( UnitBase* src = NULL ) {
			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"state=%d blocked=%lu\n", m_state, m_blockedQ.size());
			switch (m_state) {
			  case BlockedStore:
				assert( m_missEntry );
				m_state = Idle;
				miss2( m_missEntry );	
				m_missEntry = NULL;
				break;

			  case BlockedLoad:
				m_state = Idle;
				break;	
			  default:
				assert(0);
			}

			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"state=%d blocked=%lu\n", m_state, m_blockedQ.size());
			schedule();
		}

	  private:
		
		bool addEntry( Entry* entry ) {
       		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"%s addr=%#" PRIx64 " pending=%d\n", 
					entry->op == Entry::Load?"Load":"Store",entry->req->addr,m_numPending);

			entry->req->addr = alignAddr( entry->req->addr );

			++m_numPending;

			assert( m_numPending <= m_qSize ); 

            m_model.schedCallback( 0, std::bind( &CacheUnit::checkHit, this, entry ) );
				
			if ( m_numPending == m_qSize ) {
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"pending Q is full\n");
				m_blockedSrc = entry->src;
				return true;
			} else {
				return false;
			}
		}

		void checkHit( Entry* entry ) {
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"%s addr=%#" PRIx64 "\n", entry->op == Entry::Load?"Load":"Store",entry->req->addr);
            if ( isValid( entry->req->addr ) ) { 
				hit( entry );
			} else if ( isPending(entry->req->addr ) ) {
				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"pending addr=%#" PRIx64 "\n",entry->req->addr);
           		m_pendingMap[entry->req->addr].push_back( entry );
			} else {

				if ( ! blocked() && m_blockedQ.empty() ) {
					miss( entry );
				} else {
					m_blockedQ.push_back( entry );	
				}
			}
		}

		void checkHit2( ) {
			m_scheduled = false;
			assert( ! m_blockedQ.empty() );
			Entry* entry = m_blockedQ.front();
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"%s addr=%#" PRIx64 "\n", entry->op == Entry::Load?"Load":"Store",entry->req->addr);
			m_blockedQ.pop_front();
            if ( isValid( entry->req->addr ) ) { 
				hit( entry );
			} else if ( isPending(entry->req->addr ) ) {
				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"pending addr=%#" PRIx64 "\n",entry->req->addr);
           		m_pendingMap[entry->req->addr].push_back( entry );
			} else {
				miss( entry );
			}
			schedule();
		}

		void hit( Entry* entry ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);

			decNumPending();

            updateAge( entry->req->addr );
			if ( entry->callback ) {
				m_model.schedCallback( 0, entry->callback );
			}
			delete entry;
		}

		void setState( State state ) {
			m_state = state;
		}

		bool blocked() {
			return m_state != Idle; 
		}

		void miss( Entry* entry ) {

            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);

            if ( ! isPending(entry->req->addr) ) {
				Hermes::Vaddr evictAddr = evict();
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"evict addr=%#" PRIx64 "\n", evictAddr );

				if ( m_memory->store( this, new MemReq( evictAddr, m_cacheLineSize ) ) ) {
					m_missEntry = entry;
					setState( BlockedStore );
					return;
				}
			}
			miss2( entry );
		}

		void miss2( Entry* entry ) {

            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);
			if ( entry->op == Entry::Store ) {
				store( entry );
			} else {
				load( entry );
			}
			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"state=%d blocked=%lu\n", m_state, m_blockedQ.size());
		}

		void store( Entry *entry ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 "\n",entry->req->addr);
			decNumPending();
            if ( ! isPending(entry->req->addr) ) {
				insert( entry->req->addr );
			}
			if ( entry->callback ) {
				m_model.schedCallback( 0, entry->callback );
			}
			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"state=%d blocked=%lu\n", m_state, m_blockedQ.size());
			delete entry;
		}

		void load( Entry *entry ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " numIssued=%d\n",entry->req->addr, m_numIssuedLoads);

			if ( m_numIssuedLoads == m_numMSHR ) {
            	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " blocked MSHR\n",entry->req->addr);
				m_mshrEntry = entry;
				setState( BlockedMSHR );
				return;
			}
			++m_numIssuedLoads;

           	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " issue load\n",entry->req->addr);
			MemReq* req = new MemReq( entry->req->addr, m_cacheLineSize );
			if ( m_memory->load( this, req, std::bind(&CacheUnit::loadDone, this, entry->req->addr, m_model.getCurrentSimTimeNano() ) ) ) {
				setState( BlockedLoad );
			}else{
				m_state = Idle;
			}

       		m_pendingMap[entry->req->addr].push_back( entry );

			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"state=%d blocked=%lu\n", m_state, m_blockedQ.size());
		}

		void loadDone( Hermes::Vaddr addr, SimTime_t startTime )
		{
   			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"addr=%#" PRIx64 " latency=%" PRIu64 " numIssued=%d\n",
							addr,m_model.getCurrentSimTimeNano()-startTime, m_numIssuedLoads);
			while ( ! m_pendingMap[addr].empty() ) {
				decNumPending();
				if ( m_pendingMap[addr].front()->callback ) {
					m_model.schedCallback( 0, m_pendingMap[addr].front()->callback );
				}
				delete m_pendingMap[addr].front();
				m_pendingMap[addr].pop_front();
			}
			m_pendingMap.erase( addr );

			insert( addr );

			--m_numIssuedLoads;

			if ( BlockedMSHR == m_state ) {
				assert( m_mshrEntry );
				m_state = Idle;
				load( m_mshrEntry );		
				m_mshrEntry = NULL;
			}

			schedule();
		}

		void schedule() {
			if ( ! m_scheduled && Idle == m_state && ! m_blockedQ.empty() ) {
            	m_model.schedCallback( 0, std::bind( &CacheUnit::checkHit2, this ) );
				m_scheduled = true;
			}
		}

		void decNumPending() {
			assert(m_numPending);
			--m_numPending;
         	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"numPending=%d\n",m_numPending);
			if ( m_blockedSrc ) { 
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK,"unblock src\n");
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

        bool isValid( Hermes::Vaddr addr ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK," addr=%#" PRIx64 " %s mapSize=%lu\n",
					addr, m_addrMap.find(addr) != m_addrMap.end() ? "True":"False",m_addrMap.size());
            return m_addrMap.find(addr) != m_addrMap.end();
        }

        void updateAge( Hermes::Vaddr addr ) {
            //m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK," addr=%#lx %p\n",addr, m_addrMap[addr]);

            m_ageList.move_to_back( m_addrMap[addr] );
            m_addrMap[addr] = m_ageList.end();
        }

        Hermes::Vaddr evict() {

            Hermes::Vaddr addr = m_ageList.get_front_value();
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK," addr=%#" PRIx64 " ptr=%p mapSize=%lu\n", addr, m_addrMap[ addr ], m_addrMap.size() );

            m_addrMap.erase( addr );
            m_ageList.pop_front();
			return addr;
        }

        void insert( Hermes::Vaddr addr ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,CACHE_MASK," addr=%#" PRIx64 " mapSize=%lu\n",addr, m_addrMap.size());
            assert( m_addrMap.find(addr) == m_addrMap.end() );
			assert( m_addrMap.size() < m_cacheSize );
            m_ageList.push_back( addr );
            m_addrMap[addr] = m_ageList.end();
        }

		int m_cacheLineSize;
        int m_cacheSize;
        Unit* m_memory;

        std::map<Hermes::Vaddr, std::deque<Entry*> > m_pendingMap;
        List<Hermes::Vaddr> m_ageList;
        std::map<Hermes::Vaddr, List<Hermes::Vaddr>::Entry > m_addrMap;
    };
