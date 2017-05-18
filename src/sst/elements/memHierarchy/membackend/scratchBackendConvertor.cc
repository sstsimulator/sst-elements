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

ScratchBackendConvertor::ScratchBackendConvertor(Component *comp, Params& params ) : 
    SubComponent(comp), m_reqId(0)
{
    m_dbg.init("--->  ", 
            params.find<uint32_t>("debug_level", 0),
            params.find<uint32_t>("debug_mask", 0),
            (Output::output_location_t)params.find<int>("debug_location", 0 ));

    string backendName  = params.find<std::string>("backend", "memHierarchy.simpleMem");


    // extract backend parameters for memH.
    Params backendParams = params.find_prefix_params("backend.");

    m_backend = dynamic_cast<MemBackend*>( comp->loadSubComponent( backendName, comp, backendParams ) );

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
    stat_ReadReceived = registerStatistic<uint64_t>( "reads_received" );
    stat_WriteReceived = registerStatistic<uint64_t>( "writes_received" );
    stat_ReadLatency = registerStatistic<uint64_t>("read_latency");
    stat_WriteLatency = registerStatistic<uint64_t>("write_latency");
}

void ScratchBackendConvertor::handleScratchEvent(  ScratchEvent * ev ) {

    ev->setDeliveryTime(m_cycleCount);

    doReceiveStat( ev->getCmd() );

    Debug(_L10_,"Creating ScratchReq. BaseAddr = %" PRIx64 ", Size: %" PRIu32 ", %s\n",
                        ev->getBaseAddr(), ev->getSize(), ScratchCommandString[ev->getCmd()]);

    setupScratchReq(ev);
}

bool ScratchBackendConvertor::clock(Cycle_t cycle) {
    m_cycleCount++;
    doClockStat();

    int reqsThisCycle = 0;
    while ( !m_requestQueue.empty()) {
        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        ScratchReq* req = m_requestQueue.front();

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

    m_backend->clock();

    return false;
}


bool ScratchBackendConvertor::doResponse( ReqId reqId, SST::Event::id_type & respId ) {

    uint32_t id = ScratchReq::getBaseId(reqId);
    bool sendResponse = false;

    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "%s, memory request not found\n", parent->getName().c_str());
    }

    ScratchReq* req = m_pendingRequests[id];

    req->decrement( );

    if ( req->isDone() ) {
        m_pendingRequests.erase(id);
        ScratchEvent* event = req->getScratchEvent();

        if ( Write != event->getCmd()  ) {
            respId = event->getID();
            sendResponse = true;
        }

        Cycle_t latency = m_cycleCount - event->getDeliveryTime();

        doResponseStat( event->getCmd(), latency );

        // ScratchReq deletes its ScratchEvent
        delete req;

    }
    return sendResponse;
}

void ScratchBackendConvertor::notifyResponse( SST::Event::id_type id) {

    Debug(_L10_, "send response\n");
    static_cast<Scratchpad*>(parent)->handleScratchResponse( id );

}

void ScratchBackendConvertor::finish(void) {
    m_backend->finish();
}

const std::string& ScratchBackendConvertor::getClockFreq() {
    return m_backend->getClockFreq();
}

size_t ScratchBackendConvertor::getMemSize() {
    return m_backend->getMemSize();
}
