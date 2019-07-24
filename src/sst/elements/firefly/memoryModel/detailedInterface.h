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

#ifndef COMPONENTS_FIREFLY_MEMORY_MODEL_DETAILED_INTERFACE_H
#define COMPONENTS_FIREFLY_MEMORY_MODEL_DETAILED_INTERFACE_H

#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleMem.h>
#include "memoryModel/memReq.h"
#include "memoryModel/memoryModel.h"

#define MY_MASK 1
typedef void PTR;

namespace SST {
namespace Firefly {

class DetailedInterface : public SubComponent {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::DetailedInterface)
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        DetailedInterface,
        "firefly",
        "detailedInterface",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SST::Firefly::DetailedInterface
    )

private:

	enum Op { Read, Write };
	struct Entry {

		Entry( Op op, MemReq* memReq, MemoryModel::Callback* callback = NULL ) :
                op(op), memReq(memReq), callback( callback )
            { }
            Op op;
            MemReq* memReq;
            MemoryModel::Callback* callback;
            SimTime_t issueTime;
        };

	std::vector<uint32_t> m_maxRequestsPending;
	std::vector<uint32_t> m_requestsPending;
	std::queue<PTR*> m_blockedSrc;

  public:

    DetailedInterface( Component* comp, Params& ) : SubComponent( comp ) {} 
    DetailedInterface( ComponentId_t id, Params& params ) : SubComponent( id ), m_maxRequestsPending(2), m_requestsPending(2,0) 
    {
    	char buffer[100];
    	snprintf(buffer,100,"@t:%d:DetailedInterface::@p():@l ",params.find<int>("id",-1));

		m_dbg.init(buffer,
			params.find<uint32_t>("verboseLevel",0),
			params.find<uint32_t>("verboseMask",-1),
			Output::STDOUT);

	    m_maxRequestsPending[Read] = params.find<uint32_t>("maxloadmemreqpending", 16);
    	m_maxRequestsPending[Write] = params.find<uint32_t>("maxstorememreqpending", 16);

		UnitAlgebra freq = params.find<SST::UnitAlgebra>( "freq", "1Ghz" );
		m_clock_handler = new Clock::Handler<DetailedInterface>(this,&DetailedInterface::clock_handler);
		m_clock = registerClock( freq, m_clock_handler);

		m_mem_link = loadUserSubComponent<Interfaces::SimpleMem>("memInterface", ComponentInfo::SHARE_NONE, 
			m_clock , new Interfaces::SimpleMem::Handler<DetailedInterface>(this, &DetailedInterface::handleEvent) );

	    if( m_mem_link ) {
			m_dbg.verbose(CALL_INFO, 1, MY_MASK, "Loaded memory interface successfully.\n");
    	} else {
			m_dbg.fatal(CALL_INFO, -1, "Error loading memory interface module.\n");
    	}

		m_maxPending = params.find<int>( "maxPending", 16 );

		m_reqCnt.resize(2);
		m_reqCnt[Read] = registerStatistic<uint64_t>( "detailed_num_reads" );
		m_reqCnt[Write] = registerStatistic<uint64_t>( "detailed_num_writes" );
		m_reqLatency  = registerStatistic<uint64_t>( "detailed_req_latency" );
    }

	std::function<void( MemoryModel::Callback*)> m_callback; 
	std::function<void(PTR*)> m_resume; 

	void setCallback( std::function<void( MemoryModel::Callback* )> callback ) {
		m_callback = callback;
	}

	void setResume( std::function<void(PTR*)> resume ) {
		m_resume = resume;
	}

    bool store( PTR* src, MemReq* req ) {
        m_dbg.verbose(CALL_INFO,1,MY_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr, req->length);

		if ( m_pendingQ.size() < m_maxPending ) {
			src = NULL;
		} else {
			m_blockedSrc.push( src );
		}
		//m_reqCnt[Write]->addData(1);
		m_pendingQ.push( new Entry( Write, req ) );

		return src;
    }

    bool load( PTR* src, MemReq* req, MemoryModel::Callback* callback ) {
        m_dbg.verbose(CALL_INFO,1,MY_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

		if ( m_pendingQ.size() < m_maxPending ) {
			src = NULL;
		} else {
			m_blockedSrc.push( src );
		}
		//m_reqCnt[Read]->addData(1);
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
        	m_dbg.fatal(CALL_INFO, -1, "Unable to find request %" PRIu64 " in request map.\n", reqID);
    	} else{
			Entry* entry = reqFind->second;
			m_reqLatency->addData( getCurrentSimTimeNano() - entry->issueTime );
			m_inFlightAddr.erase( entry->memReq->addr );
			--m_requestsPending[entry->op];
        	m_dbg.verbose(CALL_INFO,1,MY_MASK,"id=%" PRIu64 ", inflight=%zu blockedSrc=%zu addr=%" PRIx64 " pendingQ=%zu\n",
							reqID, m_inflight.size(), m_blockedSrc.size(), entry->memReq->addr, m_pendingQ.size());
			if ( entry->callback ) {
				m_callback(entry->callback);
			}
			m_inflight.erase( ev->id );
			delete entry;
			delete ev;
		}
	}

	bool clock_handler(Cycle_t cycle) {
        m_dbg.verbose(CALL_INFO,2,MY_MASK,"pending=%zu\n", m_pendingQ.size() );

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
        		m_dbg.verbose(CALL_INFO,1,MY_MASK,"id=%" PRIu64 ", addr=%" PRIx64 ", length=%d reqPending=%d\n",
									req->id, req->addr, entry->memReq->length, m_requestsPending[entry->op]);
				entry->issueTime = getCurrentSimTimeNano();
				m_mem_link->sendRequest( req );
				m_pendingQ.pop();
				m_inFlightAddr.insert( entry->memReq->addr );
				if ( ! m_blockedSrc.empty() ) {

        			m_dbg.verbose(CALL_INFO,2,MY_MASK,"resume\n" );
					m_resume(m_blockedSrc.front()); 
					m_blockedSrc.pop();
				}
			} else {
				assert(0);
			}	
		}
        m_dbg.verbose(CALL_INFO,2,MY_MASK,"\n" );
		return false;
	}

	Clock::Handler<DetailedInterface>* 	m_clock_handler;
	TimeConverter* 					m_clock;
	Interfaces::SimpleMem* 			m_mem_link;

	int m_maxPending;
	std::queue< Entry* > m_pendingQ;
	std::map< Interfaces::SimpleMem::Request::id_t, Entry* > m_inflight;
	std::set< uint64_t > m_inFlightAddr; 
	Output m_dbg;
	std::vector< Statistic<uint64_t>* > m_reqCnt;
	Statistic<uint64_t>* m_reqLatency;
};
}
}
#endif
