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
#include "membackend/memBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

MemBackendConvertor::MemBackendConvertor(Component *comp, Params& params ) : SubComponent(comp) 
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

    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSExReqReceived  = registerStatistic<uint64_t>("requests_received_GetSEx");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_outstandingReqs    = registerStatistic<uint64_t>("outstanding_requests");
    stat_GetSLatency        = registerStatistic<uint64_t>("latency_GetS");
    stat_GetSExLatency      = registerStatistic<uint64_t>("latency_GetSEx");
    stat_GetXLatency        = registerStatistic<uint64_t>("latency_GetX");
    stat_PutMLatency        = registerStatistic<uint64_t>("latency_PutM");

    totalCycles     = registerStatistic<uint64_t>( "total_cycles" );
    cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    cyclesAttemptIssueButRejected = registerStatistic<uint64_t>( "cycles_attempted_issue_but_rejected" );
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
