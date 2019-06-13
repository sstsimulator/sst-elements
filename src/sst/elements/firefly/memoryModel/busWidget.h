// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

struct WidgetEntry {
    WidgetEntry( int cacheLineSize, MemReq* req, SimTime_t issueTime, Callback* callback = NULL  ) : curAccess( 0 ),
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
	Callback* callback;
	SimTime_t issueTime;
};

class BusLoadWidget : public Unit {

	std::string m_name;
  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SimpleMemoryModel::BusLoadWidget, SimpleMemoryModel*, Output*, int, Unit*, int, int, int )
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        BusLoadWidget,
        "firefly",
        "simpleMemory.busLoadWidget",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SimpleMemoryModel::BusLoadWidget
    )

    BusLoadWidget( Component* comp, Params& ) : Unit( comp, NULL, NULL) {}
    BusLoadWidget( ComponentId_t compId, Params& ) : Unit( compId, NULL, NULL) {}

    BusLoadWidget( Component* comp, Params&, SimpleMemoryModel*, Output*, int, Unit*, int, int, int ) : Unit( comp, NULL, NULL ) {}

    BusLoadWidget( ComponentId_t compId, Params&, SimpleMemoryModel* model, Output* dbg, int id, Unit* cache, int width, int qSize, int latency ) :
        Unit( compId, model, dbg ), m_cache(cache), m_width(width), m_qSize(qSize), m_latency(latency),
			m_blocked(false), m_scheduled(false), m_blockedSrc(NULL) , m_numPending(0), m_name("BusLoadWidget")
    {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusLoadWidget::@p():@l ";
        m_pendingQdepthStat = model->registerStatistic<uint64_t>("bus_load_widget_pending_Q_depth");
    }
    std::string& name() { return m_name; } 

    bool load( UnitBase* src, MemReq* req, Callback* callback ) {
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu pending=%d\n",
                        req->addr, req->length, m_numPending );

        WidgetEntry* entry = new WidgetEntry( m_width, req, 0, callback );
		delete req;

        ++m_numPending;
		m_pendingQdepthStat->addData( m_numPending );
		Callback* cb = new Callback;
		*cb = std::bind( &BusLoadWidget::load2, this, entry, m_numPending );
        model().schedCallback( m_latency, cb );

        if ( m_numPending == m_qSize  ) {
            dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
    }

    void load2( WidgetEntry* entry, int pending ) {
        m_pendingQ.push( entry );

        if ( pending < m_qSize + 1 ) {
            if ( ! m_blocked && ! m_scheduled ) {
            	dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
				Callback* cb = new Callback;
				*cb = std::bind( &BusLoadWidget::process, this );
                model().schedCallback( 0, cb );
                m_scheduled = true;
            }
        }
    }

	void process() {
        assert( ! m_pendingQ.empty() );
        WidgetEntry& entry = *m_pendingQ.front();

        assert( m_blocked == false );
        m_scheduled = false;

		MemReq* req = new MemReq( entry.getAddr(), m_width );
		entry.inc();
		dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

		Callback* callback = new Callback;

		if ( entry.isDone() ) {
           	dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"entry done\n");
		
			*callback = [=]() {

				SimTime_t latency = model().getCurrentSimTimeNano() - entry.issueTime;
				--m_numPending;
				dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " complete, latency=%" PRIu64 "\n",
                        entry.addr,latency);
				if ( entry.callback ) {
	                dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"tell src load is complete\n");
                   	model().schedCallback( 0, entry.callback );		
				}

               	if ( m_blockedSrc ) {
                   	dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"unblock src\n");
                   	model().schedResume( 0, m_blockedSrc, this );
                   	m_blockedSrc = NULL;
               	}

               	dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"%s\n",m_blocked? "blocked" : "not blocked");

               	if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
           			dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"schedule process()\n");
					Callback* cb = new Callback;
					*cb = std::bind( &BusLoadWidget::process, this );
                   	model().schedCallback( 0, cb );
                   	m_scheduled = true;
               	}
			}; 
			delete m_pendingQ.front();
			m_pendingQ.pop();
		} else {
			*callback = [=](){

               	dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"%s\n",m_blocked? "blocked" : "not blocked");

               	if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
           			dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,BUS_WIDGET_MASK,"schedule process()\n");
					Callback* cb = new Callback;
					*cb = std::bind( &BusLoadWidget::process, this );
                   	model().schedCallback( 0, cb );
                   	m_scheduled = true;
               	}
			};
		}
        m_blocked = m_cache->load( this, req, callback ); 

      	if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
       		dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
			Callback* cb = new Callback;
			*cb = std::bind( &BusLoadWidget::process, this );
           	model().schedCallback( 0, cb );
           	m_scheduled = true;
       	}
	}

    void resume( UnitBase* src = NULL ) {
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"\n");
        assert( m_blocked == true );
        m_blocked = false;
        if ( ! m_scheduled && ! m_pendingQ.empty() ) {
            process();
        }
    }
  private:

    int m_latency;
	int m_numPending;
	int m_qSize;
	bool m_scheduled;
	bool m_blocked;
    Unit* m_cache;
    int   m_width;
    UnitBase* m_blockedSrc;
	std::queue<WidgetEntry *> m_pendingQ;

    Statistic<uint64_t>* m_pendingQdepthStat;
};


