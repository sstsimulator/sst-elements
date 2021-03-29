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

	SST_ELI_DOCUMENT_PARAMS(
		{"id","Sets the id for output","-1"},
		{"verboseLevel","Sets the verbose level for output","0"},
		{"verboseMask","Sets the mask for output","-1"},
		{"maxloadmemreqpending","Sets the maximum number of pending loads","16"},
		{"maxstorememreqpending","Sets the maximum number of pending stores","16"},
		{"freq","Sets the frequency","1Ghz"},
	)

private:

	enum Op { Read, Write };
	struct Entry {

		Entry( Op op, MemReq* memReq, uint64_t num, MemoryModel::Callback* callback = NULL ) :
                op(op), memReq(memReq), callback( callback )
            { }
            Op op;
            MemReq* memReq;
            MemoryModel::Callback* callback;
            SimTime_t issueTime;
            uint64_t num;
        };

    uint64_t m_cnt;
    std::vector< std::queue< Entry* > > m_pendingReqQ;
    std::vector<uint32_t> m_maxRequestsPending;
    std::vector<uint32_t> m_inFlightCnt;
    std::vector<PTR*> m_blockedSrc;

  public:

    DetailedInterface( ComponentId_t id, Params& params ) : SubComponent( id ), m_maxRequestsPending(2), m_pendingReqQ(2), m_blockedSrc(2,NULL), m_inFlightCnt(2,0), m_cnt(0)
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

        // Some models use the same address space for each PID. To map each processes address
        // to a unique address the upper 8 bits contain the PID. The detailed model uses unique addresses
        // at the source and sizes the memHierarchy accordingly so we must remove the PID 
        req->addr &= ~0x0f00000000000000;
		if ( m_inFlightCnt[Write] + m_pendingReqQ[Write].size() < m_maxRequestsPending[Write] - 1 ) {
			src = NULL;
		} else {
            assert( m_blockedSrc[Write] == NULL );
            m_blockedSrc[Write] = src;
		}
		m_reqCnt[Write]->addData(1);
		m_pendingReqQ[Write].push( new Entry( Write, req, m_cnt++ ) );

		return src;
    }

    bool load( PTR* src, MemReq* req, MemoryModel::Callback* callback ) {
        m_dbg.verbose(CALL_INFO,1,MY_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);

        // Some models use the same address space for each PID. To map each processes address
        // to a unique address the upper 8 bits contain the PID. The detailed model uses unique addresses
        // at the source and sizes the memHierarchy accordingly so we must remove the PID 
        req->addr &= ~0x0f00000000000000;
       if ( m_inFlightCnt[Read] + m_pendingReqQ[Read].size() < m_maxRequestsPending[Read] - 1 ) {
            src = NULL;
        } else {
            assert( m_blockedSrc[Read] == NULL );
            m_blockedSrc[Read] = src;
        }
        m_reqCnt[Read]->addData(1);
        m_pendingReqQ[Read].push( new Entry( Read, req, m_cnt++, callback ) );
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
			--m_inFlightCnt[entry->op];
			m_dbg.verbose(CALL_INFO,1,MY_MASK,"id=%" PRIu64 ", addr=%" PRIx64 " acked\n", reqID, entry->memReq->addr);
			if ( entry->callback ) {
				m_callback(entry->callback);
			}
			m_inflight.erase( ev->id );
            if ( m_blockedSrc[entry->op] ) {
                m_dbg.verbose(CALL_INFO,2,MY_MASK,"resume\n" );
				m_resume(m_blockedSrc[entry->op] );
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
        m_dbg.verbose(CALL_INFO,2,MY_MASK,"pendingWrite=%zu pendingRead=%zu\n", m_pendingReqQ[Write].size(), m_pendingReqQ[Read].size() );

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
				m_dbg.verbose(CALL_INFO,1,MY_MASK,"id=%" PRIu64 ", addr=%" PRIx64 ", %s inFlightCnt=%d\n",
                                    req->id, req->addr, entry->op == Write ? "Write":"Read", m_inFlightCnt[entry->op]);
				entry->issueTime = getCurrentSimTimeNano();
				m_mem_link->sendRequest( req );
				m_inFlightAddr.insert( entry->memReq->addr );
			} else {
				assert(0);
			}
		}
		return false;
	}

	Clock::Handler<DetailedInterface>* 	m_clock_handler;
	TimeConverter* 					m_clock;
	Interfaces::SimpleMem* 			m_mem_link;

	std::map< Interfaces::SimpleMem::Request::id_t, Entry* > m_inflight;
	std::set< uint64_t > m_inFlightAddr;
	Output m_dbg;
	std::vector< Statistic<uint64_t>* > m_reqCnt;
	Statistic<uint64_t>* m_reqLatency;
};
}
}
#endif
