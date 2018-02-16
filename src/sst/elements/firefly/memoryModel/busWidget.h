

struct WidgetEntry {
    WidgetEntry( int cacheLineSize, MemReq* req, SimTime_t issueTime, Callback callback = NULL  ) : curAccess( 0 ), 
			size(cacheLineSize ), issueTime(issueTime), callback(callback)
	{
		addr = req->addr;
 		uint64_t mask = cacheLineSize - 1; 
		numAccess = req->length / cacheLineSize;
		if ( req->addr & mask || req->length < cacheLineSize ) {
			//printf("bump up size\n");
			numAccess += 1;
		}
		start = req->addr & ~mask;

        //printf("%s() addr=%#lx length=%lu mask=%#lx start=%#lx numAccess=%d\n",__func__, req->addr, req->length, mask, start, numAccess );
    }

    Hermes::Vaddr getAddr( ) {
        return start + size * curAccess;
    }
	void inc() {
		++curAccess;
	}

    bool isDone() { return curAccess == numAccess;  }

	Hermes::Vaddr addr;
	Hermes::Vaddr start;
	int size;
	int curAccess;
	int numAccess;
	Callback callback;
	SimTime_t issueTime;
};

class BusLoadWidget : public Unit {

	std::string m_name;
  public:
    BusLoadWidget( SimpleMemoryModel& model, Output& dbg, int id, Unit* cache, int width, int qSize ) :
        Unit( model, dbg ), m_cache(cache), m_width(width), m_qSize(qSize),
			m_blocked(false), m_scheduled(false), m_blockedSrc(NULL) , m_numPending(0), m_name("BusLoadWidget")
    {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusLoadWidget::@p():@l ";
    }
    std::string& name() { return m_name; } 

    bool load( UnitBase* src, MemReq* req, Callback callback ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu pending=%d\n",req->addr, req->length, m_numPending );

        m_pendingQ.push_back( WidgetEntry( m_width, req, m_model.getCurrentSimTimeNano(), callback ) );
		delete req;
        ++m_numPending;

        if ( m_numPending < m_qSize + 1 ) {
            if ( ! m_blocked && ! m_scheduled ) {
            	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
                m_model.schedCallback( 0, std::bind( &BusLoadWidget::process, this ) );
                m_scheduled = true;
            }
        }

        if ( m_numPending == m_qSize  ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
    }

	void process() {
        assert( ! m_pendingQ.empty() );
        WidgetEntry& entry = m_pendingQ.front();

        assert( m_blocked == false );
        m_scheduled = false;

		MemReq* req = new MemReq( entry.getAddr(), m_width );
		entry.inc();
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

		Callback callback;

		if ( entry.isDone() ) {
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"entry done\n");
		
			callback = [=]() {

				SimTime_t latency = m_model.getCurrentSimTimeNano() - entry.issueTime;
				--m_numPending;
				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " complete, latency=%" PRIu64 "\n",
                        entry.addr,latency);
				if ( entry.callback ) {
	                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"tell src load is complete\n");
                   	m_model.schedCallback( 0, entry.callback );		
				}

               	if ( m_blockedSrc ) {
                   	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"unblock src\n");
                   	m_model.schedResume( 0, m_blockedSrc, this );
                   	m_blockedSrc = NULL;
               	}

               	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"%s\n",m_blocked? "blocked" : "not blocked");

               	if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
           			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
                   	m_model.schedCallback( 0, std::bind( &BusLoadWidget::process, this ) );
                   	m_scheduled = true;
               	}
			}; 
			m_pendingQ.pop_front();
		} else {
			callback = [=](){

               	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"%s\n",m_blocked? "blocked" : "not blocked");

               	if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
           			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
                   	m_model.schedCallback( 0, std::bind( &BusLoadWidget::process, this ) );
                   	m_scheduled = true;
               	}
			};
		}
        m_blocked = m_cache->load( this, req, callback ); 

      	if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
       		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
           	m_model.schedCallback( 0, std::bind( &BusLoadWidget::process, this ) );
           	m_scheduled = true;
       	}
	}

    void resume( UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"\n");
        assert( m_blocked == true );
        m_blocked = false;
        if ( ! m_scheduled && ! m_pendingQ.empty() ) {
            process();
        }
    }
  private:
	int m_numPending;
	int m_qSize;
	bool m_scheduled;
	bool m_blocked;
    Unit* m_cache;
    int   m_width;
    UnitBase* m_blockedSrc;
	std::deque<WidgetEntry> m_pendingQ;
};


class BusStoreWidget : public Unit {
	std::string m_name;
  public:
    BusStoreWidget( SimpleMemoryModel& model, Output& dbg, int id, Unit* cache, int width, int qSize ) :
        Unit( model, dbg ), m_cache(cache), m_width(width), m_blocked(false), m_qSize(qSize), m_blockedSrc(NULL), m_scheduled(false), m_name( "BusStoreWidget" )
    {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusStoreWidget::@p():@l ";
    }

    std::string& name() { return m_name; } 
    bool store( UnitBase* src, MemReq* req ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
		assert( NULL == m_blockedSrc );

		m_pendingQ.push_back( WidgetEntry( m_width, req, 0 ) );
		delete req;

        if ( m_pendingQ.size() < m_qSize + 1) {
            if ( ! m_blocked && ! m_scheduled ) {
           		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
                m_model.schedCallback( 0, std::bind( &BusStoreWidget::process, this ) );
                m_scheduled = true;
            }
        }

        if ( m_pendingQ.size() == m_qSize  ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
    }

	void process() {
			
		assert( m_blocked == false );

		m_scheduled = false;

		WidgetEntry& entry= m_pendingQ.front();
		MemReq* req = new MemReq( entry.getAddr(), m_width );
		entry.inc();
		
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
        m_blocked = m_cache->store( this, req );

		if ( entry.isDone() ) {
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"entry done\n");
			m_pendingQ.pop_front();
        	if ( m_blockedSrc ) {
            	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"unblock src\n");
            	m_model.schedResume( 0, m_blockedSrc, this );
            	m_blockedSrc = NULL;
        	}
		}

        if ( ! m_blocked && ! m_pendingQ.empty() ) {
           	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
            m_model.schedCallback( 0, std::bind( &BusStoreWidget::process, this ) );
            m_scheduled = true;
        }
	}

    void resume(UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"\n");
        assert( m_blocked == true );
        m_blocked = false;
        if ( ! m_scheduled && ! m_pendingQ.empty() ) {
            process();
        }
    }

  private:
	int m_qSize;
	bool m_scheduled;
	bool m_blocked;
    Unit* m_cache;
    int   m_width;
    UnitBase* m_blockedSrc;
	std::deque<WidgetEntry> m_pendingQ;
};
