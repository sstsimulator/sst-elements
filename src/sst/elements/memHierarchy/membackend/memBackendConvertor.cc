// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include "membackend/memBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

MemBackendConvertor::MemBackendConvertor(Component *comp, Params& params ) : 
    SubComponent(comp), m_cycleCount(0), m_reqId(0)
{
    m_dbg.init("", 
            params.find<uint32_t>("debug_level", 0),
            params.find<uint32_t>("debug_mask", 0),
            (Output::output_location_t)params.find<int>("debug_location", 0 ));

    string backendName  = params.find<std::string>("backend", "memHierarchy.simpleMem");


    // extract backend parameters for memH.
    Params backendParams = params.find_prefix_params("backend.");

    m_backend = dynamic_cast<MemBackend*>( comp->loadSubComponent( backendName, comp, backendParams ) );

    using std::placeholders::_1;
    m_backend->setGetRequestorHandler( std::bind( &MemBackendConvertor::getRequestor, this, _1 )  );

    m_frontendRequestWidth =  params.find<uint32_t>("request_width",64);
    m_backendRequestWidth = static_cast<SimpleMemBackend*>(m_backend)->getRequestWidth();
    if ( m_backendRequestWidth > m_frontendRequestWidth ) {
        m_backendRequestWidth = m_frontendRequestWidth;
    }

    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSXReqReceived  = registerStatistic<uint64_t>("requests_received_GetSX");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_outstandingReqs    = registerStatistic<uint64_t>("outstanding_requests");
    stat_GetSLatency        = registerStatistic<uint64_t>("latency_GetS");
    stat_GetSXLatency      = registerStatistic<uint64_t>("latency_GetSX");
    stat_GetXLatency        = registerStatistic<uint64_t>("latency_GetX");
    stat_PutMLatency        = registerStatistic<uint64_t>("latency_PutM");

    stat_cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    stat_cyclesAttemptIssueButRejected = registerStatistic<uint64_t>( "cycles_attempted_issue_but_rejected" );
    stat_totalCycles     = registerStatistic<uint64_t>( "total_cycles" );
}

void MemBackendConvertor::handleMemEvent(  MemEvent* ev ) {

    ev->setDeliveryTime(m_cycleCount);

    doReceiveStat( ev->getCmd() );

    Debug(_L10_,"Creating MemReq. BaseAddr = %" PRIx64 ", Size: %" PRIu32 ", %s\n",
                        ev->getBaseAddr(), ev->getSize(), CommandString[(int)ev->getCmd()]);


    if (!setupMemReq(ev)) {
        sendResponse(ev->getID(), ev->getFlags());
    }
}

bool MemBackendConvertor::clock(Cycle_t cycle) {


    m_cycleCount++;
    doClockStat(); // TODO do we really need this? It's counted in other calls like 'stat_outstandingReqs' but not as obviously

    int reqsThisCycle = 0;
    while ( !m_requestQueue.empty()) {
        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        MemReq* req = m_requestQueue.front();
        Debug(_L10_, "Processing request: addr: %" PRIu64 ", baseAddr: %" PRIu64 ", processed: %" PRIu32 ", id: %" PRIu64 ", isWrite: %d\n",
                req->addr(), req->baseAddr(), req->processed(), req->id(), req->isWrite());

        if ( issue( req ) ) {
            stat_cyclesWithIssue->addData(1);
        } else {
            stat_cyclesAttemptIssueButRejected->addData(1);
            break;
        }

        reqsThisCycle++;
        req->increment( m_backendRequestWidth );

        if ( req->processed() >= req->size() ) {
            Debug(_L10_, "Completed issue of request\n");
            m_requestQueue.pop_front();
        }
    }

    stat_outstandingReqs->addData( m_pendingRequests.size() );

    m_backend->clock();

    return false;
}

void MemBackendConvertor::doResponse( ReqId reqId, uint32_t flags ) {

    uint32_t id = MemReq::getBaseId(reqId);
    MemEvent* resp = NULL;

    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "memory request not found\n");
    }

    MemReq* req = m_pendingRequests[id];

    req->decrement( );

    if ( req->isDone() ) {
        m_pendingRequests.erase(id);
        MemEvent* event = req->getMemEvent();
        
        Cycle_t latency = m_cycleCount - event->getDeliveryTime();

        doResponseStat( event->getCmd(), latency );
        
        if (!flags) flags = event->getFlags();
        sendResponse(event->getID(), flags); // Needs to occur before a flush is completed since flush is dependent

        // TODO clock responses
        // Check for flushes that are waiting on this event to finish
        if (m_dependentRequests.find(event) != m_dependentRequests.end()) {
            std::unordered_set<MemEvent*> flushes = m_dependentRequests.find(event)->second;
            
            for (std::unordered_set<MemEvent*>::iterator it = flushes.begin(); it != flushes.end(); it++) {
                (m_waitingFlushes.find(*it)->second).erase(event);
                if ((m_waitingFlushes.find(*it)->second).empty()) {
                    MemEvent * flush = *it;
                    sendResponse(flush->getID(), (flush->getFlags() | MemEvent::F_SUCCESS));
                    m_waitingFlushes.erase(flush);
                }
            }
            m_dependentRequests.erase(event);
        }
        delete req;
    }
}

void MemBackendConvertor::sendResponse( SST::Event::id_type id, uint32_t flags ) {

    static_cast<MemController*>(parent)->handleMemResponse( id, flags );

}

void MemBackendConvertor::finish(void) {
    m_backend->finish();
}

const std::string& MemBackendConvertor::getClockFreq() {
    return m_backend->getClockFreq();
}

size_t MemBackendConvertor::getMemSize() {
    return m_backend->getMemSize();
}

uint32_t MemBackendConvertor::getRequestWidth() {
    return m_backend->getRequestWidth();
}