class BusStoreWidget : public Unit {
	std::string m_name;
  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SimpleMemoryModel::BusStoreWidget, SimpleMemoryModel*, Output*, int, Unit*, int, int, int )
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        BusStoreWidget,
        "firefly",
        "simpleMemory.busStoreWidget",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SimpleMemoryModel::BusStoreWidget
    )

    BusStoreWidget( Component* comp, Params& ) : Unit( comp, NULL, NULL) {}
    BusStoreWidget( ComponentId_t compId, Params& ) : Unit( compId, NULL, NULL) {}

    BusStoreWidget( Component* comp, Params&, SimpleMemoryModel*, Output*, int, Unit*, int, int, int ) : Unit( comp, NULL, NULL ) {}

    BusStoreWidget( ComponentId_t compId, Params&, SimpleMemoryModel* model, Output* dbg, int id, Unit* cache, int width, int qSize, int latency ) :
        Unit( compId, model, dbg ), m_cache(cache), m_width(width), m_qSize(qSize), m_latency(latency),
        m_blocked(false), m_blockedSrc(NULL), m_scheduled(false), m_numPending(0), m_name( "BusStoreWidget" )
    {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusStoreWidget::@p():@l ";
        m_pendingQdepthStat = model->registerStatistic<uint64_t>("bus_store_widget_pending_Q_depth");
    }

    std::string& name() { return m_name; } 

    bool store( UnitBase* src, MemReq* req ) {
		return storeCB( src, req );
	}

    bool storeCB( UnitBase* src, MemReq* req, Callback* callback = NULL ) {
		assert( NULL == m_blockedSrc );

		WidgetEntry* entry = new WidgetEntry( m_width, req, model().getCurrentSimTimeNano(), callback );
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu entry=%p\n",req->addr,req->length, entry);
		delete req;

        ++m_numPending;
		m_pendingQdepthStat->addData( m_numPending );
		Callback* cb = new Callback;
		*cb = std::bind( &BusStoreWidget::store2, this, entry, m_numPending );
        model().schedCallback( m_latency, cb );

        if ( m_numPending == m_qSize  ) {
            dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
    }


    void store2( WidgetEntry* entry, int pending ) {
		m_pendingQ.push( entry );
        if ( m_numPending < m_qSize + 1) {
            if ( ! m_blocked && ! m_scheduled ) {
           		dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process() entry=%p\n", entry);
				Callback* cb = new Callback;
				*cb = std::bind( &BusStoreWidget::process, this );
                model().schedCallback( 0, cb );
                m_scheduled = true;
            }
        }
    }

	void process() {
			
		assert( m_blocked == false );

		m_scheduled = false;

		WidgetEntry& entry= *m_pendingQ.front();
		MemReq* req = new MemReq( entry.getAddr(), m_width );
		entry.inc();
		
		dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"addr=%#" PRIx64 " length=%lu entry=%p\n",req->addr,req->length,&entry);
        m_blocked = m_cache->store( this, req );

		if ( entry.isDone() ) {
           	dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"entry done entry=%p\n", &entry);
			--m_numPending;
			if ( entry.callback ) {
           		model().schedCallback( 0, entry.callback );		
			}
			delete m_pendingQ.front();
			m_pendingQ.pop();
        	if ( m_blockedSrc ) {
            	dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"unblock src\n");
            	model().schedResume( 0, m_blockedSrc, this );
            	m_blockedSrc = NULL;
        	}
		}

        if ( ! m_blocked && ! m_pendingQ.empty() ) {
           	dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"schedule process()\n");
			Callback* cb = new Callback;
			*cb = std::bind( &BusStoreWidget::process, this );
            model().schedCallback( 0, cb );
            m_scheduled = true;
        }
	}

    void resume(UnitBase* src = NULL ) {
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_WIDGET_MASK,"\n");
        assert( m_blocked == true );
        m_blocked = false;
        if ( ! m_scheduled && ! m_pendingQ.empty() ) {
            process();
        }
    }

  private:
    int m_numPending;
    int m_latency;
	int m_qSize;
	bool m_scheduled;
	bool m_blocked;
    Unit* m_cache;
    int   m_width;
    UnitBase* m_blockedSrc;
	std::queue<WidgetEntry*> m_pendingQ;

    Statistic<uint64_t>* m_pendingQdepthStat;
};
