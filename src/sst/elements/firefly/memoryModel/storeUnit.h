


class StoreUnit : public Unit {
	std::string m_name;
  public:
    StoreUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* cache, int numSlots, std::string name ) :
        Unit( model, dbg ), m_qSize(numSlots),  m_storeDelay( 1 ), m_cache(cache), m_blocked(false), m_blockedSrc(NULL), m_scheduled(false), m_name(name) {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::"+ name + "StoreUnit::@p():@l ";
    }

	std::string& name() { return m_name; }
    bool store( UnitBase* src, MemReq* req ) {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"addr=%#lx length=%lu pending=%lu\n",req->addr,req->length,m_pendingQ.size());
		assert( NULL == m_blockedSrc );
		m_pendingQ.push_back( req );

		if ( m_pendingQ.size() < m_qSize + 1) {
			if ( ! m_blocked && ! m_scheduled ) {
				m_model.schedCallback( 1, std::bind( &StoreUnit::process, this ) ); 
				m_scheduled = true;
			}
		}

        if ( m_pendingQ.size() == m_qSize  ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
	}

  private:

	void process( ) {
		MemReq* req = m_pendingQ.front();
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"addr=%#lx length=%lu\n",req->addr,req->length);

		assert( m_blocked == false );
		m_scheduled = false;

        m_blocked = m_cache->store( this, req );
		m_pendingQ.pop_front();

		if ( m_blockedSrc ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"unblock src\n");
			m_model.schedResume( 1, m_blockedSrc );
			m_blockedSrc = NULL;
		}

		if ( ! m_blocked && ! m_pendingQ.empty() ) {
			m_model.schedCallback( 1, std::bind( &StoreUnit::process, this ) ); 
			m_scheduled = true;
		}
	}

    void resume( UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"pending=%lu\n",m_pendingQ.size());
		
		assert( m_blocked == true );
		m_blocked = false;
		if ( ! m_scheduled && ! m_pendingQ.empty() ) {
			process();
		}		
    }

	
	UnitBase* m_blockedSrc;
	bool 	m_scheduled;
	bool    m_blocked;
    Unit*   m_cache;
    int     m_qSize;

    SimTime_t m_storeDelay;

    std::deque<MemReq*>       m_pendingQ;
};
