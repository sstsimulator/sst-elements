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
#include "membackend/hmcMemBackendConvertor.h"
#include "membackend/membackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

bool HMCMemBackendConvertor::clock(Cycle_t cycle) {

    doClockStat();

    int reqsThisCycle = 0;
    while ( !m_requestQueue.empty()) {
        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        MemReq* req = m_requestQueue.front();
        MemEvent* event = req->getMemEvent();

        unsigned m_requestSize = 64;
        bool issued = static_cast<HMCMemBackend*>(m_backend)->issueRequest( req->id(), req->addr(), req->isWrite(), event->getFlags(), m_requestSize );
        if (issued) {
            cyclesWithIssue->addData(1);
        } else {
            cyclesAttemptIssueButRejected->addData(1);
            break;
        }

        unsigned m_cacheLineSize = 64;
        reqsThisCycle++;
        req->increment( m_requestSize );

        if ( req->processed() >= m_cacheLineSize ) {
            Debug(_L10_, "Completed issue of request\n");
            m_requestQueue.pop_front();
        }
    }

    stat_outstandingReqs->addData( m_pendingRequests.size() );

    m_backend->clock();

    return false;
}


void HMCMemBackendConvertor::handleMemResponse( ReqId reqId, uint32_t flags  )
{
    uint32_t id = MemReq::getBaseId(reqId);

    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "memory request not found\n");
    }

    MemReq* req = m_pendingRequests[id];

    Debug(_L10_, "Finishing processing MemReq\n");

    req->decrement( );

    if ( req->isDone() ) {
        Debug(_L10_, "Finishing processing MemEvent. Addr = %" PRIx64 "\n", req->baseAddr( ) );
        sendResponse( req, flags );
        m_pendingRequests.erase(id);
    }
}

void HMCMemBackendConvertor::sendResponse( MemReq* req, uint32_t flags) {
    MemEvent* event = req->getMemEvent();

    if ( PutM != event->getCmd() ) {
        Debug(_L10_, "send response\n");
        MemEvent* resp = event->makeResponse();
        resp->setFlags(flags);
        static_cast<MemController*>(parent)->handleMemResponse( resp );
    }
    Cycle_t latency = getCurrentSimTimeNano() - event->getDeliveryTime();

    doResponseStat( event->getCmd(), latency );

    // MemReq deletes it's MemEvent
    delete req;
}

