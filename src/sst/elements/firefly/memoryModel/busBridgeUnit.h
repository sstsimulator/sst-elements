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

#include "busWidget.h"

class BusBridgeUnit : public Unit {

	struct Entry {
		Entry( UnitBase* src, MemReq* req, Callback* callback = NULL ) : src(src), req(req), callback(callback),
                addr(req->addr), length(req->length) {}
        MemReq* req;
        Callback* callback;
		UnitBase* src;
		Hermes::Vaddr addr;
		size_t length; 
		SimTime_t qd;
		SimTime_t xmit;
    };

  public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SimpleMemoryModel::BusBridgeUnit, SimpleMemoryModel*, Output*, int, Unit*, double, int, int, int, int, int, int )
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        BusBridgeUnit,
        "firefly",
        "simpleMemory.detailedUnit",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SimpleMemoryModel::BusBridgeUnit
    )

    BusBridgeUnit( Component* comp, Params& ) : Unit( comp, NULL, NULL),
		m_reqBus(this,0, "Req", true, std::bind(&BusBridgeUnit::processReq,this,std::placeholders::_1 ) ),  
		m_respBus(this,0, "Resp",false, std::bind(&BusBridgeUnit::processResp,this,std::placeholders::_1 ) ) {} 
    BusBridgeUnit( ComponentId_t compId, Params& ) : Unit( compId, NULL, NULL),
		m_reqBus(this,0, "Req", true, std::bind(&BusBridgeUnit::processReq,this,std::placeholders::_1 ) ),  
		m_respBus(this,0, "Resp",false, std::bind(&BusBridgeUnit::processResp,this,std::placeholders::_1 ) ) {} 

    BusBridgeUnit( Component* comp, Params&, SimpleMemoryModel*, Output*, int, Unit*, double, int, int, int, int, int, int ) : Unit(comp,NULL,NULL), 
		m_reqBus(this,0, "Req", true, std::bind(&BusBridgeUnit::processReq,this,std::placeholders::_1 ) ),  
		m_respBus(this,0, "Resp",false, std::bind(&BusBridgeUnit::processResp,this,std::placeholders::_1 ) ) {} 

    BusBridgeUnit( ComponentId_t compId, Params&, SimpleMemoryModel* model, Output* dbg, int id, Unit* cache, double bandwidth, int numLinks,
                int latency, int TLP_overhead, int dll_bytes, int cacheLineSize, int widgetSlots ) :
        Unit( compId, model, dbg ), m_cache(cache), m_bandwidth_GB( bandwidth ), m_numLinks(numLinks), m_blocked(2,{NULL,0}),
        m_TLP_overhead(TLP_overhead), m_DLL_bytes(dll_bytes), m_cacheLineSize( cacheLineSize ), m_latency(latency),
		m_reqBus(this,id, "Req", true, std::bind(&BusBridgeUnit::processReq,this,std::placeholders::_1 ) ), 
		m_respBus(this,id, "Resp", false,std::bind(&BusBridgeUnit::processResp,this,std::placeholders::_1 ) ) 
    {
		Params tmp;
		m_loadWidget = loadAnonymousSubComponent<BusLoadWidget>( "firefly.simpleMemory.busLoadWidget","",0,
                            ComponentInfo::SHARE_NONE, tmp, model, dbg, id, cache, cacheLineSize, widgetSlots, latency );
		m_storeWidget = loadAnonymousSubComponent<BusStoreWidget>( "firefly.simpleMemory.busStoreWidget","",0,
                            ComponentInfo::SHARE_NONE, tmp, model, dbg, id, cache, cacheLineSize, widgetSlots, latency );

        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusBridgeUnit::@p():@l ";
		m_blocked_ns = model->registerStatistic<uint64_t>("bus_blocked_ns");
    }

	~BusBridgeUnit() {
		delete m_loadWidget;
		delete m_storeWidget;
	}

    void resume( UnitBase* unit = 0 ) {
		if ( unit == m_loadWidget ) {
        	dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"load\n");
			if ( m_blocked[0].src ) { 
				model().schedResume( 0, m_blocked[0].src );
				m_blocked[0].src = NULL;
               	SimTime_t latency = model().getCurrentSimTimeNano() - m_blocked[0].time;
                if ( latency ) {
                    m_blocked_ns->addData( latency );
                }
			}
		} else {
        	dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"store\n" );
			if ( m_blocked[1].src ) { 
				model().schedResume( 0, m_blocked[1].src );
				m_blocked[1].src = NULL;
               	SimTime_t latency = model().getCurrentSimTimeNano() - m_blocked[1].time;
                if ( latency ) {
                    m_blocked_ns->addData( latency );
                }
			}
		} 
    }

    bool load( UnitBase* src, MemReq* req, Callback* callback  ) {
		
		Entry* entry = new Entry( src, req, callback );
		entry->qd = model().getCurrentSimTimeNano();
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"src=%p entry=%p addr=%#" PRIx64 " length=%lu\n",src, entry,req->addr,req->length);
        //assert( (req->addr & (m_cacheLineSize - 1) ) == 0 );
		m_reqBus.addReq( entry );
		return true;
	}

    bool write( UnitBase* src, MemReq* req, Callback* callback ) {
		Entry* entry = new Entry( src, req, callback );
		entry->qd = model().getCurrentSimTimeNano();
		src->incPendingWrites();
		m_respBus.addReq( entry );
        bool ret = src->numPendingWrites() == 10;
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu %s %p\n",
                entry,req->addr,req->length, ret ? "blocked":"",src);
		return ret; 
	}

    bool store( UnitBase* src, MemReq* req ) {

		Entry* entry = new Entry( src, req );
		entry->qd = model().getCurrentSimTimeNano();
        dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu\n",entry,req->addr,req->length);
        //assert( (req->addr & (m_cacheLineSize - 1) ) == 0 );
		m_reqBus.addReq( entry );
		return true;
    }

  private:


	class Bus {
		std::string m_prefix;
		const char* prefix() { return m_prefix.c_str(); }
	  public:
		Bus( BusBridgeUnit* unit, int id, std::string name, bool isReq, std::function<void(Entry*)> func  ) : 
				m_unit(unit), isReq(isReq), busy(false), m_processEntry( func ) {
        	m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::BusBridgeUnit::" + name + "::@p():@l ";
		}

		void addReq( Entry* req ) {  
			m_pendingReqQ.push(req); 
            unit().dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"src=%p %s %#" PRIx64 " q size=%lu\n",
                    req->src, req->callback?"load":"store", req->addr, m_pendingReqQ.size());
			process();
		}
		void addDLL( int bytes ) {  
            unit().dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"q size=%lu\n",m_pendingDLLQ.size());
			m_pendingDLLQ.push(bytes); 
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
					SimTime_t delay = unit().calcByteDelay( m_pendingDLLQ.front() );
					m_pendingDLLQ.pop();

					busy = true;
					Callback* cb = new Callback;
					*cb = std::bind( &Bus::reqArrived, this, (Entry*) NULL );
					unit().model().schedCallback( delay, cb );
				} else if ( ! m_pendingReqQ.empty() ) {
					Entry* entry = m_pendingReqQ.front();
					m_pendingReqQ.pop();

					SimTime_t delay;

					if ( isReq ) { 
						if ( entry->callback ) {
							delay = unit().calcByteDelay( unit().TLP_overhead() ); 
						} else {
							delay = unit().calcByteDelay( entry->req->length + unit().TLP_overhead() );
						} 
					} else {
						delay = unit().calcByteDelay( entry->length + unit().TLP_overhead() - 4 );
					}
					busy = true;
					
					entry->xmit = unit().model().getCurrentSimTimeNano();
                    SimTime_t now = unit().model().getCurrentSimTimeNano();
                    unit().dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu delay=%" PRIu64 " latency=%" PRIu64 "\n",
                                    entry,entry->addr, entry->length, delay, now - entry->qd );
					Callback* cb = new Callback;
					*cb = std::bind( &Bus::reqArrived, this, entry );
					unit().model().schedCallback( delay, cb );
				}
			}
		}
		bool isReq;
		bool busy;
		std::queue<Entry* > m_pendingReqQ;
		std::queue<int> m_pendingDLLQ;
		
		BusBridgeUnit& unit() { return *m_unit; }
		BusBridgeUnit* m_unit;
	};
			
	void processResp( Entry* entry ) {
		SimTime_t now = model().getCurrentSimTimeNano();
		dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"entry=%p addr=%#" PRIx64 " length=%lu latency=%" PRIu64"\n",
                entry,entry->addr, entry->length, now - entry->qd );
		m_reqBus.addDLL( numDLLbytes() );
		if ( entry->callback ) { 
            model().schedCallback(m_latency, entry->callback );
		}
		if( -1 == (int64_t) entry->addr ) {
			if ( entry->src->numPendingWrites() == 10 ) {
		        dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"unblock src\n");
				model().schedResume( 0, entry->src );
			}
			entry->src->decPendingWrites();
            delete entry->req;
		}

		delete entry;
	}

	void processReq( Entry* entry ) {

		SimTime_t now = model().getCurrentSimTimeNano();
		dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"entry=%p qd_latency=%" PRIu64 " xmit_latency=%" PRIu64 "\n",
                    entry, entry->xmit - entry->qd, now - entry->xmit);
        SimTime_t issueTime = model().getCurrentSimTimeNano();

		m_respBus.addDLL( numDLLbytes() );
		
		UnitBase* resumeSrc = NULL;

		if ( entry->callback ) {
			Hermes::Vaddr addr = entry->req->addr;
            size_t length = entry->req->length;
			Callback* cb = new Callback;
			*cb = [=]() {
       				dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"processReq",1,BUS_BRIDGE_MASK,"load done entry=%p addr=%#" PRIx64 " length=%lu latency=%" PRIu64 "\n",
									entry, addr, length, model().getCurrentSimTimeNano() - now );
					m_respBus.addReq( entry );
				}; 
			if ( m_loadWidget->load( this, entry->req, cb ) ) 
			{
				m_blocked[0].src = entry->src;
				m_blocked[0].time = model().getCurrentSimTimeNano();
			} else {
				resumeSrc = entry->src;
			}
			
		} else {
			if ( m_storeWidget->store( this, entry->req ) ) {
				m_blocked[1].src = entry->src;
				m_blocked[1].time = model().getCurrentSimTimeNano();
			} else {
				resumeSrc = entry->src;
			}

			dbg().verbosePrefix(prefix(),CALL_INFO,1,BUS_BRIDGE_MASK,"delete store entry=%p\n",entry);
			delete entry;
		}

		if ( resumeSrc ) {
			dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"resume %p\n",resumeSrc);
			model().schedResume( 0, resumeSrc );
		} else {
			dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"blocked\n");
		} 
	}	

    SimTime_t calcByteDelay( size_t numBytes ) {
		
		double delay = (numBytes/(m_numLinks/8))/m_bandwidth_GB;
		dbg().verbosePrefix(prefix(),CALL_INFO,2,BUS_BRIDGE_MASK,"bytes=%lu delay=%f\n",numBytes, (float) delay );
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
	struct BlockedInfo {
		UnitBase* src;
		SimTime_t time;
	};
	std::vector<BlockedInfo> m_blocked;

	Unit* m_cache;
	Unit* m_loadWidget;
	Unit* m_storeWidget;
    int m_latency;
    int m_TLP_overhead;
    int m_DLL_bytes;
	int m_numLinks;
    int m_cacheLineSize;
	double m_bandwidth_GB;
    Statistic<uint64_t>* m_blocked_ns;
};
