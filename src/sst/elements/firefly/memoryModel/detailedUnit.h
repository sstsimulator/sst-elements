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

		Entry( Op op, MemReq* memReq, Callback* callback = NULL ) :
                op(op), memReq(memReq), callback( callback )
            { }
            Op op;
            MemReq* memReq;
            Callback* callback;
            SimTime_t issueTime;
        };

	std::vector<uint32_t> m_maxRequestsPending;
	std::vector<uint32_t> m_requestsPending;
	std::queue<UnitBase*> m_blockedSrc;
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
            Unit( compId, model, dbg ), m_maxRequestsPending(2), m_requestsPending(2,0)
    {
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
		m_maxPending = params.find<int>( "maxPending", 32 );

		m_clock_handler = new Clock::Handler<DetailedUnit>(this,&DetailedUnit::clock_handler);
		m_clock = model->registerClock( freq, m_clock_handler);
		m_reqCnt.resize(2);
		m_reqCnt[Read] = model->registerStatistic<uint64_t>( "detailed_num_reads" );
		m_reqCnt[Write] = model->registerStatistic<uint64_t>( "detailed_num_writes" );
		m_reqLatency  = model->registerStatistic<uint64_t>( "detailed_req_latency" );
    }

    bool store( UnitBase* src, MemReq* req ) {
        dbg().verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr, req->length);

		if ( m_pendingQ.size() < m_maxPending ) {
			src = NULL;
		} else {
			m_blockedSrc.push( src );
		}
		m_reqCnt[Write]->addData(1);
		m_pendingQ.push( new Entry( Write, req ) );

		return src;
    }

    bool load( UnitBase* src, MemReq* req, Callback* callback ) {
        dbg().verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

		if ( m_pendingQ.size() < m_maxPending ) {
			src = NULL;
		} else {
			m_blockedSrc.push( src );
		}
		m_reqCnt[Read]->addData(1);
		m_pendingQ.push( new Entry( Read, req, callback ) );
		return src;
	}

	void init( unsigned int phase ) {
		m_mem_link->init(phase);
	}

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
			--m_requestsPending[entry->op];
        	dbg().verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"id=%" PRIu64 ", inflight=%zu blockedSrc=%zu addr=%" PRIx64 " pendingQ=%zu\n",
							reqID, m_inflight.size(), m_blockedSrc.size(), entry->memReq->addr, m_pendingQ.size());
			if ( entry->callback ) {
				model().schedCallback( 0, entry->callback );
			}
			m_inflight.erase( ev->id );
			delete entry;
			delete ev;
		}
	}

	bool clock_handler(Cycle_t cycle) {
        dbg().verbosePrefix(prefix(),CALL_INFO,2,DETAILED_MASK,"pending=%zu\n", m_pendingQ.size() );

		if ( ! m_pendingQ.empty() ) {
			Entry* entry = m_pendingQ.front();

			if ( m_inFlightAddr.find( entry->memReq->addr ) != m_inFlightAddr.end() ) {
				return false;	
			}

			if ( m_requestsPending[entry->op] <  m_maxRequestsPending[entry->op] ) {
				SST::Interfaces::SimpleMem::Request* req = NULL;
				++m_requestsPending[entry->op];
				req = new Interfaces::SimpleMem::Request(
                    entry->op == Read ? Interfaces::SimpleMem::Request::Read : Interfaces::SimpleMem::Request::Write,
                    entry->memReq->addr, entry->memReq->length);
				m_inflight[req->id] = entry; 
        		dbg().verbosePrefix(prefix(),CALL_INFO,1,DETAILED_MASK,"id=%" PRIu64 ", addr=%" PRIx64 ", reqPending=%d\n",
									req->id, req->addr, m_requestsPending[entry->op]);
				entry->issueTime = model().getCurrentSimTimeNano();
				m_mem_link->sendRequest( req );
				m_pendingQ.pop();
				m_inFlightAddr.insert( entry->memReq->addr );
				if ( ! m_blockedSrc.empty() ) {

        			dbg().verbosePrefix(prefix(),CALL_INFO,2,DETAILED_MASK,"resume\n" );
					model().schedResume( 0, m_blockedSrc.front(), this );
					m_blockedSrc.pop();
				}
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

	int m_maxPending;
	std::queue< Entry* > m_pendingQ;
	std::map< Interfaces::SimpleMem::Request::id_t, Entry* > m_inflight;
	std::set< uint64_t > m_inFlightAddr; 
	std::vector< Statistic<uint64_t>* > m_reqCnt;
	Statistic<uint64_t>* m_reqLatency;
};
