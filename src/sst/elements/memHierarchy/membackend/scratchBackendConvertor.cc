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


#include <sst_config.h>
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/scratchpad.h"
#include "membackend/scratchBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

ScratchBackendConvertor::ScratchBackendConvertor(ComponentId_t id, Params& params ) :
    SubComponent(id), m_reqId(0)
{ 
    m_dbg.init("",
            params.find<uint32_t>("debug_level", 0),
            params.find<uint32_t>("debug_mask", 0),
            (Output::output_location_t)params.find<int>("debug_location", 0 ));

    m_backend = loadUserSubComponent<MemBackend>("backend");
    if (!m_backend) {
        // extract backend parameters for memH.
        string backendName  = params.find<std::string>("backend", "memHierarchy.simpleMem");
        Params backendParams = params.get_scoped_params("backend");
        m_backend = loadAnonymousSubComponent<MemBackend>(backendName, "backend", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, backendParams);
    }

    using std::placeholders::_1;
    m_backend->setGetRequestorHandler( std::bind( &ScratchBackendConvertor::getRequestor, this, _1 )  );

    m_frontendRequestWidth =  params.find<uint32_t>("request_width",64);
    m_backendRequestWidth = static_cast<SimpleMemBackend*>(m_backend)->getRequestWidth();
    if ( m_backendRequestWidth > m_frontendRequestWidth ) {
        m_backendRequestWidth = m_frontendRequestWidth;
    }

    stat_cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    stat_cyclesAttemptIssueButRejected = registerStatistic<uint64_t>( "cycles_attempted_issue_but_rejected" );
    stat_totalCycles     = registerStatistic<uint64_t>( "total_cycles" );
    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSXReqReceived  = registerStatistic<uint64_t>("requests_received_GetSX");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_GetSLatency        = registerStatistic<uint64_t>("latency_GetS");
    stat_GetSXLatency      = registerStatistic<uint64_t>("latency_GetSX");
    stat_GetXLatency        = registerStatistic<uint64_t>("latency_GetX");
    stat_PutMLatency        = registerStatistic<uint64_t>("latency_PutM");

}

void ScratchBackendConvertor::setCallbackHandler(std::function<void(Event::id_type)> respCB) {
    m_notifyResponse = respCB;
}

void ScratchBackendConvertor::handleMemEvent(  MemEvent * ev ) {

    ev->setDeliveryTime(m_cycleCount);

    doReceiveStat( ev->getCmd() );

    Debug(_L10_,"Creating MemReq. BaseAddr = %" PRIx64 ", Size: %" PRIu32 ", %s\n",
                        ev->getBaseAddr(), ev->getSize(), CommandString[(int)ev->getCmd()]);

    setupMemReq(ev);
}

bool ScratchBackendConvertor::clock(Cycle_t cycle) {
    m_cycleCount++;
    doClockStat();

    int reqsThisCycle = 0;
    while ( !m_requestQueue.empty()) {
        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        MemReq* req = m_requestQueue.front();

        if ( issue( req ) ) {
            stat_cyclesWithIssue->addData(1);
        } else {
            stat_cyclesAttemptIssueButRejected->addData(1);
            break;
        }

        reqsThisCycle++;
        req->increment( m_backendRequestWidth );

        if ( req->processed() >= m_backendRequestWidth ) {
            Debug(_L10_, "Completed issue of request\n");
            m_requestQueue.pop_front();
        }
    }

    bool unclock = m_backend->clock(cycle);

    return false;
}


bool ScratchBackendConvertor::doResponse( ReqId reqId, SST::Event::id_type & respId ) {

    uint32_t id = MemReq::getBaseId(reqId);
    bool sendResponse = false;

    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "%s, memory request not found\n", getName().c_str());
    }

    MemReq* req = m_pendingRequests[id];

    req->decrement( );

    if ( req->isDone() ) {
        m_pendingRequests.erase(id);
        MemEvent* event = req->getMemEvent();

        if ( !event->queryFlag(MemEvent::F_NORESPONSE ) ) {
            respId = event->getID();
            sendResponse = true;
        }

        Cycle_t latency = m_cycleCount - event->getDeliveryTime();

        doResponseStat( event->getCmd(), latency );

        // MemReq deletes its Event
        delete req;

    }
    return sendResponse;
}

void ScratchBackendConvertor::notifyResponse( SST::Event::id_type id) {

    m_notifyResponse(id);

}

void ScratchBackendConvertor::finish(void) {
    m_backend->finish();
}

size_t ScratchBackendConvertor::getMemSize() {
    return m_backend->getMemSize();
}
