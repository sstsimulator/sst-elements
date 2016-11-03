// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
    SubComponent(comp), m_flushEvent(NULL)
{
    m_dbg.init("--->  ", 
            params.find<uint32_t>("debug_level", 0),
            params.find<uint32_t>("debug_mask", 0),
            (Output::output_location_t)params.find<int>("debug_location", 0 ));

    string backendName  = params.find<std::string>("backend", "memHierarchy.simpleMem");

    // extract backend parameters for memH.
    Params backendParams = params.find_prefix_params("backend.");
    m_backend = dynamic_cast<MemBackend*>( comp->loadSubComponent( backendName, comp, backendParams ) );
    m_backend->setConvertor(this);

    m_frontendRequestWidth =  params.find<uint32_t>("request_width",64);
    m_backendRequestWidth = static_cast<SimpleMemBackend*>(m_backend)->getRequestWidth();

    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSExReqReceived  = registerStatistic<uint64_t>("requests_received_GetSEx");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_outstandingReqs    = registerStatistic<uint64_t>("outstanding_requests");
    stat_GetSLatency        = registerStatistic<uint64_t>("latency_GetS");
    stat_GetSExLatency      = registerStatistic<uint64_t>("latency_GetSEx");
    stat_GetXLatency        = registerStatistic<uint64_t>("latency_GetX");
    stat_PutMLatency        = registerStatistic<uint64_t>("latency_PutM");

    cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    cyclesAttemptIssueButRejected = registerStatistic<uint64_t>( "cycles_attempted_issue_but_rejected" );
    totalCycles     = registerStatistic<uint64_t>( "total_cycles" );
}

void MemBackendConvertor::handleMemEvent(  MemEvent* ev ) {

    ev->setDeliveryTime(getCurrentSimTimeNano());

    doReceiveStat( ev->getCmd() );

    Debug(_L10_,"Creating MemReq. BaseAddr = %" PRIx64 ", Size: %" PRIu32 ", %s\n",
                        ev->getBaseAddr(), ev->getSize(), CommandString[ev->getCmd()]);

    if ( ! m_flushEvent ) {
        m_flushEvent = setupMemReq( ev );
    } else {
        m_waiting.push_back( ev );
    } 
}

bool MemBackendConvertor::clock(Cycle_t cycle) {

    doClockStat();

    int reqsThisCycle = 0;
    while ( !m_requestQueue.empty()) {
        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        MemReq* req = m_requestQueue.front();

        if ( issue( req ) ) {
            cyclesWithIssue->addData(1);
        } else {
            cyclesAttemptIssueButRejected->addData(1);
            break;
        }

        reqsThisCycle++;
        req->increment( m_backendRequestWidth );

        if ( req->processed() >= m_backendRequestWidth ) {
            Debug(_L10_, "Completed issue of request\n");
            m_requestQueue.pop_front();
        }
    }

    stat_outstandingReqs->addData( m_pendingRequests.size() );

    m_backend->clock();

    return false;
}

MemEvent* MemBackendConvertor::doResponse( ReqId reqId ) {

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
        if ( PutM != event->getCmd()  ) {
            resp = event->makeResponse();
        }
        Cycle_t latency = getCurrentSimTimeNano() - event->getDeliveryTime();

        doResponseStat( event->getCmd(), latency );

        // MemReq deletes it's MemEvent
        delete req;

    }
    return resp;
}

void MemBackendConvertor::sendResponse( MemEvent* resp ) {

    Debug(_L10_, "send response\n");
    static_cast<MemController*>(parent)->handleMemResponse( resp );

    if ( m_flushEvent && m_pendingRequests.empty() ) {

        MemEvent* flush = m_flushEvent->makeResponse();

        flush->setSuccess(true);
        static_cast<MemController*>(parent)->handleMemResponse( flush );

        delete m_flushEvent; 
        m_flushEvent = NULL;

        while ( ! m_waiting.empty() && ! m_flushEvent ) {
            m_flushEvent = setupMemReq( m_waiting.front( ) ); 
            m_waiting.pop_front();
        }
    }
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
