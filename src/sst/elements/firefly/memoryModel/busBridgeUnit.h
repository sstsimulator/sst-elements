    

#include "busWidget.h"

class BusBridgeUnit : public Unit {


	struct Entry {
        Entry( UnitBase* src, MemReq* req, Callback callback = NULL ) : src(src), req(req), callback(callback),
				addr(req->addr), length(req->length) {}
        MemReq* req;
        Callback callback;
		UnitBase* src;
		Hermes::Vaddr addr;
		size_t length; 
		SimTime_t qd;
		SimTime_t xmit;
    };

  public:
    BusBridgeUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* cache, double bandwidth, int numLinks,
                            int TLP_overhead, int dll_bytes, int cacheLineSize, int widgetSlots ) :
        Unit( model, dbg ), m_cache(cache), m_bandwidth_GB( bandwidth ), m_numLinks(numLinks), m_blocked(2,NULL),
        m_TLP_overhead(TLP_overhead), m_DLL_bytes(dll_bytes),
		m_reqBus(*this,id, "Req", true, std::bind(&BusBridgeUnit::processReq,this,std::placeholders::_1 ) ), 
		m_respBus(*this,id, "Resp", false,std::bind(&BusBridgeUnit::processResp,this,std::placeholders::_1 ) ) 
    {
		m_loadWidget = new BusLoadWidget( model, dbg, id, cache, cacheLineSize, widgetSlots );
		m_storeWidget = new BusStoreWidget( model, dbg, id, cache, cacheLineSize, widgetSlots );
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusBridgeUnit::@p():@l ";
    }

    void resume( UnitBase* unit = 0 ) {
		if ( unit == m_loadWidget ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"load\n");
			if ( m_blocked[0] ) { 
				m_model.schedResume( 1, m_blocked[0] );
				m_blocked[0] = NULL;
			}
		} else {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"store\n" );
			if ( m_blocked[1] ) { 
				m_model.schedResume( 1, m_blocked[1] );
				m_blocked[1] = NULL;
			}
		} 
    }

    bool load( UnitBase* src, MemReq* req, Callback callback  ) {
		
		Entry* entry = new Entry( src, req, callback );
		entry->qd = m_model.getCurrentSimTimeNano();
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu\n",entry,req->addr,req->length);
		m_reqBus.addReq( entry );
		return true;
	}

    bool write( UnitBase* src, MemReq* req, Callback callback ) {
		Entry* entry = new Entry( src, req, callback );
		entry->qd = m_model.getCurrentSimTimeNano();
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu\n",entry,req->addr,req->length);
		src->incPendingWrites();
		m_respBus.addReq( entry );
		return src->numPendingWrites() == 10;
	}

    bool store( UnitBase* src, MemReq* req ) {

		Entry* entry = new Entry( src, req );
		entry->qd = m_model.getCurrentSimTimeNano();
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu\n",entry,req->addr,req->length);
		m_reqBus.addReq( entry );
		return true;
    }

  private:


	class Bus {
		std::string m_prefix;
		const char* prefix() { return m_prefix.c_str(); }
	  public:
		Bus( BusBridgeUnit& unit, int id, std::string name, bool isReq, std::function<void(Entry*)> func  ) : 
				m_unit(unit), isReq(isReq), busy(false), m_processEntry( func ) {
        	m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusBridgeUnit::" + name + "::@p():@l ";
		}

		void addReq( Entry* req ) {  
			m_pendingReqQ.push_back(req); 
            m_unit.m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"q size=%lu\n",m_pendingReqQ.size());
			process();
		}
		void addDLL( int bytes ) {  
            m_unit.m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"q size=%lu\n",m_pendingDLLQ.size());
			m_pendingDLLQ.push_back(bytes); 
			process();
		}

	  private:
		std::function<void(Entry*)> m_processEntry;	

		void reqArrived( Entry* entry ) {
			busy = false;
			process();
			if ( entry ) {
				m_processEntry( entry );
			}
		}

		void process() {
			if ( ! busy ) {
			
				if ( ! m_pendingDLLQ.empty() ) { 
					SimTime_t delay = m_unit.calcByteDelay( m_pendingDLLQ.front() );
					m_pendingDLLQ.pop_front();

					busy = true;
					m_unit.m_model.schedCallback( delay, std::bind( &Bus::reqArrived, this, (Entry*) NULL ) );
				} else if ( ! m_pendingReqQ.empty() ) {
					Entry* entry = m_pendingReqQ.front();
					m_pendingReqQ.pop_front();

					SimTime_t delay;

					if ( isReq ) { 
						if ( entry->callback ) {
							delay = m_unit.calcByteDelay( m_unit.TLP_overhead() ); 
						} else {
							delay = m_unit.calcByteDelay( entry->req->length + m_unit.TLP_overhead() );
						} 
					} else {
						delay = m_unit.calcByteDelay( entry->length + m_unit.TLP_overhead() - 4 );
					}
					busy = true;
					
					entry->xmit = m_unit.m_model.getCurrentSimTimeNano();
                    SimTime_t now = m_unit.m_model.getCurrentSimTimeNano();
                    m_unit.m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu delay=%" PRIu64 " Time=%" PRIu64 "\n",
                                    entry,entry->addr, entry->length, delay, now - entry->qd );
					m_unit.m_model.schedCallback( delay, std::bind( &Bus::reqArrived, this, entry ) );
				}
			}
		}
		bool isReq;
		bool busy;
		std::deque<Entry* > m_pendingReqQ;
		std::deque<int> m_pendingDLLQ;
		BusBridgeUnit& m_unit;
	};
			
	void processResp( Entry* entry ) {
		SimTime_t now = m_model.getCurrentSimTimeNano();
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu Time=%" PRIu64"\n",
                entry,entry->addr, entry->length, now - entry->qd );
		m_reqBus.addDLL( numDLLbytes() );
		if ( entry->callback ) { 
			entry->callback();
		}
		if( 0 == entry->addr ) {
			if ( entry->src->numPendingWrites() == 10 ) {
				m_model.schedResume( 1, entry->src );
			}
			entry->src->decPendingWrites();
		}

		delete entry;
	}

	void processReq( Entry* entry ) {

		SimTime_t now = m_model.getCurrentSimTimeNano();
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p qdTime=%" PRIu64 " xmitTime=%" PRIu64 "\n",
                    entry, entry->xmit - entry->qd, now - entry->xmit);
        SimTime_t issueTime = m_model.getCurrentSimTimeNano();

		m_respBus.addDLL( numDLLbytes() );
		
		UnitBase* resumeSrc = NULL;

		if ( entry->callback ) {
			Hermes::Vaddr addr = entry->req->addr;
            size_t length = entry->req->length;
			if ( m_loadWidget->load( this, entry->req, 
				[=]() {
       				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu\n",
									entry, addr, length );
					m_respBus.addReq( entry );
				}) ) 
			{
				m_blocked[0] = entry->src;
			} else {
				resumeSrc = entry->src;
			}
			
		} else {
			if ( m_storeWidget->store( this, entry->req ) ) {
				m_blocked[1] = entry->src;
			} else {
				resumeSrc = entry->src;
			}

			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"delete store entry=%p\n",entry);
			delete entry;
		}

		if ( resumeSrc ) {
			m_model.schedResume( 1, resumeSrc );
		} else {
			m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"blocked\n");
		} 
	}	

    SimTime_t calcByteDelay( size_t numBytes ) {
		
		double delay = (numBytes/(m_numLinks/8))/m_bandwidth_GB;
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"bytes=%lu delay=%f\n",numBytes, (float) delay );
        return round(delay);
    }

    size_t TLP_overhead() {
        return m_TLP_overhead;
    }

	int numDLLbytes() {
        return m_DLL_bytes;
	}

	Bus m_reqBus;
	Bus m_respBus;
	std::vector<UnitBase*> m_blocked;

	Unit* m_cache;
	Unit* m_loadWidget;
	Unit* m_storeWidget;
    int m_TLP_overhead;
    int m_DLL_bytes;
	int m_numLinks;
	double m_bandwidth_GB;
};
