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

class DetailedUnit : public Unit {
	enum Op { Read, Write };
	struct Entry {

		Entry( Op op, MemReq* memReq, uint64_t num, Callback* callback = NULL ) :
                op(op), memReq(memReq), num(num), callback( callback )
            { }
            Op op;
            MemReq* memReq;
            Callback* callback;
            SimTime_t issueTime;
			uint64_t num;
        };

	uint64_t m_cnt;
	std::vector< std::queue< Entry* > > m_pendingReqQ;
	std::vector<uint32_t> m_maxRequestsPending;
	std::vector<uint32_t> m_inFlightCnt;
	std::vector<UnitBase*> m_blockedSrc;
  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SimpleMemoryModel::DetailedUnit, SimpleMemoryModel*, Output*, int )
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        DetailedUnit,
        "firefly",
        "simpleMemory.detailedUnit",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
		SimpleMemoryModel::DetailedUnit
    )

    DetailedUnit( Component* comp, Params& ) : Unit( comp, NULL, NULL) {}
    DetailedUnit( ComponentId_t compId, Params& ) : Unit( compId, NULL, NULL) {}

    DetailedUnit( Component* comp, Params&, SimpleMemoryModel*, Output*, int) : Unit( comp, NULL, NULL ) {}

    DetailedUnit( ComponentId_t compId, Params& params, SimpleMemoryModel* model, Output* dbg, int id ) :
            Unit( compId, dbg )//, m_maxRequestsPending(2), m_pendingReqQ(2), m_blockedSrc(2,NULL), m_inFlightCnt(2,0), m_cnt(0)
    {
#if 0
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::DetailedUnit::@p():@l ";
        Params detailedParams = params.find_prefix_params("detailedModel.");
        std::string name = detailedParams.find<std::string>("name","memHierarchy.memInterface");

		m_mem_link = loadUserSubComponent<Interfaces::SimpleMem>("memory", ComponentInfo::SHARE_NONE, getTimeConverter("1"), 
				new Interfaces::SimpleMem::Handler<DetailedUnit>(this, &DetailedUnit::handleEvent) );

	    m_maxRequestsPending[Read] = params.find<uint32_t>("maxloadmemreqpending", 16);
    	m_maxRequestsPending[Write] = params.find<uint32_t>("maxstorememreqpending", 16);

	    if( m_mem_link ) {
			dbg->verbose(CALL_INFO, 1, DETAILED_MASK, "Loaded memory interface successfully.\n");
    	} else {
			dbg->fatal(CALL_INFO, -1, "Error loading memory interface module.\n");
    	}

		dbg->verbose(CALL_INFO, 1, DETAILED_MASK, "Initializing memory interface...\n");

		UnitAlgebra freq = params.find<SST::UnitAlgebra>( "freq", "1Ghz" );

		m_clock_handler = new Clock::Handler<DetailedUnit>(this,&DetailedUnit::clock_handler);
		m_clock = model->registerClock( freq, m_clock_handler);
		m_reqCnt.resize(2);
		m_reqCnt[Read] = model->registerStatistic<uint64_t>( "detailed_num_reads" );
		m_reqCnt[Write] = model->registerStatistic<uint64_t>( "detailed_num_writes" );
		m_reqLatency  = model->registerStatistic<uint64_t>( "detailed_req_latency" );
#endif
    }

    bool store( UnitBase* src, MemReq* req ) {
#if 0
        dbg().verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr, req->length);

		if ( m_inFlightCnt[Write] + m_pendingReqQ[Write].size() < m_maxRequestsPending[Write] - 1 ) {
			src = NULL;
		} else {
			assert( m_blockedSrc[Write] == NULL ); 
			m_blockedSrc[Write] = src;
		}
		m_reqCnt[Write]->addData(1);
		m_pendingReqQ[Write].push( new Entry( Write, req, m_cnt++ ) );

		return src;
#endif
    }

    bool load( UnitBase* src, MemReq* req, Callback* callback ) {
#if 0
        dbg().verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

		if ( m_inFlightCnt[Read] + m_pendingReqQ[Read].size() < m_maxRequestsPending[Read] - 1 ) {
			src = NULL;
		} else {
			assert( m_blockedSrc[Read] == NULL ); 
			m_blockedSrc[Read] = src;
		}
		m_reqCnt[Read]->addData(1);
		m_pendingReqQ[Read].push( new Entry( Read, req, m_cnt++, callback ) );
		return src != NULL;
#endif
	}

	void init( unsigned int phase ) {
		m_mem_link->init(phase);
	}

