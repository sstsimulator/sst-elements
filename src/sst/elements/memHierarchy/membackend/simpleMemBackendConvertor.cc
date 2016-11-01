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
#include <sst/core/params.h>
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include "membackend/simpleMemBackendConvertor.h"
#include "membackend/membackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

SimpleMemBackendConvertor::SimpleMemBackendConvertor(Component *comp, Params& params ) : 
    MemBackendConvertor(comp,params), m_reqId(0)
{ 
    m_frontendRequestWidth =  params.find<uint32_t>("request_width",64);
    m_backendRequestWidth = static_cast<SimpleMemBackend*>(m_backend)->getRequestWidth(); 
}

void SimpleMemBackendConvertor::handleMemEvent(  MemEvent* ev ) {

    ev->setDeliveryTime(getCurrentSimTimeNano());

    doReceiveStat( ev->getCmd() );


    Debug(_L10_,"Creating MemReq. BaseAddr = %" PRIx64 ", Size: %" PRIu32 ", %s\n",
                        ev->getBaseAddr(), ev->getSize(), CommandString[ev->getCmd()]);

    uint32_t id = genReqId();
    MemReq* req = new MemReq( ev, id );
    m_requestQueue.push_back( req );
    m_pendingRequests[id] = req;

}

bool SimpleMemBackendConvertor::clock(Cycle_t cycle) {

    totalCycles->addData(1);

    int reqsThisCycle = 0;
    while ( !m_requestQueue.empty()) {
        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        MemReq* req = m_requestQueue.front();

        bool issued = static_cast<SimpleMemBackend*>(m_backend)->issueRequest( req->id(), req->addr(), req->isWrite(), m_backendRequestWidth );
        if (issued) {
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

void SimpleMemBackendConvertor::handleMemResponse( ReqId reqId ) {

    uint32_t id = MemReq::getBaseId(reqId);

    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "memory request not found\n");
    }

    MemReq* req = m_pendingRequests[id];

    Debug(_L10_, "Finishing processing MemReq\n");

    req->decrement( );

    if ( req->isDone() ) {
        Debug(_L10_, "Finishing processing MemEvent. Addr = %" PRIx64 "\n", req->baseAddr( ) );
        sendResponse( req );
        m_pendingRequests.erase(id);
    }
}

void SimpleMemBackendConvertor::sendResponse( MemReq* req ) {
    MemEvent* event = req->getMemEvent();

    if ( PutM != event->getCmd() ) {
        Debug(_L10_, "send response\n");
        static_cast<MemController*>(parent)->handleMemResponse( event->makeResponse() );
    }
    Cycle_t latency = getCurrentSimTimeNano() - event->getDeliveryTime();

    doResponseStat( event->getCmd(), latency );

    // MemReq deletes it's MemEvent
    delete req;
}

SimpleMemBackendConvertor::~SimpleMemBackendConvertor() {

    while ( m_requestQueue.size()) {
        delete m_requestQueue.front();
        m_requestQueue.pop_front();
    }

    PendingRequests::iterator iter = m_pendingRequests.begin();
    for ( ; iter != m_pendingRequests.end(); ++ iter ) {
        delete iter->second;
    }
}

const std::string& SimpleMemBackendConvertor::getRequestor( ReqId reqId ) {
    uint32_t id = MemReq::getBaseId(reqId);
    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "memory request not found\n");
    }

    return m_pendingRequests[id]->getMemEvent()->getRqstr();
}