#if 0
  private:

	void handleEvent( Interfaces::SimpleMem::Request* ev ) {

		Interfaces::SimpleMem::Request::id_t reqID = ev->id;
    	std::map<Interfaces::SimpleMem::Request::id_t, Entry*>::iterator reqFind = m_inflight.find(reqID);

		if(reqFind == m_inflight.end()) {
        	dbg().fatal(CALL_INFO, -1, "Unable to find request %" PRIu64 " in request map.\n", reqID);
    	} else{
			Entry* entry = reqFind->second;
			m_reqLatency->addData( model().getCurrentSimTimeNano() - entry->issueTime );
			m_inFlightAddr.erase( entry->memReq->addr );
			--m_inFlightCnt[entry->op];
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"id=%" PRIu64 ", addr=%" PRIx64 " acked\n", reqID, entry->memReq->addr);
			if ( entry->callback ) {
				model().schedCallback( 0, entry->callback );
			}
			m_inflight.erase( ev->id );
			if ( m_blockedSrc[entry->op] ) {
       			m_dbg.verbosePrefix(prefix(),CALL_INFO,2,DETAILED_MASK,"resume\n" );
				m_model.schedResume( 0, m_blockedSrc[entry->op], this );
				m_blockedSrc[entry->op] = NULL;
			}
			delete entry;
			delete ev;
		}
	}

	Entry* nextEntry() {
		Entry* entry = NULL;
		if ( ! m_pendingReqQ[Read].empty() && ! m_pendingReqQ[Write].empty() ) {
			if ( m_pendingReqQ[Read].front()->num < m_pendingReqQ[Write].front()->num ) {
				entry = m_pendingReqQ[Read].front();	
			} else {
				entry = m_pendingReqQ[Write].front();	
			}
		} else if ( ! m_pendingReqQ[Read].empty() ) {
			entry = m_pendingReqQ[Read].front();	
		} else if ( ! m_pendingReqQ[Write].empty() ) {
			entry = m_pendingReqQ[Write].front();	
		}

		return entry;
	}

	void popEntry() {
		if ( ! m_pendingReqQ[Read].empty() && ! m_pendingReqQ[Write].empty() ) {
			if ( m_pendingReqQ[Read].front()->num < m_pendingReqQ[Write].front()->num ) {
				m_pendingReqQ[Read].pop();
			} else {
				m_pendingReqQ[Write].pop();
			}
		} else if ( ! m_pendingReqQ[Read].empty() ) {
			m_pendingReqQ[Read].pop();
		} else if ( ! m_pendingReqQ[Write].empty() ) {
			m_pendingReqQ[Write].pop();
		}
	}

	bool clock_handler(Cycle_t cycle) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,DETAILED_MASK,"pendingWrite=%zu pendingRea%zu\n", m_pendingReqQ[Write].size(), m_pendingReqQ[Read].size() );

		Entry* entry;
		if ( ( entry = nextEntry() ) ) {

			if ( m_inFlightAddr.find( entry->memReq->addr ) != m_inFlightAddr.end() ) {
				return false;	
			}

			popEntry(); 

			if ( m_inFlightCnt[entry->op] < m_maxRequestsPending[entry->op] ) {
				SST::Interfaces::SimpleMem::Request* req = NULL;
				++m_inFlightCnt[entry->op];
				req = new Interfaces::SimpleMem::Request(
                    entry->op == Read ? Interfaces::SimpleMem::Request::Read : Interfaces::SimpleMem::Request::Write,
                    entry->memReq->addr, entry->memReq->length);
				m_inflight[req->id] = entry; 
        		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"id=%" PRIu64 ", addr=%" PRIx64 ", %s inFlightCnt=%d\n",
									req->id, req->addr, entry->op == Write ? "Write":"Read", m_inFlightCnt[entry->op]);
				entry->issueTime = m_model.getCurrentSimTimeNano();
				m_mem_link->sendRequest( req );
				m_inFlightAddr.insert( entry->memReq->addr );

			} else {
				assert(0);
			}	
		}
		return false;
	}

	TimeConverter* 					m_clock;
	Clock::Handler<DetailedUnit>* 	m_clock_handler;
	Interfaces::SimpleMem* 			m_mem_link;
	Link* 							m_selfLink;

	std::map< Interfaces::SimpleMem::Request::id_t, Entry* > m_inflight;
	std::set< uint64_t > m_inFlightAddr; 
	std::vector< Statistic<uint64_t>* > m_reqCnt;
	Statistic<uint64_t>* m_reqLatency;
#endif
};
